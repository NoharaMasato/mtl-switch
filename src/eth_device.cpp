#include "eth_device.hpp"

EthernetDevice::EthernetDevice() {
  msoc.sd = 0;
}

void EthernetDevice::set_device(std::string name) {
  device_name = name;
  device_type = ETHERNET;
  msoc.sd = _InitRawSocket(name.c_str(), 1, 0);
}

std::ostream& operator<<(std::ostream& os, const EthernetDevice& a) {
  os << "device name [" << a.device_name << "]" << std::endl;
  return os;
}

// private method
int EthernetDevice::_InitRawSocket(const char *device,int promiscFlag,int ipOnly)
{
  struct ifreq	ifreq;
  struct sockaddr_ll	sa;
  int	soc;

  if(ipOnly){ // ipOnlyはipパケットだけを取得するという意味？？
    if((soc=socket(PF_PACKET,SOCK_RAW,htons(ETH_P_IP)))<0){
      std::cerr << "socket" << std::endl;
      return(-1);
    }
  }
  else{
    if((soc=socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL)))<0){
      std::cerr << "socket" << std::endl;
      return(-1);
    }
  }

  memset(&ifreq,0,sizeof(struct ifreq)); // この辺はstring.hというlibcからincludeしているが、C++の場合はどう書くのか調査しても良さそう
  strncpy(ifreq.ifr_name,device,sizeof(ifreq.ifr_name)-1);
  if(ioctl(soc,SIOCGIFINDEX,&ifreq)<0){
    std::cerr << "ioctl" << std::endl;
    close(soc);
    return(-1);
  }
  sa.sll_family=PF_PACKET;
  if(ipOnly){
    sa.sll_protocol=htons(ETH_P_IP);
  }
  else{
    sa.sll_protocol=htons(ETH_P_ALL);
  }
  sa.sll_ifindex=ifreq.ifr_ifindex;
  if(bind(soc,(struct sockaddr *)&sa,sizeof(sa))<0){
    std::cerr << "bind" << std::endl;
    close(soc);
    return(-1);
  }

  if(promiscFlag){
    if(ioctl(soc,SIOCGIFFLAGS,&ifreq)<0){
      std::cerr << "ioctl" << std::endl;
      close(soc);
      return(-1);
    }
    ifreq.ifr_flags=ifreq.ifr_flags|IFF_PROMISC;
    if(ioctl(soc,SIOCSIFFLAGS,&ifreq)<0){
      std::cerr << "ioctl" << std::endl;
      close(soc);
      return(-1);
    }
  }

  return(soc);
}

// 全てのdeviceをループで見て、nicの名前にswitch_nameが含まれているものだけを返す
int get_all_ethernet_device(std::string swtich_name, EthernetDevice* devices) {
  int itr = 0;
  struct ifaddrs *ifaddr;

  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    exit(1);
  }

  std::set<std::string> used;

  // このループではprotocl familyごとにinterfaceが区別されているが、switchが持てるfamilyはAF_PACKETしかないため問題ない
  for (ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL)
      continue;

    if (((std::string)ifa->ifa_name).find(swtich_name) != std::string::npos  // switch_nameが含まれていたら(switchをネームスペースに入れるのであれば必要ない)
      && ((std::string)ifa->ifa_name).find("10000") == std::string::npos // IPを持つswitchのインターフェースは[switch名]-eth10000としておく
      && used.find((std::string)ifa->ifa_name) == used.end()) {
      devices[itr].set_device(ifa->ifa_name);
      used.insert((std::string)ifa->ifa_name);
      std::cout << ifa->ifa_name << ", ";
      if (devices[itr].msoc.sd == -1) {
        std::cerr << "socket " << devices[itr].device_name << " initialization falid" << std::endl;
        exit(1);
      }
      itr++;
    }
  }
  return (itr);
}
