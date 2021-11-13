#include "packet.hpp"

Tag::Tag() {
  flow_id = 0;
  is_attached = false;
  allowed_switch_ids = {};
}

std::string Tag::allowed_switch_string(){
  std::string ret = "";

  for (u_int8_t sw_id : allowed_switch_ids) {
    ret += "s" + std::to_string(sw_id) + ",";
  }
  if (ret.size() == 0) {
    ret = "none";
  } else {
    ret.pop_back();
  }
  return ret;
}

bool five_tuple::operator==(const five_tuple &other) const {
  return (src_ip == other.src_ip &&
          dst_ip == other.dst_ip && src_port == other.src_port &&
          dst_port == other.dst_port && ip_version == other.ip_version);
}

std::string five_tuple::to_string() {
  return std::string(inet_ntoa(*(struct in_addr *)&src_ip)) + ":" +
         std::string(inet_ntoa(*(struct in_addr *)&dst_ip)) + ":" +
         std::to_string(ntohs(src_port)) + ":" +
         std::to_string(ntohs(dst_port)) + ":" +
         std::to_string(ip_version);
}

Packet::Packet(u_char *packet, int len, int ip_hdr_start) {
  data_buffer = packet;
  pkt_len = len;
  header_size = ip_hdr_start + IP_HEADER_SIZE;
  is_tcp = false;

  // parse ethernet header
  ethernet_header = (struct ether_header *)packet;

  // parse ip header
  if (ntohs(ethernet_header->ether_type)==ETHERTYPE_IP) {
    ip_header = (struct iphdr *)(packet + ip_hdr_start);

    // parse tcp header
    int tcp_header_start = ip_hdr_start + IP_HEADER_SIZE;
    if (ip_header->protocol == IPPROTO_TCP) {
      is_tcp = true;
      tcp_header = (struct tcphdr *)(packet + tcp_header_start);
      payload = (const u_char *)((unsigned char *)tcp_header + (tcp_header->doff * 4));
      header_size += tcp_header->doff * 4;

      // parse tag
      // tcp header全体のバイト数: tcp_header->doff * 4 -- (1)
      // tcp headerのoptions以外の部分のバイト数: 20 -- (2)
      // optionsの長さ: (1) - (2)
      if (PIGGYBACK_POSITION == 1) {
        if (memcmp(payload - 4, "tag", 4) == 0) { // tagがついている場合
          tag.is_attached = true;
          tag.flow_id = *(short int *)(payload - 6); // sizeof("tag") + sizeof(short int)戻ったところから見ると、flow_idがわかる
          for (int i = 0;i < TAG_PIGGYBACK_SIZE; i++) {
            u_int8_t a_sw = *(u_int8_t *)(payload - TAG_SIZE + i);
            if (a_sw) { // 0じゃない場合はallowed_switchである
              tag.allowed_switch_ids.push_back(a_sw);
            }
          }
        }
      } else if (PIGGYBACK_POSITION == 2) {
        if (memcmp(data_buffer + pkt_len - 4, "tag", 4) == 0){
          tag.is_attached = true;
          tag.flow_id = *(short int *)(data_buffer + pkt_len - 6);
          for (int i = 0;i < TAG_PIGGYBACK_SIZE; i++) {
            u_int8_t a_sw = *(u_int8_t *)(data_buffer + pkt_len - TAG_SIZE + i);
            if (a_sw) {
              tag.allowed_switch_ids.push_back(a_sw);
            }
          }
        }
      }

    // parse udp header
    } else if (ip_header->protocol == IPPROTO_UDP) {
      is_udp = true;
      udp_header = (struct udphdr *)(packet + tcp_header_start);
      payload = (const u_char *)((unsigned char *)udp_header + UDP_HEADER_SIZE);
      header_size += UDP_HEADER_SIZE;
    }
  }
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

unsigned short Packet::src_port() {
  if (ip_header->protocol == IPPROTO_TCP)
    return tcp_header->th_sport;
  else if (ip_header->protocol == IPPROTO_UDP)
    return udp_header->uh_sport;
  else
    return 0;
}

unsigned short Packet::dst_port() {
  if (ip_header->protocol == IPPROTO_TCP)
    return tcp_header->th_dport;
  else if (ip_header->protocol == IPPROTO_UDP)
    return udp_header->uh_dport;
  else
    return 0;
}

void Packet::attach_tag(short int fid) {
  if (PIGGYBACK_POSITION == 1) {
    memmove(data_buffer + header_size + TAG_SIZE, data_buffer + header_size, pkt_len - header_size); // tcp payloadをtagの分だけ開けて、後ろに移動する
    memcpy(data_buffer + header_size + TAG_SIZE - 4, "tag", 4); // "tag"という文字列自体をtagの最後に目印としてつける
    memcpy(data_buffer + header_size + TAG_PIGGYBACK_SIZE, &fid, sizeof(fid));
    memset(data_buffer + header_size, 0, TAG_PIGGYBACK_SIZE); // allowed_switchの部分を0埋めする
    tcp_header->doff += TAG_SIZE/4;
    ip_header->tot_len = htons(ntohs(ip_header->tot_len) + TAG_SIZE);
  } else if (PIGGYBACK_POSITION == 2) {
    memcpy(data_buffer + pkt_len + 8, "tag", 4); // "tag"という文字列自体をtagの最後に目印としてつける
    memcpy(data_buffer + pkt_len + 6, &fid, sizeof(fid));
    memset(data_buffer + pkt_len, 0, TAG_PIGGYBACK_SIZE); // allowed_switchの部分を0埋めする
  }
  tag.is_attached = true;
  tag.flow_id = fid;
}

void Packet::add_allowed_switch_to_tag(const std::vector<u_int8_t>& allowed_switch_ids) {
  if (allowed_switch_ids.size() > TAG_PIGGYBACK_SIZE) {
    std::cerr << "piggyback buffer error, size: " << allowed_switch_ids.size() << std::endl;
    return;
  }
  for (size_t i = 1; i <= allowed_switch_ids.size(); i++) {
    if (PIGGYBACK_POSITION == 1) {
      memcpy(data_buffer + header_size + TAG_PIGGYBACK_SIZE - i, &allowed_switch_ids[i - 1], sizeof(u_int8_t)); // 後ろから順に埋めていく
    } else if (PIGGYBACK_POSITION == 2) {
      memcpy(data_buffer + pkt_len + TAG_PIGGYBACK_SIZE - i, &allowed_switch_ids[i - 1], sizeof(u_int8_t)); // 後ろから順に埋めていく
    }
  }
}

five_tuple Packet::to_five_tuple() {
  return five_tuple{ip_header->saddr, ip_header->daddr, ip_header->protocol, src_port(),
                    dst_port()};
}

void Packet::print_meta_data() {
        std::cout << "\n\n=====packet start=====\n"
                  << "packet length " << pkt_len << "\n"
                  << "src mac       " << ether_ntoa(src_ether_addr()) << "\n"
                  << "dst mac       " << ether_ntoa(dst_ether_addr()) << "\n";
    if (ntohs(ethernet_header->ether_type)==ETHERTYPE_IP) {
        std::cout << "src ip        " << inet_ntoa(*(struct in_addr *)&ip_header->saddr) << "\n"
                  << "dst ip        " << inet_ntoa(*(struct in_addr *)&ip_header->daddr) << "\n";
      if (ip_header->protocol == IPPROTO_TCP || ip_header->protocol == IPPROTO_UDP) {
        std::cout << "src port      " << ntohs(src_port()) << "\n"
                  << "dst port      " << ntohs(dst_port()) << "\n"
                  << "content       " << payload << "\n";
      }
    }
        std::cout << "=====packet end=======\n";
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

std::string Packet::to_4tuple_string() {
  return std::string(inet_ntoa(*(struct in_addr *)&(ip_header->saddr))) + ":" +
         std::string(inet_ntoa(*(struct in_addr *)&(ip_header->daddr))) + ":" +
         "*" + ":" +
         std::to_string(ntohs(dst_port())) + ":" +
         std::to_string(ip_header->protocol);
}
