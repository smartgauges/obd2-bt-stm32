#ifndef CHANNEL_H
#define CHANNEL_H

#include <QWidget>
#include <QVector>

namespace Ui
{
	class channel;
}

enum e_type
{
	e_type_byte = 0,
	e_type_word_le,
	e_type_word_be,

};

class channel_t : public QWidget
{
	Q_OBJECT

	private:
		uint32_t idx;
		Ui::channel * ui;

	private slots:
		void slt_btn_en(bool en);
		void slt_activated();

	public:
		channel_t(uint32_t idx, QWidget *parent = 0);
		~channel_t();
		void set_ids(QVector <uint32_t>);

	signals:
		void sig_enabled(int idx, uint32_t id, int type, int off, double mul, int add);
		void sig_disabled(int idx);
};

#endif

