#ifndef CC3K_SOCKET_H_
#define CC3K_SOCKET_H_

#include <cc3k_type.h>

#define CC3K_MAX_SOCKETS 8

#define AF_INET              2

// IPv6 is not supported
//#define AF_INET6             23

typedef enum _cc3k_protocol_type_t
{
  IPPROTO_TCP = 6,
  IPPROTO_UDP = 17
} cc3k_protocol_type_t;

typedef enum _cc3k_socket_type_t
{
  SOCK_STREAM = 1,
  SOCK_DGRAM = 2
} cc3k_socket_type_t;

typedef struct _cc3k_socket_t cc3k_socket_t;

typedef void (cc3k_data_callback_t)(cc3k_t *driver, cc3k_socket_t *socket, uint8_t *data, uint16_t length, cc3k_sockaddr_t *from);

typedef enum _cc3k_socket_state_t
{
  SOCKET_STATE_INIT,        // Socket is in the initial state
  SOCKET_STATE_CREATE,      // Socket is being created on the chip
  SOCKET_STATE_CREATED,     // Socket has been created, socket descriptor is valid
  SOCKET_STATE_BINDING, 
  SOCKET_STATE_BOUND,
  SOCKET_STATE_LISTENING,
  SOCKET_STATE_ACCEPTING,
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

  /** @brief Milliseconds reamining before retrying */
  uint32_t retry_timeout;

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

  // Flag to indicate if this socket is a server
  // TODO: Clean this up
  int bind;

  /** @brief Socket data reception callback */
  cc3k_data_callback_t *receive_callback;

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
cc3k_status_t cc3k_socket_manager_loop(cc3k_socket_manager_t *socket_manager, uint32_t dt);

cc3k_status_t cc3k_socket_init(cc3k_socket_t *socket, cc3k_socket_type_t type);
cc3k_status_t cc3k_socket_bind(cc3k_socket_t *socket, cc3k_sockaddr_t *sa);

cc3k_status_t cc3k_socket_add(cc3k_t *driver, cc3k_socket_t *socket);

/**
 * @brief Handle a parsed data event from the CC3000
 */
cc3k_status_t cc3k_socket_data_event(cc3k_socket_manager_t *socket_manager, int32_t sd, uint8_t *data, uint32_t data_length, cc3k_sockaddr_t *from);

#endif
