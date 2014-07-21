/**
 * @file cc3k.h
 *
 * CC3K Driver API
 */

#ifndef _CC3K_H
#define _CC3K_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cc3k_t cc3k_t;

#include <cc3k_type.h>
#include <cc3k_packet.h>
#include <cc3k_command.h>
#include <cc3k_event.h>
#include <cc3k_socket.h>

#define CC3K_BUFFER_SIZE 1500+200

/**
 * @brief Driver SPI protocol state
 */
typedef enum _cc3k_state_t
{
	CC3K_STATE_INIT,              // Initial state
  CC3K_STATE_SIMPLE_LINK_START, // Initial simple link start command issued
  CC3K_STATE_COMMAND_REQUEST,   // /CS asserted, waiting for IRQ before sending command
  CC3K_STATE_SEND_COMMAND,      // Sending a command, waiting for SPI completion
  CC3K_STATE_COMMAND,           // Sent a command, waiting for response
  CC3K_STATE_IDLE,              // Idle
  CC3K_STATE_READ_HEADER,       // CS low, IRQ received, clocking in data
  CC3K_STATE_READ_PAYLOAD,      // Reading payload bytes
  CC3K_STATE_EVENT,             // Event was received for the main loop to process
  CC3K_STATE_DATA_REQUEST,      // Driver is requesting data transmission to chip. CS low, waiting for IRQ
  CC3K_STATE_DATA,              // Performing SPI transaction, and waiting for a response
  CC3K_STATE_DATA_RX_REQUEST,   // Receiving a data frame
  CC3K_STATE_DATA_RX,           // Receiving a data frame
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

  /** @brief Event hook */
  void (*eventCallback)(uint16_t opcode, uint8_t *data, uint16_t length);
  void (*commandCallback)(uint16_t opcode, uint8_t *data, uint16_t length);
  void (*dataCallback)();
  void (*transitionCallback)(cc3k_state_t from, cc3k_state_t to);

} cc3k_config_t;

typedef struct _cc3k_stats_t
{
  uint32_t interrupts;
  uint32_t unhandled_interrupts;
  uint32_t spi_done;
  uint32_t commands;
  uint32_t events;
  uint32_t unsolicited;
  uint32_t socket_writes;
  uint32_t socket_reads;
  uint32_t tx;
  uint32_t rx;
} cc3k_stats_t;

/**
 * @brief IP Configuration structure
 */
typedef struct _cc3k_ipconfig_t
{
  uint32_t ip;
  uint32_t netmask;
  uint32_t default_gateway;
  uint32_t dhcp_server;
  uint32_t dns_server;
} cc3k_ipconfig_t;

/**
 * @brief Driver Context
 */
struct _cc3k_t
{
  cc3k_config_t *config;

  cc3k_ipconfig_t ipconfig;
  uint8_t dhcp_complete;

  cc3k_stats_t stats;
  
  /** @brief Socket Manager context */
  cc3k_socket_manager_t socket_manager;

	/** @brief Current operational state */
	cc3k_state_t state;
	cc3k_state_t last_state;
  cc3k_state_t int_state;

  int spi_busy;
  int spi_unhandled;

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
};

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
 * @brief Send an asynchronous command to the chip
 */
cc3k_status_t cc3k_send_command(cc3k_t *driver, uint16_t opcode, uint8_t *arg, uint8_t args_length);

/**
 * @brief Create a command packet in the transmit buffer
 */
cc3k_status_t cc3k_command(cc3k_t *driver, uint16_t opcode, uint8_t *arg, uint8_t argument_length);
cc3k_status_t cc3k_data(cc3k_t *driver, uint8_t opcode, uint8_t *arg, uint8_t arg_length,
  uint8_t *payload, uint16_t payload_length, uint8_t *footer, uint8_t footer_length);

cc3k_status_t cc3k_set_debug(cc3k_t *driver, uint32_t level);

cc3k_status_t cc3k_process_event(cc3k_t *driver, uint16_t opcode, uint8_t *arg, uint8_t arg_length);
cc3k_status_t cc3k_recv_event(cc3k_socket_manager_t *socket_manager, int32_t sd, int32_t length);
cc3k_status_t cc3k_tcp_close_wait_event(cc3k_socket_manager_t *socket_manager, cc3k_tcp_close_wait_event_t *ev);

cc3k_status_t cc3k_loop(cc3k_t *driver, uint32_t time_ms);

/**
 * @brief Start connecting to an AP
 */
cc3k_status_t cc3k_wlan_connect(cc3k_t *driver, cc3k_security_type_t security_type, const char *ssid, uint8_t ssid_length, char *key, uint8_t key_length);

cc3k_status_t cc3k_socket(cc3k_t *driver, int family, int type, int protocol);
cc3k_status_t cc3k_connect(cc3k_t *driver, int sd, cc3k_sockaddr_t *sa);
cc3k_status_t cc3k_recv(cc3k_t *driver, int sd, uint16_t length);

#ifdef __cplusplus
} // End of extern "C"
#endif

#endif
