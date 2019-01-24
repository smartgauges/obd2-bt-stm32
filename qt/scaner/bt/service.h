#ifndef SERVICE_H
#define SERVICE_H

#include "ui_service.h"

#include <qbluetoothglobal.h>
#include <QDialog>
#include <QBluetoothSocket>

QT_FORWARD_DECLARE_CLASS(QBluetoothAddress)
QT_FORWARD_DECLARE_CLASS(QBluetoothServiceInfo)
QT_FORWARD_DECLARE_CLASS(QBluetoothServiceDiscoveryAgent)

QT_USE_NAMESPACE

class ServiceDiscoveryDialog : public QDialog
{
	Q_OBJECT

	public:
		ServiceDiscoveryDialog(const QString &name, const QBluetoothAddress &address, QWidget *parent = 0);
		~ServiceDiscoveryDialog();

	public slots:
		void addService(const QBluetoothServiceInfo&);

	private slots:
		void itemActivated(QListWidgetItem *item);
		void socketRead();
		void socketError(QBluetoothSocket::SocketError err);
		void socketConnected();
		void socketDisconnected();

	private:
		QBluetoothServiceDiscoveryAgent *discoveryAgent;
		Ui_ServiceDiscovery *ui;
		QBluetoothSocket * socket;
		QBluetoothAddress address;
};

#endif

