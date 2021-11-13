#include "checksum.hpp"

// (参考) https://gist.github.com/david-hoze/0c7021434796997a4ca42d7731a7073a

unsigned short recalculate_ip_checksum(Packet *pkt) {
  unsigned int count = pkt->ip_header->ihl<<2;
  unsigned long sum = 0;
  unsigned short *addr = (unsigned short *)pkt->ip_header;

  while (count > 1) {
    sum += * addr++;
    count -= 2;
  }
  //if any bytes left, pad the bytes and add
  if(count > 0) {
    sum += ((*addr)&htons(0xFF00));
  }
  //Fold sum to 16 bits: add carrier to result
  while (sum>>16) {
      sum = (sum & 0xffff) + (sum >> 16);
  }
  //one's complement
  sum = ~sum;
  return ((unsigned short)sum);
}

unsigned short recalculate_udp_checksum(Packet *pkt){
  unsigned long sum = 0;
  struct iphdr *pIph = (struct iphdr *)pkt->ip_header;
  unsigned short *ipPayload = (unsigned short *)pkt->udp_header;
  unsigned short udpLen = htons(pkt->udp_header->len);

  sum += (pIph->saddr>>16)&0xFFFF;
  sum += (pIph->saddr)&0xFFFF;
  sum += (pIph->daddr>>16)&0xFFFF;
  sum += (pIph->daddr)&0xFFFF;
  sum += htons(IPPROTO_UDP);
  sum += pkt->udp_header->len;

  while (udpLen > 1) {
    sum += * ipPayload++;
    udpLen -= 2;
  }
  if(udpLen > 0) {
    sum += ((*ipPayload)&htons(0xFF00));
  }
  while (sum>>16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }
  sum = ~sum;
  sum = ((unsigned short)sum == 0x0000)?0xFFFF:(unsigned short)sum;

  return (unsigned short)sum;
}

unsigned short recalculate_tcp_checksum(Packet *pkt){
    unsigned long sum = 0;
    struct iphdr *pIph = (struct iphdr *)pkt->ip_header;
    unsigned short *ipPayload = (unsigned short *)pkt->tcp_header;
    unsigned short tcpLen = ntohs(pIph->tot_len) - (pIph->ihl<<2);
    sum += (pIph->saddr>>16)&0xFFFF;
    sum += (pIph->saddr)&0xFFFF;
    sum += (pIph->daddr>>16)&0xFFFF;
    sum += (pIph->daddr)&0xFFFF;
    sum += htons(IPPROTO_TCP);
    sum += htons(tcpLen);

    while (tcpLen > 1) {
        sum += * ipPayload++;
        tcpLen -= 2;
    }
    if(tcpLen > 0) {
        sum += ((*ipPayload)&htons(0xFF00));
    }
    while (sum>>16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    sum = ~sum;
    return (unsigned short)sum;
}

void recaluculate_checksum(Packet *pkt) { // checksumを計算し直す
  // tcp headerのchecksumの計算
  if (pkt->ip_header->protocol == IPPROTO_TCP) {
    pkt->tcp_header->check = 0;
    pkt->tcp_header->check = recalculate_tcp_checksum(pkt);
  } else if (pkt->ip_header->protocol == IPPROTO_UDP) {
    pkt->udp_header->check = 0; // 一度checksumを初期化する
    pkt->udp_header->check = recalculate_udp_checksum(pkt);
  }

  // ip headerのchecksumの計算
  pkt->ip_header->check = 0; 
  pkt->ip_header->check = recalculate_ip_checksum(pkt);
}
