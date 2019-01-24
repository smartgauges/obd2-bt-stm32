#include "service.h"

#include <qbluetoothaddress.h>
#include <qbluetoothservicediscoveryagent.h>
#include <qbluetoothserviceinfo.h>
#include <qbluetoothlocaldevice.h>
#include <qbluetoothuuid.h>

ServiceDiscoveryDialog::ServiceDiscoveryDialog(const QString &name, const QBluetoothAddress &address, QWidget *parent) : QDialog(parent), ui(new Ui_ServiceDiscovery)
{
	ui->setupUi(this);

	//Using default Bluetooth adapter
	QBluetoothLocalDevice localDevice;
	QBluetoothAddress adapterAddress = localDevice.address();

	/*
	 * In case of multiple Bluetooth adapters it is possible to
	 * set which adapter will be used by providing MAC Address.
	 * Example code:
	 *
	 * QBluetoothAddress adapterAddress("XX:XX:XX:XX:XX:XX");
	 * discoveryAgent = new QBluetoothServiceDiscoveryAgent(adapterAddress);
	 */

	discoveryAgent = new QBluetoothServiceDiscoveryAgent(adapterAddress);

	discoveryAgent->setRemoteAddress(address);

	setWindowTitle(name);

	connect(discoveryAgent, SIGNAL(serviceDiscovered(QBluetoothServiceInfo)), this, SLOT(addService(QBluetoothServiceInfo)));
	connect(discoveryAgent, SIGNAL(finished()), ui->status, SLOT(hide()));

	discoveryAgent->start();

	connect(ui->list, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(itemActivated(QListWidgetItem*)));
}

ServiceDiscoveryDialog::~ServiceDiscoveryDialog()
{
	delete discoveryAgent;
	delete ui;
}

void ServiceDiscoveryDialog::addService(const QBluetoothServiceInfo &info)
{
	if (info.serviceName().isEmpty())
		return;

	QString line = info.serviceName();
	if (!info.serviceDescription().isEmpty())
		line.append("\n\t" + info.serviceDescription());
	if (!info.serviceProvider().isEmpty())
		line.append("\n\t" + info.serviceProvider());

	ui->list->addItem(line);

	qDebug() << info;

	socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);

	//socket->connectToService(QBluetoothAddress(selectedDevice.address()), QBluetoothUuid(QBluetoothUuid::SerialPort));
	socket->connectToService(info);

	connect(socket, SIGNAL(error(QBluetoothSocket::SocketError)), this, SLOT(socketError(QBluetoothSocket::SocketError)));
	connect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
	connect(socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
	connect(socket, SIGNAL(readyRead()), this, SLOT(socketRead()));
}

void ServiceDiscoveryDialog::itemActivated(QListWidgetItem *)
{
	//selectedDevice = listOfDevices.at(i);
#if 0
	//qDebug() << "User select a device: " << selectedDevice.name() << " ("<< selectedDevice.address().toString().trimmed() << ")";
	socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);

	//socket->connectToService(QBluetoothAddress(selectedDevice.address()), QBluetoothUuid(QBluetoothUuid::SerialPort));
	QBluetoothServiceInfo info;
	info.setDevice(address);
	socket->connectToService(QBluetoothUuid(QBluetoothUuid::SerialPort));

	connect(socket, SIGNAL(error(QBluetoothSocket::SocketError)), this, SLOT(socketError(QBluetoothSocket::SocketError)));
	connect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
	connect(socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
	connect(socket, SIGNAL(readyRead()), this, SLOT(socketRead()));
#endif
}

void ServiceDiscoveryDialog::socketError(QBluetoothSocket::SocketError err)
{
	qDebug() << "socketError" << err;
}

void ServiceDiscoveryDialog::socketConnected()
{
	qDebug() << "socketConnected";
//	socket->write("at\r");
//	socket->write("at+version\r");
//	socket->write("at+rv\r");
	socket->write("AT+I\r");
}

void ServiceDiscoveryDialog::socketDisconnected()
{
	qDebug() << "socketDisconnected";
}

void ServiceDiscoveryDialog::socketRead()
{
	qDebug() << "socketRead";
	QByteArray recievedData = socket->readAll();
	qDebug() << recievedData;

	//emit dataRecieved(recievedData);
}

