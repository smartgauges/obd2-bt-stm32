#include <QColor>
#include <QIcon>

#include "bt_list.h"

BtListModel::BtListModel(QObject *parent) : QAbstractListModel(parent)
{
	reset();
}

int BtListModel::columnCount(const QModelIndex &) const
{
	return e_col_nums;
}

int BtListModel::rowCount(const QModelIndex &) const
{
	return devs.size();
}

QVariant BtListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal) {

		switch (section) {

			case e_col_addr:
				return tr("addr");
			case e_col_name:
				return tr("name");
		}
	}

	return QVariant();
}

QVariant BtListModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() >= (int)devs.size())
		return QVariant();

	if (index.column() >= e_col_nums)
		return QVariant();

	const info_t & info = devs[index.row()];

	if (role == Qt::DisplayRole) {

		switch (index.column()) {

			case e_col_addr:
				return info.dev.address().toString();

			case e_col_name:
				return info.dev.name();
		}
	}
	else if (role == Qt::TextColorRole) {

		if (info.status == QBluetoothLocalDevice::Paired || info.status == QBluetoothLocalDevice::AuthorizedPaired )
			return QColor(Qt::green);
		else
			return QColor(Qt::black);
	}
	else if (role == Qt::DecorationRole) {

		if (index.column() == e_col_name && info.serial) {

			QVariant v;
			v.setValue(QIcon(":/images/sp.png"));

			return v;
		}
	}

	return QVariant();
}

bool BtListModel::add(const QBluetoothDeviceInfo & dev, QBluetoothLocalDevice::Pairing status, bool serial)
{
	info_t info;
	info.dev = dev;
	info.status = status;
	info.serial = serial;

	for (int i = 0; i < devs.size();i++) {

		if (devs[i].dev == dev) {

			devs[i] = info;
			emit dataChanged(QModelIndex(), QModelIndex());
			return true;
		}
	}

	beginResetModel();
	devs.push_back(info);
	endResetModel();

	return true;
}

void BtListModel::reset()
{
	beginResetModel();
	devs.resize(0);
	endResetModel();
}

QBluetoothDeviceInfo BtListModel::get_dev(uint32_t idx)
{
	if (idx >= (uint32_t)devs.size())
		return QBluetoothDeviceInfo();

	return devs[idx].dev;
}

bool BtListModel::is_serial(uint32_t idx)
{
	if (idx >= (uint32_t)devs.size())
		return false;

	return devs[idx].serial;
}

void BtListModel::set_serial(const QBluetoothAddress & address)
{
	for (int i = 0; i < devs.size();i++) {

		if (devs[i].dev.address() == address) {

			devs[i].serial = true;
			emit dataChanged(QModelIndex(), QModelIndex());
			break;
		}
	}
}

bool BtListModel::is_checked(uint32_t idx)
{
	if (idx >= (uint32_t)devs.size())
		return false;

	return devs[idx].checked;
}

void BtListModel::set_checked(const QBluetoothAddress & address)
{
	for (int i = 0; i < devs.size();i++) {

		if (devs[i].dev.address() == address) {

			devs[i].checked = true;
			emit dataChanged(QModelIndex(), QModelIndex());
			break;
		}
	}
}

