#ifndef MAIN_H
#define MAIN_H

#include <QMainWindow>
#include <QMap>
#include <QTimer>
#include <QElapsedTimer>

#include "list.h"

namespace Ui
{
	class can;
}

class main_t : public QMainWindow
{
	Q_OBJECT

	private:
		Ui::can * ui;
		uint8_t log_level;
		uint32_t rx_cnt;
		uint32_t tx_cnt;
		uint32_t err_cnt;
		uint32_t msg_sync;

		QVector <msg_can_t> rx_msgs;
		QMap <uint32_t, info_t> rx_id2idx;

		QVector <msg_can_t> tx_msgs;
		QMap <uint32_t, uint32_t> tx_id2idx;
		uint32_t tx_idx = 0;

		bool flag_started;
		QTimer timer;
		uint32_t bttest_min, bttest_max, bttest_avr, bttest_cnt;
		QElapsedTimer etimer;

		ListModel list;

		void rx_task();
		void tx_task();

	private slots:
		void slt_log(uint8_t level, const QString & log);
		void slt_opened();
		void slt_closed();
		void slt_msg(const QByteArray &);
		void slt_btn_start();
		void slt_btn_open();
		void slt_btn_save();
		void slt_btn_filter();
		void slt_btn_bttest();
		void slt_timer();
		void slt_cb_tx(int idx);
		void slt_cb_select_tx_id(int idx);
		void slt_btn_bt();
		void slt_btn_com();

	signals:
		void sig_msg(const QByteArray &);

	public:
		main_t(QMainWindow *parent = 0);
		~main_t();
};

#endif

