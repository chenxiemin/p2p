#ifndef _CXM_P2P_NETWORK_H_
#define _CXM_P2P_NETWORK_H_

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <time.h>
#include <errno.h>

#ifdef WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdlib.h>
#include <io.h>
#include <winsock.h>
#include <ws2ipdef.h>
#endif

#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#endif

