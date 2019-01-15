#include "msg.h"

int usart2_write(const uint8_t * ptr, int len);

void hdlc_put_msg(const struct msg_t * msg)
{
	uint8_t buf[msg->len * 2 + 2];
	uint8_t idx = 0;

	uint8_t * p = (uint8_t *)msg;

	buf[idx++] = HDLC_FD;
	for (uint16_t i = 0; i < msg->len; i++) {

		if (p[i] == HDLC_FD || p[i] == HDLC_ESCAPE) {

			buf[idx++] = HDLC_ESCAPE;
			buf[idx++] = p[i] & ~(1 << 5);
		}
		else
			buf[idx++] = p[i];
	}
	buf[idx++] = HDLC_FD;

	usart2_write(buf, idx);
}

enum rx_state
{
	RX_ST_WAIT_START,
	RX_ST_DATA,
	RX_ST_ESCAPE,
};

#define MSG_BUFFER_SIZE 128
static uint8_t rx_buffer[MSG_BUFFER_SIZE];
static uint8_t rx_idx = 0;
static uint8_t rx_state = RX_ST_WAIT_START;

struct msg_t * hdlc_get_msg(uint8_t ch)
{
	switch (rx_state) {

		case RX_ST_WAIT_START:
	
			if (ch != HDLC_FD)
				break;

			rx_idx = 0;
			rx_state = RX_ST_DATA;

			break;

		case RX_ST_DATA:

			if (ch == HDLC_ESCAPE) {

				/* drop the escape octet, but change state */
				rx_state = RX_ST_ESCAPE;
				break;
			}
			else if (ch == HDLC_FD) {

				/* message is finished */
				/* start all over again */
				rx_state = RX_ST_WAIT_START;
				struct msg_t * msg = (struct msg_t *)rx_buffer;
				if (msg->len == rx_idx && rx_idx >= 2)
					return msg;
				else {

					/* XX XX FD FD */
					rx_idx = 0;
					rx_state = RX_ST_DATA;

					return 0;
				}
				break;
			}

			/* default case: store the octet */
			rx_buffer[rx_idx++] = ch;

			if (rx_idx >= MSG_BUFFER_SIZE)
				rx_state = RX_ST_WAIT_START;

			break;

		case RX_ST_ESCAPE:

			/* transition back to normal DATA state */
			rx_state = RX_ST_DATA;

			/* store bif-5-inverted octet in buffer */
			ch |= (1 << 5);
			rx_buffer[rx_idx++] = ch;

			if (rx_idx >= MSG_BUFFER_SIZE)
				rx_state = RX_ST_WAIT_START;
			break;
	}

	return 0;
}

