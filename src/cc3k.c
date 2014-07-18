#include <stdlib.h>
#include <cc3k.h>
#include <cc3k_command.h>
#include <strings.h>

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
  driver->state = state;
}

cc3k_status_t cc3k_send_command(cc3k_t *driver, uint16_t opcode, uint8_t *arg, uint8_t args_length)
{
  // Populate the transmit buffer with the command
  cc3k_command(driver, opcode, arg, args_length);
  driver->stats.commands++;

  // Transition into the command request state, and assert /CS
  // In this state, the ISR will be called when the chip is ready
  // to receive the command. The SPI transmissing will kickoff then.
  _transition(driver, CC3K_STATE_COMMAND_REQUEST);
  _assert_cs(driver, 1);
  return CC3K_OK;
}

cc3k_status_t cc3k_init(cc3k_t *driver, cc3k_config_t *config)
{
  // Patch source parameter to simple link start command
  const uint8_t patch_source = 0x00;

  bzero(driver, sizeof(cc3k_t));

  driver->config = config;

  _int_enable(driver, 0);

  _assert_cs(driver, 0);
  _chip_enable(driver, 0);
  (*config->delayMicroseconds)(100000); 
  _chip_enable(driver, 1);

  // Wait for the interrupt line to fall after enabling the chip
  _interrupt_poll_wait(driver);
  _assert_cs(driver, 1);


  // The first command sent to the chip is different than the rest, so we will
  // handle it manually here with asynchronous SPI calls.
  // This will send a CC3K_COMMAND_SIMPLE_LINK_START packet

  // Construct the command packet in the transmit buffer
  cc3k_command(driver, CC3K_COMMAND_SIMPLE_LINK_START, (uint8_t *)&patch_source, 1);
  driver->stats.commands++;

  // NOTE Special timing sequence for fist transaction
  // Send the first 4 bytes of the SPI header
  _spi_sync(driver, driver->packet_tx_buffer, driver->packet_rx_buffer, 4);
  // Delay for 50uS
  (*config->delayMicroseconds)(50); 
  // Send the remaining 6 bytes of the COMMAND_SIMPLE_LINK_START packet
  _spi_sync(driver, driver->packet_tx_buffer+4, driver->packet_rx_buffer + 4, 6);

  // Enable interrupts
  _int_enable(driver, 1);
  _assert_cs(driver, 0);

  driver->state = CC3K_STATE_SIMPLE_LINK_START;

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
  cc3k_command_connect_t cmd;
  
  if(driver->state != CC3K_STATE_IDLE)
    return CC3K_INVALID;

  bzero(&cmd, sizeof(cc3k_command_connect_t));

  cmd.ssid_offset = 0x1C;
  cmd.ssid_length = ssid_length;
  cmd.security_type = security_type;
  cmd.key_offset = 16 + CC3K_SSID_MAX;
  cmd.key_length = key_length;
  cmd.pad = 0;
  bzero(cmd.bssid, sizeof(cmd.bssid));
  memcpy(cmd.ssid, ssid, ssid_length);
  memcpy(cmd.key, key, key_length);
  return cc3k_send_command(driver, CC3K_COMMAND_WLAN_CONNECT, &cmd, sizeof(cc3k_command_connect_t));
}

cc3k_status_t cc3k_read_header(cc3k_t *driver)
{
  cc3k_spi_header_t *spi_header;
  spi_header = (cc3k_spi_header_t *)driver->packet_tx_buffer;
  spi_header->type = CC3K_PACKET_TYPE_READ;
  spi_header->length = 0;
  spi_header->busy = 0;

  _transition(driver, CC3K_STATE_READ_HEADER);
  _assert_cs(driver, 1);
  _spi(driver, driver->packet_tx_buffer, driver->packet_rx_buffer, 5);
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

  switch(driver->state)
  {
    case CC3K_STATE_READ_HEADER:
      spi_rx_header = (cc3k_spi_rx_header_t *)driver->packet_rx_buffer;

      length = HI(spi_rx_header->length);
      length |= LO(spi_rx_header->length);

      _transition(driver, CC3K_STATE_READ_PAYLOAD);
      _spi(driver, driver->packet_tx_buffer, driver->packet_rx_buffer + sizeof(cc3k_spi_rx_header_t), length);
      break;

    case CC3K_STATE_READ_PAYLOAD:
      driver->stats.events++;
      _transition(driver, CC3K_STATE_EVENT);
      _assert_cs(driver, 0);
      break;

    case CC3K_STATE_COMMAND:
      // SPI transmission has completed, deassert /CS and wait for
      // and IRQ indicating the chip has a response
      _assert_cs(driver, 0);
      break;
  }

  return CC3K_OK;
}


cc3k_status_t cc3k_interrupt(cc3k_t *driver)
{
  // Called when interrupts were requested and the IRQ pin has fallen

  driver->stats.interrupts++;

  switch(driver->state)
  {
    case CC3K_STATE_COMMAND_REQUEST:
      // There is a pending command in the transmit buffer, and the chip has
      // notified us that it is ready to accept the command.
      _transition(driver, CC3K_STATE_COMMAND);
      _spi(driver, driver->packet_tx_buffer, driver->packet_rx_buffer, driver->packet_tx_buffer_length);
      break;
    case CC3K_STATE_SIMPLE_LINK_START:
      cc3k_read_header(driver);
      break;
    case CC3K_STATE_COMMAND:
      cc3k_read_header(driver);
      break;
    case CC3K_STATE_IDLE:
      cc3k_read_header(driver);
      break;
    case CC3K_STATE_INIT:
    case CC3K_STATE_READ_HEADER:
    case CC3K_STATE_READ_PAYLOAD:
      break;
    default:
      driver->stats.unhandled_interrupts++;
      driver->unhandled_state = driver->state;
      driver->interrupt_pending = 1;
      break;
  }

	return CC3K_OK;
}

static cc3k_status_t _process_event(cc3k_t *driver)
{
  cc3k_command_header_t *event_header;
  uint8_t *payload;
  
  event_header = (cc3k_command_header_t *)(driver->packet_rx_buffer + sizeof(cc3k_spi_rx_header_t)); 

  if(event_header->type != CC3K_PAYLOAD_TYPE_EVENT)
    return CC3K_INVALID;

  payload = driver->packet_rx_buffer + sizeof(cc3k_spi_rx_header_t) + sizeof(cc3k_command_header_t);

  if(event_header->opcode >= 0x4100)
  {
    // Async unsolocited event
    driver->stats.unsolicited++;
  }
  else
  {

    // This event should be a response to the last sent command
    if(event_header->opcode != driver->command)
      return CC3K_INVALID;

    // Handle the first simple link start command and kickoff a read_buffer_size
    switch(event_header->opcode)
    {
      case CC3K_COMMAND_SIMPLE_LINK_START:
        cc3k_send_command(driver, CC3K_COMMAND_READ_BUFFER_SIZE, NULL, 0);
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

  //_int_enable(driver, 0);

  switch(driver->state)
  {
    case CC3K_STATE_EVENT:
      _int_enable(driver, 0);

      // Transition to the IDLE state
      _transition(driver, CC3K_STATE_IDLE);

      // Process the event in packet_rx_buffer
      status = _process_event(driver);

      _int_enable(driver, 1);

      // Check if event processing failed
      if(status != CC3K_OK)
        return status;

      break;

    case CC3K_STATE_IDLE:

      //cc3k_send_command(driver, CC3K_COMMAND_IOCTL_STATUSGET, NULL, 0);

      // Check timer
      if(time_ms - driver->last_update > 10000)
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

  // Check if there is a pending interrupt
  if(driver->interrupt_pending)
  {
    cc3k_read_header(driver);
    driver->interrupt_pending = 0;
  }

  //_int_enable(driver, 1);

  return CC3K_OK;
}
