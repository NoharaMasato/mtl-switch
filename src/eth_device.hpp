#ifndef ETH_DEVICE_H_
#define ETH_DEVICE_H_

#include <iostream>
#include <string>
#include <set>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <netinet/if_ether.h>
#include <ifaddrs.h>

#include "multi_thread_socket.hpp"

enum ETHER_DEVICE_TYPE { LOOPBACK, ETHERNET, NONE };

class EthernetDevice {
  int _InitRawSocket(const char *device,int promiscFlag,int ipOnly);

 public:
  ETHER_DEVICE_TYPE device_type;
  MultiThreadSocket msoc;
  std::string device_name;

  EthernetDevice();
  void set_device(std::string name);
  friend std::ostream& operator<<(std::ostream& os, const EthernetDevice& a);
};

int get_all_ethernet_device(std::string swtich_name, EthernetDevice* devices);

#endif
