#include <QDebug>

#include "channel.h"
#include "ui_channel.h"

channel_t::channel_t(uint32_t i, QWidget *parent) : QWidget(parent), idx(i), ui(new Ui::channel)
{
	ui->setupUi(this);

	ui->lbl_name->setText(QString("Channel %1").arg(idx + 1));

	ui->combo_type->addItem("byte");
	ui->combo_type->addItem("word(le)");
	ui->combo_type->addItem("word(be)");

	ui->combo_offset->addItem("0");
	ui->combo_offset->addItem("1");
	ui->combo_offset->addItem("2");
	ui->combo_offset->addItem("3");
	ui->combo_offset->addItem("4");
	ui->combo_offset->addItem("5");
	ui->combo_offset->addItem("6");
	ui->combo_offset->addItem("7");

	connect(ui->btn_en, &QRadioButton::toggled, this, &channel_t::slt_btn_en);
	connect(ui->combo_offset, SIGNAL(activated(int)), this, SLOT(slt_activated()));
	connect(ui->combo_type, SIGNAL(activated(int)), this, SLOT(slt_activated()));
	connect(ui->combo_id, SIGNAL(activated(int)), this, SLOT(slt_activated()));
	connect(ui->sbox_mul, SIGNAL(valueChanged(double)), this, SLOT(slt_activated()));
	connect(ui->sbox_add, SIGNAL(valueChanged(int)), this, SLOT(slt_activated()));
}

channel_t::~channel_t()
{
}

void channel_t::set_ids(QVector <uint32_t> ids)
{
	ui->combo_id->clear();
	for (int i = 0; i < ids.size(); i++) {

		uint32_t id = ids[i];
		uint32_t _id = id & 0xfff;
		QString txt = QString("%1").arg(id, 2, 16, QChar('0'));
		if (_id >= 0x7e8 && _id < 0x7ef) {

			uint16_t pid = id >> 16;
			QString spid = "?";
			switch (pid) {

				case 0x4:
					spid = "load";
					break;
				case 0x5:
					spid = "coolant temp";
					break;
				case 0xb:
					spid = "map";
					break;
				case 0xc:
					spid = "rpm";
					break;
				case 0xd:
					spid = "speed";
					break;
				case 0xf:
					spid = "air temp";
					break;
				case 0x10:
					spid = "maf";
					break;
				case 0x11:
					spid = "throttle";
					break;
				case 0x33:
					spid = "abp";
					break;
				case 0x46:
					spid = "air temp";
					break;
				default:
					break;
			}
			txt = QString("obd2(%1) %2(%3)").arg(_id - 0x7e8).arg(pid, 2, 16, QChar('0')).arg(spid);
		}

		ui->combo_id->addItem(txt, id);
	}
}

void channel_t::slt_btn_en(bool en)
{
	if (en) {

		uint32_t id = ui->combo_id->currentData().toInt();
		int off = ui->combo_offset->currentIndex();
		int type = ui->combo_type->currentIndex();
		double mul = ui->sbox_mul->value();
		int add = ui->sbox_add->value();

		emit sig_enabled(idx, id, type, off, mul, add);
	}
	else
		emit sig_disabled(idx);
}

void channel_t::slt_activated()
{
	if (ui->btn_en->isChecked())
		slt_btn_en(true);
}

