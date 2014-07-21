#ifndef CC3K_SOCKET_H_
#define CC3K_SOCKET_H_

#include <cc3k_type.h>

#define CC3K_MAX_SOCKETS 8

#define AF_INET              2

// IPv6 is not supported
//#define AF_INET6             23

#define IPPROTO_TCP          6           // Transport Control Protocol
#define IPPROTO_UDP          17          // User Datagram Protocol

#define SOCK_STREAM          1
#define SOCK_DGRAM           2

typedef struct _cc3k_socket_t cc3k_socket_t;

typedef void (cc3k_data_callback_t)(cc3k_socket_t *socket, uint8_t *data, uint16_t length);

typedef enum _cc3k_socket_state_t
{
  SOCKET_STATE_INIT,        // Socket is in the initial state
  SOCKET_STATE_CREATE,      // Socket is being created on the chip
  SOCKET_STATE_CREATED,     // Socket has been created, socket descriptor is valid
  SOCKET_STATE_CONNECTING,  // Socket is waiting for a connect response
  SOCKET_STATE_READY,       // Socket is established
  SOCKET_STATE_CLOSING,     // Socket is waiting for close response
  SOCKET_STATE_CLOSE_WAIT,  // Socket received close wait event, closing this side 
  SOCKET_STATE_FAILED       // Failed to initialize the socket
} cc3k_socket_state_t;

struct _cc3k_socket_t
{
  /** @brief State of the socket */
  cc3k_socket_state_t state;

  /** @brief Socket descriptor returned by the chip */
  uint32_t sd;

  uint32_t family;
  uint32_t type;
  uint32_t protocol;

  // For now, store the sockaddr in here
  cc3k_sockaddr_t sockaddr;

  int rx;
  int rx_bytes;

  uint8_t readable;

};

/**
 * @brief Socket Manager context
 *
 * Keeps track of socket states with the CC3000 chip
 * to provide an asynchronous callback mechanism
 */
typedef struct _cc3k_socket_manager_t
{
  cc3k_t *driver;

  /** @brief Array of pointers to all active sockets */
  cc3k_socket_t *socket[CC3K_MAX_SOCKETS]; 

  /** @brief Current socket with a pending command */
  cc3k_socket_t *current;

  /** @brief Number of used sockets */
  int num_sockets;

  /** @brief Flag to indicate if a select call is pending */
  // TODO: Move boolean flags to a bitmask
  uint8_t select_pending;

} cc3k_socket_manager_t;

cc3k_status_t cc3k_socket_manager_init(cc3k_t *driver, cc3k_socket_manager_t *socket_manager);
cc3k_status_t cc3k_socket_manager_loop(cc3k_socket_manager_t *socket_manager);

cc3k_status_t cc3k_socket_add(cc3k_t *driver, cc3k_socket_t *socket);

#endif
