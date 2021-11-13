#ifndef LOG_H_
#define LOG_H_

#include <iostream>
#include "config.hpp"

class Log {
  std::string funcName;
public:
  Log(const std::string &fName){
    funcName = fName;
    if (PRINT_DEBUG == 1) {
      std::cout << funcName << ": ";
    }
  }

  template <class T>
  Log &operator<<(const T &v){
    // 関数名で場合分けすると出力する場所を分けられる ex.) funcName == "cache_injection"
    if (PRINT_DEBUG == 1) { // ifdefにすると、warningが大量に出るため、ifにしておく 
      std::cout << v;
    }
    return *this;
  }

  ~Log(){
    if (PRINT_DEBUG == 1) {
      std::cout << std::endl;
    }
  }
};

#define MYCOUT Log(__FUNCTION__)

#endif
