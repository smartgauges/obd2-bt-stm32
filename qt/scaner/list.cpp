#include <QColor>

#include "list.h"

ListModel::ListModel(QObject *parent) : QAbstractListModel(parent)
{
	reset();
}

int ListModel::columnCount(const QModelIndex &) const
{
	return e_col_nums;
}

int ListModel::rowCount(const QModelIndex &) const
{
	return ids.size();
}

QVariant ListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal) {

		switch (section) {

			case e_col_num:
				return tr("count");
			case e_col_period:
				return tr("period");
			case e_col_id:
				return tr("id");
			case e_col_data0:
				return tr("b0");
			case e_col_data1:
				return tr("b1");
			case e_col_data2:
				return tr("b2");
			case e_col_data3:
				return tr("b3");
			case e_col_data4:
				return tr("b4");
			case e_col_data5:
				return tr("b5");
			case e_col_data6:
				return tr("b6");
			case e_col_data7:
				return tr("b7");
		}
	}

	return QVariant();
}

QVariant ListModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() >= (int)ids.size())
		return QVariant();

	if (index.column() >= e_col_nums)
		return QVariant();

	const info_t & info = ids[index.row()];

	uint8_t idx = index.column() - e_col_data0;
	if (role == Qt::DisplayRole) {

		switch (index.column()) {

			case e_col_num:
				return info.num;

			case e_col_period:
				return info.period;

			case e_col_id:
				return QString("%1").arg(info.id, 6, 16, QChar('0'));

//			case e_col_len:
//				return 8;

			case e_col_data0:
			case e_col_data1:
			case e_col_data2:
			case e_col_data3:
			case e_col_data4:
			case e_col_data5:
			case e_col_data6:
			case e_col_data7:
				//return QString("%1 %2").arg(info.data[idx].byte, 2, 16, QChar('0')).arg(info.data[idx].cnt);
				return QString("%1").arg(info.data[idx].byte, 2, 16, QChar('0'));
		}
	}
	else if (role == Qt::TextColorRole) {

		if (index.column() >= e_col_data0 && index.column() <= e_col_data7) {

			uint8_t idx = index.column() - e_col_data0;
			return QVariant::fromValue(info.data[idx].cnt >= TICK_MAX_COLOR ? QColor(0, 0, 0) : QColor(0, 0, 255));
		}
	}

	return QVariant();
}

bool ListModel::set_state(const QVector <info_t> & _ids)
{
	if (ids.size() != _ids.size()) {

		beginResetModel();
		ids = _ids;
		endResetModel();

		return true;
	}
	else {

		ids = _ids;

		emit dataChanged(QModelIndex(), QModelIndex());
	}

	return false;
}

void ListModel::reset()
{
	beginResetModel();
	ids.resize(0);
	endResetModel();
}


