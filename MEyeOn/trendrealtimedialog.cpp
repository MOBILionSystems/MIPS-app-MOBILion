#include "trendrealtimedialog.h"
#include "ui_trendrealtimedialog.h"

TrendRealTimeDialog::TrendRealTimeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TrendRealTimeDialog)
{
    ui->setupUi(this);
    initPlot();
}

TrendRealTimeDialog::~TrendRealTimeDialog()
{
    delete ui;
}

void TrendRealTimeDialog::addPoint(double x, double y)
{
    if(!xPoints.empty() && x == xPoints.last())
        return;
    xPoints.push_back(x);
    yPoints.push_back(y);
    replot();
}

void TrendRealTimeDialog::msPlot(QVector<double> xVector, QVector<double> yVector)
{
    ui->msRealTimePlot->graph(0)->setData(xVector, yVector);
    ui->msRealTimePlot->yAxis->rescale(true);
    ui->msRealTimePlot->xAxis->rescale(true);
    ui->msRealTimePlot->replot();
}

void TrendRealTimeDialog::resetPlot()
{
    xPoints.clear();
    yPoints.clear();
    replot();
}

// refer to adc.cpp; https://www.qcustomplot.com/index.php/tutorials/basicplotting
void TrendRealTimeDialog::initPlot()
{
    ui->trendRealTimePlot->xAxis->setLabel("voltages");
    ui->trendRealTimePlot->yAxis->setLabel("Summed weighted heights");
    ui->trendRealTimePlot->addGraph();

    ui->msRealTimePlot->xAxis->setLabel("m/z");
    ui->msRealTimePlot->yAxis->setLabel("Intensity");
    ui->msRealTimePlot->addGraph();
}

void TrendRealTimeDialog::replot()
{
    ui->trendRealTimePlot->graph(0)->setData(xPoints, yPoints);
    ui->trendRealTimePlot->yAxis->rescale(true);
    ui->trendRealTimePlot->xAxis->rescale(true);
    ui->trendRealTimePlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 10));
    ui->trendRealTimePlot->replot();
}
