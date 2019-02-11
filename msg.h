#ifndef MSG_H
#define MSG_H

#include <inttypes.h>

#define HDLC_FD      0x7E
#define HDLC_ESCAPE  0x7D

enum e_log_level
{
	e_log_warn = 0,
	e_log_info = 1,
	e_log_debug = 2,
	e_log_all
};

#pragma pack(1)
typedef struct msg_t
{
	uint8_t len;
	uint8_t type;
	uint8_t data[0];
} __attribute__ ((__packed__)) msg_t;

typedef struct msg_flash_t
{
	uint16_t addr;
	uint8_t num;
	uint16_t data[0];
} __attribute__ ((__packed__)) msg_flash_t;

enum e_can_types
{
	e_can_simple = 0x0,
	e_can_statistic = 0x1,
	e_can_odd = 0x2,
	e_can_ext = 0x40,
	e_can_rtr = 0x80,
};

typedef struct msg_can_t
{
	uint32_t num;
	uint32_t id;
	uint8_t type;
	uint8_t len;
	uint8_t data[8];
} __attribute__ ((__packed__)) msg_can_t;

enum e_mode
{
	e_mode_bl,
	e_mode_sniffer,
	e_mode_silent_obd2,
};

typedef struct msg_status_t
{
	uint8_t mode;
	uint8_t version;
	uint8_t speed;
	uint32_t num_ids;
	uint32_t num_bytes;
} __attribute__ ((__packed__)) msg_status_t;

typedef struct msg_debug_t
{
	uint8_t len;
	uint8_t data[0];
} __attribute__ ((__packed__)) msg_debug_t;

enum e_cmd_types
{
	e_cmd_unset_filter = 0,
	e_cmd_set_filter
};

enum e_msg_types
{
	e_type_ping = 0,
	e_type_pong,
	e_type_status,
	e_type_erase,
	e_type_write,
	e_type_read,
	e_type_start,
	e_type_can,
	e_type_cmd,
	e_type_debug,
	e_type_unknown,
};
#pragma pack()

#endif

