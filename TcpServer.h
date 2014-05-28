#pragma once
//License: Public Domain
//Author: elf
//EMail: elf198012@gmail.com

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "Tcp.h"
#include "Epoll.h"


typedef struct TcpServer_s TcpServer;

typedef void *(*TcpServer_Pool_New)(TcpServer *pServer);
typedef void (*TcpServer_Pool_Free)(TcpServer *pServer, void *pBuffer);

typedef Bool (*TcpServer_Filter)(TcpServer *pServer, U32 nIp, U16 nPort);

struct TcpServer_s{
 IO_FIELDS;
 int fdEpoll;
// struct sockaddr_in address;
 void *pContext;
 
 TcpServer_Pool_New pfNewConnection;
 TcpServer_Pool_Free pfFreeConnection;
 TcpServer_Pool_New pfNewBuffer;
 TcpServer_Pool_Free pfFreeBuffer;

 TcpServer_Filter Filter;
 IoObject iooConnectionTemplate;
};

static inline void TcpServer_onInputReady(TcpServer *pServer);
static inline  void TcpServer_onIosReady(TcpServer *pServer, U32 events);

static inline void TcpServer_Initialize(TcpServer *pServer, int fdEpoll, void *pContext, TcpServer_Pool_New pfNewConnection,
    TcpServer_Pool_Free pfFreeConnection, TcpServer_Pool_New pfNewBuffer, TcpServer_Pool_Free pfFreeBuffer,
    TcpServer_Filter Filter,
    IoErrorHandler onServerError,
    IosReadyHandler onConnectionIosReady, IoReadyHandler onConnectionInputReady, IoReadyHandler onConnectionOutputReady, IoErrorHandler onConnectionError){
  pServer->fdEpoll = fdEpoll;
  pServer->pContext = pContext;
  pServer->pfNewConnection = pfNewConnection;
  pServer->pfFreeConnection = pfFreeConnection;
  pServer->pfNewBuffer = pfNewBuffer;
  pServer->pfFreeBuffer = pfFreeBuffer;
  pServer->Filter = Filter;
  IoObject_Initialize((IoObject*)pServer, -1, (IosReadyHandler)TcpServer_onIosReady, (IoReadyHandler)TcpServer_onInputReady, null, (IoErrorHandler)onServerError);
  IoObject_Initialize(&pServer->iooConnectionTemplate, -1, (IosReadyHandler)onConnectionIosReady,
                      (IoReadyHandler)onConnectionInputReady, (IoReadyHandler)onConnectionOutputReady, onConnectionError);
}

static inline void TcpServer_Finilize(TcpServer *pServer){
}

static inline IoObject *TcpServer_NewConnection(TcpServer *pServer){
  return pServer->pfNewConnection(pServer->pContext);
}

static inline void TcpServer_FreeConnection(TcpServer *pServer, IoObject *pConnection){
  return pServer->pfFreeConnection(pServer->pContext, pConnection);
}

static inline Byte *TcpServer_NewBuffer(TcpServer *pServer){
  return pServer->pfNewBuffer(pServer->pContext);
}

static inline void TcpServer_FreeBuffer(TcpServer *pServer, Byte *pBuffer){
  return pServer->pfFreeBuffer(pServer->pContext, pBuffer);
}

static inline Bool TcpServer_onConnection(TcpServer *pServer, int fdConnection){
  IoObject *pConnection = TcpServer_NewConnection(pServer);
  if(pConnection){
    IoObject_Initialize(pConnection, fdConnection, pServer->iooConnectionTemplate.onIosReady,
                        pServer->iooConnectionTemplate.onInputReady, pServer->iooConnectionTemplate.onOutputReady,
                        pServer->iooConnectionTemplate.onError);
    if(!Epoll_Add(pServer->fdEpoll, pConnection)){
      TcpServer_FreeConnection(pServer, pConnection);
      fprintf(stderr, "Error: add connection to epoll failed!\n");
      return false;
    }
  }
  else{
    fprintf(stderr, "Error: Allocate connection failed!\n");
    return false;
  }

  return true;
}

static inline Bool TcpServer_Accept(TcpServer *pServer, struct sockaddr_in *pAddress, socklen_t *pnAddressSize){
  int fdServer = pServer->fd;
  *pnAddressSize  = sizeof(struct sockaddr_in);
  int fdConnection = accept4(fdServer, (struct sockaddr*)pAddress, pnAddressSize, SOCK_NONBLOCK);
  if(fdConnection >= 0){
    if(pServer->Filter(pServer, ntohl(pAddress->sin_addr.s_addr), ntohs(pAddress->sin_port)) ||
    !TcpServer_onConnection(pServer, fdConnection))
      Tcp_Close(fdConnection);
    return true;
  }
  else{
    if(errno == EAGAIN)
      return false;
    else if(errno == EINTR)
      return true;
    else if(errno == EMFILE){
#ifdef DEBUG
      static U32 s_nTimes = 0;
      fprintf(stderr, "Error: Reach max files, %u\n", ++s_nTimes);
#endif
      return false;
    }
    else{
      U32 nError = errno;
      Tcp_Close(fdServer);
      pServer->onError((IoObject*)pServer, nError);
      return false;
    }
  }
}

static inline void TcpServer_onInputReady(TcpServer *pServer){
 struct sockaddr_in addr;
 socklen_t nAddr;
 while(TcpServer_Accept(pServer, &addr, &nAddr));
}

static inline  void TcpServer_onIosReady(TcpServer *pServer, U32 events){
  if(events & (EPOLLHUP | EPOLLERR)){
    U32 nError = errno;
    Tcp_Close(pServer->fd);
    pServer->onError((IoObject*)pServer, nError);
  }
  else{

#ifdef DEBUG
    if(events != EPOLLIN)
      fprintf(stderr, "Error(%d): events != EPOLLIN\n", events);
#endif

    TcpServer_onInputReady(pServer);
  }
}

static inline int TcpServer_Start(TcpServer *pServer, U16 nPort){
  int fd = Tcp_Open();
  if(fd == -1)
    return -1;

  if(!Tcp_ReuseAddress(fd)){
    Tcp_Close(fd);
    return -2;
  }

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htons(INADDR_ANY);
  address.sin_port = htons(nPort);
  if(bind(fd, (struct sockaddr*)&address, sizeof(address))){
    Tcp_Close(fd);
    return -3;
  }

  if(listen(fd, SOMAXCONN)){
    Tcp_Close(fd);
    return -4;
  }

  pServer->fd = fd;
  if(!Epoll_Add(pServer->fdEpoll, (IoObject*)pServer)){
   fprintf(stderr, "Error: Add server to epoll failed!\n");
   Tcp_Close(fd);
   return -5;
 }
 return 0;
}

static inline void TcpServer_Stop(TcpServer *pServer){
 Tcp_Close(pServer->fd);
}
