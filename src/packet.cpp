#include "packet.hpp"

Packet::Packet(u_char *packet, int len, int ip_hdr_start) {
  data_buffer = packet;
  pkt_len = len;
  header_size = ip_hdr_start + IP_HEADER_SIZE;
  is_tcp = false;

  // parse ethernet header
  ethernet_header = (struct ether_header *)packet;
}

struct ether_addr *Packet::src_ether_addr() {
  return (struct ether_addr *)&(ethernet_header->ether_shost);
}

struct ether_addr *Packet::dst_ether_addr() {
  return (struct ether_addr *)&(ethernet_header->ether_dhost);
}

long long Packet::src_ether_addr_ll() {
  long long ret = 0;

  ret += ((long long)ethernet_header->ether_shost[0] << 40);
  ret += ((long long)ethernet_header->ether_shost[1] << 32);
  ret += ((long long)ethernet_header->ether_shost[2] << 24);
  ret += ((long long)ethernet_header->ether_shost[3] << 16);
  ret += ((long long)ethernet_header->ether_shost[4] << 8);
  ret += ethernet_header->ether_shost[5];
  return ret;
}

long long Packet::dst_ether_addr_ll() {
  long long ret = 0;

  ret += ((long long)ethernet_header->ether_dhost[0] << 40);
  ret += ((long long)ethernet_header->ether_dhost[1] << 32);
  ret += ((long long)ethernet_header->ether_dhost[2] << 24);
  ret += ((long long)ethernet_header->ether_dhost[3] << 16);
  ret += ((long long)ethernet_header->ether_dhost[4] << 8);
  ret += ethernet_header->ether_dhost[5];
  return ret;
}

void Packet::print_meta_data() {
        std::cout << "\n\n=====packet start=====\n"
                  << "packet length " << pkt_len << "\n"
                  << "src mac       " << ether_ntoa(src_ether_addr()) << "\n"
                  << "dst mac       " << ether_ntoa(dst_ether_addr()) << "\n"
                  << "=====packet end=======\n";
}

void Packet::print_row_data() {
  for (int i(0); i < pkt_len;) {
    printf("%02x ", (int)(data_buffer[i]));
    i++;
    if (i % 16 == 0)
      std::cout << std::endl;
    else if (i % 8 == 0)
      std::cout << " ";
  }
  std::cout << "\n\n";
}
