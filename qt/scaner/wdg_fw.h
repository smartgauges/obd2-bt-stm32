#ifndef WDG_FW_H
#define WDG_FW_H

#include <QWidget>
#include <QTimer>
#include <QByteArray>
#include "list.h"

namespace Ui
{
	class fw;
}

class wdg_fw_t : public QWidget
{
	Q_OBJECT

	public:
		explicit wdg_fw_t(QWidget * parent = 0);

	private:
		Ui::fw * ui;
		info_t info;
		QTimer timer;
		uint32_t offset;
		uint16_t crc;
		QByteArray fw;
		bool send();

	private slots:
		void slt_flash();
		void slt_open();
		void slt_timer();
		void slt_rst();
		void slt_erase();
		void slt_cb_select_speed(int idx);

	signals:
		void sig_msg(const QByteArray & ba);
};

#endif

