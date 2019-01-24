#include <QDebug>
#include <QtWidgets>
#include <QDateTime>
#include <QFileDialog>

#include "main.h"
#include "ui_main.h"

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

void main_t::open(const QString & fileName)
{
	if (fileName.isEmpty())
		return;

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	ui->graph->clear();

	QVector <msg_can_t> tx_msgs;
	int msg_idx = 0;

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

		msg_can_t msg;
		memset(&msg, 0, sizeof(msg));
		msg.num = 0;
		msg.type = 0;
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

			msg.id |= msg.data[2] << 16;
		}
		if (((msg_idx & 1) == 1) != ((msg.type & e_can_odd) == e_can_odd))
			msg_idx++;

		int vec_idx = -1;
		for (int i = 0; i < msgs.size(); i++) {

			if (msgs[i][0].id == msg.id)
				vec_idx = i;
		}

		//new id
		if (vec_idx == -1) {

			//qDebug() << "vec_idx:" << vec_idx << "msg_idx:" << msg_idx << "id:" << msg.id;
			//qDebug() << list;

			msgs.resize(msgs.size() + 1);
			vec_idx = msgs.size() - 1;
			msgs[vec_idx].resize(msg_idx + 1);
			msgs[vec_idx][0].id = msg.id;
		}

		QVector < msg_can_t > & vec = msgs[vec_idx];
		msgs[vec_idx].resize(msg_idx + 1);
		vec[msg_idx] = msg;
	}

	set_status(QString("file %1 open, %2 ids msgs loaded, %3 times").arg(fileName).arg(msgs.size()).arg(msg_idx));

	file.close();

	QVector < uint32_t > ids;
	for (int i = 0; i < msgs.size(); i++) {
		ids.push_back(msgs[i][0].id);
	}
	for (int i = 0; i < NUM_CHANNELS; i++) {
		channels[i]->set_ids(ids);
	}

	ui->graph->prepare(msg_idx);
}

void main_t::slt_channel_enabled(int idx, uint32_t id, int type, int off, double mul, int add)
{ 
	for (int i = 0; i < msgs.size(); i++) {

		if (msgs[i][0].id == id) {

			QVector < msg_can_t > & vec = msgs[i];

			QVector <double> data(vec.size());

			for (int j = 0; j < vec.size(); j++) {

				if (type == e_type_byte)
					data[j] = vec[j].data[off] * mul + add;
				else if (type == e_type_word_le)
					data[j] = (vec[j].data[off] + ((off < 7) ? 256 * vec[j].data[off + 1] : 0)) * mul + add;
				else if (type == e_type_word_be)
					data[j] = (vec[j].data[off] * 256 + ((off < 7) ? vec[j].data[off + 1] : 0)) * mul + add;
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
	fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "/sdcard/can.csv", tr("csv (*.csv)"));
#else
	fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "can.csv", tr("csv (*.csv)"));
#endif

	open(fileName);
}

int main(int argc, char ** argv)
{
	QApplication app(argc, argv);

	main_t m;

	if (argc == 2)
		m.open(argv[1]);

	m.show();

	return app.exec();
}

