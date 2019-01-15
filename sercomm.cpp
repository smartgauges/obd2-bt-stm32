/* Serial communications layer, based on HDLC */

/* (C) 2010 by Harald Welte <laforge@gnumonks.org>
 *
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <osmocom/core/msgb.h>

# define SERCOMM_RX_MSG_SIZE	2048
# ifndef ARRAY_SIZE
#  define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
# endif
# include <sercomm.h>

void sercomm_t::init(void)
{
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(tx.dlci_queues); i++)
		INIT_LLIST_HEAD(&tx.dlci_queues[i]);

	rx.msg = NULL;
	initialized = 1;

	/* set up the echo dlci */
	//register_rx_cb(SC_DLCI_ECHO, &sendmsg);
}

int sercomm_t::is_initialized(void)
{
	return initialized;
}

/* user interface for transmitting messages for a given DLCI */
void sercomm_t::sendmsg(uint8_t dlci, struct msgb *msg)
{
	//unsigned long flags;
	uint8_t *hdr;

	/* prepend address + control octet */
	hdr = msgb_push(msg, 2);
	hdr[0] = dlci;
	hdr[1] = HDLC_C_UI;

	msgb_enqueue(&tx.dlci_queues[dlci], msg);
}

/* how deep is the Tx queue for a given DLCI */
unsigned int sercomm_t::tx_queue_depth(uint8_t dlci)
{
	struct llist_head *le;
	unsigned int num = 0;

	llist_for_each(le, &tx.dlci_queues[dlci]) {
		num++;
	}

	return num;
}

/* fetch one octet of to-be-transmitted serial data */
int sercomm_t::drv_pull(uint8_t *ch)
{
	//unsigned long flags;

	/* we may be called from interrupt context, but we stiff need to lock
	 * because sercomm could be accessed from a FIQ context ... */

	if (!tx.msg) {
		unsigned int i;
		/* dequeue a new message from the queues */
		for (i = 0; i < ARRAY_SIZE(tx.dlci_queues); i++) {
			tx.msg = msgb_dequeue(&tx.dlci_queues[i]);
			if (tx.msg)
				break;
		}
		if (tx.msg) {
			/* start of a new message, send start flag octet */
			*ch = HDLC_FLAG;
			tx.next_char = tx.msg->data;
			return 1;
		} else {
			/* no more data avilable */
			return 0;
		}
	}

	if (tx.state == RX_ST_ESCAPE) {
		/* we've already transmitted the ESCAPE octet,
		 * we now need to transmit the escaped data */
		*ch = *tx.next_char++;
		tx.state = RX_ST_DATA;
	} else if (tx.next_char >= tx.msg->tail) {
		/* last character has already been transmitted,
		 * send end-of-message octet */
		*ch = HDLC_FLAG;
		/* we've reached the end of the message buffer */
		msgb_free(tx.msg);
		tx.msg = NULL;
		tx.next_char = NULL;
	/* escaping for the two control octets */
	} else if (*tx.next_char == HDLC_FLAG ||
		   *tx.next_char == HDLC_ESCAPE ||
		   *tx.next_char == 0x00) {
		/* send an escape octet */
		*ch = HDLC_ESCAPE;
		/* invert bit 5 of the next octet to be sent */
		*tx.next_char ^= (1 << 5);
		tx.state = RX_ST_ESCAPE;
	} else {
		/* standard case, simply send next octet */
		*ch = *tx.next_char++;
	}

	return 1;
}

/* dispatch an incoming message once it is completely received */
void sercomm_t::dispatch_rx_msg(uint8_t dlci, struct msgb *msg)
{
	if (dlci < _SC_DLCI_MAX)
		emit sig_dlci(dlci, msg);

	msgb_free(msg);
}

/* the driver has received one byte, pass it into sercomm layer */
int sercomm_t::drv_rx_char(uint8_t ch)
{
	uint8_t *ptr;

	/* we are always called from interrupt context in this function,
	 * which means that any data structures we use need to be for
	 * our exclusive access */
	if (!rx.msg)
		rx.msg = sercomm_alloc_msgb(SERCOMM_RX_MSG_SIZE);

	if (msgb_tailroom(rx.msg) == 0) {
		//cons_puts("sercomm_drv_rx_char() overflow!\n");
		msgb_free(rx.msg);
		rx.msg = sercomm_alloc_msgb(SERCOMM_RX_MSG_SIZE);
		rx.state = RX_ST_WAIT_START;

		return 0;
	}

	switch (rx.state) {
	case RX_ST_WAIT_START:
		if (ch != HDLC_FLAG)
			break;
		rx.state = RX_ST_ADDR;
		break;
	case RX_ST_ADDR:
		rx.dlci = ch;
		rx.state = RX_ST_CTRL;
		break;
	case RX_ST_CTRL:
		rx.ctrl = ch;
		rx.state = RX_ST_DATA;
		break;
	case RX_ST_DATA:
		if (ch == HDLC_ESCAPE) {
			/* drop the escape octet, but change state */
			rx.state = RX_ST_ESCAPE;
			break;
		} else if (ch == HDLC_FLAG) {
			/* message is finished */
			dispatch_rx_msg(rx.dlci, rx.msg);
			/* allocate new buffer */
			rx.msg = NULL;
			/* start all over again */
			rx.state = RX_ST_WAIT_START;

			/* do not add the control char */
			break;
		}
		/* default case: store the octet */
		ptr = msgb_put(rx.msg, 1);
		*ptr = ch;
		break;
	case RX_ST_ESCAPE:
		/* store bif-5-inverted octet in buffer */
		ch ^= (1 << 5);
		ptr = msgb_put(rx.msg, 1);
		*ptr = ch;
		/* transition back to normal DATA state */
		rx.state = RX_ST_DATA;
		break;
	}

	return 1;
}

sercomm_t::sercomm_t()
{
	memset(&tx, 0, sizeof(tx));
	memset(&rx, 0, sizeof(rx));

	initialized = 0;
}

