#include <cc3k.h>
#include <cc3k_type.h>
#include <cc3k_packet.h>
#include <string.h>

// Fill in the SPI header
cc3k_status_t cc3k_spi_header(cc3k_t *driver, int type, uint16_t payload_length)
{
	cc3k_spi_header_t *spi_header;

	spi_header = (cc3k_spi_header_t *)driver->packet_tx_buffer;

	// If the total packet length is odd, add a padding byte
	if((payload_length + sizeof(cc3k_spi_header_t)) % 2 != 0)
    payload_length++;

	spi_header->type = type;
	spi_header->length = HI(payload_length);
  spi_header->length |= LO(payload_length);
	spi_header->busy = 0;

  driver->packet_tx_buffer_length = payload_length + sizeof(cc3k_spi_header_t);

  return CC3K_OK;
}

cc3k_status_t cc3k_command(cc3k_t *driver, uint16_t opcode, uint8_t *arg, uint8_t argument_length)
{
  cc3k_command_header_t *cmd_header;
	cmd_header = (cc3k_command_header_t *)(driver->packet_tx_buffer + sizeof(cc3k_spi_header_t));

  cmd_header->type = CC3K_PAYLOAD_TYPE_COMMAND;
  cmd_header->opcode = opcode;
  cmd_header->argument_length = argument_length;

  if(argument_length > 0)
    memcpy((uint8_t *)cmd_header + sizeof(cc3k_command_header_t), arg, argument_length);

  // Keep the last command opcode in the driver context
  driver->command = opcode;


  return cc3k_spi_header(driver, CC3K_PACKET_TYPE_WRITE, sizeof(cc3k_command_header_t) + argument_length); 
}

/**
cc3k_status_t cc3k_packet(cc3k_t *driver, int type, uint8_t *payload, uint16_t payload_length)
{
	cc3k_spi_header_t *spi_header;

	spi_header = (cc3k_spi_header_t *)driver->packet_tx_buffer;

	spi_header->type = type;
	spi_header->length = ((payload_length & 0xFF00) >> 8);
  spi_header->length |= ((payload_length & 0x00FF) << 8);
	spi_header->busy = 0;

	// If the total packet length is odd, add a padding byte
	if((payload_length + sizeof(cc3k_spi_header_t)) % 2 != 0)
	{
		// Add a padding byte
		spi_header->length++;
	}

  memcpy(driver->packet_tx_buffer + sizeof(cc3k_spi_header_t), payload, payload_length);
	
	return CC3K_OK;
}
*/
