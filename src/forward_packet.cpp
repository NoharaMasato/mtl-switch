#include "forward_packet.hpp"

int send_forward_packet(std::string forward_packet_msg, std::string addr) {
  int sockfd;
  struct sockaddr_in     servaddr;

  // Creating socket file descriptor
  if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  memset(&servaddr, 0, sizeof(servaddr));

  // Filling server information
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(FORWARD_PACKET_PORT);

  if(inet_pton(AF_INET, addr.c_str(), &servaddr.sin_addr)<=0) {
    std::cerr << "Invalid address/ Address not supported" << std::endl;
    return (-1);
  }

  sendto(sockfd, forward_packet_msg.c_str(), forward_packet_msg.size(),
    MSG_CONFIRM, (const struct sockaddr *) &servaddr,
      sizeof(servaddr));
  printf("Hello message sent.\n");

  close(sockfd);
  return (0);
}

bool check_forward_packet(Packet *pkt) {
  if (pkt->is_udp && ntohs(pkt->dst_port()) == FORWARD_PACKET_PORT) {
    // forward packetである
    // ここでparseをして、自分が許可されているかどうかをチェックする(parse部分はpiggybackをparseする部分と同じで良さそう)
  }
  return true;
}
