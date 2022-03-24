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

// refer to adc.cpp
void TrendRealTimeDialog::initPlot()
{
    ui->trendRealTimePlot->xAxis->setLabel("voltages");
    ui->trendRealTimePlot->yAxis->setLabel("Summed weighted heights");
    ui->trendRealTimePlot->addGraph();

    QVector<double> x, y;
    for(int i = 0; i < 5; i++){
        x.push_back(i);
        y.push_back(i * i);
    }

    ui->trendRealTimePlot->graph(0)->setData(x, y);
    ui->trendRealTimePlot->yAxis->rescale(true);
    ui->trendRealTimePlot->xAxis->rescale(true);
    ui->trendRealTimePlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 10));
    ui->trendRealTimePlot->replot();
}
