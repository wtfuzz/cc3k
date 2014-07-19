#include <cc3k.h>

#include "../ap.h"

cc3k_status_t cc3k_process_event(cc3k_t *driver, uint16_t opcode, uint8_t *arg, uint8_t arg_length)
{
  cc3k_buffer_size_t *buffer_info;
  cc3k_status_event_t *status_event;
  cc3k_socket_event_t *socket_event;

  switch(opcode)
  {
    case CC3K_COMMAND_READ_BUFFER_SIZE:
      buffer_info = (cc3k_buffer_size_t *)arg;
      driver->buffers = buffer_info->count;
      break;

    case CC3K_COMMAND_IOCTL_STATUSGET:
      status_event = (cc3k_status_event_t *)arg;
      driver->wlan_status = status_event->wlan_status;

      if(driver->wlan_status == 0)
        cc3k_wlan_connect(driver, CC3K_SEC_WPA2, SSID, strlen(SSID), KEY, strlen(KEY));
      break;

    case CC3K_EVENT_WLAN_DHCP:
      memcpy(&driver->ipconfig, arg+1, sizeof(cc3k_ipconfig_t));
      break;

    case CC3K_COMMAND_SOCKET:
      socket_event = (cc3k_socket_event_t *)arg;
      cc3k_socket_event(&driver->socket_manager, socket_event->result);
      break;
    case CC3K_COMMAND_BIND:
      socket_event = (cc3k_socket_event_t *)arg;
      cc3k_bind_event(&driver->socket_manager, socket_event->result);
      break;
    case CC3K_COMMAND_CONNECT:
      socket_event = (cc3k_socket_event_t *)arg;
      cc3k_connect_event(&driver->socket_manager, socket_event->result);
      break;
    case CC3K_COMMAND_LISTEN:
      socket_event = (cc3k_socket_event_t *)arg;
      break;
    case CC3K_COMMAND_ACCEPT:
      socket_event = (cc3k_socket_event_t *)arg;
      break;
  }

  return CC3K_OK;
}
