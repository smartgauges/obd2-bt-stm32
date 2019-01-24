#ifndef LIST_H
#define LIST_H

#include <QAbstractListModel>
#include "msg.h"

#define TICK_HZ 10
//2 sec
#define MAX_COLOR_SEC 2
#define TICK_MAX_COLOR (MAX_COLOR_SEC * TICK_HZ)

struct bc_t
{
	uint8_t byte;
	uint8_t cnt;

	void reset()
	{
		byte = 0;
		cnt = 0;
	};

	void set(uint8_t b)
	{
		if (byte != b)
			cnt = 0;

		byte = b;
	};

	void inc(uint8_t max)
	{
		if (cnt++ >= max)
			cnt = max;
	};
};

struct info_t
{
	uint32_t id;
	uint32_t num;
	uint32_t period;
	uint32_t prev_num;
	uint32_t ticks;
	bc_t data[8];
	//QString desc;
	uint32_t idx;

	void reset()
	{
		num = 0;
		//desc = "";
		for (int i = 0; i < 8; i++)
			data[i].reset();
	};

	void set(const uint8_t _data[8])
	{
		for (int i = 0; i < 8; i++)
			data[i].set(_data[i]);
	};

	void inc(uint8_t max)
	{
		for (int i = 0; i < 8; i++)
			data[i].inc(max);
	};
};

class ListModel : public QAbstractListModel
{ 
	Q_OBJECT

	private:
		QVector <info_t> ids;

	public:
		ListModel(QObject *parent = 0);

		enum enum_can
		{
			e_col_num = 0,
			e_col_period,
			e_col_id,
//			e_col_len,
			e_col_data0,
			e_col_data1,
			e_col_data2,
			e_col_data3,
			e_col_data4,
			e_col_data5,
			e_col_data6,
			e_col_data7,
			e_col_nums
		};

		int rowCount(const QModelIndex &parent = QModelIndex()) const;

		int columnCount(const QModelIndex &parent = QModelIndex()) const;

		QVariant data(const QModelIndex &index, int role) const;

		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	public:

		void reset();
		bool set_state(const QVector <info_t> &);
};

#endif

