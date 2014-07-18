#ifndef CC3K_SOCKET_H_
#define CC3K_SOCKET_H_

#include <cc3k_type.h>

#define CC3K_MAX_SOCKETS 8

typedef enum _cc3k_socket_state_t
{
  SOCKET_STATE_INIT,    // Socket is in the initial state
  SOCKET_STATE_CREATE,  // Socket is being created on the chip
  SOCKET_STATE_READY,   // Socket has been created, socket desciptor is valid
  SOCKET_STATE_FAILED   // Failed to initialize the socket
} cc3k_socket_state_t;

typedef struct _cc3k_socket_t
{
  /** @brief State of the socket */
  cc3k_socket_state_t state;

  /** @brief Socket descriptor returned by the chip */
  uint32_t sd;
} cc3k_socket_t;

/**
 * @brief Socket Manager context
 *
 * Keeps track of socket states with the CC3000 chip
 * to provide an asynchronous callback mechanism
 */
typedef struct _cc3k_socket_manager_t
{
  /** @brief Array of pointers to all active sockets */
  cc3k_socket_t *sockets[CC3K_MAX_SOCKETS]; 

} cc3k_socket_manager_t;

cc3k_status_t cc3k_socket_manager_init(cc3k_socket_manager_t *socket_manager);
cc3k_status_t cc3k_socket_manager_loop(cc3k_socket_manager_t *socket_manager);

#endif
