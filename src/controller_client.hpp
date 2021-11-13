#ifndef CONTROLLER_CLIENT_H_
#define CONTROLLER_CLIENT_H_

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

#include <string>

#include "config.hpp"
#include "get_next_line.h"
#include "response.hpp"
#include "log.hpp"
#include "redis.hpp"

void send_message(const std::string& message);
std::string receive_message();
int init_socket(std::string controller_ip);
void *cache_injection(void *);

#endif
