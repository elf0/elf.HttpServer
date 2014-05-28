//License: Public Domain
//Author: elf
//EMail: elf198012@gmail.com
/*
bench:
  ab -n 100000 -c 100 http://127.0.0.1:8000/
  Document Length: 132 bytes
  21417.99 rps
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include "MPool.h"
#include "TcpServer.h"
#include "Tcp.h"
#include "Epoll.h"

#define MAX_CONNECTIONS 10000
#define BUFFER_SIZE 65536
#define MAX_OBJECT_SIZE 4096

static Memory g_mBuffers;
static Memory g_mObjects;

static MPool g_poolBuffers;
static MPool g_poolConnections;

static inline Bool InitializeMemories(){
    U32 nBufferSize = MAX_CONNECTIONS * BUFFER_SIZE;
    Byte *pBegin = Memory_Allocate(&g_mBuffers, nBufferSize);//malloc(nBufferSize);
    MPool_Initialize(&g_poolBuffers, pBegin, pBegin + nBufferSize, MAX_CONNECTIONS, BUFFER_SIZE);

    nBufferSize = MAX_CONNECTIONS * MAX_OBJECT_SIZE;
    pBegin = Memory_Allocate(&g_mObjects, nBufferSize);//malloc(nBufferSize);
    MPool_Initialize(&g_poolConnections, pBegin, pBegin + nBufferSize, MAX_CONNECTIONS, MAX_OBJECT_SIZE);
    return true;
}

static inline void FinalizeMemories(){
    Memory_Free(&g_mBuffers);
    Memory_Free(&g_mObjects);
}

static void *BufferPool_Pop(TcpServer *pServer){
    return MPool_Pop(&g_poolBuffers);
}

static void BufferPool_Push(TcpServer *pServer, void *pPacket){
    MPool_Push(&g_poolBuffers, pPacket);
}

#include "Connection.h"

static void Server_onIoError(TcpServer *pServer, U32 nError){
    //  perror("Error");
    fprintf(stderr, "Error: %d\n", nError);
}

static inline Bool Filter(TcpServer *pServer, U32 nIp, U16 nPort){
    //fprintf(stderr, "Address: %u.%u.%u.%u: %u\n", nIp>>24, nIp >> 16 & 0xFF, nIp >> 8 & 0xFF, nIp & 0xFF, nPort);
    return false;
}



static void onConnectionIosReady(Connection *pConnection, U32 events){
    if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
        U32 nError = errno;
        close(pConnection->fd);
        ConnectionPool_Push(null, pConnection);

        if(events & EPOLLRDHUP){//connection closed by peer
            //fprintf(stderr, "Info: Disconnected by peer\n");
        }
        else{
            pConnection->onError((IoObject*)pConnection, nError);
            fprintf(stderr, "Error: onConnectionIoReady(%X, %u)\n", events, nError);
        }
        return;
    }

    if(events & EPOLLIN){
        Connection_onInputReady(pConnection);
    }

    if(events & EPOLLOUT){
        Connection_onOutputReady(pConnection);
    }
}

static TcpServer g_server;

int main(int argc, char *argv[]){
    InitializeMemories();

    int fdEpoll = Epoll_Open();
    if(fdEpoll == -1){
        fprintf(stderr, "Error: Open epoll failed!\n");
        return 2;
    }


    TcpServer_Initialize(&g_server, fdEpoll, NULL, ConnectionPool_Pop,
                         ConnectionPool_Push, BufferPool_Pop, BufferPool_Push,
                         Filter,
                         (IoErrorHandler)Server_onIoError,
                         (IosReadyHandler)onConnectionIosReady, (IoReadyHandler)Connection_onInputReady, (IoReadyHandler)Connection_onOutputReady, (IoErrorHandler)Connection_onIoError);

    int r = TcpServer_Start(&g_server, 8000);
    if(r < 0){
        fprintf(stderr, "Error(%d): Start server failed!\n", r);
        Epoll_Close(fdEpoll);
        return 3;
    }



    Epoll_Loop(fdEpoll);

    TcpServer_Stop(&g_server);
    Epoll_Close(fdEpoll);
    FinalizeMemories();
    return 0;
}
