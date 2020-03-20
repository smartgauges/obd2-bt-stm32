#include <QDebug>
#include <QtWidgets>
#include <QDateTime>
#include <QFileDialog>

#include "main.h"
#include "ui_main.h"

enum e_page
{
	e_bt = 0,
	e_com,
	e_info,
};

main_t::main_t(QMainWindow * parent) : QMainWindow(parent), ui(new Ui::can)
{
	flag_started = false;

	log_level = e_log_all;
	rx_cnt = 0;
	err_cnt = 0;
	msg_sync = 0;

	tx_cnt = 0;
	tx_idx = 0;

	ui->setupUi(this);

	ui->frame->setVisible(false);

	ui->cb_tx_mode->addItem("Off");
	ui->cb_tx_mode->addItem("File");
	ui->cb_tx_mode->addItem("Msg");
	ui->cb_tx_mode->addItem("Obd2");
	connect(ui->cb_tx_mode, SIGNAL(currentIndexChanged(int)), this, SLOT(slt_cb_tx(int)));

	ui->cb_select_tx_id->addItem("Off");
	ui->cb_select_tx_id->addItem("Custom");
	ui->cb_select_tx_id->addItem("Tacho");
	ui->cb_select_tx_id->addItem("Speed");
	ui->cb_select_tx_id->addItem("Drive");
	ui->cb_select_tx_id->addItem("Temp");
	ui->cb_select_tx_id->addItem("Obd2");
	connect(ui->cb_select_tx_id, SIGNAL(currentIndexChanged(int)), this, SLOT(slt_cb_select_tx_id(int)));

	ui->view->setModel(&list);
	QCoreApplication::processEvents();
	ui->view->header()->resizeSections(QHeaderView::ResizeToContents);
	ui->view->header()->setMinimumSectionSize(10);
	ui->view->header()->setStretchLastSection(true);
	ui->view->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->view->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->view->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->view->setUniformRowHeights(true);
	ui->view->setRootIsDecorated(false);
	//ui->view->setColumnWidth(0, 40);

	ui->view->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->view->setSelectionMode(QAbstractItemView::MultiSelection);

	connect(ui->btn_start, &QToolButton::clicked, this, &main_t::slt_btn_start);
	connect(ui->btn_save, &QToolButton::clicked, this, &main_t::slt_btn_save);
	connect(ui->btn_open, &QToolButton::clicked, this, &main_t::slt_btn_open);
	connect(ui->btn_filter, &QToolButton::clicked, this, &main_t::slt_btn_filter);
	connect(ui->btn_bttest, &QToolButton::clicked, this, &main_t::slt_btn_bttest);

	timer.setSingleShot(false);
	timer.start(1000 / TICK_HZ);
	connect(&timer, &QTimer::timeout, this, &main_t::slt_timer);

	ui->debug->setReadOnly(true);

	slt_closed();

#ifndef Q_OS_WIN32
	//bt
	connect(ui->bt, &wdg_bt_t::sig_log, this, &main_t::slt_log);
	connect(ui->bt, SIGNAL(sig_msg(const QByteArray &)), this, SLOT(slt_msg(const QByteArray &)));
	connect(ui->bt, &wdg_bt_t::sig_opened, this, &main_t::slt_opened);
	connect(ui->bt, &wdg_bt_t::sig_closed, this, &main_t::slt_closed);
	ui->bt->start();
	connect(this, SIGNAL(sig_msg(const QByteArray &)), ui->bt, SLOT(slt_msg(const QByteArray &)));
#endif

	//com
	connect(ui->com, &wdg_com_t::sig_log, this, &main_t::slt_log);
	connect(ui->com, SIGNAL(sig_msg(const QByteArray &)), this, SLOT(slt_msg(const QByteArray &)));
	connect(ui->com, &wdg_com_t::sig_opened, this, &main_t::slt_opened);
	connect(ui->com, &wdg_com_t::sig_closed, this, &main_t::slt_closed);
	connect(this, SIGNAL(sig_msg(const QByteArray &)), ui->com, SLOT(slt_msg(const QByteArray &)));

	//fw
#ifndef Q_OS_WIN32
	connect(ui->tab_fw, SIGNAL(sig_msg(const QByteArray &)), ui->bt, SLOT(slt_msg(const QByteArray &)));
#endif
	connect(ui->tab_fw, SIGNAL(sig_msg(const QByteArray &)), ui->com, SLOT(slt_msg(const QByteArray &)));

#ifdef Q_OS_WIN32
	slt_btn_com();
#else
	slt_btn_bt();
#endif
	//ui->stack->setCurrentIndex(e_info);

	bttest_min = 0;
	bttest_max = 0;
	bttest_avr = 0;
	bttest_cnt = 0;
}

main_t::~main_t()
{
}

void main_t::slt_cb_tx(int idx)
{
	ui->tx_stack->setCurrentIndex(idx);
}

void main_t::slt_cb_select_tx_id(int idx)
{
	bool d0 = true;
	bool d1 = true;
	bool d2 = true;
	bool d3 = true;
	bool d4 = true;
	bool d5 = true;
	bool d6 = true;
	bool d7 = true;

	//off
	if (0 == idx) {

		d0 = false;
		d1 = false;
		d2 = false;
		d3 = false;
		d4 = false;
		d5 = false;
		d6 = false;
		d7 = false;
	}
	//custom
	else if (1 == idx) {

	}
	//tacho
	else if (2 == idx) {

		ui->sb_id->setValue(0x316);
		d0 = false;
		d1 = false;
		ui->sb_d2->setValue(0x0);
		ui->sb_d3->setValue(0x10);
		d4 = false;
		d5 = false;
		d6 = false;
		d7 = false;
	}
	//speed
	else if (3 == idx) {

		ui->sb_id->setValue(0x440);
		d0 = false;
		d1 = false;
		ui->sb_d2->setValue(0x20);
		d3 = false;
		d4 = false;
		d5 = false;
		d6 = false;
		d7 = false;
	}
	//selector
	else if (4 == idx) {

		ui->sb_id->setValue(0x43f);

		ui->sb_d0->setValue(0x00);
		d0 = false;

		ui->sb_d1->setValue(0x50);
		d1 = true;

		ui->sb_d2->setValue(0x00);
		d2 = false;

		ui->sb_d3->setValue(0x00);
		d3 = false;

		ui->sb_d4->setValue(0x00);
		d4 = false;

		ui->sb_d5->setValue(0x00);
		d5 = false;

		ui->sb_d6->setValue(0x00);
		d6 = false;

		ui->sb_d7->setValue(0x00);
		d7 = false;
	}
	//temp
	else if (5 == idx) {

		ui->sb_id->setValue(0x329);

		ui->sb_d0->setValue(0x00);
		d0 = false;

		ui->sb_d1->setValue(0x00);
		d1 = false;

		ui->sb_d2->setValue(0x7a);
		d2 = true;

		ui->sb_d3->setValue(0x10);
		d3 = true;

		ui->sb_d4->setValue(0x11);
		d4 = true;

		ui->sb_d5->setValue(0x00);
		d5 = true;

		ui->sb_d6->setValue(0x00);
		d6 = true;

		ui->sb_d7->setValue(0x00);
		d7 = true;
	}
	//Obd2
	else if (6 == idx) {

		ui->sb_id->setValue(0x7df);

		ui->sb_d0->setValue(0x2);
		d0 = false;

		ui->sb_d1->setValue(0x01);
		d1 = true;

		ui->sb_d2->setValue(0xd);
		d2 = true;

		ui->sb_d3->setValue(0x00);
		d3 = false;

		ui->sb_d4->setValue(0x00);
		d4 = false;

		ui->sb_d5->setValue(0x00);
		d5 = false;

		ui->sb_d6->setValue(0x00);
		d6 = false;

		ui->sb_d7->setValue(0x00);
		d7 = false;
	}

	ui->sb_d0->setEnabled(d0);
	ui->sb_d1->setEnabled(d1);
	ui->sb_d2->setEnabled(d2);
	ui->sb_d3->setEnabled(d3);
	ui->sb_d4->setEnabled(d4);
	ui->sb_d5->setEnabled(d5);
	ui->sb_d6->setEnabled(d6);
	ui->sb_d7->setEnabled(d7);
}

void main_t::slt_log(uint8_t level, const QString & log)
{
	if (level > log_level)
		return;

	QDateTime dt = QDateTime::currentDateTime();

	ui->debug->textCursor().insertText(dt.toString("hh:mm:ss.z ") + log + "\n");
	ui->status->showMessage(log);
}

void main_t::slt_btn_start()
{ 
}

void main_t::slt_btn_save()
{ 
	if (!rx_msgs.size())
		return;

	QString fileName;
#ifdef Q_OS_ANDROID
	fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "/sdcard/Download/can.csv", tr("csv (*.csv)"));
#else
	fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "can.csv", tr("csv (*.csv)"));
#endif
	if (fileName.isEmpty())
		return;

	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return;

	QTextStream out(&file);
	for (int i = 0; i < rx_msgs.size(); i++) {

		msg_can_t & msg = rx_msgs[i];

		//QString("%0%1").arg(prefix).arg(data, 2, 16, QChar('0'));

		out << msg.num << ",";
		out << QString("%1").arg(msg.id, 8, 16, QChar('0')) << ",";
		out << msg.len << ",";
		for (int j = 0; j < msg.len; j++)
			out << QString("%1").arg(msg.data[j], 2, 16, QChar('0')) << ",";
		out << msg.type << ",";
		out << endl;
	}

	slt_log(0, QString("file %1 saved").arg(fileName));

	file.close();
}

void main_t::slt_btn_bttest()
{
	if (ui->btn_bttest->isChecked()) {

		bttest_min = 0;
		bttest_max = 0;
		bttest_avr = 0;
		bttest_cnt = 0;

		QByteArray ba(sizeof(msg_t), 0);
		struct msg_t * m = (struct msg_t *)ba.data();
		m->len = ba.size();
		m->type = e_type_ping;

		etimer.start();
		emit sig_msg(ba);
	}
}

void main_t::slt_btn_filter()
{
	if (ui->btn_filter->isChecked()) {

		QModelIndexList l = ui->view->selectionModel()->selectedRows(ListModel::e_col_id);

		QByteArray ba(sizeof(msg_t) + 5, 0);
		struct msg_t * m = (struct msg_t *)ba.data();
		m->len = ba.size();
		m->type = e_type_cmd;

		for (int i = 0; i < l.size(); i++) {

			uint8_t * cmd = m->data;
			cmd[0] = e_cmd_set_filter;

			//qDebug() << list.get_id(l[i].row());

			uint32_t * id = (uint32_t *)(m->data + 1);
			*id = list.get_id(l[i].row());

			emit sig_msg(ba);
		}
	}
	else {
		ui->view->selectionModel()->clearSelection();

		QByteArray ba(sizeof(msg_t) + 1, 0);
		struct msg_t * m = (struct msg_t *)ba.data();
		m->len = ba.size();
		m->type = e_type_cmd;
		uint8_t * cmd = m->data;
		cmd[0] = e_cmd_unset_filter;

		emit sig_msg(ba);
	}
}

void main_t::slt_btn_open()
{ 
	QString fileName;
#ifdef Q_OS_ANDROID
	fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "/sdcard/Download/can.csv", tr("csv (*.csv)"));
#else
	fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "can.csv", tr("csv (*.csv)"));
#endif
	if (fileName.isEmpty())
		return;

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	tx_cnt = 0;
	tx_idx = 0;
	tx_msgs.clear();
	ui->cb_tx_id->clear();
	tx_id2idx.clear();
	ui->cb_tx_id->addItem("All");

	while (!file.atEnd()) {

		QString line = file.readLine();
		if (line.isEmpty())
			break;

		QStringList list;
		list << line.split(',');
		//qDebug() << list;

		if (list.size() > 3) {

			msg_can_t msg;
			msg.num = 0;
			msg.type = 0;
			bool ok;
			msg.id = list[1].toInt(&ok, 16);
			msg.len = list[2].toInt();

			for (size_t i = 0; i < msg.len; i++) {

				msg.data[i] = list[3 + i].toInt(&ok, 16);
			}

			tx_msgs.push_back(msg);

			if (tx_id2idx.find(msg.id) == tx_id2idx.end()) {

				tx_id2idx[msg.id] = tx_id2idx.size();
				ui->cb_tx_id->addItem(QString("%1").arg(msg.id, 0, 16));
			}

			//qDebug() << line << QString("%1").arg(msg.id, 0, 16) << msg.len << QString("%1").arg(msg.data[0], 0, 16) << QString("%1").arg(msg.data[1], 0, 16) << QString("%1").arg(msg.data[2], 0, 16);
		}
	}

	//qDebug() << "end";

	slt_log(0, QString("file %1 open, %2 msgs loaded").arg(fileName).arg(tx_msgs.size()));

	file.close();
}

void main_t::slt_opened()
{
	ui->frame->setVisible(true);
	ui->stack->setCurrentIndex(e_info);
	
	rx_cnt = 0;
	err_cnt = 0;
	rx_msgs.clear();
	rx_id2idx.clear();
	ui->lbl_state->setText("Opened");

	flag_started = true;
	QPixmap pixmap(":/images/pause.png");
	QIcon icon(pixmap);
	ui->btn_start->setIcon(icon);
}

void main_t::slt_closed()
{
	ui->lbl_state->setText("Closed");

	flag_started = false;
	QPixmap pixmap(":/images/start.png");
	QIcon icon(pixmap);
	ui->btn_start->setIcon(icon);
}

void main_t::rx_task()
{
	QVector <info_t> ids;
	ids.resize(rx_id2idx.size());
	QMap<uint32_t, info_t>::iterator i = rx_id2idx.begin();
	while (i != rx_id2idx.end()) {

		info_t & info = i.value();
		info.inc(TICK_MAX_COLOR);

		ids[info.idx] = info;
		memcpy(ids[info.idx].data, info.data, sizeof(info.data));
		if (++info.ticks >= TICK_HZ) {

			info.period = info.num - info.prev_num;
			ids[info.idx].period = info.period;
			info.prev_num = info.num;
			info.ticks = 0;
		}

		i++;
	}
	if (list.set_state(ids)) {

		QCoreApplication::processEvents();
		ui->view->header()->resizeSections(QHeaderView::ResizeToContents);
	}
}

static uint8_t obd2_cnt = 0;
void main_t::tx_task()
{
	int tx_state = ui->cb_tx_mode->currentIndex();

	if (0 == tx_state)
		return;

	QByteArray ba(sizeof(msg_t) + sizeof(msg_can_t), 0);
	struct msg_t * m = (struct msg_t *)ba.data();
	m->len = sizeof(msg_t) + sizeof(msg_can_t);
	m->type = e_type_can;

	struct msg_can_t * msg = (struct msg_can_t *)m->data;

	//file
	if (1 == tx_state) {

		if (!tx_msgs.size())
			return;

		uint32_t id = 0;
		uint32_t idx = ui->cb_tx_id->currentIndex();
		if (idx > 0) {

			QMap<uint32_t, uint32_t>::const_iterator i = tx_id2idx.begin();
			while (i != tx_id2idx.end()) {

				if (idx == i.value()) {

					id = i.key();
					break;
				}

				i++;
			}
		}

		for (size_t i = 0; i < 50; i++) {

			if (++tx_idx >= (uint32_t)tx_msgs.size())
				tx_idx = 0;

			*msg = tx_msgs[tx_idx];
			if (msg->id != id && id)
				continue;

			tx_cnt++;
			emit sig_msg(ba);
		}
	}
	//msg
	if (2 == tx_state) {

		if (0 == ui->cb_select_tx_id->currentIndex())
			return;

		memset(msg, 0x0, sizeof(msg_can_t));
		msg->type = 0;
		msg->id = ui->sb_id->value();
		msg->len = ui->sb_len->value();
		msg->data[0] = ui->sb_d0->value();
		msg->data[1] = ui->sb_d1->value();
		msg->data[2] = ui->sb_d2->value();
		msg->data[3] = ui->sb_d3->value();
		msg->data[4] = ui->sb_d4->value();
		msg->data[5] = ui->sb_d5->value();
		msg->data[6] = ui->sb_d6->value();
		msg->data[7] = ui->sb_d7->value();

		//qDebug() << QString("%1").arg(msg.id, 0, 16) << msg.length << QString("%1").arg(msg.data[0], 0, 16) << QString("%1").arg(msg.data[1], 0, 16) << QString("%1").arg(msg.data[2], 0, 16);

		emit sig_msg(ba);
	}
	//obd2
	if (3 == tx_state) {

		memset(msg, 0x0, sizeof(msg_can_t));
		msg->id = 0x7df;//broadcast
		msg->type = 0;
		msg->len = 8;//len
		msg->data[0] = 0x2;//number of additional data

		uint8_t ids[] = {
			0x4/*calc engine load*/,
			0x5/*coolant temp*/,
			0x6/*short term fuel trim*/,
			0x7/*long term fuel trim*/,
			0xb/*map*/,
			0xc/*rpm*/,
			0xd/*speed */,
			0xe/*timing advance*/,
			0xf/*intake air temp*/,
			0x10/*maf*/,
			0x11/* throttle*/,
			0x14/*oxygen1*/,
			0x15/*oxygen2*/,
			0x33/*air pressure*/,
			0x46/*air temp*/
		};

		msg->data[1] = 0x1;//show current data
		msg->data[2] = ids[obd2_cnt];//PID
		msg->data[3] = 0x55;
		msg->data[4] = 0x55;
		msg->data[5] = 0x55;
		msg->data[6] = 0x55;
		msg->data[7] = 0x55;

		emit sig_msg(ba);

		if (++obd2_cnt >= sizeof(ids))
			obd2_cnt = 0;
	}
}

void main_t::slt_timer()
{
	ui->lbl_err->setText(QString("%1").arg(err_cnt));
	ui->lbl_rx->setText(QString("%1").arg(rx_cnt));
	ui->lbl_tx->setText(QString("%1").arg(tx_cnt));

	if (!flag_started)
		return;

	rx_task();
	tx_task();
}

typedef enum e_speed_t
{
	e_speed_125 = 0,
	e_speed_250,
	e_speed_500,
	e_speed_1000,
	e_speed_nums
} e_speed_t;

void main_t::slt_msg(const QByteArray & ba)
{
	msg_t * m = (msg_t *)ba.data();

	//qDebug() << "main slt_msg" << m->type;

	if (m->type == e_type_debug) {

		struct msg_debug_t * msg = (msg_debug_t *)m->data;

		QByteArray str((const char *)msg->data, msg->len);
		slt_log(0, str);

		return;
	}
	else if (m->type == e_type_erase) {

		slt_log(0, "flash erased");
		return;
	}
	else if (m->type == e_type_pong) {

		uint32_t ms = etimer.elapsed();
		if (!bttest_min || bttest_min > ms)
			bttest_min = ms;
		if (bttest_max < ms)
			bttest_max = ms;
		bttest_avr += ms;
		bttest_avr /= 2;
		bttest_cnt++;

		ui->label_bttest->setText(QString("%1 %2 %3 %4").arg(bttest_cnt).arg(bttest_min).arg(bttest_avr).arg(bttest_max));

		if (ui->btn_bttest->isChecked()) {

			QByteArray ba(sizeof(msg_t), 0);
			struct msg_t * m = (struct msg_t *)ba.data();
			m->len = ba.size();
			m->type = e_type_ping;

			etimer.start();
			emit sig_msg(ba);
		}

		return;
	}

	if ((m->type == e_type_status) && (m->len == (sizeof(struct msg_t) + sizeof(struct msg_status_t)))) {

		struct msg_status_t * msg = (msg_status_t *)m->data;
		QString mode = "unknown";
		if (msg->mode == e_mode_bl)
			mode = "bl";
		else if (msg->mode == e_mode_sniffer)
			mode = "sniffer";

		QString version = QString("v%1").arg(msg->version & ~0x80);

		QString speed = "unknown";
		if (msg->speed == e_speed_125)
			speed = "125";
		else if (msg->speed == e_speed_250)
			speed = "250";
		else if (msg->speed == e_speed_500)
			speed = "500";
		else if (msg->speed == e_speed_1000)
			speed = "1000";

		QString pkts = QString("pkts:%1").arg(msg->num_bytes);

		QString str = "mode:" + mode + " " + version + " " + "speed:" + speed + " " + pkts;
		ui->status->showMessage(str);

		return;
	}

	if ((m->type != e_type_can) || (m->len != (sizeof(struct msg_t) + sizeof(struct msg_can_t))))
		return;

	struct msg_can_t * msg = (msg_can_t *)m->data;

	rx_cnt++;
#if 0
	if (msg_sync != msg.num) {

		err_cnt++;
		msg_sync = msg.num;

		slt_log(0, QString("msg.num = %1").arg(msg.num));
	}
#endif

	QMap <uint32_t, info_t>::iterator iter = rx_id2idx.find(msg->id);
	if (iter == rx_id2idx.end()) {

		info_t info;
		info.idx = rx_id2idx.size();
		info.id = msg->id;
		info.set(msg->data);
		info.num = msg->num;
		info.period = 0;
		info.prev_num = 0;
		info.ticks = 0;

		rx_id2idx[msg->id] = info;
	}
	else {
		info_t & info = iter.value();
		info.set(msg->data);
		info.num = msg->num;
	}

	//solaris
	/* 0x440 - x   x    speed   x    x x x x - speed */
	if (msg->id == 0x440) {

		ui->tab_rpm->setSpeed(msg->data[2]);
	}
	else if (msg->id == 0x316) {

		uint16_t t = msg->data[2] | msg->data[3] << 8;
		ui->tab_rpm->setTaho(t/4);
	}

	//lr2
	//0x150 - x    x     x      x    x    x  speed   x - (speed * 2.5)
	if (msg->id == 0x150) {

		int v = (((uint16_t)msg->data[6]) << 8 | msg->data[7]) & 0xffff;
		v /= 100;
		ui->tab_rpm->setSpeed(v);
		//qDebug() << "speed:" << v;
	}
	else if (msg->id == 0x12a) {

		int v = (((uint16_t)msg->data[6]) << 8 | msg->data[7]) & 0xfff;
		ui->tab_rpm->setTaho(v);
	}

	msg_sync++;

	rx_msgs.push_back(*msg);
}

void main_t::slt_btn_bt()
{
	ui->stack->setCurrentIndex(e_bt);
}

void main_t::slt_btn_com()
{
	ui->stack->setCurrentIndex(e_com);
}

int main(int argc, char ** argv)
{
	QApplication app(argc, argv);

	main_t m;

	m.show();

	return app.exec();
}

