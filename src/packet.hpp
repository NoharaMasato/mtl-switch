#ifndef PACKET_H_
#define PACKET_H_

#include <string.h>

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <iostream>
#include <string>
#include <vector>

class Packet; // checksumとpacketがお互いに依存しあっているので、ここにprototype宣言が必要になる
#include "checksum.hpp"
#include "config.hpp"

#define ETHERNET_HEADER_SIZE 14
#define IP_HEADER_SIZE 20
#define UDP_HEADER_SIZE 8

#define TAG_PIGGYBACK_SIZE 6
#define TAG_SIZE 12 // 目印: "tag", 4byte, flowID 2byte, allowedswitches 1byte * 6

struct five_tuple {
  uint32_t src_ip, dst_ip;
  unsigned int ip_version;
  unsigned short src_port, dst_port;
  bool operator==(const five_tuple &other) const;
  std::string to_string();
};

template <>
struct std::hash<five_tuple> {
  std::size_t operator()(const five_tuple &k) const {
    return (size_t)(k.src_ip ^ k.dst_ip ^ k.src_port ^
                    k.dst_port);
  }
};

class Tag {
public:
  bool is_attached;
  short int flow_id;
  std::vector<u_int8_t> allowed_switch_ids;
  Tag();
  std::string allowed_switch_string();
};

class Packet {
  int pkt_len, header_size; // header_sizeはheader全体のサイズ
  u_char *data_buffer;

  struct ether_header *ethernet_header;
 public:
  // constractor
  Packet(u_char *packet, int len, int ip_hdr_start);

  // data link layer
  struct ether_addr *src_ether_addr(), *dst_ether_addr();
  long long src_ether_addr_ll();
  long long dst_ether_addr_ll();

  // ip layer
  struct iphdr *ip_header;

  // transport layer
  unsigned short src_port(), dst_port();
  struct tcphdr *tcp_header;
  struct udphdr *udp_header;
  bool is_tcp, is_udp;

  // app layer
  const u_char *payload;

  // return five tuple
  five_tuple to_five_tuple();

  // tagはpiggybackしたもの全体(目印のtagという文字列は除く）で、flow_idはその中のidの部分
  Tag tag;
  void attach_tag(short int flow_id); // flowidをtagとしてつける
  void add_allowed_switch_to_tag(const std::vector<u_int8_t>&); // piggybackする用

  // create new packet
  void create_new_packet_with_tag(u_char *buf, int tag);

  // print packet
  void print_meta_data();
  void print_row_data();

  // 送信元ポートはなんでも良い
  std::string to_4tuple_string();
};

typedef struct packet_buf {
  int size;
  int port_idx;
  u_char buf[MTU];
  bool used;
} packet_buf;

#endif
