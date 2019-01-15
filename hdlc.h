#ifndef HDLC_H
#define HDLC_H

#include <inttypes.h>

#define HDLC_FD      0x7E
#define HDLC_ESCAPE  0x7D

void hdlc_put_msg(const struct msg_t * msg);
struct msg_t * hdlc_get_msg(uint8_t ch);

#endif

