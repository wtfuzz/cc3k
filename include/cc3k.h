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
 * @brief Driver Context
 */
typedef struct _cc3k_t
{
	/** @brief Current operational state */
	cc3k_state_t state;

	/**
    * @brief Pointer to SPI Packet buffer
    * Only one buffer should be required, as CC3000 SPI is simplex
    */ 
	uint8_t *packet_buffer;
	uint16_t packet_buffer_length;
} cc3k_t;

#endif
