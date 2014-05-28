#pragma once
//License: Public Domain
//Author: elf
//EMail: elf198012@gmail.com

#include "IoObject.h"

#include <sys/epoll.h>

static inline int Epoll_Open(){
  return epoll_create1(0);
}

static inline void Epoll_Close(int fd){
  close(fd);
}

static inline Bool Epoll_Add(int fdEpoll, IoObject *pObject){
  struct epoll_event event;
  event.data.ptr = pObject;
  event.events = EPOLLET;
  if(pObject->onInputReady)
    event.events |= EPOLLIN | EPOLLRDHUP;

  if(pObject->onOutputReady)
    event.events |= EPOLLOUT;

  return epoll_ctl(fdEpoll, EPOLL_CTL_ADD, pObject->fd, &event) == 0;
}

static inline void Epoll_Loop(int fdEpoll){
#define EVENTS 256
  IoObject *pObject;
  struct epoll_event events[EVENTS];
  while(true){
    int n = epoll_wait(fdEpoll, events, EVENTS, -1);
    if(n == -1){
      if(errno == EINTR)
        continue;

      perror("epoll_wait");
      return;
    }

    int i;
    for(i = 0; i < n; ++i){
      pObject = (IoObject*)events[i].data.ptr;
      pObject->onIosReady(pObject, events[i].events);
    }
  }
}
