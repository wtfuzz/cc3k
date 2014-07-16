/**
 * @file cc3k_packet.h
 *
 * CC3K SPI Packet Interface
 */

#ifndef _CC3K_SPI_PACKET_H
#define _CC3K_SPI_PACKET_H

#include <inttypes.h>

#define HI(word) ((word & 0xFF00) >> 8)
#define LO(word) ((word & 0x00FF) << 8)

typedef enum _cc3k_spi_packet_type_t
{
	CC3K_PACKET_TYPE_WRITE	= 0x1,
  CC3K_PACKET_TYPE_REPLY = 0x02,
	CC3K_PACKET_TYPE_READ	= 0x3
} cc3k_spi_packet_type_t;

/**
 * @brief SPI Packet Header
 *
 * Describes the 5 byte SPI packet header
 */
typedef struct _cc3k_spi_header_t
{
	uint8_t type;       		// Operation (CC3K_PACKET_TYPE_WRITE or CC3K_PACKET_TYPE_READ)
	uint16_t length;      	// Length bytes for write operation, 0 for read
	uint16_t busy;      		// Busy bytes, always 0
} __attribute__ ((packed)) cc3k_spi_header_t;

typedef struct _cc3k_spi_rx_header_t
{
	uint8_t type;       		// Operation (CC3K_PACKET_TYPE_REPLY)
	uint16_t busy;      		// Busy bytes, always 0
	uint16_t length;      	// Length bytes for read operation
} __attribute__ ((packed)) cc3k_spi_rx_header_t;

/**
 * @brief SPI Payload Types
 */
typedef enum _cc3k_payload_type_t
{
	CC3K_PAYLOAD_TYPE_COMMAND	= 0x01,
	CC3K_PAYLOAD_TYPE_DATA		= 0x02,
	//CC3K_PAYLOAD_TYPE_PATCH		= 0x03,
	CC3K_PAYLOAD_TYPE_EVENT		= 0x04
} cc3k_payload_type_t;

/**
 * @brief Header for Command and Event messages
 *
 * This 4 byte header is common between commands and events.
 * If the type is CC3K_PAYLOAD_TYPE_COMMAND, it is an outbound message for the CC3K
 * If the type is CC3K_PAYLOAD_TYPE_EVENT, it is an event arriving from the CC3K
 */
typedef struct _cc3k_command_header_t
{
	uint8_t type;	// CC3K_PAYLOAD_TYPE_COMMAND or CC3K_PAYLOAD_TYPE_EVENT
	uint16_t opcode;
	uint8_t argument_length;
} __attribute__ ((packed)) cc3k_command_header_t;

/**
 * @brief Data packet header
 */
typedef struct _cc3k_data_header_t
{
	uint8_t type;	// Always CC3K_PAYLOAD_TYPE_DATA
	uint8_t opcode;
	uint8_t argument_length;
	uint16_t payload_length;
} __attribute__ ((packed)) cc3k_data_header_t;

#endif
