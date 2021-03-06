#ifndef MAIN_COM_H
#define MAIN_COM_H

#include <QMainWindow>
#include "wdg_com.h"

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

