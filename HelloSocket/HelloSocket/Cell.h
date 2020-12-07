#ifndef _CELL_INCLUDED
#define _CELL_INCLUDED

#ifdef _WIN32
#define FD_SETSIZE      2506
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#define strcpy_s strcpy
#define closksocket close
#endif

#include "MessageHeader.hpp"
#include <stdio.h>

#ifndef RECVBUFSIZE
#define RECVBUFSIZE 10240
#define SENDBUFSIZE RECVBUFSIZE
#endif

#ifndef CLIENT_HEART_DEAD_TIME
#define CLIENT_HEART_DEAD_TIME 60000
#endif

#ifndef CLIENT_SEND_CHECK_TIME
#define CLIENT_SEND_CHECK_TIME 200
#endif

class Client;
class CellServer;
class INetEvent;

#endif
