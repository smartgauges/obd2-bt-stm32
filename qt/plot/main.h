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

struct qmsg_can_t : public msg_can_t
{
	QString dev;
	qmsg_can_t();
};		   

class main_t : public QMainWindow
{
	Q_OBJECT

	private:
		Ui::plot * ui;
		void set_status(const QString &);
		channel_t * channels[NUM_CHANNELS];
		QVector < QVector < qmsg_can_t > > msgs;
		void open_file_csv(const QString & fileName);
		void open_file_log(const QString & fileName);
		qid_t id2name(uint32_t id);

	private slots:
		void slt_btn_open();
		void slt_channel_enabled(int idx, uint32_t id, int type, int off, uint16_t mask, double mul, double add);
		void slt_channel_disabled(int idx);

	public:
		main_t(QMainWindow *parent = 0);
		~main_t();
		void open_file(const QString &);
};

#endif

