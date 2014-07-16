#include <cc3k.h>

cc3k_status_t cc3k_process_event(cc3k_t *driver, uint16_t opcode, uint8_t *arg, uint8_t arg_length)
{
  cc3k_buffer_size_t *buffer_info;
  cc3k_status_event_t *status_event;

  switch(opcode)
  {
    case CC3K_COMMAND_READ_BUFFER_SIZE:
      buffer_info = (cc3k_buffer_size_t *)arg;
      driver->buffers = buffer_info->count;
      break;

    case CC3K_COMMAND_IOCTL_STATUSGET:
      status_event = (cc3k_status_event_t *)arg;
      driver->wlan_status = status_event->wlan_status;
      break;
  }

  return CC3K_OK;
}
