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
  _assert_cs(driver, 1);
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

cc3k_status_t cc3k_command_send(cc3k_t *driver, uint16_t command)
{
  
  return CC3K_OK;
}

cc3k_status_t cc3k_init(cc3k_t *driver, cc3k_config_t *config)
{
  // Patch source parameter to simple link start command
  const uint8_t patch_source = 0x00;

  bzero(driver, sizeof(cc3k_t));

  driver->config = config;

  _int_enable(driver, 0);

  _chip_enable(driver, 0);
  (*config->delayMicroseconds)(100000); 
  _chip_enable(driver, 1);

  // Wait for the interrupt line to fall after enabling the chip
  _interrupt_poll_wait(driver);
  _assert_cs(driver, 1);

  // Enable interrupts
  _int_enable(driver, 1);

  // The first command sent to the chip is different than the rest, so we will
  // handle it manually here with asynchronous SPI calls.
  // This will send a CC3K_COMMAND_SIMPLE_LINK_START packet

  // Construct the command packet in the transmit buffer
  cc3k_command(driver, CC3K_COMMAND_SIMPLE_LINK_START, (uint8_t *)&patch_source, 1);

  // NOTE Special timing sequence for fist transaction
  // Send the first 4 bytes of the SPI header
  _spi_sync(driver, driver->packet_tx_buffer, driver->packet_rx_buffer, 4);
  // Delay for 50uS
  (*config->delayMicroseconds)(50); 
  // Send the remaining 6 bytes of the COMMAND_SIMPLE_LINK_START packet
  _spi_sync(driver, driver->packet_tx_buffer+4, driver->packet_rx_buffer + 4, 6);

  _assert_cs(driver, 0);

  // Check the returned data. The first 4 bytes should be 0x02, 0x00, 0xFF, 0x00
  // NOTE: It seems this isn't the case depending on chip patch level?

/*
  if(driver->packet_rx_buffer[0] != 0x02)
    return CC3K_ERROR;
  if(driver->packet_rx_buffer[2] != 0xFF)
    return CC3K_ERROR;
*/

  driver->state = CC3K_STATE_COMMAND;

	return CC3K_OK;	
}

cc3k_status_t cc3k_start(cc3k_t *driver)
{

  cc3k_command_send(driver, CC3K_COMMAND_SIMPLE_LINK_START);

  // Send CC3K_COMMAND_SIMPLE_LINK_START
  // Send CC3K_COMMAND_READ_BUFFER_SIZE
  

  return CC3K_OK;
}

static inline void _transition(cc3k_t *driver, cc3k_state_t state)
{
  driver->state = state;
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
      _transition(driver, CC3K_STATE_EVENT);
      _assert_cs(driver, 0);
      break;
  }


  return CC3K_OK;
}

cc3k_status_t cc3k_read_header(cc3k_t *driver)
{
  cc3k_spi_header_t *spi_header;
  spi_header = (cc3k_spi_header_t *)driver->packet_tx_buffer;
  spi_header->type = CC3K_PACKET_TYPE_READ;
  spi_header->length = 0;
  spi_header->busy = 0;

  _transition(driver, CC3K_STATE_READ_HEADER);
  _spi(driver, driver->packet_tx_buffer, driver->packet_rx_buffer, 5);
  return CC3K_OK;
}

cc3k_status_t cc3k_interrupt(cc3k_t *driver)
{
  // Called when interrupts were requested and the IRQ pin has fallen

  driver->stats.interrupts++;

  switch(driver->state)
  {
    case CC3K_STATE_COMMAND:
      cc3k_read_header(driver);
      break;

    case CC3K_STATE_IDLE:
      // Asynch unsolocited event
      // Transition to the READING state
      _transition(driver, CC3K_STATE_READ_HEADER);
      //_spi_read_header(driver);

      break;
  }

	return CC3K_OK;
}


cc3k_status_t cc3k_loop(cc3k_t *driver, uint32_t time_ms)
{
  uint32_t dt;

  if(driver->last_time_ms == 0)
    dt = 0;
  else
    dt = time_ms - driver->last_time_ms;

  driver->last_time_ms = time_ms;

  switch(driver->state)
  {
    case CC3K_STATE_EVENT:
      // Process the event in packet_rx_buffer

      // Transition to the idle state
      _transition(driver, CC3K_STATE_IDLE);
      break;

    default:
      // Do nothing
      break;
  }

  return CC3K_OK;
}
