#include <stdio.h>
#include <stdlib.h>

#include <cc3k.h>

void enable_interrupt(int enable)
{
  fprintf(stderr, "INT %s\n", enable == 1 ? "Enable" : "Disable");
}

int read_interrupt(void)
{
  fprintf(stderr, "Read Interrupt\n");
  return 0;
}

void assert_chip_select(int assert)
{
  fprintf(stderr, "CS %s\n", assert == 1 ? "LOW" : "HIGH");
}

void enable_chip(int enable)
{
  fprintf(stderr, "EN %s\n", enable == 1 ? "Enable" : "Disable");
}

void spi_sync(uint8_t *out, uint8_t *in, uint16_t length)
{
  fprintf(stderr, "Out %p in %p length %d\n", out, in, length);
}

int main(int argc, char **argv)
{
  cc3k_status_t status;

  cc3k_config_t config;
  cc3k_t cc3k;

  config.assertChipSelect = assert_chip_select;
  config.enableInterrupt = enable_interrupt;
  config.readInterrupt = read_interrupt;
  config.enableChip = enable_chip;
  config.spiTransaction = spi_sync;

  status = cc3k_init(&cc3k, &config);
  if(status != CC3K_OK)
  {
    fprintf(stderr, "ERROR Init returned %d\n", status);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
