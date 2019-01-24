#include "graph.h"

#define PEN_WIDTH 2.5f

graph_t::graph_t(QWidget *parent) : QWidget(parent)
{
	setLayout(&mainLayout);
	view = new QChartView;
	mainLayout.addWidget(view);

	xAxis = new QValueAxis;
	xAxis->setTitleText(tr("Time"));
	xAxis->setTitleBrush(Qt::magenta);
	xAxis->setLabelsColor(Qt::magenta);

	yAxis = new QValueAxis;
	yAxis->setTitleText(tr("Values"));
	yAxis->setTitleBrush(Qt::yellow);
	yAxis->setLabelsColor(Qt::yellow);

	view->chart()->setTheme(QChart::ChartThemeBlueCerulean);

	//QBrush colors[NUM_CHANNELS] = { Qt::white, Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta, Qt::yellow, Qt::gray, Qt::black };
	QBrush colors[NUM_CHANNELS] = { Qt::white, Qt::red, Qt::green, Qt::magenta, Qt::cyan };

	for (int i = 0; i < NUM_CHANNELS; i++) {

		pen[i].setWidthF(PEN_WIDTH);
		pen[i].setBrush(colors[i]);

		diagram[i].setName(QString("Channel %1").arg(i + 1));
		diagram[i].setPen(pen[i]);
#ifndef Q_OS_WIN32
		diagram[i].setUseOpenGL(true);
#endif
		view->chart()->addSeries(&diagram[i]);
		view->chart()->setAxisX(xAxis, &diagram[i]);
		view->chart()->setAxisY(yAxis, &diagram[i]);
	}

	view->setRubberBand(QChartView::RectangleRubberBand);
}

graph_t::~graph_t()
{
}

void graph_t::prepare(int x)
{
	xAxis->setRange(0, x);
	yAxis->setRange(-10, 300);
}

void graph_t::set(int idx, QVector<double> & data)
{
	if (idx > NUM_CHANNELS)
		return;

	diagram[idx].clear();

	paint(data, &diagram[idx]);
}

void graph_t::clear(int idx)
{
	if (idx > NUM_CHANNELS)
		return;

	diagram[idx].clear();
}

void graph_t::clear()
{
	for (int i = 0; i < NUM_CHANNELS; i++)
		diagram[i].clear();
}

void graph_t::paint(QVector <double> & data, QLineSeries *series)
{
	for(int i(0); i < data.size(); i++)
		series->append(i, data[i]);
}

