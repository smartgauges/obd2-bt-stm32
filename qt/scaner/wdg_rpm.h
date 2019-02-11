#ifndef WDG_RPM_H
#define WDG_RPM_H

#include <QWidget>
#include <QTime>
#include <math.h>
#include <inttypes.h>

class wdg_rpm_t : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(int Speed READ Speed WRITE setSpeed);
	Q_PROPERTY(int Taho READ Taho WRITE setTaho);

	public:
		wdg_rpm_t(QWidget *parent = 0);
		~wdg_rpm_t();
		QSize sizeHint() const;
		int Speed() const;
		void setSpeed(int);
		int Taho() const;
		void setTaho(int);

	protected:
		void paintEvent(QPaintEvent *event);

	private:
		int speed_value;
		int taho_value;
};

#endif

