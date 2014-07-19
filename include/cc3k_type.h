/**
 * @file cc3k_type.h
 *
 * Type declarations
 */

#ifndef _CC3K_TYPE_H
#define _CC3K_TYPE_H

#include <inttypes.h>

typedef enum _cc3k_status_t
{
	CC3K_OK,
	CC3K_ERROR,
  CC3K_INVALID,
  CC3K_INVALID_STATE,
} cc3k_status_t;

/**
 * @brief Socket address
 */
typedef struct _cc3k_sockaddr_t
{
  uint16_t family;
  uint16_t port;    // Big endian
  uint32_t addr;    // Big endian
} cc3k_sockaddr_t;

#endif
