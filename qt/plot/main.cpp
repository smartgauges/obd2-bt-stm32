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
	t = 0;
	memset(data, 0, sizeof(data));
}

main_t::main_t(QMainWindow * parent) : QMainWindow(parent), ui(new Ui::plot)
{
	ui->setupUi(this);

	ui->cb_step->addItem("1000");
	ui->cb_step->addItem("500");
	ui->cb_step->addItem("250");
	ui->cb_step->addItem("100");
	connect(ui->cb_step, SIGNAL(activated(int)), this, SLOT(slt_step_activated()));
	connect(ui->rb_opengl, &QRadioButton::toggled, this, &main_t::slt_btn_opengl);
	connect(ui->rb_group, &QRadioButton::toggled, this, &main_t::slt_btn_group);

	for (int i = 0; i < NUM_CHANNELS; i++) {

		channels[i] = new channel_t(i, this);
		ui->panel->addWidget(channels[i]);

		connect(channels[i], &channel_t::sig_enabled, this, &main_t::slt_channel_enabled);
		connect(channels[i], &channel_t::sig_disabled, this, &main_t::slt_channel_disabled);
		if (!i)
			connect(channels[i], &channel_t::sig_change_id, this, &main_t::slt_change_id);
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

	setWindowTitle(fileName);

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

			msgs.resize(msgs.size() + 1);
			msg_idx = msgs.size() - 1;
			msgs[msg_idx].resize(time_idx + 1);
			msgs[msg_idx][0].id = msg.id;

			//qDebug() << "msg_idx:" << msg_idx << "time_idx:" << time_idx << "id:" << msg.id;
			//qDebug() << list;
		}

		QVector < qmsg_can_t > & vec = msgs[msg_idx];
		msg.t = time_idx;
		vec.push_back(msg);
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

void main_t::slt_step_activated()
{
	QString f = windowTitle();
	if (!f.isEmpty()) {

		QFileInfo fi(f);
		if (fi.suffix() == "log")
			open_file(f);
	}
}

void main_t::slt_btn_opengl(bool en)
{
	ui->graph->set_opengl(en);
}

void main_t::slt_btn_group(bool en)
{
	for (int i = 0; i < NUM_CHANNELS; i++) {

		channels[i]->slt_btn_en(en);
		//channels[i]->setEnabled(!en);
		channels[i]->set_group(en);
	}

	channels[0]->setEnabled(true);
}

void main_t::slt_change_id(uint32_t idx)
{
	if (ui->rb_group->isChecked()) {

		for (int i = 1; i < NUM_CHANNELS; i++)
			channels[i]->slt_set_id(idx);
	}
}

void main_t::open_file_log(const QString & fileName)
{
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	bool ok;
	int step = ui->cb_step->currentText().toInt(&ok, 10);
	if (step < 100 || step > 1000)
		step = 1000;

	ui->graph->clear();

	msgs.clear();

	int start_sec = -1;
	int max_sec = 0;
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
		uint32_t msec = tlist[1].toInt(&ok, 10)/1000;

		if (-1 == start_sec)
			start_sec = sec;
		sec = sec - start_sec;

		QStringList flist = list[2].split('#');
		if (flist.size() != 2)
			continue;

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

		//qDebug() << "msg_idx:" << msg_idx;

		//new id
		if (msg_idx == -1) {

			msgs.resize(msgs.size() + 1);
			msg_idx = msgs.size() - 1;
		}
		QVector < qmsg_can_t > & vec = msgs[msg_idx];
		time_t prev_sec = sec;
		uint32_t prev_msec = msec;
		uint32_t t_idx = vec.size();
		//qDebug() << "t_idx:" << t_idx;

		if (t_idx) {

			double prev_t = vec[t_idx - 1].t;
			prev_sec = prev_t;
			prev_msec = (prev_t - prev_sec) * 1000;
		}

		int32_t delta = (sec - prev_sec) * 1000;
		delta += msec - prev_msec;

		if (delta >= step || !t_idx) {

			prev_sec = sec;
			prev_msec = msec;

			msg.t = sec + ((double)msec)/1000.0;

			//qDebug() << t << msg.t;

			vec.push_back(msg);
		}

		max_sec = sec;
	}

	set_status(QString("file %1 open, %2 ids msgs loaded, step %3ms").arg(fileName).arg(msgs.size()).arg(step));

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

	ui->graph->prepare(max_sec);
}

uint8_t reverse(uint8_t b)
{
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;

	return b;
}

void main_t::slt_channel_enabled(int idx, uint32_t id, int type, int off, uint16_t mask, double mul, double add, bool swap)
{ 
	for (int i = 0; i < msgs.size(); i++) {

		if (msgs[i][0].id == id) {

			QVector < qmsg_can_t > & vec = msgs[i];

			QVector <data_t> data(vec.size());

			for (int j = 0; j < vec.size(); j++) {

				data[j].t = vec[j].t;

				if (type == e_type_byte) {

					uint8_t b = vec[j].data[off];
					if (swap)
						b = reverse(b);
					data[j].value = (b & mask) * mul + add;
				}
				else if (type == e_type_word_le) {
					data[j].value = ((vec[j].data[off] + ((off < 7) ? 256 * vec[j].data[off + 1] : 0)) & mask) * mul + add;
				}
				else if (type == e_type_word_be) {
					data[j].value = ((vec[j].data[off] * 256 + ((off < 7) ? vec[j].data[off + 1] : 0)) & mask) * mul + add;
				}
				else
					data[0].value = 0;
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

