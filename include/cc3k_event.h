#ifndef CC3K_EVENT_H_
#define CC3K_EVENT_H_

typedef enum _cc3k_event_opcode_t
{
  CC3K_EVENT_FREE_BUFFER        = 0x4100,
  CC3K_EVENT_WLAN_CONNECT       = 0x8001,
  CC3K_EVENT_WLAN_DISCONNECT    = 0x8002,
  CC3K_EVENT_WLAN_DHCP          = 0x8010,
  CC3K_EVENT_PING_REPORT        = 0x8040,
  CC3K_EVENT_KEEPALIVE          = 0x8200,
  CC3K_EVENT_TCP_CLOSE_WAIT     = 0x8800
} cc3k_event_opcode_t;

/**
 * @brief Get status IOCTL event payload
 */
typedef struct _cc3k_status_event_t
{
  uint8_t status;
  uint32_t wlan_status;
} __attribute__ ((packed)) cc3k_status_event_t;

typedef struct _cc3k_socket_event_t
{
  uint8_t status;
  uint32_t result;
} __attribute__ ((packed)) cc3k_socket_event_t;

#endif