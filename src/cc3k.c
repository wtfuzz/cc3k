#include <stdlib.h>
#include <cc3k.h>
#include <cc3k_command.h>
#include <cc3k_data.h>
#include <string.h>

#include "../ap.h"

#ifdef CC3K_DEBUG
#include <stdio.h>
#endif

// Set the internal chip debug mask
// This controls which messages the chip will output
// on the 1.8v UART pins. These are not accessible on the Spark
#define CC3K_DEBUG_MASK 0x00000000

static cc3k_status_t _process_event(cc3k_t *driver);
static cc3k_status_t cc3k_read_header(cc3k_t *driver);

/**
 * @brief Poll the interrupt pin and wait for it to fall
 */
static void _interrupt_poll_wait(cc3k_t *driver)
{
  while(1)
  {
    if((*driver->config->readInterrupt)() == 0)
      break;
  }
}


static inline void _assert_cs(cc3k_t *driver, int assert)
{
  (*driver->config->assertChipSelect)(assert);
}

static inline void _spi_sync(cc3k_t *driver, uint8_t *out, uint8_t *in, uint16_t length)
{
  (*driver->config->spiTransaction)(out, in, length, 0);
}

static inline void _spi(cc3k_t *driver, uint8_t *out, uint8_t *in, uint16_t length)
{
  driver->spi_busy = 1;
  (*driver->config->spiTransaction)(out, in, length, 1);
}

static inline void _int_enable(cc3k_t *driver, int enable)
{
  (*driver->config->enableInterrupt)(enable);
}

static inline void _chip_enable(cc3k_t *driver, int enable)
{
  (*driver->config->enableChip)(enable);
}

static inline void _transition(cc3k_t *driver, cc3k_state_t state)
{
  if(driver->config->transitionCallback)
    (*driver->config->transitionCallback)(driver->state, state);

#ifdef CC3K_DEBUG
  fprintf(stderr, "Transition %s -> %s\n", state_names[driver->state], state_names[state]);
#endif

  driver->state = state;
}

static void _send_command(cc3k_t *driver)
{
  _transition(driver, CC3K_STATE_SEND_COMMAND);
  _spi(driver, driver->packet_tx_buffer, driver->packet_rx_buffer, driver->packet_tx_buffer_length);
}

cc3k_status_t cc3k_send_command(cc3k_t *driver, uint16_t opcode, uint8_t *arg, uint8_t args_length)
{
  // Check if we are busy processing an existing command
  if( (driver->state != CC3K_STATE_IDLE) || (driver->command != 0) )
    return CC3K_BUSY;

#ifdef CC3K_DEBUG
  fprintf(stderr, "Sending command 0x%04X\n", opcode);
#endif

  if(driver->config->commandCallback)
    (*driver->config->commandCallback)(opcode, arg, args_length);

  // Populate the transmit buffer with the command
  cc3k_command(driver, opcode, arg, args_length);
  driver->stats.commands++;

  // Store the pending command opcode in the driver context
  driver->command = opcode;

  // Transition into the command request state, and assert /CS
  // In this state, the ISR will be called when the chip is ready
  // to receive the command. The SPI transmissing will kickoff then.

  if((*driver->config->readInterrupt)() == 0)
  {
    // The IRQ line is low before asserting CS
    driver->irq_preempt++;
    _int_enable(driver, 1);
    return CC3K_BUSY;
  }

  _transition(driver, CC3K_STATE_COMMAND_REQUEST);
  _assert_cs(driver, 1);

  return CC3K_OK;
}

cc3k_status_t cc3k_send_data(cc3k_t *driver, uint8_t opcode, uint8_t *arg, uint8_t args_length,
  uint8_t *payload, uint16_t payload_length, uint8_t *footer, uint8_t footer_length)
{
  cc3k_data(driver, opcode, arg, args_length, payload, payload_length, footer, footer_length);
  driver->stats.tx++;
  _transition(driver, CC3K_STATE_DATA_REQUEST);
  _assert_cs(driver, 1); 
  return CC3K_OK;
}

cc3k_status_t cc3k_init(cc3k_t *driver, cc3k_config_t *config)
{
  // Patch source parameter to simple link start command
  const uint8_t patch_source = 0x00;

  bzero(driver, sizeof(cc3k_t));

  driver->config = config;

  cc3k_socket_manager_init(driver, &driver->socket_manager);

  _int_enable(driver, 0);

  _assert_cs(driver, 0);
  _chip_enable(driver, 0);
  (*config->delayMicroseconds)(100000); 
  _chip_enable(driver, 1);
  (*config->delayMicroseconds)(10000); 

  // Wait for the interrupt line to fall after enabling the chip
  _interrupt_poll_wait(driver);

  // Delay before asserting the CS line
  (*config->delayMicroseconds)(10000);
  _assert_cs(driver, 1);

  // The first command sent to the chip is different than the rest, so we will
  // handle it manually here with synchronous SPI calls.
  // This will send a CC3K_COMMAND_SIMPLE_LINK_START packet

  // Construct the command packet in the transmit buffer
  cc3k_command(driver, CC3K_COMMAND_SIMPLE_LINK_START, (uint8_t *)&patch_source, 1);
  driver->stats.commands++;

  // NOTE Special timing sequence for fist transaction
  // Send the first 4 bytes of the SPI header
  _spi_sync(driver, driver->packet_tx_buffer, driver->packet_rx_buffer, 4);
  // Delay for 50uS
  (*config->delayMicroseconds)(500); 
  // Send the remaining 6 bytes of the COMMAND_SIMPLE_LINK_START packet
  _spi_sync(driver, driver->packet_tx_buffer+4, driver->packet_rx_buffer + 4, 6);

  // Enable interrupts
  _int_enable(driver, 1);
  _assert_cs(driver, 0);

  _transition(driver, CC3K_STATE_SIMPLE_LINK_START);

	return CC3K_OK;	
}

cc3k_status_t cc3k_wlan_connect(
  cc3k_t *driver,
  cc3k_security_type_t security_type,
  const char *ssid,
  uint8_t ssid_length,
  char *key,
  uint8_t key_length)
{
  cc3k_command_wlan_connect_t cmd;
  
  if(driver->state != CC3K_STATE_IDLE)
    return CC3K_INVALID;

  bzero(&cmd, sizeof(cc3k_command_wlan_connect_t));

  cmd.ssid_offset = 0x1C;
  cmd.ssid_length = ssid_length;
  cmd.security_type = security_type;
  cmd.key_offset = 16 + CC3K_SSID_MAX;
  cmd.key_length = key_length;
  cmd.pad = 0;
  bzero(cmd.bssid, sizeof(cmd.bssid));
  memcpy(cmd.ssid, ssid, ssid_length);
  memcpy(cmd.key, key, key_length);
  return cc3k_send_command(driver, CC3K_COMMAND_WLAN_CONNECT, (uint8_t *)&cmd, sizeof(cc3k_command_wlan_connect_t));
}

static cc3k_status_t cc3k_read_header(cc3k_t *driver)
{
  cc3k_spi_header_t *spi_header;
  spi_header = (cc3k_spi_header_t *)driver->packet_tx_buffer;
  spi_header->type = CC3K_PACKET_TYPE_READ;
  spi_header->length = 0;
  spi_header->busy = 0;

  _transition(driver, CC3K_STATE_READ_HEADER);
  _assert_cs(driver, 1);
  _spi(driver, driver->packet_tx_buffer, driver->packet_rx_buffer, 10);
  return CC3K_OK;
}

cc3k_status_t cc3k_spi_done(cc3k_t *driver)
{
  uint16_t length;
  cc3k_spi_rx_header_t *spi_rx_header;

  // Called when an SPI transfer is complete
  // This could be from a DMA interrupt handler
  // Or a busy wait in the SPI transaction callback

  driver->stats.spi_done++;

  driver->spi_busy = 0;

  switch(driver->state)
  {
    case CC3K_STATE_READ_HEADER:
      spi_rx_header = (cc3k_spi_rx_header_t *)driver->packet_rx_buffer;

      length = HI(spi_rx_header->length);
      length |= LO(spi_rx_header->length);

      // Check if there is more SPI packet payload to receive (we already received the 5 byte minimum)
      if(length - 5 > 0)
      {
        _transition(driver, CC3K_STATE_READ_PAYLOAD);
        _spi(driver, driver->packet_tx_buffer, driver->packet_rx_buffer + sizeof(cc3k_spi_rx_header_t) + 5, length-5);
      }
      else
      {
        // Packet is complete, deselect the chip and process the event
        _int_enable(driver, 1);
        _assert_cs(driver, 0);

        driver->stats.events++;
        _transition(driver, CC3K_STATE_IDLE);
        _process_event(driver);
      }
      break;

    case CC3K_STATE_READ_PAYLOAD:

      _int_enable(driver, 1);
      _assert_cs(driver, 0);

      driver->stats.events++;
      _transition(driver, CC3K_STATE_IDLE);
      _process_event(driver);
      break;

    case CC3K_STATE_SEND_COMMAND:
      _transition(driver, CC3K_STATE_COMMAND);
      
      // Re-enable interrupts to get notification of a response,
      // or an unsolicited event
      _int_enable(driver, 1);
      _assert_cs(driver, 0);
      break;
    case CC3K_STATE_DATA:
      // SPI transmission has completed, deassert /CS and wait for
      _assert_cs(driver, 0);
      break;

    default:
      driver->spi_unhandled++;
      break;
  }

  return CC3K_OK;
}


cc3k_status_t cc3k_interrupt(cc3k_t *driver)
{
  // Called when interrupts were requested and the IRQ pin has fallen

#ifdef CC3K_DEBUG
  fprintf(stderr, "Interrupt state %s\n", state_names[driver->state]);
#endif

  driver->stats.interrupts++;

  driver->int_state = driver->state;

  switch(driver->state)
  {
    case CC3K_STATE_COMMAND_REQUEST:
      // There is a pending command in the transmit buffer, and the chip has
      // notified us that it is ready to accept the command.
      _send_command(driver);
      break;
    case CC3K_STATE_DATA_REQUEST:
      _transition(driver, CC3K_STATE_DATA);
      _spi(driver, driver->packet_tx_buffer, driver->packet_rx_buffer, driver->packet_tx_buffer_length);
      break;

    case CC3K_STATE_SIMPLE_LINK_START:
      cc3k_read_header(driver);
      break;

    case CC3K_STATE_DATA:
    case CC3K_STATE_DATA_RX:
    case CC3K_STATE_COMMAND:
    case CC3K_STATE_IDLE:
      // Disable interrupts until we have finished receiving the response
      _int_enable(driver, 0);

      // The chip has a response for us. Start reading the SPI header
      cc3k_read_header(driver);
      break;
    case CC3K_STATE_INIT:
      break;
    default:
#ifdef CC3K_DEBUG
      fprintf(stderr, "Unhandled interrupt in state %s\n", state_names[driver->state]);
#endif
/*
      driver->stats.unhandled_interrupts++;
      driver->unhandled_state = driver->state;
      driver->interrupt_pending = 1;
*/
      break;
  }

	return CC3K_OK;
}

static cc3k_status_t _process_event(cc3k_t *driver)
{
  cc3k_command_header_t *event_header;
  uint8_t *payload;
  
  event_header = (cc3k_command_header_t *)(driver->packet_rx_buffer + sizeof(cc3k_spi_rx_header_t));

#ifdef CC3K_DEBUG
  fprintf(stderr, "Processing event type %04X\n", event_header->type);
#endif

  if(event_header->type == CC3K_PAYLOAD_TYPE_DATA)
  {
    cc3k_data_header_t *data_header;
    data_header = (cc3k_data_header_t *)event_header;

    uint16_t size;
    size = HI(data_header->payload_length);
    size |= LO(data_header->payload_length);

    if(driver->config->dataCallback)
      (*driver->config->dataCallback)();
    driver->stats.rx++;
    driver->stats.bytes_rx += size;
#ifdef CC3K_DEBUG
    fprintf(stderr, "Bytes %d\n", size);
    fprintf(stderr, "Received %d packets %d bytes\n", driver->stats.rx, driver->stats.bytes_rx);
#endif
    return CC3K_OK;
  }

  if(event_header->type != CC3K_PAYLOAD_TYPE_EVENT)
  {
#ifdef CC3K_DEBUG
    fprintf(stderr, "Buffer does not contain data or event!\n");
#endif
    return CC3K_INVALID;
  }

  payload = driver->packet_rx_buffer + sizeof(cc3k_spi_rx_header_t) + sizeof(cc3k_command_header_t);

  if(driver->config->eventCallback)
    (*driver->config->eventCallback)(event_header->opcode, payload, event_header->argument_length);

#ifdef CC3K_DEBUG
  fprintf(stderr, "Event opcode 0x%04X\n", event_header->opcode);
#endif

  if(event_header->opcode >= 0x4100)
  {
    // Async unsolocited event
    driver->stats.unsolicited++;

    // Check if we were waiting for a command before the unsolicited event arrived
    // This will cause a transition back to the COMMAND state to wait for
    // the response to the original command request

    // We remain in this state to prevent other commands from being sent until this one
    // has completed. We *might* be able to interleave some commands but will leave that for later
    if(driver->command != 0)
      _transition(driver, CC3K_STATE_COMMAND);

    _assert_cs(driver, 0);
  }
  else
  {
    cc3k_recv_event_t *recv_event;

    // This event should be a response to the last sent command
    //if(event_header->opcode != driver->command)
      //return CC3K_INVALID;

    // This is a response to the pending command.
    // Reset the pending command in the driver for the unsolicited event logic

    if(event_header->opcode == driver->command)
      driver->command = 0;

    if(event_header->opcode == CC3K_COMMAND_RECV ||
       event_header->opcode == CC3K_COMMAND_RECVFROM)
    {
      recv_event = (cc3k_recv_event_t *)payload;
#ifdef CC3K_DEBUG
      fprintf(stderr, "Recv %d len %d flags 0x%08X\n", recv_event->sd, recv_event->length, recv_event->flags);
#endif
  
      _transition(driver, CC3K_STATE_DATA_RX);

      return CC3K_OK;

    }

    _assert_cs(driver, 0);

    // Handle the first simple link start command and kickoff a read_buffer_size
    switch(event_header->opcode)
    {
      case CC3K_COMMAND_SIMPLE_LINK_START:
        cc3k_set_debug(driver, CC3K_DEBUG_MASK);
        break;
      case CC3K_COMMAND_NETAPP_SET_DEBUG:
        cc3k_send_command(driver, CC3K_COMMAND_READ_BUFFER_SIZE, NULL, 0);
        break;
      case CC3K_COMMAND_READ_BUFFER_SIZE:
        cc3k_wlan_connect(driver, CC3K_SEC_WPA2, SSID, strlen(SSID), KEY, strlen(KEY));
        break;
      default:
        break;
    }
  }

  cc3k_process_event(driver, event_header->opcode, payload, event_header->argument_length);
 
  return CC3K_OK; 
}

cc3k_status_t cc3k_loop(cc3k_t *driver, uint32_t time_ms)
{
  /** Milliseconds elapsed since last loop iteration */
  uint32_t dt;
  cc3k_status_t status;

  if(driver->last_time_ms == 0)
  {
    dt = 0;
  }
  else
  {
    dt = time_ms - driver->last_time_ms;
  }

  driver->last_time_ms = time_ms;

  switch(driver->state)
  {
    case CC3K_STATE_IDLE:
      // Check timer
      if( (time_ms - driver->last_update > 2000))
      {
        // Get status
        cc3k_send_command(driver, CC3K_COMMAND_IOCTL_STATUSGET, NULL, 0);
        driver->last_update = time_ms;
      }

      break;

    default:
      // Do nothing
      break;
  }

  // Run the socket manager
  if( (driver->wlan_status == WLAN_STATUS_CONNECTED) &&
      (driver->dhcp_complete == 1) )
    cc3k_socket_manager_loop(&driver->socket_manager, dt);

  driver->last_state = driver->state;

  //_int_enable(driver, 1);

  return CC3K_OK;
}

cc3k_status_t cc3k_set_debug(cc3k_t *driver, uint32_t level)
{
  return cc3k_send_command(driver, CC3K_COMMAND_NETAPP_SET_DEBUG, (uint8_t *)&level, sizeof(uint32_t));
}

/**
 * Chip socket command functions
 */

cc3k_status_t cc3k_socket(cc3k_t *driver, int family, int type, int protocol)
{
  cc3k_command_socket_t cmd;

  cmd.family = family;
  cmd.type = type;
  cmd.protocol = protocol;

  return cc3k_send_command(driver, CC3K_COMMAND_SOCKET, (uint8_t *)&cmd, sizeof(cc3k_command_socket_t));
}

cc3k_status_t cc3k_connect(cc3k_t *driver, int sd, cc3k_sockaddr_t *sa)
{
  cc3k_command_connect_t cmd;
  cmd.sd = sd;
  cmd.unk = 0x8;
  cmd.addr_length = sizeof(cc3k_sockaddr_t);
  cmd.addr = *sa;

  return cc3k_send_command(driver, CC3K_COMMAND_CONNECT, (uint8_t *)&cmd, sizeof(cc3k_command_connect_t));
}

cc3k_status_t cc3k_close(cc3k_t *driver, int sd)
{
  uint32_t s = sd;
  return cc3k_send_command(driver, CC3K_COMMAND_CLOSE, (uint8_t *)&s, sizeof(uint32_t));
}

cc3k_status_t cc3k_select(cc3k_t *driver, uint8_t maxfd, uint32_t read_fd, uint32_t write_fd, uint32_t except_fd)
{
  cc3k_command_select_t cmd;
  cc3k_status_t status;

  if(driver->state != CC3K_STATE_IDLE)
    return CC3K_BUSY;

  cmd.maxfd = maxfd;
  cmd.ca = 0x14;
  cmd.cb = 0x14;
  cmd.cc = 0x14;
  cmd.cd = 0x14;
  cmd.blocking = 0;
  cmd.read_fd = read_fd;
  cmd.write_fd = write_fd;
  cmd.except_fd = except_fd;
  cmd.timeout_sec = 1;
  cmd.timeout_usec = 0; //250000; 


  status = cc3k_send_command(driver, CC3K_COMMAND_SELECT, (uint8_t *)&cmd, sizeof(cc3k_command_select_t));

  
#ifdef CC3K_DEBUG
  if(status == CC3K_OK)
  {
    fprintf(stderr, "Select maxfd %d rfd 0x%08X wfd 0x%08X efd 0x%08X\n",
      cmd.maxfd, cmd.read_fd, cmd.write_fd, cmd.except_fd);
  }
#endif
  return status;
}

cc3k_status_t cc3k_recv(cc3k_t *driver, int sd, uint16_t length)
{
  cc3k_command_recv_t cmd;
  cmd.sd = sd;
  cmd.length = length;
  cmd.flags = 0;
  return cc3k_send_command(driver, CC3K_COMMAND_RECV, (uint8_t *)&cmd, sizeof(cc3k_command_recv_t)); 
}

cc3k_status_t cc3k_sendto(cc3k_t *driver, int sd, uint8_t *payload, uint16_t payload_length, cc3k_sockaddr_t *sa)
{
  cc3k_data_sendto_t arg;
  arg.sd = sd;
  arg.unk = 0x14;
  arg.payload_length = payload_length;
  arg.flags = 0;
  arg.offset = payload_length + 8;
  arg.unused_length = 0x8;

  return cc3k_send_data(driver,
    CC3K_DATA_SENDTO,
    (uint8_t *)&arg,
    sizeof(cc3k_data_sendto_t),
    payload, payload_length,
    (uint8_t *)sa, sizeof(cc3k_sockaddr_t));
}
