#include <stdlib.h>
#include <cc3k.h>

cc3k_status_t cc3k_init(cc3k_t *driver)
{

  if(driver->config == NULL)
    return CC3K_INVALID;

  driver->state = CC3K_STATE_INIT;

	return CC3K_OK;	
}

cc3k_status_t cc3k_start(cc3k_t *driver)
{

  // Send CC3K_COMMAND_SIMPLE_LINK_START
  // Send CC3K_COMMAND_READ_BUFFER_SIZE
  

  return CC3K_OK;
}

cc3k_status_t cc3k_interrupt(cc3k_t *driver)
{
	return CC3K_OK;
}
