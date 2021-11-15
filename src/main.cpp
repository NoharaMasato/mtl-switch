#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "pthread.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <chrono>

#include "eth_device.hpp"
#include "packet.hpp"
extern "C"{
  #include "thpool.h"
}

#define MAX_DEVICE_CNT 100
#define THREAD_POOL_SIZE 4
#define MAX_BUF_SIZE 1000

// [TODO] 複数スレッドで同じ変数を書き換え、読み込みをするときに読み込む方のスレッドでlockは必要？

EthernetDevice DEVICE[MAX_DEVICE_CNT];
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
  int ip_header_start, outport;
  std::string redis_flow_id_ret, redis_action_ret;

  ip_header_start = DEVICE[p_buf[p_buf_idx].port_idx].device_type == ETHERNET? ETHERNET_HEADER_SIZE : 4;
  Packet pkt(p_buf[p_buf_idx].buf, p_buf[p_buf_idx].size, ip_header_start); // bufのデータを整形している

  pthread_mutex_lock(&mutex_main);
  mac_to_port[pkt.src_ether_addr_ll()] = p_buf[p_buf_idx].port_idx + 1; // 0をportを学習していないとするため、ポート番号は1から始める
  pthread_mutex_unlock(&mutex_main);

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

  Bridge();

  end = std::chrono::system_clock::now();
  time = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - start) .count() / 1000.0);

  std::cout << "=====report====="
            << "\n"
            << "パケット処理量:" << total_packet_size / 1e9 * 8 << "[Gb]\n"
            << "処理時間" << time << " [ms]\n"
            << "スループット" << total_packet_size / time / 1e6 * 8
            << " [Gbps]\n"
            << "================";

  return 0;
}
