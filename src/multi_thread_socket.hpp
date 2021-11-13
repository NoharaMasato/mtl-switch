#ifndef MULTI_THREAD_SOCKET_H_
#define MULTI_THREAD_SOCKET_H_

#include <unistd.h>
#include "pthread.h"

#include "config.hpp"

class MultiThreadSocket {
  pthread_mutex_t mutex;

public:
  int sd;

  MultiThreadSocket() {
    pthread_mutex_init(&mutex, NULL);
    sd = 0;
  }

  ~MultiThreadSocket() {
    close(sd);
  }

  int mread(u_char *buf) {
    int ret_size;

    pthread_mutex_lock(&mutex);
    ret_size = read(sd, buf, MTU);
    pthread_mutex_unlock(&mutex);

    if(ret_size <= 0) {
      perror("read");
    }
    return ret_size;
  }

  int mwrite(u_char *buf, int size) {
    int ret_size;

    pthread_mutex_lock(&mutex);
    ret_size = write(sd, buf, size);
    pthread_mutex_unlock(&mutex);

    if(ret_size <= 0) {
      perror("write");
    }
    return ret_size;
  }
};

#endif
