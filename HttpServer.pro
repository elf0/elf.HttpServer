TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    server.c

HEADERS += \
    Connection.h \
    IoObject.h \
    Memory.h \
    MPool.h \
    Tcp.h \
    TcpServer.h \
    Type.h \
    Epoll.h

DEFINES += _GNU_SOURCE

QMAKE_CFLAGS_DEBUG += -DDEBUG
QMAKE_CFLAGS_RELEASE += -DNDEBUG
