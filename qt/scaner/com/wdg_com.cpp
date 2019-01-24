#include <QDebug>
#include <QFile>
#include <QSerialPortInfo>

#include "wdg_com.h"

enum rx_state
{
	RX_ST_WAIT_START,
	RX_ST_DATA,
	RX_ST_ESCAPE,
};

wdg_com_t::wdg_com_t(QWidget * parent) : QWidget(parent), ui(new Ui::com)
{
	ui->setupUi(this);

	rx_state = RX_ST_WAIT_START;

	QList<QSerialPortInfo> list = QSerialPortInfo::availablePorts();
	for (int i = 0; i < list.size(); i++) {

		const QSerialPortInfo & spi = list[i];
		ui->cb_port->addItem(spi.portName());
	}

	connect(ui->btn_open, &QPushButton::clicked, this, &wdg_com_t::slt_open);
	connect(&sp, &QSerialPort::readyRead, this, &wdg_com_t::serial_read_cb);
	connect(&sp, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(slt_error(QSerialPort::SerialPortError)));
}

wdg_com_t::~wdg_com_t()
{
	//stop();
}

void wdg_com_t::slt_error(QSerialPort::SerialPortError error)
{
	if (error == QSerialPort::ResourceError) {

		emit sig_log(e_log_warn, "serial error: " + sp.errorString());
		stop();
	}
}

void wdg_com_t::slt_open()
{
	QString com = ui->cb_port->currentText();

	sp.setPortName(com);

	sp.setBaudRate(QSerialPort::Baud38400);
	sp.setDataBits(QSerialPort::Data8);
	sp.setParity(QSerialPort::NoParity);
	sp.setStopBits(QSerialPort::OneStop);
	sp.setFlowControl(QSerialPort::NoFlowControl);

	bool r = sp.open(QIODevice::ReadWrite);
	if (!r) {

		emit sig_log(e_log_warn, "Cannot open serial device: " + com + " " + sp.errorString());
		return;
	}

	emit sig_log(e_log_info, "open serial device: " + com);
	emit sig_opened();

}

void wdg_com_t::slt_msg(const QByteArray & msg)
{
	if (!sp.isOpen())
		return;

	QByteArray ba;
	ba.append(HDLC_FD);
	for (uint16_t i = 0; i < msg.size(); i++) {

		if (msg[i] == HDLC_FD || msg[i] == HDLC_ESCAPE) {

			ba.append(HDLC_ESCAPE);
			ba.append(msg[i] & ~(1 << 5));
		}
		else
			ba.append(msg[i]);
	}
	ba.append(HDLC_FD);

	//qDebug() << "com slt_msg:" << ba;
	sp.write(ba);
}

void wdg_com_t::serial_read_cb()
{
	qint64 nbytes = sp.bytesAvailable();

	//log.sprintf("in available %lld bytes", nbytes);
	//emit sig_log(e_log_debug, log);

	if (nbytes <= 0)
		return;

	for (int i = 0; i < nbytes; ++i) {

		char b;
		sp.read(&b, 1);

		QByteArray msg = get_msg(b);
		if (!msg.isEmpty()) {

			//qDebug() << "msg:" << msg;
			emit sig_msg(msg);
		}
	}

	nbytes = sp.bytesAvailable();
}

void wdg_com_t::stop()
{
	emit sig_log(e_log_info, "close comport");

	sp.close();

	emit sig_closed();
}

QByteArray wdg_com_t::get_msg(char ch)
{
	//qDebug() << QString("%1").arg((int)ch , 0, 16);
	switch (rx_state) {

		case RX_ST_WAIT_START:
	
			if (ch != HDLC_FD)
				break;

			ba.clear();
			rx_state = RX_ST_DATA;

			break;

		case RX_ST_DATA:

			if (ch == HDLC_ESCAPE) {

				/* drop the escape octet, but change state */
				rx_state = RX_ST_ESCAPE;
				break;
			}
			else if (ch == HDLC_FD) {

				/* message is finished */
				/* start all over again */
				rx_state = RX_ST_WAIT_START;
				struct msg_t * msg = (struct msg_t *)ba.data();
				//qDebug() << ba << ba.size() << msg->len;
				if (msg->len == ba.size() && ba.size() >= 2)
					return ba;
				else {

					ba.clear();
					rx_state = RX_ST_DATA;
					return 0;
				}
				break;
			}

			/* default case: store the octet */
			ba.append(ch);

			break;

		case RX_ST_ESCAPE:

			/* transition back to normal DATA state */
			rx_state = RX_ST_DATA;

			/* store bif-5-inverted octet in buffer */
			ch |= (1 << 5);
			ba.append(ch);

			break;
	}

	return QByteArray();
}

