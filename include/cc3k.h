/**
 * @file cc3k.h
 *
 * CC3K Driver API
 */

#ifndef _CC3K_H
#define _CC3K_H

#include <cc3k_type.h>
#include <cc3k_packet.h>
#include <cc3k_command.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CC3K_BUFFER_SIZE 1500

typedef enum _cc3k_state_t
{
	CC3K_STATE_INIT,            // Initial state
  CC3K_STATE_SIMPLE_LINK_START, // Initial simple link start command issued
  CC3K_STATE_COMMAND_REQUEST, // /CS asserted, waiting for IRQ before sending command
  CC3K_STATE_COMMAND,         // Sent a command, waiting for response
  CC3K_STATE_IDLE,            // Idle
  CC3K_STATE_READ_HEADER,     // CS low, IRQ received, clocking in data
  CC3K_STATE_READ_PAYLOAD,    // Reading payload bytes
  CC3K_STATE_EVENT,           // Event was received for the main loop to process
  CC3K_STATE_WRITE_REQUEST,
  CC3K_STATE_WRITING,
} cc3k_state_t;

/**
 * @brief Driver configuration
 *
 * Contains callback function pointers that the
 * caller must implement to access the SPI peripheral
 * and GPIO pins of the host.
 */
typedef struct _cc3k_config_t
{
  /** @brief Delay for number of microseconds */
  void (*delayMicroseconds)(uint32_t us);
  /** @brief Enable/Disable chip with EN pin */
  void (*enableChip)(int enable);
  /** @brief Read the /INT pin */
  int (*readInterrupt)(void);
  /** @brief Enable/Disable Interrupts */
  void (*enableInterrupt)(int enable);
  /** @brief Assert/Deassert CS pin */
  void (*assertChipSelect)(int assert);
  /** @brief Synchronous SPI Send/Receive */
  void (*spiTransaction)(uint8_t *out, uint8_t *in, uint16_t length, int async);
} cc3k_config_t;

typedef struct _cc3k_stats_t
{
  uint32_t interrupts;
  uint32_t unhandled_interrupts;
  uint32_t spi_done;
  uint32_t commands;
  uint32_t events;
  uint32_t socket_writes;
  uint32_t socket_reads;
} cc3k_stats_t;

/**
 * @brief Driver Context
 */
typedef struct _cc3k_t
{
  cc3k_config_t *config;

  cc3k_stats_t stats;

	/** @brief Current operational state */
	cc3k_state_t state;

  /** @brief State of last unhandled interrupt */
  cc3k_state_t unhandled_state;

  /** @brief Last command sent */
  cc3k_command_t command;

  /** @brief Flag to indicate an unhandled interrupt is pending */
  // TODO: Turn this into a bitmask
  uint8_t interrupt_pending;

	/**
    * @brief Pointer to SPI Packet buffer
    * Only one buffer should be required, as CC3000 SPI is simplex
    */ 
	uint8_t packet_tx_buffer[CC3K_BUFFER_SIZE];
	uint8_t packet_rx_buffer[CC3K_BUFFER_SIZE];

  uint16_t packet_tx_buffer_length;
  uint16_t packet_rx_buffer_length;

  uint32_t last_time_ms;
  uint32_t last_update;

  /** @brief Number of buffers available on the chip */
  uint8_t buffers;

  // TODO: This doesn't need to be 32 bit
  uint32_t wlan_status;
} cc3k_t;

cc3k_status_t cc3k_init(cc3k_t *driver, cc3k_config_t *config);

/**
 * @brief SPI tranfer complete notification
 *
 * This may be called from a DMA ISR
 */
cc3k_status_t cc3k_spi_done(cc3k_t *driver);

/**
 * @brief Driver interrupt handler
 *
 * This must be called on each falling edge of the IRQ pin
 * if the driver has requested that interrupts are enabled.
 *
 * The driver requests interrupts using the enableInterrupt callback in the driver config structure
 */
cc3k_status_t cc3k_interrupt(cc3k_t *driver);

/**
 * @brief Create a command packet in the transmit buffer
 */
cc3k_status_t cc3k_command(cc3k_t *driver, uint16_t opcode, uint8_t *arg, uint8_t argument_length);

cc3k_status_t cc3k_process_event(cc3k_t *driver, uint16_t opcode, uint8_t *arg, uint8_t arg_length);

cc3k_status_t cc3k_loop(cc3k_t *driver, uint32_t time_ms);

#ifdef __cplusplus
} // End of extern "C"
#endif

#endif
