#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "can.h"
#include "led.h"

typedef struct speed_t
{
	uint32_t sjw;
	uint32_t ts1;
	uint32_t ts2;
	uint32_t brp;
} speed_t;

/* APB1 48 MHz */
static speed_t speeds[e_speed_nums] = 
{
	{ CAN_BTR_SJW_1TQ, CAN_BTR_TS1_13TQ, CAN_BTR_TS2_2TQ, 24 },
	{ CAN_BTR_SJW_1TQ, CAN_BTR_TS1_13TQ, CAN_BTR_TS2_2TQ, 12 },
	{ CAN_BTR_SJW_1TQ, CAN_BTR_TS1_13TQ, CAN_BTR_TS2_2TQ, 6 },
	{ CAN_BTR_SJW_1TQ, CAN_BTR_TS1_13TQ, CAN_BTR_TS2_2TQ, 3 },
};

//#define TEST_LOOP
uint8_t can_set_speed(e_speed_t speed)
{
	nvic_disable_irq(NVIC_CEC_CAN_IRQ);
	can_disable_irq(CAN1, CAN_IER_FMPIE0);

	/* Reset CAN. */
	can_reset(CAN1);

	/* CAN cell init. apb1 48 MHZ */
	int ret = can_init(CAN1,
		     false,           /* TTCM: Time triggered comm mode? */
		     true,            /* ABOM: Automatic bus-off management? */
		     false,           /* AWUM: Automatic wakeup mode? */
		     false,           /* NART: No automatic retransmission? */
		     false,           /* RFLM: Receive FIFO locked mode? */
		     false,           /* TXFP: Transmit FIFO priority? */
		     speeds[speed].sjw,
		     speeds[speed].ts1,
		     speeds[speed].ts2,
		     speeds[speed].brp,
#ifdef TEST_LOOP
		     true,
		     true
#else
		     false,
		     false
#endif
		     );

	if (ret)
		return ret;

	/* CAN filter 0 init. */
	can_filter_id_mask_32bit_init(CAN1,
				0,     /* Filter ID */
				0,     /* CAN ID */
				0,     /* CAN ID mask */
				0,     /* FIFO assignment (here: FIFO0) */
				true); /* Enable the filter. */

	/* Enable CAN RX interrupt. */
	can_enable_irq(CAN1, CAN_IER_FMPIE0);
	nvic_enable_irq(NVIC_CEC_CAN_IRQ);

	return 0;
}

#define MSGS_SIZE 80
struct msg_can_t msgs[MSGS_SIZE];
static uint8_t msgs_size = 0;

#define GPIO_CAN_RX GPIO11
#define GPIO_CAN_TX GPIO12

uint8_t can_setup(void)
{
	/* Enable peripheral clocks. */
	rcc_periph_clock_enable(RCC_CAN);

#if 0
	msgs_size = 18;
	for (int i = 0; i < msgs_size; i++)
		msgs[i].id = i;
#endif

	/* Configure CAN pin: RX (input pull-up). */
	//gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_CAN_RX | GPIO_CAN_TX);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_CAN_RX);
	gpio_set_af(GPIOA, GPIO_AF4, GPIO_CAN_RX);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_CAN_TX);
	gpio_set_af(GPIOA, GPIO_AF4, GPIO_CAN_TX);

	/* NVIC setup. */
	nvic_enable_irq(NVIC_CEC_CAN_IRQ);
	nvic_set_priority(NVIC_CEC_CAN_IRQ, 1);

	return can_set_speed(e_speed_125);
}

uint8_t can_get_msgs_num(void)
{
	return msgs_size;
}

uint8_t can_get_msg(struct msg_can_t * msg, uint8_t idx)
{
	if (idx >= msgs_size)
		return 0;

	*msg = msgs[idx];

	return 1;
}

void cec_can_isr(void)
{
	uint32_t fmi;
	struct msg_can_t msg;
	uint8_t i, j;

	bool rtr = 0, ext = 0;
	can_receive(CAN1, 0, false, &msg.id, &ext, &rtr, &fmi, &msg.len, msg.data);

	msg.type = e_can_statistic;
	if (rtr)
		msg.type |= e_can_rtr;
	if (ext)
		msg.type |= e_can_ext;

	uint8_t found = 0;
	for (i = 0; i < msgs_size; i++) {

		if (msgs[i].id == msg.id) {

			//obd2 check
			if (0x7e8 <= msg.id && 0x7ef >= msg.id) {

				//Number of additional data bytes
				if (msgs[i].data[0] != msg.data[0])
					continue;
				//Custom service Same as query, except that 40h is added to the service value
				if (msgs[i].data[1] != msg.data[1])
					continue;
				//PID
				if (msgs[i].data[2] != msg.data[2])
					continue;
			}

			msgs[i].len = msg.len;
			for (j = 0; j < 8; j++)
				msgs[i].data[j] = msg.data[j];
			msgs[i].num++;
			found = 1;
			break;
		}
	}

	if (!found && msgs_size < MSGS_SIZE) {

		msgs[msgs_size] = msg;
		msgs[msgs_size].num = 1;
		msgs_size++;
	}

	can_fifo_release(CAN1, 0);
}

void can_snd_msg(struct msg_can_t * msg)
{
	if (!can_available_mailbox(CAN1)) {

		CAN_TSR(CAN1) |= CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_TABRQ2;
	}

	bool rtr = msg->type & e_can_rtr;
	bool ext = msg->type & e_can_ext;
	can_transmit(CAN1, msg->id, ext, rtr, msg->len, msg->data);
}

