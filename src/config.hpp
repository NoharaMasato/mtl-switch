#ifndef CONFIG_H_
#define CONFIG_H_

#include <unordered_map>
#include <vector>
#include <string>

#define PRINT_DEBUG 0

#define CONTROLLER_IP "10.0.100.100"
// #define CONTROLLER_IP "127.0.0.1"
#define CONTROLLER_PORT 8888

#define REDIS_HOST "127.0.0.1:6379"

// フローテーブルのレコードのタイムアウト時間(0の場合はtimeoutなし)
// TimeoutQueueを使わずにredisのタイムアウトを設定すると、
// queueにどんどん溜まっていくことになりアプリケーション側で、queueの更新が起こりまくるようになる
#define REDIS_EXPIRE_SEC 10

// 一つのredisのテーブルに入れられる最大フローエントリ数(ホスト数よりも大きくするとslashingは起こらなくなる)
// set associative cacheの場合はway数も考える
#define MAX_FLOW_ENTRY_CNT_UPSTREAM 100
#define MAX_FLOW_ENTRY_CNT_DOWNSTREAM 45

// 一つのswitchが持てる最大インターフェース数
#define MAX_DEVICE_CNT 100

// 最大スレッド数
#define THREAD_POOL_SIZE 4
#define MAX_BUF_SIZE 3000

// どこにpiggyback情報を載せるか(1: tcp option, 2: footer)
#define PIGGYBACK_POSITION 2

// forward packetをudpで送信するためのポート
#define FORWARD_PACKET_PORT 3333

// mininetのmyclass.pyの設定と合わせる
#define MTU 9000

// flow managementを行うかどうか(評価用)
#define MANAGE_FLOW 0

// global変数
extern std::string switch_name;
extern int switch_id;
extern std::unordered_map<int, std::vector<u_int8_t>> PIGGYBACK_INFO;
extern pthread_mutex_t mutex_piggyback;

#endif
