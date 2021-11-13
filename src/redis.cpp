#include "redis.hpp"

// 使うキャッシュの種類を設定
// SetAssociativeCache cache;
SimpleCache cache;

void insert_flow_table(int flow_id, std::string value){
  cache.insert(flow_id, value);
}

std::string select_string(std::string key) {
  return cache.select(key);
}
