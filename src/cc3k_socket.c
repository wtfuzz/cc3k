/**
 * @file CC3K Driver Socket Manager
 */

#include <stdlib.h>
#include <cc3k.h>
#include <socket.h>
#include <string.h>

static cc3k_status_t _find_socket(cc3k_socket_manager_t *socket_manager, int32_t sd, cc3k_socket_t **socket)
{
  int i;
  *socket = NULL;
  for(i=0;i<CC3K_MAX_SOCKETS;i++)
  {
    if((socket_manager->socket[i] != NULL) && (socket_manager->socket[i]->sd == sd))
      *socket = socket_manager->socket[i];
  }
  return CC3K_OK;
}

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
  if(result == 0)
  {
    // Successfully connected
    socket_manager->current->state = SOCKET_STATE_READY;
  }
  else
  {
    // Connection failed
    socket_manager->current->state = SOCKET_STATE_FAILED;
  }    

  return CC3K_OK;
}

cc3k_status_t cc3k_close_event(cc3k_socket_manager_t *socket_manager, uint32_t result)
{
  socket_manager->current->state = SOCKET_STATE_INIT;
}

cc3k_status_t cc3k_bind_event(cc3k_socket_manager_t *socket_manager, uint32_t result)
{
  return CC3K_OK;
}

cc3k_recv_event(cc3k_socket_manager_t *socket_manager, int32_t sd, int32_t length)
{
  cc3k_socket_t *socket;

  // Find the socket by descriptor
  _find_socket(socket_manager, sd, &socket);

  if(socket)
  {
    socket->rx++;
    socket->rx_bytes += length;
  }

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
     
      if(socket->type == SOCK_STREAM)
      { 
        if(cc3k_connect(socket_manager->driver, socket->sd, &socket->sockaddr) == CC3K_OK)
        {
          socket_manager->current = socket;
          socket->state = SOCKET_STATE_CONNECTING;
        }
      }
      break;
    case SOCKET_STATE_CONNECTING:
      break;
    case SOCKET_STATE_READY:
      if(socket->type == SOCK_STREAM)
      {
        cc3k_recv(socket_manager->driver, socket->sd, 1000);
      }
      break;
    case SOCKET_STATE_FAILED:
      // Close the socket
      if(cc3k_close(socket_manager->driver, socket->sd) == CC3K_OK)
      {
        socket_manager->current = socket;
        socket->state = SOCKET_STATE_CLOSING;
      }
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

    if(socket->state == SOCKET_STATE_READY)
    {
      // Add the socket to the fd set for select
    }
  } 

  return CC3K_OK;
}

cc3k_status_t cc3k_socket_add(cc3k_t *driver, cc3k_socket_t *socket)
{
  int i;

  if(driver->socket_manager.num_sockets + 1 == CC3K_MAX_SOCKETS)
    return CC3K_INVALID;

  i = driver->socket_manager.num_sockets++;

  driver->socket_manager.socket[i] = socket;

  return CC3K_OK;
}

cc3k_status_t cc3k_socket_write(cc3k_socket_t *socket, uint8_t *data, int length)
{
  switch(socket->type)  
  {
    case SOCK_STREAM:
      break;
    case SOCK_DGRAM:
      break;
  }
  return CC3K_OK;
}
