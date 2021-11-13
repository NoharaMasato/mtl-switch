#ifndef REDIS_H_
#define REDIS_H_

#include <acl_cpp/lib_acl.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <queue>
#include <chrono>
#include <vector>
#include <queue>

#include "config.hpp"
#include "log.hpp"

// 制限時間付きqueue
class TimeQueue {
public:
  std::queue<std::pair<std::string, std::chrono::system_clock::time_point>> q;
  int delta; // 制限時間

  TimeQueue(int d) {
    if (d == 0) {// timeoutしない
      delta = 10000000;
    } else {
      delta = d;
    }
  }

  void pop_timeout() {
    std::chrono::system_clock::time_point now;
    now = std::chrono::system_clock::now();
    while (q.size() != 0 && static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(now-q.front().second).count()/1000000.0) >= delta) {
      q.pop();
    }
  }

  int size() {
    pop_timeout();
    return q.size();
  }

  void pop() {
    // pop_timeout();
    q.pop();
  }

  void push(std::string s) {
    std::chrono::system_clock::time_point now;
    now = std::chrono::system_clock::now();
    q.push({s, now});
  }

  std::string front() {
    // pop_timeout();
    return q.front().first;
  }
};

class RedisCache {
protected:
  pthread_mutex_t mutex;
  acl::redis_client conn;
  acl::redis cmd;

public:
  RedisCache() : conn(REDIS_HOST, 10, 10), cmd(&conn) {
    pthread_mutex_init(&mutex, NULL);
  }

  std::string select(std::string key) {
    pthread_mutex_lock(&mutex);

    acl::string value;
    std::string redis_key;

    redis_key = switch_name + ":" + key;
    if (cmd.get(redis_key.c_str(), value) == false) {
      // redisに通信ができない場合はここにくる。一方、keyが存在しない場合は空の文字列が返される
      std::cerr << "[redis get key error] key: " << redis_key.c_str() << std::endl;
    }
    pthread_mutex_unlock(&mutex);
    return value.c_str();
  }
};

class SetAssociativeCache : public RedisCache {
  int size, way;
  int slashing_cnt;
  std::vector<std::queue<int>> keys;
  // cacheに保持できるkeyの数はsize * way個になる

public:
  SetAssociativeCache() {
    if (switch_name == "s1") {
      size = MAX_FLOW_ENTRY_CNT_UPSTREAM;
    } else {
      size = MAX_FLOW_ENTRY_CNT_DOWNSTREAM;
    }
    way = 2; // 2-way associative cache
    slashing_cnt = 0;
    keys.assign(size, std::queue<int>());
  }

  void insert(int key, std::string value){
    std::string redis_key;

    redis_key = switch_name + ":" + std::to_string(key);
    MYCOUT << "key: " << redis_key << ", value: " << value;

    pthread_mutex_lock(&mutex);

    // この部分があるかないかでvcrmの方のcurlの結果が変わる
    int idx = hash(key);
    if ((int)keys[idx].size() >= way) { // いっぱいなので、消す場合
      std::cout << "slashing, cnt: " << slashing_cnt <<", " << keys[idx].front() << std::endl;
      if (cmd.del_one((switch_name + ":" + std::to_string(keys[idx].front())).c_str()) < 0) {
        printf("redis delete error\n");
      }
      keys[idx].pop();
      slashing_cnt++;
    }
    keys[idx].push(key);

    if (cmd.set(redis_key.c_str(), value.c_str()) == false) {
      std::cerr << "redis set error" << std::endl;
    }

    if (REDIS_EXPIRE_SEC) {
      if (cmd.expire(redis_key.c_str(), REDIS_EXPIRE_SEC) < 0) {
        printf("expire key: %s error: %s\r\n", std::to_string(key).c_str(), cmd.result_error());
      }
    }
    pthread_mutex_unlock(&mutex);

    value.clear();
  }

  int hash(int key) {
    return key % size;
  }
};

class SimpleCache : public RedisCache {
public:
  void insert(int key, std::string value){
    std::string redis_key;

    redis_key = switch_name + ":" + std::to_string(key);
    MYCOUT << "key: " << redis_key << ", value: " << value;

    pthread_mutex_lock(&mutex);

    if (cmd.set(redis_key.c_str(), value.c_str()) == false) {
      std::cerr << "redis set error" << std::endl;
    }

    if (REDIS_EXPIRE_SEC) {
      if (cmd.expire(redis_key.c_str(), REDIS_EXPIRE_SEC) < 0) {
        printf("expire key: %s error: %s\r\n", std::to_string(key).c_str(), cmd.result_error());
      }
    }

    pthread_mutex_unlock(&mutex);

    value.clear();
  }
};

void redis_init();
void insert_flow_table(int flow_id, std::string value);
std::string select_string(std::string key);

#endif
