/**
 * @file CC3K Driver Socket Manager
 */

#include <stdlib.h>
#include <cc3k.h>
#include <socket.h>
#include <string.h>

/**
 * These are called from the event processor when a socket event is received
 * The current socket index associated with this event is stored in the socket manager
 */
cc3k_status_t cc3k_socket_event(cc3k_socket_manager_t *socket_manager, uint32_t sd)
{
  socket_manager->current->sd = sd;
  socket_manager->current->state = SOCKET_STATE_CREATED;
  return CC3K_OK;
}

cc3k_status_t cc3k_connect_event(cc3k_socket_manager_t *socket_manager, uint32_t result)
{
  socket_manager->current->state = SOCKET_STATE_READY;
  return CC3K_OK;
}

cc3k_status_t cc3k_bind_event(cc3k_socket_manager_t *socket_manager, uint32_t result)
{
  return CC3K_OK;
}

static void _socket_update(cc3k_socket_manager_t *socket_manager, cc3k_socket_t *socket)
{
  switch(socket->state)
  {
    case SOCKET_STATE_INIT:
      // Ask the chip for a new socket
      if(cc3k_socket(socket_manager->driver, socket->family, socket->type, socket->protocol) == CC3K_OK)
      {
        socket_manager->current = socket;
        socket->state = SOCKET_STATE_CREATE;
      }
      break;
    case SOCKET_STATE_CREATE:
      // Waiting for chip to respond with a descriptor
      break;
    case SOCKET_STATE_CREATED:
      // Socket is ready, descriptor is valid

      // If this is a TCP client socket, connect to the endpoint
      if(cc3k_connect(socket_manager->driver, socket->sd, &socket->sockaddr) == CC3K_OK)
      {
        socket_manager->current = socket;
        socket->state = SOCKET_STATE_CONNECTING;
      }
      break;
    case SOCKET_STATE_CONNECTING:
      break;
    case SOCKET_STATE_READY:
      break;
    case SOCKET_STATE_FAILED:
      // What do we do now?
      break;
  } 
}

cc3k_status_t cc3k_socket_manager_init(cc3k_t *driver, cc3k_socket_manager_t *socket_manager)
{
  bzero(socket_manager, sizeof(cc3k_socket_manager_t));

  socket_manager->driver = driver;
  return CC3K_OK;
}

cc3k_status_t cc3k_socket_manager_loop(cc3k_socket_manager_t *socket_manager)
{
  int i;
  cc3k_socket_t *socket;

  // Update each of the registered sockets
  for(i=0;i<CC3K_MAX_SOCKETS;i++)
  {
    socket = socket_manager->socket[i];
    if(socket == NULL)
      continue;

    _socket_update(socket_manager, socket);
  } 

  return CC3K_OK;
}


int socket(int domain, int family, int protocol)
{
  return -1;
}

int bind(int socket, const sockaddr *address, socklen_t address_len)
{

  return -1;
}

int connect(int socket, const sockaddr *address, socklen_t address_len)
{
  return -1;
}

int closesocket(int socket)
{
  return -1;
}
