#include <qbluetoothaddress.h>
#include <qbluetoothdevicediscoveryagent.h>
#include <qbluetoothlocaldevice.h>
#include <qbluetoothservicediscoveryagent.h>
#include <qbluetoothserviceinfo.h>
#include <qbluetoothuuid.h>

#include "wdg_bt.h"

enum rx_state
{
	RX_ST_WAIT_START,
	RX_ST_DATA,
	RX_ST_ESCAPE,
};

wdg_bt_t::wdg_bt_t(QWidget *parent) : QWidget(parent), ui(new Ui::bt)
{
	ui->setupUi(this);

	rx_state = RX_ST_WAIT_START;

	ui->view->setModel(&list);

	ui->view->header()->setStretchLastSection(true);
	ui->view->header()->setSectionResizeMode(BtListModel::e_col_name, QHeaderView::ResizeToContents);
	ui->view->header()->setSectionResizeMode(BtListModel::e_col_addr, QHeaderView::ResizeToContents);

	ui->view->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->view->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->view->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->view->setUniformRowHeights(true);
	ui->view->setHeaderHidden(true);

	connect(ui->view->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(slt_selection_changed(const QItemSelection &)));

	connect(ui->btn_connect, SIGNAL(clicked()), this, SLOT(slt_connect()));

#ifndef DISCOVER_OFF
	device_agent = 0;
	service_agent = 0;

	connect(ui->btn_scan, SIGNAL(clicked()), this, SLOT(slt_scan()));
#endif

	ui->btn_connect->setEnabled(false);
	ui->btn_scan->setEnabled(false);
}

bool wdg_bt_t::start()
{
	localDevice = new QBluetoothLocalDevice(this);

	if (!localDevice->isValid()) {

		emit sig_log(e_log_debug, "Bluetooth is not available on this device");
		return false;
	}

	emit sig_log(e_log_debug, "Bluetooth is available on this device");

	connect(localDevice, SIGNAL(hostModeStateChanged(QBluetoothLocalDevice::HostMode)), this, SLOT(hostModeStateChanged(QBluetoothLocalDevice::HostMode)));
	connect(localDevice, &QBluetoothLocalDevice::error, this, &wdg_bt_t::handleLocalDeviceError);

	//localDevice->setHostMode(QBluetoothLocalDevice::HostDiscoverable);
	localDevice->powerOn();

	//hostModeStateChanged(localDevice->hostMode());

	emit sig_log(e_log_debug, QString("Local device: %1(%2)").arg(localDevice->name()).arg(localDevice->address().toString().trimmed()));

	socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);

	connect(socket, SIGNAL(error(QBluetoothSocket::SocketError)), this, SLOT(slt_socket_error(QBluetoothSocket::SocketError)));
	connect(socket, SIGNAL(connected()), this, SLOT(slt_socket_connected()));
	connect(socket, SIGNAL(disconnected()), this, SLOT(slt_socket_disconnected()));
	connect(socket, SIGNAL(readyRead()), this, SLOT(slt_socket_read()));
	connect(socket, SIGNAL(stateChanged(QBluetoothSocket::SocketState)), SLOT(slt_socket_state_changed(QBluetoothSocket::SocketState)));

#ifndef DISCOVER_OFF
	device_agent = new QBluetoothDeviceDiscoveryAgent(this);
	service_agent = new QBluetoothServiceDiscoveryAgent(localDevice->address(), this);

	//device_agent->setInquiryType(QBluetoothDeviceDiscoveryAgent::GeneralUnlimitedInquiry);
	//device_agent->setInquiryType(QBluetoothDeviceDiscoveryAgent::LimitedInquiry);

	connect(device_agent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(slt_add_device(QBluetoothDeviceInfo)));
	connect(device_agent, SIGNAL(finished()), this, SLOT(slt_device_scan_finished()));

	connect(service_agent, SIGNAL(serviceDiscovered(QBluetoothServiceInfo)), this, SLOT(slt_add_service(QBluetoothServiceInfo)));
	connect(service_agent, SIGNAL(finished()), this, SLOT(slt_service_scan_finished()));

	ui->btn_scan->setEnabled(true);
#else
	QBluetoothAddress address("00:1d:a5:68:98:8C");
	QBluetoothDeviceInfo info(address, "OBD2", QBluetoothDeviceInfo::UncategorizedDevice << 8);
	list.add(info, QBluetoothLocalDevice::Paired, true);
#endif

	return true;
}

wdg_bt_t::~wdg_bt_t()
{
}

void wdg_bt_t::handleLocalDeviceError(QBluetoothLocalDevice::Error error)
{
	emit sig_log(e_log_debug, QString("ERROR: %1").arg(error));
}

#ifndef DISCOVER_OFF
void wdg_bt_t::slt_scan()
{
	emit sig_log(e_log_debug, "Device discover started ...");

	if (device_agent) {

		//device_agent->start(QBluetoothDeviceDiscoveryAgent::ClassicMethod);
		device_agent->start();
		ui->btn_scan->setEnabled(false);
	}
}

#include <QDebug>
void wdg_bt_t::slt_add_device(const QBluetoothDeviceInfo &info)
{
	QBluetoothLocalDevice::Pairing status = localDevice->pairingStatus(info.address());

	QString str;
	bool found = false;

	emit sig_log(e_log_debug, QString("Found device:%1 %2").arg(info.address().toString()).arg(str));

	list.add(info, status, found);

	if (!service_agent->isActive())
		start_check_serial(info);
}

void wdg_bt_t::slt_device_scan_finished()
{
	ui->btn_scan->setEnabled(true);

	emit sig_log(e_log_debug, "Device discover finished");

	QList<QBluetoothDeviceInfo> listOfDevices = device_agent->discoveredDevices();
	if (listOfDevices.isEmpty()) {

		emit sig_log(e_log_debug, "No devices found");
		return;
	}

	emit sig_log(e_log_debug, QString("Found %1 devices").arg(listOfDevices.size()));
}

void wdg_bt_t::start_check_serial(const QBluetoothDeviceInfo & info)
{
	emit sig_log(e_log_debug, "Service discover started " + info.address().toString() + " ...");

	service_agent->setRemoteAddress(info.address());
	service_agent->start();
}

void wdg_bt_t::slt_add_service(const QBluetoothServiceInfo &info)
{
	if (info.serviceName().isEmpty())
		return;

	bool found = false;
	foreach (QBluetoothUuid uuid, info.serviceClassUuids()) {

		if (uuid == QBluetoothUuid(QBluetoothUuid::SerialPort))
			found = true;
	}

	QString line = info.serviceName();
	emit sig_log(e_log_debug, QString("Found service:%1").arg(line));

	if (service_agent && found) {

		list.set_serial(service_agent->remoteAddress());
		ui->btn_connect->setEnabled(true);
	}
}

void wdg_bt_t::slt_service_scan_finished()
{
	emit sig_log(e_log_debug, "Service discover finished");

	QBluetoothAddress addr = service_agent->remoteAddress();
	list.set_checked(addr);

	for (int i = 0; i < list.rowCount(); i++) {

		if (!list.is_checked(i)) {

			QBluetoothDeviceInfo info = list.get_dev(i);
			start_check_serial(info);
			break;
		}
	}
}
#endif

void wdg_bt_t::slt_selection_changed(const QItemSelection & selection)
{
	QModelIndexList sel_list = selection.indexes();

	if (sel_list.size() != BtListModel::e_col_nums)
		ui->btn_connect->setEnabled(false);

	QModelIndex index = sel_list[BtListModel::e_col_name];
#ifndef DISCOVER_OFF
	if (device_agent && !device_agent->isActive()) {

		bool sp = list.is_serial(index.row());
		ui->btn_connect->setEnabled(sp);
	}
#else
	bool sp = list.is_sp(index.row());
	ui->btn_connect->setEnabled(sp);
#endif
}

void wdg_bt_t::slt_connect()
{
	QModelIndexList sel_list = ui->view->selectionModel()->selectedIndexes();

	if (sel_list.size() != BtListModel::e_col_nums)
		return;

	emit sig_log(e_log_debug, "Connecting ...");

	QModelIndex index = sel_list[BtListModel::e_col_name];
	QBluetoothDeviceInfo info = list.get_dev(index.row());

	socket->connectToService(info.address(), QBluetoothUuid(QBluetoothUuid::SerialPort));
}

void wdg_bt_t::hostModeStateChanged(QBluetoothLocalDevice::HostMode mode)
{
	bool on = !(mode == QBluetoothLocalDevice::HostPoweredOff);

	emit sig_log(e_log_debug, QString("hostMode: %1").arg(mode));

	ui->btn_scan->setEnabled(on);
}

void wdg_bt_t::slt_socket_error(QBluetoothSocket::SocketError err)
{
	emit sig_log(e_log_debug, QString("socket error:%1").arg(err));
}

void wdg_bt_t::slt_socket_state_changed(QBluetoothSocket::SocketState state)
{
	if (state == QBluetoothSocket::UnconnectedState) {

		ui->btn_connect->setEnabled(true);
	}

	emit sig_log(e_log_debug, QString("socket state:%1").arg(state));
}

void wdg_bt_t::slt_socket_connected()
{
	emit sig_opened();
	emit sig_log(e_log_debug, "socket connected");
}

void wdg_bt_t::slt_socket_disconnected()
{
	emit sig_closed();
	emit sig_log(e_log_debug, "socket disconnected");
}

QByteArray wdg_bt_t::get_msg(char ch)
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

void wdg_bt_t::slt_socket_read()
{
	//QByteArray recievedData = socket->readAll();
	//emit sig_log(e_log_debug, recievedData);

	qint64 nbytes = socket->bytesAvailable();

	//emit sig_log(e_log_debug, QString().sprintf("in available %lld bytes", nbytes));

	if (nbytes <= 0)
		return;

	for (int i = 0; i < nbytes; ++i) {

		char b;
		socket->read(&b, 1);

		QByteArray msg = get_msg(b);
		if (!msg.isEmpty()) {

			//qDebug() << msg;
			emit sig_msg(msg);
		}
	}
}

void wdg_bt_t::slt_msg(const QByteArray & msg)
{
	if (!socket->isOpen())
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

	//qDebug() << "bt slt_msg:" << ba;
	socket->write(ba);
}

