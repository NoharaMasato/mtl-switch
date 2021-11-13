#include "controller_client.hpp"

int sock;

int init_socket(std::string controller_ip) {
  struct sockaddr_in serv_addr;
  std::string hello_message;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    std::cerr << "Socket creation error" << std::endl;
    return (-1);
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(CONTROLLER_PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, controller_ip.c_str(), &serv_addr.sin_addr) <= 0) {
    std::cerr << "Invalid address/ Address not supported" << std::endl;
    return (-1);
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    std::cerr << "Connection Failed" << std::endl;
    return (-1);
  }
  // hello messageをswitch_nameと同時に送信する
  hello_message = "hello" + std::to_string(switch_id);
  if (send(sock , hello_message.c_str(), hello_message.size(), 0) < 0) {
    std::cerr << "Controller hello message failed" << std::endl;
  }
  return (0);
}

void send_message(const std::string& message) {
  if (send(sock , message.c_str() , message.size() , 0) < 0) {
    std::cerr << "request to controller failed. message: " << message << std::endl;
  }
}


// [TODO] readをcontrollerからの応答を待ち、処理をブロックしても、パケットを処理するスレッドとは別スレッドなので問題はないが、readはなぜが、non-blockingになる。
// non-blockingだと、controllerからのメッセージを待つloopが大量にloopしてしまうが、今のところはそんなに問題はなさそう
// コントローラからのメッセージの受け取りは送信と別スレッドで行う（キャッシュインジェクションにも対応するため）
// https://stackoverflow.com/questions/12773509/read-is-not-blocking-in-socket-programming　（このページが参考になりそう）
void *cache_injection(void *) {
  Response res;
  std::string action, tag, injected;
  char *buf;

  while (true) {
    get_next_line(sock, &buf);
    if (strlen(buf)) {
      res = Response::parse_string(buf);
      insert_flow_table(res.flow_id, res.action);

      if (res.action == "4") { // piggybackするという指示であれば
        pthread_mutex_lock(&mutex_piggyback);
        PIGGYBACK_INFO[res.flow_id] = res.allowed_switch_ids;
        pthread_mutex_unlock(&mutex_piggyback);
      }

      MYCOUT << "Response from controller about flow_id: " << res.flow_id
             << ", action: " << res.action
             << ", injection: " << (res.injected? "true":"false")
             << ", piggyback: " << (res.piggyback? "true":"false")
             << ", allowed_switches: " << res.allowed_switch_string();
    } else {
      // ここが何回も回る
    }
    free(buf);
  }
}
