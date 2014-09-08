#ifndef CC3K_DATA_H_
#define CC3K_DATA_H_

typedef enum _cc3k_data_opcode_t
{
  CC3K_DATA_SEND = 0x81,
  CC3K_DATA_RECV = 0x82,
  CC3K_DATA_SENDTO = 0x83,
  CC3K_DATA_RECVFROM = 0x84,
} cc3k_data_opcode_t;

typedef struct _cc3k_data_sendto_t
{
  uint32_t sd;
  uint32_t unk; // 0x14
  uint32_t payload_length;
  uint32_t flags; // 0x0
  uint32_t offset;  // payload_length + 8
  uint32_t unused_length; // 0x8
} cc3k_data_sendto_t;

typedef struct _cc3k_data_recvfrom_t
{
  uint32_t sd;
  uint32_t unk; // 0x0C?
  uint32_t payload_length;
  uint32_t flags;
  uint32_t zero1;
  uint32_t zero2;
} cc3k_data_recvfrom_t;

#endif
