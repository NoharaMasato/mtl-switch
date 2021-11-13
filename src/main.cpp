#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "pthread.h"

#include <chrono>
#include <iostream>
#include <string>
#include <unordered_map>

#include "config.hpp"
#include "eth_device.hpp"
#include "packet.hpp"
#include "redis.hpp"
#include "checksum.hpp"
#include "response.hpp"
#include "controller_client.hpp"
#include "log.hpp"
extern "C"{
  #include "thpool.h"
}

// [TODO] 複数スレッドで同じ変数を書き換え、読み込みをするときに読み込む方のスレッドでlockは必要？

EthernetDevice DEVICE[MAX_DEVICE_CNT];
enum ACTION { FORWARD, DROP, ANONYMIZE, FORWARD_PACKET, PIGGY_BACK }; // 3は許可されているプラス先行パケットを作成して送る, 4はpiggybackして送る
int device_cnt, EndFlag = 0;
long long int total_packet_size = 0;
std::unordered_map<long long int, int> mac_to_port;
pthread_mutex_t mutex_main, mutex_buf, mutex_piggyback;
threadpool thpool = thpool_init(THREAD_POOL_SIZE);

packet_buf p_buf[MAX_BUF_SIZE];
int buf_used_cnt = 0;

// ファイル間で共通の変数でconfig.hppにextern宣言されているので、他のファイルでも同じ変数を参照可能
std::string switch_name;
int switch_id;
std::unordered_map<int, std::vector<u_int8_t>> PIGGYBACK_INFO;

int get_next_buf_idx() {
  static int idx;
  idx = (idx + 1) % MAX_BUF_SIZE; // 前のが使われていなかったとしても次に1つ進める
  while (p_buf[idx].used) {
    idx = (idx + 1) % MAX_BUF_SIZE;
  }
  pthread_mutex_lock(&mutex_buf);
  p_buf[idx].used = true;
  buf_used_cnt += 1;
  pthread_mutex_unlock(&mutex_buf);
  if (buf_used_cnt == MAX_BUF_SIZE) {
    std::cerr << "buf is full" << std::endl;
  }
  return idx;
}

void packet_forward(void *arg){
  int p_buf_idx = (long long int)(arg);
  ACTION action;
  int ip_header_start, flow_id, outport;
  std::string redis_flow_id_ret, redis_action_ret;

  action = FORWARD; // デフォルトはFoward
  ip_header_start = DEVICE[p_buf[p_buf_idx].port_idx].device_type == ETHERNET? ETHERNET_HEADER_SIZE : 4;
  Packet pkt(p_buf[p_buf_idx].buf, p_buf[p_buf_idx].size, ip_header_start); // bufのデータを整形している

  pthread_mutex_lock(&mutex_main);
  mac_to_port[pkt.src_ether_addr_ll()] = p_buf[p_buf_idx].port_idx + 1; // 0をportを学習していないとするため、ポート番号は1から始める
  pthread_mutex_unlock(&mutex_main);

  if (MANAGE_FLOW && pkt.is_tcp){

    if (!pkt.tag.is_attached) { // tagがなかったら新しいパケットを作り、tagをつけて、送信する
      redis_flow_id_ret = select_string(pkt.to_five_tuple().to_string());
      if (redis_flow_id_ret.size() == 0) {
        redis_flow_id_ret = select_string(pkt.to_4tuple_string());
      }
      if (redis_flow_id_ret.size()) { // tagテーブルにある場合のみtagをつける
        flow_id = std::stoi(redis_flow_id_ret);
        // [TODO] ここでサイズによって、piggybackか先行パケットかを決める
        pkt.attach_tag(flow_id);
        p_buf[p_buf_idx].size += TAG_SIZE;
      }
      // tagをつけるときもつけないときもchecksumを計算し直す
      recaluculate_checksum(&pkt);
    }

    if (pkt.tag.is_attached) { // tagがある場合のみactionを検索する（tagがない場合にはそのままforwardする）
      // piggybackの中に自分のswitch idがあれば、その情報をテーブルに入れる
      for (int a_id : pkt.tag.allowed_switch_ids){
        if (a_id == switch_id) { // piggybackの中に自分のswitch idが入っていたら
          insert_flow_table(pkt.tag.flow_id, "0");
        }
      }
      redis_action_ret = select_string(std::to_string(pkt.tag.flow_id)); // tagを元にflowテーブルからactionを検索する
      if (redis_action_ret.size()) { // flowテーブルにデータがある場合
        action = (ACTION)std::stoi(redis_action_ret);
      } else { // flowテーブルにtagに関するエントリがない場合はコントローラにたづねて、その結果をフローテーブルに入れる
        send_message(std::to_string(pkt.tag.flow_id));
        MYCOUT << "Ask to controller about tag: " << pkt.tag.flow_id;
        while (true) { // injectionのスレッドが、redisに値を入れてくれるのを待つ
          redis_action_ret = select_string(std::to_string(pkt.tag.flow_id));
          if (redis_action_ret.size()) { // redisに値があった場合はループから抜ける
            action = (ACTION)std::stoi(redis_action_ret);
            break;
          }
        }
      }
    }
    // コントローラから帰ってきたallowed_switchの情報はそのスイッチではpkt.tagに反映されない
    MYCOUT << "packet: " << pkt.to_five_tuple().to_string()
           << ", flow_id: " << pkt.tag.flow_id
           << ", allowed_switches: " << pkt.tag.allowed_switch_string()
           << ", action: " << action
           << ", len: " << ntohs(pkt.ip_header->tot_len) << " : " << p_buf[p_buf_idx].size;
  }

  if (action == PIGGY_BACK) { // piggybackの場合は転送の前にpiggyback情報を負荷する
    pthread_mutex_lock(&mutex_piggyback);
    pkt.add_allowed_switch_to_tag(PIGGYBACK_INFO[pkt.tag.flow_id]);
    PIGGYBACK_INFO[pkt.tag.flow_id] = {}; // 一度piggybackをしたら、次のパケットはpiggybackしなくていいので、この配列をからにする
    pthread_mutex_unlock(&mutex_piggyback);
  }

  if (action == FORWARD || action == PIGGY_BACK) {
    pthread_mutex_lock(&mutex_main);
    outport = mac_to_port[pkt.dst_ether_addr_ll()];
    pthread_mutex_unlock(&mutex_main);
    if (outport != 0) {
      DEVICE[outport - 1].msoc.mwrite(p_buf[p_buf_idx].buf, p_buf[p_buf_idx].size); // outportは0-indexedにしている
    } else { // dstのmacをまだ学習していない場合はFLODDする
      for (int j = 0; j < device_cnt; j++) {
        if (j != p_buf[p_buf_idx].port_idx) { // inportには送信しない
          DEVICE[j].msoc.mwrite(p_buf[p_buf_idx].buf, p_buf[p_buf_idx].size);
        }
      }
    }
  } else if (action == DROP) { // dropなので何もしない
  } else if (action == FORWARD_PACKET) { // 先行パケットを送信してから転送するパケットを送信する
  }

  pthread_mutex_lock(&mutex_buf);
  p_buf[p_buf_idx].used = false;
  buf_used_cnt -= 1;
  pthread_mutex_unlock(&mutex_buf);
}

int Bridge()
{
  struct pollfd	targets[MAX_DEVICE_CNT];
  int	nready, i, size, p_buf_idx;

  for (int i = 0; i < device_cnt; i++) {
    targets[i].fd=DEVICE[i].msoc.sd;
    targets[i].events=POLLIN|POLLERR;
  }
  std::cout << "device_cnt: " << device_cnt << std::endl;

  while(EndFlag==0){
    switch(nready=poll(targets,device_cnt,100)){ // pollの引数: fdの配列, fdの数, timeout(ms)
      case	-1:
        if(errno!=EINTR){
          perror("poll");
        }
        break;
      case	0:
        break;
      default:
        for(i=0;i<device_cnt;i++){
          if(targets[i].revents&(POLLIN|POLLERR)){

            p_buf_idx = get_next_buf_idx();
            size = DEVICE[i].msoc.mread(p_buf[p_buf_idx].buf);

            total_packet_size += size;

            p_buf[p_buf_idx].size = size;
            p_buf[p_buf_idx].port_idx = i;
            thpool_add_work(thpool, packet_forward, (void*)(uintptr_t)p_buf_idx);
          }
        }
        break;
    }
  }
  return(0);
}

void EndSignal(int sig __attribute__((unused))) {
  EndFlag=1;
}

int main(int argc, char *argv[]) {
  std::chrono::system_clock::time_point start, end;
  double time;

  if (argc != 2) {
    std::cerr << "[usage] ./FMSwitch switch_name" << std::endl;
    exit(1);
  }

  switch_name = argv[1];
  switch_id = stoi(switch_name.substr(1));

  device_cnt = get_all_ethernet_device(switch_name, DEVICE);

  pthread_mutex_init(&mutex_main, NULL);
  pthread_mutex_init(&mutex_buf, NULL);
  pthread_mutex_init(&mutex_piggyback, NULL);

  start = std::chrono::system_clock::now();

  signal(SIGINT,EndSignal);
  signal(SIGTERM,EndSignal);
  signal(SIGQUIT,EndSignal);

  std::cout << "[Flow Management Switch start]" << std::endl;

  if (init_socket(CONTROLLER_IP) == -1) {
    std::cerr << " : [Controller Connection Faild]" << std::endl;
  }
  pthread_t pthread;
  pthread_create(&pthread, NULL, &cache_injection, NULL);

  Bridge();

  end = std::chrono::system_clock::now();
  time = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - start) .count() / 1000.0);

  MYCOUT << "=====report====="
         << "\n"
         << "パケット処理量:" << total_packet_size / 1e9 * 8 << "[Gb]\n"
         << "処理時間" << time << " [ms]\n"
         << "スループット" << total_packet_size / time / 1e6 * 8
         << " [Gbps]\n"
         << "================";

  return 0;
}
