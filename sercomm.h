#ifndef SERCOMM_H
#define SERCOMM_H

#include <QObject>

#include <osmocom/core/msgb.h>

#define HDLC_FLAG	0x7E
#define HDLC_ESCAPE	0x7D

#define HDLC_C_UI	0x03
#define HDLC_C_P_BIT	(1 << 4)
#define HDLC_C_F_BIT	(1 << 4)

/* a low sercomm_dlci means high priority.  A high DLCI means low priority */
enum sercomm_dlci {
	SC_DLCI_HIGHEST = 0,
	SC_DLCI_WD      = 3,
	SC_DLCI_DEBUG   = 4,
	SC_DLCI_L1A_L23 = 5,
	SC_DLCI_LOADER  = 9,
	SC_DLCI_CONSOLE = 10,
	SC_DLCI_BATTERY = 11,
	SC_DLCI_ECHO    = 128,
	_SC_DLCI_MAX
};

/* receiving messages for a given DLCI */
typedef void (*dlci_cb_t)(uint8_t dlci, struct msgb *msg);

class sercomm_t : public QObject
{
	Q_OBJECT
	private:
		enum rx_state {
			RX_ST_WAIT_START,
			RX_ST_ADDR,
			RX_ST_CTRL,
			RX_ST_DATA,
			RX_ST_ESCAPE,
		};

		int initialized;

		/* transmit side */
		struct {
			struct llist_head dlci_queues[_SC_DLCI_MAX];
			struct msgb *msg;
			enum rx_state state;
			uint8_t *next_char;
		} tx;

		/* receive side */
		struct {
			struct msgb *msg;
			enum rx_state state;
			uint8_t dlci;
			uint8_t ctrl;
		} rx;

	private:
		void dispatch_rx_msg(uint8_t dlci, struct msgb *msg);
	public:
		sercomm_t();

		void init(void);
		int is_initialized(void);

		/* User Interface: Tx */

		/* user interface for transmitting messages for a given DLCI */
		void sendmsg(uint8_t dlci, struct msgb *msg);
		/* how deep is the Tx queue for a given DLCI */
		unsigned int tx_queue_depth(uint8_t dlci);

		/* User Interface: Rx */

		int register_rx_cb(uint8_t dlci, dlci_cb_t cb);

		/* Driver Interface */

		/* fetch one octet of to-be-transmitted serial data. returns 0 if no more data */
		int drv_pull(uint8_t *ch);
		/* the driver has received one byte, pass it into sercomm layer.
		   returns 1 in case of success, 0 in case of unrecognized char */
		int drv_rx_char(uint8_t ch);

	signals:
		void sig_dlci(uint8_t dlci, struct msgb * msg);
};

static inline struct msgb *sercomm_alloc_msgb(unsigned int len)
{
	return msgb_alloc_headroom(len+4, 4, "sercomm_tx");
}

#endif

