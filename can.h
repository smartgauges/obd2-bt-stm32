#ifndef CAN_H
#define CAN_H

#include "msg.h"

typedef enum e_speed_t
{
	e_speed_125 = 0,
	e_speed_250,
	e_speed_500,
	e_speed_1000,
	e_speed_nums
} e_speed_t;
 
uint8_t can_setup(void);
uint8_t can_get_msgs_num(void);
uint8_t can_get_msg(struct msg_can_t * msg, uint8_t idx);
void can_snd_msg(struct msg_can_t * msg);
uint8_t can_set_speed(e_speed_t speed);

#endif

