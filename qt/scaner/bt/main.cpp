#include <QDebug>
#include <QLoggingCategory>
#include "QCoreApplication"

#include "main.h"
#include "ui_main.h"

main_t::main_t(QMainWindow * parent) : QMainWindow(parent), ui(new Ui::main)
{
	ui->setupUi(this);

	connect(ui->bt, &wdg_bt_t::sig_log, this, &main_t::slt_log);
	ui->bt->start();
}

main_t::~main_t()
{
}

void main_t::slt_log(uint8_t, const QString & log)
{
	ui->log->appendPlainText(log);
}

int main(int argc, char ** argv)
{
	QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));

	QApplication app(argc, argv);

	main_t m;

	m.show();

	return app.exec();
}

