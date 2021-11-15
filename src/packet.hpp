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

#define ETHERNET_HEADER_SIZE 14
#define IP_HEADER_SIZE 20
#define UDP_HEADER_SIZE 8

#define MTU 1500

class Packet {
  int pkt_len, header_size; // header_sizeはheader全体のサイズ
  u_char *data_buffer;

  struct ether_header *ethernet_header;

public:
  Packet(u_char *packet, int len, int ip_hdr_start);

  // data link layer
  struct ether_addr *src_ether_addr(), *dst_ether_addr();
  long long src_ether_addr_ll();
  long long dst_ether_addr_ll();

  // print packet
  void print_meta_data();
  void print_row_data();
};

typedef struct packet_buf {
  int size;
  int port_idx;
  u_char buf[MTU];
  bool used;
} packet_buf;

#endif
