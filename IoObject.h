#pragma once
//License: Public Domain
//Author: elf
//EMail: elf198012@gmail.com

#include "Type.h"

typedef struct IoObject_s IoObject;

typedef void (*IosReadyHandler)(IoObject *pObject, U32 events);
typedef void (*IoReadyHandler)(IoObject *pObject);
typedef void (*IoErrorHandler)(IoObject *pObject, U32 nError);
typedef Bool (*IoConnectionHandler)(IoObject *pObject, int fd);

#define IO_EVENT_HANDLERS \
    IosReadyHandler onIosReady; \
    IoReadyHandler onInputReady; \
    IoReadyHandler onOutputReady; \
    IoErrorHandler onError

#define IO_FIELDS \
 int fd; \
 IO_EVENT_HANDLERS

struct IoObject_s{
  IO_FIELDS;
};

static inline void IoObject_Initialize(IoObject *pObject, int fd, IosReadyHandler onIosReady,
                                       IoReadyHandler onInputReady, IoReadyHandler onOutputReady, IoErrorHandler onError){
  pObject->fd = fd;
  pObject->onIosReady = onIosReady;
  pObject->onInputReady = onInputReady;
  pObject->onOutputReady = onOutputReady;
  pObject->onError = onError;
}
