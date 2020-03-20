#include <QtWidgets>
#include <QFileDialog>
#include <QDebug>

#include "wdg_fw.h"
#include "ui_fw.h"
#include "crc_xmodem.h"
#include "bl.h"

wdg_fw_t::wdg_fw_t(QWidget * parent) : QWidget(parent), ui(new Ui::fw)
{
	ui->setupUi(this);

	ui->frame->setEnabled(true);

	ui->btn_open->setEnabled(true);
	ui->btn_flash->setEnabled(false);
	ui->btn_rst->setEnabled(true);

	connect(ui->btn_flash, SIGNAL(clicked()), this, SLOT(slt_flash()));
	connect(ui->btn_open, SIGNAL(clicked()), this, SLOT(slt_open()));
	connect(ui->btn_rst, SIGNAL(clicked()), this, SLOT(slt_rst()));
	connect(ui->btn_erase, SIGNAL(clicked()), this, SLOT(slt_erase()));

	timer.setSingleShot(true);
	connect(&timer, SIGNAL(timeout()),this, SLOT(slt_timer()));

	ui->cb_speed->addItem("125");
	ui->cb_speed->addItem("250");
	ui->cb_speed->addItem("500");
	ui->cb_speed->addItem("1000");
	ui->cb_speed->addItem("auto");

	connect(ui->cb_speed, SIGNAL(currentIndexChanged(int)), this, SLOT(slt_cb_select_speed(int)));
}

bool wdg_fw_t::send()
{
	uint8_t buf[512];
	msg_t * msg = (msg_t *)buf;
	msg->type = e_type_write;

	msg_flash_t * flash = (msg_flash_t *)msg->data;
	flash->addr = offset;
	flash->num = 32;

	uint32_t len = flash->num * sizeof(uint16_t);

	if ((offset + len) > (uint32_t)fw.size())
		len = fw.size() - offset;

	flash->num = (len/sizeof(uint16_t)) + ((len % 2) ? 1 : 0);
	memset(flash->data, 0, flash->num * sizeof(uint16_t));

	//printf("len:%d num:%d\n", len, flash->num);

	ui->pb->setValue((100.0 * (offset + len))/ fw.size());

	uint8_t * p = (uint8_t *)flash->data;
	for (uint32_t i = 0; i < len; i++) {

		uint8_t byte = fw[offset + i];
		crc = crc_xmodem_update(crc, byte);
		p[i] = byte;
	}

	msg->len = sizeof(msg_t) + sizeof(msg_flash_t) + flash->num * sizeof(uint16_t);
	emit sig_msg(QByteArray((char *)buf, msg->len));

	offset += len;

#if 0
	printf("send %d:", flash->num);
	uint16_t * p16 = (uint16_t *)flash->data;
	for (uint32_t i = 0; i < flash->num; i++)
		printf("%04x", p16[i]);
	printf("\n");
#endif

	if (offset >= (uint32_t)fw.size()) { 

		//printf("len = 0x%x, crc 0x%x\n", len, crc);

		uint8_t buf[512];
		msg_t * msg = (msg_t *)buf;
		msg->type = e_type_write;

		msg_flash_t * flash = (msg_flash_t *)msg->data;
		flash->addr = ADDR_APP_LEN - ADDR_APP;
		flash->num = 2;
		uint16_t * p = (uint16_t *)flash->data;
		p[0] = fw.size();
		p[1] = crc;
		msg->len = sizeof(msg_t) + sizeof(msg_flash_t) + flash->num * sizeof(uint16_t);

		emit sig_msg(QByteArray((char *)buf, msg->len));

		return false;
	}
	else
		return true;
}

void wdg_fw_t::slt_rst()
{
	QByteArray ba(sizeof(msg_t), 0);
	struct msg_t * m = (struct msg_t *)ba.data();
	m->len = sizeof(msg_t);
	m->type = e_type_start;

	emit sig_msg(ba);
}

void wdg_fw_t::slt_erase()
{
	QByteArray ba(sizeof(msg_t), 0);
	struct msg_t * m = (struct msg_t *)ba.data();
	m->len = sizeof(msg_t);
	m->type = e_type_erase;

	emit sig_msg(ba);
}

void wdg_fw_t::slt_flash()
{
	offset = 0;
	crc = 0;
	ui->btn_flash->setEnabled(false);
	ui->btn_open->setEnabled(false);

	if (send())
		timer.start(50);
}

void wdg_fw_t::slt_timer()
{
	if (send())
		timer.start(50);
	else {
		ui->btn_flash->setEnabled(true);
		ui->btn_open->setEnabled(true);
	}
}

#define MAX_FW_SIZE (28 * 1024)
void wdg_fw_t::slt_open()
{
#ifdef Q_OS_ANDROID
	QString filename = QFileDialog::getOpenFileName(this, tr("Open firmware file"), "/sdcard/Download/", tr("Binary Files (*.bin)"));
#else
	QString filename = QFileDialog::getOpenFileName(this, tr("Open firmware file"), "./", tr("Binary Files (*.bin)"));
#endif

	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly)) {

		//emit slt_log(e_log_warn, "Can't open file:" + QString(filename));
		return;
	}

	fw = file.readAll();
	file.close();

	uint32_t len = fw.size();
	if (len) {

		if (len > MAX_FW_SIZE)
			return;

		ui->btn_flash->setEnabled(true);
	}

	qDebug() << "file size:" << len << "fw size:" << fw.size();
}

void wdg_fw_t::slt_cb_select_speed(int idx)
{
	QByteArray ba(sizeof(msg_t) + 2, 0);
	struct msg_t * m = (struct msg_t *)ba.data();
	m->len = ba.size();
	m->type = e_type_cmd;

	uint8_t * cmd = m->data;
	cmd[0] = e_cmd_set_speed;
	cmd[1] = idx;

	emit sig_msg(ba);
}

