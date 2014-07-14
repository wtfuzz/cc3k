#include <stdio.h>
#include <stdlib.h>

#include <cc3k.h>

void enable_interrupt(int enable)
{
  fprintf(stderr, "INT %s\n", enable == 1 ? "Enable" : "Disable");
}

void assert_chip_select(int assert)
{
  fprintf(stderr, "CS %s\n", assert == 1 ? "LOW" : "HIGH");
}

int main(int argc, char **argv)
{
  cc3k_status_t status;

  cc3k_config_t config;
  cc3k_t cc3k;

  config.assertChipSelect = assert_chip_select;
  config.enableInterrupt = enable_interrupt;

  status = cc3k_init(&cc3k);
  if(status != CC3K_OK)
  {
    fprintf(stderr, "Init returned %d\n", status);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
