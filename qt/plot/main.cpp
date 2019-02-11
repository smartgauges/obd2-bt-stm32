#include <QDebug>
#include <QtWidgets>
#include <QDateTime>
#include <QFileDialog>

#include "main.h"
#include "ui_main.h"

qmsg_can_t::qmsg_can_t()
{
	dev = "";
	id = 0;
	num = 0;
	type = 0;
	len = 0;
	memset(data, 0, sizeof(data));
}

main_t::main_t(QMainWindow * parent) : QMainWindow(parent), ui(new Ui::plot)
{
	ui->setupUi(this);

	for (int i = 0; i < NUM_CHANNELS; i++) {

		channels[i] = new channel_t(i, this);
		ui->panel->addWidget(channels[i]);

		connect(channels[i], &channel_t::sig_enabled, this, &main_t::slt_channel_enabled);
		connect(channels[i], &channel_t::sig_disabled, this, &main_t::slt_channel_disabled);
	}

	connect(ui->btn_open, &QToolButton::clicked, this, &main_t::slt_btn_open);
}

main_t::~main_t()
{
}

void main_t::open_file(const QString & fileName)
{
	if (fileName.isEmpty())
		return;

	QFileInfo fi(fileName);
	if (fi.suffix() == "csv")
		open_file_csv(fileName);
	else if (fi.suffix() == "log")
		open_file_log(fileName);
}

qid_t main_t::id2name(uint32_t id)
{
	uint32_t _id = id & 0xfff;

	qid_t qid;
	qid.id = id;
	qid.name = QString("%1").arg(qid.id, 2, 16, QChar('0'));

	if (_id >= 0x7e8 && _id < 0x7ef) {

		uint16_t pid = id >> 16;
		QString spid = "?";
		switch (pid) {

			case 0x4:
				spid = "load";
				break;
			case 0x5:
				spid = "coolant temp";
				break;
			case 0xb:
				spid = "map";
				break;
			case 0xc:
				spid = "rpm";
				break;
			case 0xd:
				spid = "speed";
				break;
			case 0xf:
				spid = "air temp";
				break;
			case 0x10:
				spid = "maf";
				break;
			case 0x11:
				spid = "throttle";
				break;
			case 0x33:
				spid = "abp";
				break;
			case 0x46:
				spid = "air temp";
				break;
			default:
				break;
		}
		qid.name = QString("obd2(%1) %2(%3)").arg(_id - 0x7e8).arg(pid, 2, 16, QChar('0')).arg(spid);
	}

	return qid;
}

void main_t::open_file_csv(const QString & fileName)
{
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	ui->graph->clear();

	int time_idx = 0;

	msgs.clear();

	while (!file.atEnd()) {

		QString line = file.readLine();
		if (line.isEmpty())
			break;

		QStringList list;
		list << line.split(',');
		//qDebug() << n++ << list;

		if (list.size() < 4 || list.size() > 13)
			continue;

		qmsg_can_t msg;
		bool ok;
		msg.id = list[1].toInt(&ok, 16);
		msg.len = list[2].toInt();

		if (msg.len <= 0 || msg.len > 8)
			continue;

		if (list.size() < (3 + msg.len))
			continue;

		for (size_t i = 0; i < msg.len; i++) {

			msg.data[i] = list[3 + i].toInt(&ok, 16);
		}

		msg.type = list[3 + msg.len].toInt(&ok, 16);

		//obd2 hack
		if (msg.id >= 0x7e8 && msg.id <= 0x7ef) {

			//PID
			msg.id |= msg.data[2] << 16;
		}
		if (((time_idx & 1) == 1) != ((msg.type & e_can_odd) == e_can_odd))
			time_idx++;

		int msg_idx = -1;
		for (int i = 0; i < msgs.size(); i++) {

			if (msgs[i][0].id == msg.id)
				msg_idx = i;
		}

		//new id
		if (msg_idx == -1) {

			//qDebug() << "msg_idx:" << msg_idx << "time_idx:" << time_idx << "id:" << msg.id;
			//qDebug() << list;

			msgs.resize(msgs.size() + 1);
			msg_idx = msgs.size() - 1;
			msgs[msg_idx].resize(time_idx + 1);
			msgs[msg_idx][0].id = msg.id;
		}

		QVector < qmsg_can_t > & vec = msgs[msg_idx];
		msgs[msg_idx].resize(time_idx + 1);
		vec[time_idx] = msg;
	}

	set_status(QString("file %1 open, %2 ids msgs loaded, %3 times").arg(fileName).arg(msgs.size()).arg(time_idx));

	file.close();

	QVector < qid_t > ids;
	for (int i = 0; i < msgs.size(); i++) {
		qid_t qid = id2name(msgs[i][0].id);
		ids.push_back(qid);
	}
	for (int i = 0; i < NUM_CHANNELS; i++) {
		channels[i]->set_ids(ids);
	}

	ui->graph->prepare(time_idx);
}

void main_t::open_file_log(const QString & fileName)
{
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	ui->graph->clear();

	int time_idx = 0;

	msgs.clear();

	time_t cur_sec = 0;
	while (!file.atEnd()) {

		QString line = file.readLine();
		if (line.isEmpty())
			break;

		//(0000000138.725841) can1 501#0101000001000000
		QStringList list;
		list << line.split(' ');

		if (list.size() != 3)
			continue;

		QString t = list[0];
		if (t[0] != '(' && t[t.size() - 1] != ')')
			continue;

		t.remove('(');
		t.remove(')');

		QStringList tlist = t.split('.');

		if (tlist.size() != 2)
			continue;

		bool ok;
		time_t sec = tlist[0].toInt(&ok, 10);
		//uint32_t usec = tlist[1].toInt(&ok, 10);

		if (cur_sec != sec) {

			time_idx++;
			cur_sec = sec;
		}

		QStringList flist = list[2].split('#');
		if (flist.size() != 2)
			continue;

		//int r = parse_canframe(ascframe, &msg);

		QString data = flist[1];
		data.remove('\n');

		qmsg_can_t msg;
		msg.dev = list[1];
		msg.id = flist[0].toInt(&ok, 16);

		msg.len = data.size() / 2;

		if (msg.len <= 0 || msg.len > 8)
			continue;

		for (int i = 0; i < msg.len; i++) {

			QString byte(data.data() + i * 2, 2);
			msg.data[i] = byte.toInt(&ok, 16);
		}

		//obd2 hack
		if (msg.id >= 0x7e8 && msg.id <= 0x7ef) {

			msg.id |= msg.data[2] << 16;
		}

		//qDebug() << sec << msg.dev << msg.len << hex << msg.id << msg.data[0] << msg.data[1];

		int msg_idx = -1;
		for (int i = 0; i < msgs.size(); i++) {

			if (msgs[i][0].id == msg.id && msgs[i][0].dev == msg.dev)
				msg_idx = i;
		}

		//new id
		if (msg_idx == -1) {

			//qDebug() << "msg_idx:" << msg_idx << "time_idx:" << time_idx << "id:" << msg.id;
			//qDebug() << list;

			msgs.resize(msgs.size() + 1);
			msg_idx = msgs.size() - 1;
			msgs[msg_idx].resize(time_idx + 1);
			msgs[msg_idx][0].id = msg.id;
			msgs[msg_idx][0].dev = msg.dev;
		}

		QVector < qmsg_can_t > & vec = msgs[msg_idx];
		msgs[msg_idx].resize(time_idx + 1);
		vec[time_idx] = msg;
	}

	set_status(QString("file %1 open, %2 ids msgs loaded, %3 times").arg(fileName).arg(msgs.size()).arg(time_idx));

	file.close();

	QVector < qid_t > ids;
	for (int i = 0; i < msgs.size(); i++) {
		qid_t qid = id2name(msgs[i][0].id);
		qid.name = msgs[i][0].dev + " " + qid.name;
		ids.push_back(qid);
	}

	for (int i = 0; i < NUM_CHANNELS; i++) {
		channels[i]->set_ids(ids);
	}

	ui->graph->prepare(time_idx);
}

void main_t::slt_channel_enabled(int idx, uint32_t id, int type, int off, uint16_t mask, double mul, double add)
{ 
	for (int i = 0; i < msgs.size(); i++) {

		if (msgs[i][0].id == id) {

			QVector < qmsg_can_t > & vec = msgs[i];

			QVector <double> data(vec.size());

			for (int j = 0; j < vec.size(); j++) {

				if (type == e_type_byte) {
					data[j] = (vec[j].data[off] & mask) * mul + add;
				}
				else if (type == e_type_word_le) {
					data[j] = ((vec[j].data[off] + ((off < 7) ? 256 * vec[j].data[off + 1] : 0)) & mask) * mul + add;
				}
				else if (type == e_type_word_be) {
					data[j] = ((vec[j].data[off] * 256 + ((off < 7) ? vec[j].data[off + 1] : 0)) & mask) * mul + add;
				}
				else
					data[0] = 0;
			}

			ui->graph->set(idx, data);
		}
	}
}

void main_t::slt_channel_disabled(int idx)
{
	ui->graph->clear(idx);
}

void main_t::set_status(const QString & msg)
{
	ui->statusbar->showMessage(msg);
}

void main_t::slt_btn_open()
{ 
	QString fileName;
#ifdef Q_OS_ANDROID
	fileName = QFileDialog::getOpenFileName(this, tr("Open dump file"), "/sdcard/Downloads", tr("csv (*.csv);;log (*.log)"));
#else
	fileName = QFileDialog::getOpenFileName(this, tr("Open dump file"), "./", tr("csv (*.csv);;log (*.log)"));
#endif

	open_file(fileName);
}

int main(int argc, char ** argv)
{
	QApplication app(argc, argv);

	main_t m;

	if (argc == 2)
		m.open_file(argv[1]);

	m.show();

	return app.exec();
}

