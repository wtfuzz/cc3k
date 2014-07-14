/**
 * @file socket.h
 *
 * BSD-ish Sockets
 */

#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <inttypes.h>

#define  AF_INET                2
#define  AF_INET6               23

#define IPPROTO_TCP             6           // Transport Control Protocol
#define IPPROTO_UDP             17          // User Datagram Protocol

#define  SOCK_STREAM            1
#define  SOCK_DGRAM             2

typedef unsigned long socklen_t;

typedef struct _sockaddr_t
{
  uint16_t sa_family;
  uint8_t sa_data[14];
} sockaddr;

int socket(int domain, int family, int protocol);
int bind(int socket, const sockaddr *address, socklen_t address_len);
int connect(int socket, const sockaddr *address, socklen_t address_len);
int closesocket(int socket);

int send(int socket, const void *buf, int len, int flags);

int sendto(int socket, const void *buf, int len, int flags,
                  const sockaddr *to, socklen_t tolen);

int recv(int socket, void *buf, int len, int flags);
int gethostbyname(char* hostname, int length, uint32_t* ip);

#endif
