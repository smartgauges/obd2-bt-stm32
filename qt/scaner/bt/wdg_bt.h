#ifndef WDG_BT_H
#define WDG_BT_H

#include <qbluetoothglobal.h>
#include <qbluetoothlocaldevice.h>
#include <QBluetoothSocket>

#include "ui_bt.h"
#include "bt_list.h"
#include "msg.h"

QT_FORWARD_DECLARE_CLASS(QBluetoothDeviceDiscoveryAgent)
QT_FORWARD_DECLARE_CLASS(QBluetoothServiceDiscoveryAgent)
QT_FORWARD_DECLARE_CLASS(QBluetoothDeviceInfo)

QT_USE_NAMESPACE

//#ifdef Q_OS_ANDROID
//#define DISCOVER_OFF
//#endif
class wdg_bt_t : public QWidget
{
	Q_OBJECT

	public:
		wdg_bt_t(QWidget *parent = 0);
		~wdg_bt_t();
		bool start();

	public slots:
		void slt_msg(const QByteArray &);

	private slots:
#ifndef DISCOVER_OFF
		void slt_scan();
		void slt_add_device(const QBluetoothDeviceInfo&);
		void slt_device_scan_finished();

		void slt_add_service(const QBluetoothServiceInfo &info);
		void slt_service_scan_finished();
#endif
		void slt_connect();

		void slt_selection_changed(const QItemSelection & selection);

		void hostModeStateChanged(QBluetoothLocalDevice::HostMode);
		void handleLocalDeviceError(QBluetoothLocalDevice::Error error);

		void slt_socket_read();
		void slt_socket_error(QBluetoothSocket::SocketError err);
		void slt_socket_connected();
		void slt_socket_disconnected();
		void slt_socket_state_changed(QBluetoothSocket::SocketState);

	signals:
		void sig_opened();
		void sig_closed();
		void sig_log(uint8_t, const QString & log);
		void sig_msg(const QByteArray &);

	private:
		QBluetoothLocalDevice *localDevice;
#ifndef DISCOVER_OFF
		QBluetoothDeviceDiscoveryAgent *device_agent;
		QBluetoothServiceDiscoveryAgent *service_agent;
#endif
		QBluetoothSocket * socket;

		Ui::bt * ui;
		BtListModel list;
		QByteArray ba;
		uint8_t rx_state;
	private:
		QByteArray get_msg(char ch);
		void start_check_serial(const QBluetoothDeviceInfo & info);
};

#endif

