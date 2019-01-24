#ifndef GRAPH_H
#define GRAPH_H

#include <QWidget>
#include <QHBoxLayout>

#include <QtCharts/QSplineSeries>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>

using namespace QtCharts;

#define NUM_CHANNELS 5
class graph_t : public QWidget
{
	Q_OBJECT

	private:
		QHBoxLayout mainLayout;

		QChartView *view;

		QValueAxis *xAxis;
		QValueAxis *yAxis;

		QPen pen[NUM_CHANNELS];
		QLineSeries diagram[NUM_CHANNELS];

		void paint(QVector <double> & data, QLineSeries *series);

	public:
		graph_t(QWidget *parent = 0);
		~graph_t();

		void set(int idx, QVector<double> & data);
		void clear(int idx);
		void clear();

		void prepare(int x);
};

#endif

