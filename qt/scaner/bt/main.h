#ifndef MAIN_BT_H
#define MAIN_BT_H

#include <QMainWindow>
#include "wdg_bt.h"

namespace Ui
{
	class main;
}

class main_t : public QMainWindow
{
	Q_OBJECT
	private:
		Ui::main * ui;
	private slots:
		void slt_log(uint8_t, const QString & log);

	public:
		main_t(QMainWindow *parent = 0);
		~main_t();
};

#endif

