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

#endif
