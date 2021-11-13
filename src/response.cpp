#include "response.hpp"

void Response::set(u_int8_t _sw_id, short int _flow_id, std::string _action,
  bool _injected, bool _piggyback,
  std::vector<u_int8_t> _allowed_switch_ids) {
  sw_id = _sw_id;
  flow_id = _flow_id;
  action = _action;
  injected = _injected;
  piggyback = _piggyback;
  allowed_switch_ids = _allowed_switch_ids;
}

std::string Response::to_string() {
  ptree pt;
  std::stringstream ss;

  pt.put("Data.switch_id", sw_id);
  pt.put("Data.flow_id", flow_id);
  pt.put("Data.action", action);
  pt.put("Data.injected", std::to_string(injected));
  pt.put("Data.piggyback", std::to_string(piggyback));

  ptree child;
  for (u_int8_t a_sw_id : allowed_switch_ids) {
    ptree tmp;
    tmp.put("", a_sw_id);
    child.push_back(std::make_pair("", tmp));
  }
  if (allowed_switch_ids.size() == 0) { // 一つもallowed_switchesがないときは、空の配列を入れる（もう少しいい方法がない？）
    ptree tmp;
    tmp.put("", "");
    child.push_back(std::make_pair("", tmp));
  }

  pt.add_child("Data.allowed_switch_ids", child);

  json_parser::write_json(ss, pt, false);

  return ss.str();
}

Response Response::parse_string(std::string str) {
  ptree pt;
  std::istringstream is(str);
  std::vector<u_int8_t> a_sw;
  Response res;

  read_json(is, pt);
  res.sw_id = stoi(pt.get<std::string>("Data.switch_id"));
  res.flow_id = stoi(pt.get<std::string>("Data.flow_id"));
  res.action = pt.get<std::string>("Data.action");
  res.injected = stoi(pt.get<std::string>("Data.injected"));
  res.piggyback= stoi(pt.get<std::string>("Data.piggyback"));

  for (auto& sw : pt.get_child("Data.allowed_switch_ids")) {
    std::string tmp = sw.second.get_value<std::string>();
    if (tmp.size()) {
      a_sw.push_back(stoi(tmp));
    }
  }
  res.allowed_switch_ids = a_sw;

  if (res.piggyback) res.action = "4"; // controllerからの返答は常にforwardか、dropのため、switch側で、piggybackを変更する

  return res;
}

void Response::send_response(int sd) {
  std::string res = to_string();

  send(sd , res.c_str() ,res.size() , 0);
  std::cout << res << std::flush;
}

std::string Response::allowed_switch_string() {
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
