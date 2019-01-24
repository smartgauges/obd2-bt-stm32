#ifndef BT_LIST_H
#define BT_LIST_H

#include <qbluetoothaddress.h>
#include <qbluetoothdevicediscoveryagent.h>
#include <qbluetoothlocaldevice.h>
#include <qbluetoothserviceinfo.h>

#include <QAbstractListModel>

class BtListModel : public QAbstractListModel
{ 
	Q_OBJECT

	private:
		typedef struct info_t
		{ 
			QBluetoothLocalDevice::Pairing status;
			QBluetoothDeviceInfo dev;
			bool serial;
			bool checked;
		} info_t;
		QVector <info_t> devs;

	public:
		BtListModel(QObject *parent = 0);

		enum enum_can
		{
			e_col_name = 0,
			e_col_addr,
			e_col_nums
		};

		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex &parent = QModelIndex()) const;
		QVariant data(const QModelIndex &index, int role) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	public:

		void reset();
		bool add(const QBluetoothDeviceInfo &, QBluetoothLocalDevice::Pairing, bool serial);
		QBluetoothDeviceInfo get_dev(uint32_t idx);
		bool is_serial(uint32_t idx);
		void set_serial(const QBluetoothAddress & address);
		bool is_checked(uint32_t idx);
		void set_checked(const QBluetoothAddress & address);
};

#endif

