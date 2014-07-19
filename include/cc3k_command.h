#ifndef _CC3K_COMMAND_H
#define _CC3K_COMMAND_H

/**
 * @brief Command opcodes
 */
typedef enum _cc3k_command_t
{
	CC3K_COMMAND_WLAN_CONNECT           = 0x0001,
	CC3K_COMMAND_WLAN_DISCONNECT        = 0x0002,
	CC3K_COMMAND_IOCTL_SET_SCANPARAM    = 0x0003,
	CC3K_COMMAND_IOCTL_SET_CONNPOLICY   = 0x0004,
	CC3K_COMMAND_IOCTL_ADD_PROFILE      = 0x0005,
	CC3K_COMMAND_IOCTL_DEL_PROFILE      = 0x0006,
	CC3K_COMMAND_IOCTL_GET_SCANRESULTS  = 0x0007,
	CC3K_COMMAND_SET_EVENT_MASK         = 0x0008,
	CC3K_COMMAND_IOCTL_STATUSGET        = 0x0009,
	CC3K_COMMAND_NVMEM_READ             = 0x0201,
	CC3K_COMMAND_NVMEM_CREATE_ENTRY     = 0x0203,
	CC3K_COMMAND_NVMEM_WRITE_PATCH      = 0x0204,
	CC3K_COMMAND_NVMEM_SWAP_ENTRY       = 0x0205,
	CC3K_COMMAND_READ_SP_VERSION        = 0x0207,
	CC3K_COMMAND_SOCKET                 = 0x1001,
	CC3K_COMMAND_BIND                   = 0x1002,
	CC3K_COMMAND_RECV                   = 0x1004,
	CC3K_COMMAND_ACCEPT                 = 0x1005,
	CC3K_COMMAND_LISTEN                 = 0x1006,
	CC3K_COMMAND_CONNECT                = 0x1007,
	CC3K_COMMAND_SELECT                 = 0x1008,
	CC3K_COMMAND_SETSOCKOPT             = 0x1009,
	CC3K_COMMAND_GETSOCKOPT             = 0x100A,
	CC3K_COMMAND_CLOSE                  = 0x100B,
	CC3K_COMMAND_RECVFROM               = 0x100D,
	CC3K_COMMAND_GETHOSTBYNAME          = 0x1010,
	CC3K_COMMAND_MDNS_ADVERTISE         = 0x1011,
	CC3K_COMMAND_GETMSS                 = 0x1012,
	CC3K_COMMAND_NETAPP_DHCP            = 0x2001,
	CC3K_COMMAND_NETAPP_PING_SEND       = 0x2002,
	CC3K_COMMAND_NETAPP_PING_REPORT     = 0x2003,
	CC3K_COMMAND_NETAPP_PING_STOP       = 0x2004,
	CC3K_COMMAND_NETAPP_GETIPCONFIG     = 0x2005,
	CC3K_COMMAND_NETAPP_ARPFLUSH        = 0x2006,
	CC3K_COMMAND_NETAPP_SET_DEBUG       = 0x2008,
	CC3K_COMMAND_NETAPP_SET_TIMERS      = 0x2009,
	CC3K_COMMAND_SIMPLE_LINK_START      = 0x4000,
	CC3K_COMMAND_READ_BUFFER_SIZE       = 0x400B
} cc3k_command_t;

/**
 * @brief WLAN Status types
 *
 * These types are returned from a CC3K_COMMAND_IOCTL_STATUSGET request
 */
typedef enum _cc3k_wlan_status_t
{
  WLAN_STATUS_DISCONNECTED,
  WLAN_STATUS_SCANNING,
  WLAN_STATUS_CONNECTING,
  WLAN_STATUS_CONNECTED
} cc3k_wlan_status_t;


/**
 * @brief Read Buffer Size event payload
 */
typedef struct _cc3k_buffer_size_t
{
  uint8_t status;
  uint8_t count;
  uint16_t size;
} cc3k_buffer_size_t;

/**
 * @brief Security type definitions
 */
typedef enum _cc3k_security_type_t
{
  CC3K_SEC_OPEN,
  CC3K_SEC_WEP,
  CC3K_SEC_WPA,
  CC3K_SEC_WPA2
} cc3k_security_type_t;

#define CC3K_SSID_MAX 32
#define CC3K_KEY_MAX 64

/**
 * @brief Connect command header
 */
typedef struct _cc3k_command_wlan_connect_t
{
  uint32_t ssid_offset;   // Always 0x0000001C (28 bytes to beginning of SSID)
  uint32_t ssid_length;   // Length of the SSID following this header
  uint32_t security_type; // cc3k_security_type_t

  // I think we can use this as an offset to the beginning of the key,
  // Allowing us to declare the ssid and key buffers directly in the struct
  uint32_t key_offset;    // Bytes from here to key (16 + CC3K_SSID_MAX)
  uint32_t key_length;    // Length of the key following the SSID
  uint16_t pad;           // Always zero
  uint8_t bssid[6];       // BSSID (fill with 0 for any)
  uint8_t ssid[CC3K_SSID_MAX];
  uint8_t key[CC3K_KEY_MAX];
} __attribute__ ((packed)) cc3k_command_wlan_connect_t;

/**
 * @brief Socket command header
 */
typedef struct _cc3k_command_socket_t
{
  uint32_t family;
  uint32_t type;
  uint32_t protocol;
} __attribute__ ((packed)) cc3k_command_socket_t;

typedef struct _cc3k_command_connect_t
{
  uint32_t sd;
  uint32_t unk;
  uint32_t addr_length;
  cc3k_sockaddr_t addr;
} __attribute__ ((packed)) cc3k_command_connect_t;

#endif
