#ifndef __INCLUDED_GLOBAL_DEFINES_H_
#define __INCLUDED_GLOBAL_DEFINES_H_

#define MAX_MTU_SIZE 1592

#define MAX_CP_SIZE 1575

#define MAX_VALID_SIZE (20*1024*1024)

#define SEND_TIME_OUT  2000

#define MAX_ALLOCA_STACK_ALLOCATION 1048576

#define TYPE_SERVER 1

#define USE_STL

#define USE_POOL 0

#define FOR_TEST

#define USE_PACKET_LOST_CHECK 

#define USE_BOOST 0

#ifdef _WIN32
#include <WinSock2.h>
#include <windows.h>
#include <Ws2tcpip.h>
#else 
#include <sys/socket.h>    
#include <sys/epoll.h>    
#include <netinet/in.h>    
#include <arpa/inet.h>    
#include <fcntl.h>    
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <string.h>
#include <stddef.h>
#endif

#define UINT64 unsigned long long
#define DWORD  unsigned long
#define WORD   unsigned short
#define BYTE   unsigned char

#ifdef USE_STL
#include <string>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
using namespace std;
#endif

#ifdef _WIN32

#else
#define Sleep sleep
#endif

#endif