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
	connect(ui->sbox_add, SIGNAL(valueChanged(double)), this, SLOT(slt_activated()));
	connect(ui->le_mask, SIGNAL(textChanged(const QString &)), this, SLOT(slt_activated()));
	connect(ui->btn_swap, &QRadioButton::toggled, this, &channel_t::slt_btn_swap);
}

channel_t::~channel_t()
{
}

void channel_t::set_ids(QVector <qid_t> ids)
{
	ui->combo_id->clear();
	for (int i = 0; i < ids.size(); i++) {

		ui->combo_id->addItem(ids[i].name, ids[i].id);
	}
}

void channel_t::slt_btn_en(bool en)
{
	if (en) {

		uint32_t id = ui->combo_id->currentData().toInt();
		int off = ui->combo_offset->currentIndex();
		int type = ui->combo_type->currentIndex();
		double mul = ui->sbox_mul->value();
		double add = ui->sbox_add->value();
		bool ok;
		uint16_t mask = ui->le_mask->displayText().toInt(&ok, 16);
		bool swap = ui->btn_swap->isChecked();

		emit sig_enabled(idx, id, type, off, mask, mul, add, swap);
	}
	else
		emit sig_disabled(idx);
}

void channel_t::slt_activated()
{
	if (ui->btn_en->isChecked())
		slt_btn_en(true);
}

void channel_t::slt_btn_swap(bool)
{
	if (ui->btn_en->isChecked())
		slt_btn_en(true);
}

