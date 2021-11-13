#ifndef RESPONSE_H_
#define RESPONSE_H_

#include <string>
#include <vector>
#include <iostream>
#include <sys/socket.h>
#include <stdint.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>

using namespace boost::property_tree;

/*
Responseの仕様
{
  "Data": {
    "switch_id": "5",      # リクエストを送ったswitch
    "flow_id": 1           # 尋ねられたflow_id
    "action": "0",         # リクエストを送ったswitchがとるaction(0: forward, 1: drop) switch側は4: piggybackも使う
    "injected": "0",       # キャッシュインジェクションかどうか
    "piggyback": "0",      # piggybackをするかどうか(piggybackとinjectedが両方1になることはない)
    "allowed_swith_ids": [ # piggybackが1の時に、piggybackでつける番号
      "1", "2"
    ]
  }
}
*/

class Response {
public:
  std::string action;
  u_int8_t sw_id;
  bool injected, piggyback;
  short flow_id;
  std::vector<u_int8_t> allowed_switch_ids;

  void set(u_int8_t sw_id, short flow_id, std::string action, bool injected, bool piggyback, std::vector<u_int8_t> allowed_switch_ids);

  std::string to_string(); // Response型をjsonのstringにして返す
  static Response parse_string(std::string str); // string型のjsonをResponse型にして返す
  void send_response(int sd);
  void print_response();
  std::string allowed_switch_string();
};

#endif
