#include <cc3k.h>
#include <cc3k_type.h>
#include <cc3k_packet.h>

cc3k_status_t cc3k_packet_write(cc3k_t *driver, uint8_t *payload, uint16_t payload_length)
{
	cc3k_spi_header_t *spi_header;

	spi_header = (cc3k_spi_header_t *)driver->packet_buffer;

	spi_header->type = CC3K_PACKET_TYPE_WRITE;
	spi_header->length = payload_length;
	spi_header->busy = 0;

	// If the total packet length is odd, add a padding byte
	if((payload_length + sizeof(cc3k_spi_header_t)) % 2 != 0)
	{
		// Add a padding byte
		spi_header->length++;
	}
	
	return CC3K_OK;
}


cc3k_status_t cc3k_packet_read()
{
	cc3k_spi_header_t spi_header;

	spi_header.type = CC3K_PACKET_TYPE_READ;
	spi_header.length = 0;
	spi_header.busy = 0;
	
	return CC3K_OK;
}
