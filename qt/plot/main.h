#ifndef MAIN_H
#define MAIN_H

#include <QMainWindow>
#include <QMap>

#include "graph.h"
#include "channel.h"
#include "msg.h"

namespace Ui
{
	class plot;
}

class main_t : public QMainWindow
{
	Q_OBJECT

	private:
		Ui::plot * ui;
		void set_status(const QString &);
		channel_t * channels[NUM_CHANNELS];
		QVector < QVector < msg_can_t > > msgs;

	private slots:
		void slt_btn_open();
		void slt_channel_enabled(int idx, uint32_t id, int type, int off, double mul, int add);
		void slt_channel_disabled(int idx);

	public:
		main_t(QMainWindow *parent = 0);
		~main_t();
		void open(const QString &);
};

#endif

