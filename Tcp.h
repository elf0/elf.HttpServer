#pragma once
//License: Public Domain
//Author: elf
//EMail: elf198012@gmail.com

#include "Type.h"

#include <unistd.h>

static inline int Tcp_Open(){
  return socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
}

static inline void Tcp_Close(int fd){
  close(fd);
}

static inline Bool Tcp_ReuseAddress(int fd){
  int on = 1;
  return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == 0;
}

static inline int Tcp_Read(int fd, Byte *pBuffer, U32 nSize){
  int n;
RETRY:
  n = recv(fd, pBuffer, nSize, 0);
  if(n > 0){
  //  Tcp_onReceived(pBuffer, n);
    return n;
  }

  if(n == 0){
#ifdef DEBUG
  fprintf(stderr, "recv() == 0\n");
#endif
    close(fd);
//    Tcp_onDisconnected();
    return -1;
  }
  
#ifdef DEBUG
  fprintf(stderr, "Error(%d): recv() < 0\n", errno);
#endif

  if(errno == EAGAIN)//no data/time out
    return 0;
  else if(errno == EINTR)
    goto RETRY;

  return -2;
}

static inline int Tcp_Write(int fd, Byte *pBuffer, U32 nSize){
  int n;
RETRY:
  n = send(fd, pBuffer, nSize, 0);
  if(n > 0){
//    Tcp_onSent(pBuffer, n);
    return n;
  }

  if(n == 0){
#ifdef DEBUG
  fprintf(stderr, "send() == 0\n");
#endif
    close(fd);
  //  Tcp_onDisconnected();
    return -1;
  }
  
#ifdef DEBUG
  fprintf(stderr, "Error(%d): send() < 0\n", errno);
#endif

  if(errno == EAGAIN)
    return 0;
  else if(errno == EINTR)
    goto RETRY;

  return -2;
}
