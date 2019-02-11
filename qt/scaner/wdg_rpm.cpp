#include <QPainter>

#include "wdg_rpm.h"

wdg_rpm_t::wdg_rpm_t(QWidget *parent) : QWidget(parent)
{
	speed_value = 0;
	taho_value = 0;
}

wdg_rpm_t::~wdg_rpm_t()
{
}

int wdg_rpm_t::Speed() const
{
	return speed_value;
}

void wdg_rpm_t::setSpeed(int s)
{
	if (s != speed_value) {

		speed_value = s;
		update();
	}
}

int wdg_rpm_t::Taho() const
{
	return taho_value;
}

void wdg_rpm_t::setTaho(int t)
{
	if (t != taho_value) {

		taho_value = t;
		update();
	}
}

QSize wdg_rpm_t::sizeHint() const 
{
	return QSize(100, 100);
}

static double norm(double value, double in_min, double in_max, double out_min, double out_max)
{
	return (((value - in_min) * (out_max - out_min)) / (in_max - in_min)) + out_min;
}

void wdg_rpm_t::paintEvent(QPaintEvent *)
{
	QPainter painter(this);

	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

	QRect r = rect();

	int width = r.width();
	int height = r.height();

	painter.fillRect(r, QBrush(Qt::gray));

	double arc_len = 270;
	double arc_start = 225;

	double t = height/4;
	QPixmap taho(":/images/venator/taho.png");
	painter.drawPixmap(QPointF(width/2 - taho.width()/2, t - taho.height()/2), taho);

	double s = (3 * height) / 4;
	QPixmap speed(":/images/venator/speed.png");
	painter.drawPixmap(QPointF(width/2 - speed.width()/2, s - speed.height()/2), speed);

	double arc = norm(taho_value, 0, 8000, 0, arc_len);
	painter.save();
	painter.translate(width/2, t);
	painter.rotate(-arc_start + arc);
	QPixmap arrow(":/images/venator/arrow0.png");
	painter.drawPixmap(QPointF(0, - arrow.height()/2), arrow);
	painter.restore();

	arc = norm(speed_value, 0, 200, 0, arc_len);
	painter.save();
	painter.translate(width/2, s);
	painter.rotate(-arc_start + arc);
	painter.drawPixmap(QPointF(0, - arrow.height()/2), arrow);
	painter.restore();

	QPixmap circle(":/images/venator/circle1.png");
	painter.drawPixmap(QPointF(width/2 - circle.width()/2, t - circle.height()/2), circle);
	painter.drawPixmap(QPointF(width/2 - circle.width()/2, s - circle.height()/2), circle);

	if (taho_value > 1500 && taho_value < 2200) {

		QPixmap green(":/images/venator/green.png");
		painter.drawPixmap(QPointF(width/2 - green.width()/2, t - green.height()/2), green);
	}
	else if (taho_value > 3500) {

		QPixmap red(":/images/venator/red.png");
		painter.drawPixmap(QPointF(width/2 - red.width()/2, t - red.height()/2), red);
	}

	if (speed_value > 120) {

		QPixmap red(":/images/venator/red.png");
		painter.drawPixmap(QPointF(width/2 - red.width()/2, s - red.height()/2), red);
	}
}

