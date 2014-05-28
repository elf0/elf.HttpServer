#pragma once
//License: Public Domain
//Author: elf
//EMail: elf198012@gmail.com

#include <sys/stat.h>
#include <sys/sendfile.h>

typedef struct Connection_s Connection;

struct Connection_s{
  IO_FIELDS;
  int fdSendFile;
  off_t nOffset;
  size_t nFileSize;
};

static void *ConnectionPool_Pop(TcpServer *pServer){
  Connection *pConnection = MPool_Pop(&g_poolConnections);
  pConnection->fdSendFile = -1;
  return pConnection;
}

static void ConnectionPool_Push(TcpServer *pServer, void *pConnection){
  return MPool_Push(&g_poolConnections, pConnection);
}

static inline void Connection_Close(Connection *pConnection){
    close(pConnection->fd);
    ConnectionPool_Push(null, pConnection);
}

static inline void Connection_onRead(Connection *pConnection, Byte *pBuffer, U32 nSize){
     if(pConnection->fdSendFile == -1){
       pConnection->fdSendFile = open("./www/index.html", O_RDONLY);

       if(pConnection->fdSendFile != -1){
         struct stat stFile;
         if(fstat(pConnection->fdSendFile, &stFile) != -1){
           pConnection->nOffset = 0;
           pConnection->nFileSize = stFile.st_size;
         }
       }
       else{
           Tcp_Write(pConnection->fd, (Byte*)"HTTP/1.1 404 Not Found\r\n\r\n", 26);
           Connection_Close(pConnection);
       }
     }
//     pBuffer[n] = 0;
//     puts(pBuffer);
     Tcp_Write(pConnection->fd, (Byte*)"HTTP/1.1 200 OK\r\n\r\n", 19);
}

static inline void Connection_onInputReady(Connection *pConnection){
    Byte *pBuffer = BufferPool_Pop(null);
//   if(!pBuffer)
//    fprintf(stderr, "Error: BufferPool_Pop() failed\n");
    
   int n = Tcp_Read(pConnection->fd, pBuffer, BUFFER_SIZE);
   if(n > 0){
     Connection_onRead(pConnection, pBuffer, n);
   }

   BufferPool_Push(null, pBuffer);
#ifdef DEBUG
    {
      static U32 nMaxPacket = 0;
      U32 nPacketPeak = MPool_Peak(&g_poolBuffers);
      if(nPacketPeak > nMaxPacket){
        nMaxPacket = nPacketPeak;
        fprintf(stderr, "nMaxPacket: %u\n", nMaxPacket);
      }
      
      static U32 nMaxConnection = 0;
      U32 nConnectionPeak = MPool_Peak(&g_poolConnections);
      if(nConnectionPeak > nMaxConnection){
        nMaxConnection = nConnectionPeak;
        fprintf(stderr, "nMaxConnection: %lu\n", nMaxConnection/g_poolConnections.nObjectSize);
      }
    }
#endif
}

static inline void Connection_onOutputReady(Connection *pConnection){
     if(pConnection->fdSendFile != -1){
       size_t nBytes = pConnection->nFileSize - pConnection->nOffset;
       int n;
         while(nBytes){
//RETRY:
           n = sendfile(pConnection->fd, pConnection->fdSendFile, &pConnection->nOffset, nBytes);
           if(n > 0){
//             pConnection->nOffset += n;
             nBytes -= n;
           }
           else if(n == -1){
             if(errno == EAGAIN)
               return;
//             else if(errno == EINTR)
//               goto RETRY;

             fprintf(stderr, "Error(%d): sendfile\n", errno);
             Connection_Close(pConnection);
             close(pConnection->fdSendFile);
             pConnection->fdSendFile = -1;
             return;
           }
           else{
             fprintf(stderr, "sendfile: %d, %lu\n", n, nBytes);
             Connection_Close(pConnection);
             close(pConnection->fdSendFile);
             pConnection->fdSendFile = -1;
             return;
           }
         }
       
       
         //fprintf(stderr, "Send file end\n");
         Connection_Close(pConnection);

         close(pConnection->fdSendFile);
         pConnection->fdSendFile = -1;
     }
}

static void Connection_onIoError(Connection *pConnection, U32 nError){
  fprintf(stderr, "Error: %d\n", nError);
}
