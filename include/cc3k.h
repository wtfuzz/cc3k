/**
 * @file cc3k.h
 *
 * CC3K Driver API
 */

#ifndef _CC3K_H
#define _CC3K_H

#include <cc3k_type.h>
#include <cc3k_packet.h>

typedef enum _cc3k_state_t
{
	CC3K_STATE_INIT,
	CC3K_STATE_CONNECTING,
	CC3K_STATE_CONNECTED,
	CC3K_STATE_DHCP,
	CC3K_STATE_READY,
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
  /** @brief Enable/Disable chip with EN pin */
  void (*enableChip)(int enable);
  /** @brief Read the CS pin */
  int (*readChipSelect)(void);
  /** @brief Enable/Disable Interrupts */
  void (*enableInterrupt)(int enable);
  /** @brief Assert/Deassert CS pin */
  void (*assertChipSelect)(int assert);
  /** @brief Send SPI data */
  /** @brief Read SPI data */
} cc3k_config_t;

/**
 * @brief Driver Context
 */
typedef struct _cc3k_t
{
  cc3k_config_t *config;

	/** @brief Current operational state */
	cc3k_state_t state;

	/**
    * @brief Pointer to SPI Packet buffer
    * Only one buffer should be required, as CC3000 SPI is simplex
    */ 
	uint8_t *packet_buffer;
	uint16_t packet_buffer_length;
} cc3k_t;

cc3k_status_t cc3k_init(cc3k_t *driver);

#endif
