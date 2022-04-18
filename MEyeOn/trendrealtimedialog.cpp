#include "trendrealtimedialog.h"
#include "ui_trendrealtimedialog.h"

TrendRealTimeDialog::TrendRealTimeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TrendRealTimeDialog)
{
    dataProcess = new DataProcess(this);
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

void TrendRealTimeDialog::msPlot(QJsonArray dataPointsArray)
{
    QVector<double> xVector, yVector;
    QJsonArray::Iterator i = dataPointsArray.begin();
    while (i != dataPointsArray.end()) {
        xVector.append(calibrate(i->toArray().at(0).toDouble()));
        yVector.append(i->toArray().at(1).toDouble());
        i++;
    }
    ui->msRealTimePlot->graph(0)->setData(xVector, yVector);
    ui->msRealTimePlot->yAxis->rescale(true);
    ui->msRealTimePlot->xAxis->setRange(0, massH);
    ui->msRealTimePlot->yAxis->setRange(0, massIntensityH);
    ui->msRealTimePlot->replot();

    //    double trendValue = 0;
    //    trendValue = dataProcess->sumProcess(dataPointsArray);
    //    addPoint(currentParameter, trendValue);
}

void TrendRealTimeDialog::mobilityPlot(QJsonArray dataPointsArray)
{
    QVector<double> xVector, yVector;
    QJsonArray::Iterator i = dataPointsArray.begin();
    while (i != dataPointsArray.end()) {
        xVector.append(i->toArray().at(0).toDouble());
        yVector.append(i->toArray().at(1).toDouble());
        i++;
    }
    ui->mobilityRealTimePlot->graph(0)->setData(xVector, yVector);
    ui->mobilityRealTimePlot->xAxis->rescale(true);
    ui->mobilityRealTimePlot->yAxis->setRange(0, mobilityH);
    ui->mobilityRealTimePlot->xAxis->setRange(0, mobilityIntensityH);
    ui->mobilityRealTimePlot->replot();
}

void TrendRealTimeDialog::heatmapPlot(QJsonArray dataPointsArray)
{
    int sizeX = ui->heatmapView->width();
    int sizeY = ui->heatmapView->height();
    double xPixelStep = (heatmapMassH - heatmapMassL) / sizeX;
    double yPixelStep = (heatmapMobilityH - heatmapMobilityL) / sizeY;

    QImage image = QImage( sizeX, sizeY, QImage::Format_ARGB32);
    QJsonArray::Iterator i = dataPointsArray.begin();

    while (i != dataPointsArray.end()) {
        QJsonObject object = i->toObject();
        int xindex = (calibrate(object.value("x").toDouble()) - heatmapMassL) / xPixelStep;
        xindex = xindex < 0 ? 0 : xindex;
        xindex = xindex >= sizeX ? (sizeX - 1) : xindex;


        int yindex = (object.value("y").toDouble() - heatmapMobilityL) / yPixelStep;
        yindex = yindex < 0 ? 0 : yindex;
        yindex = yindex >= sizeY ? (sizeY - 1) : yindex;
        yindex = sizeY - 1 - yindex;

        image.setPixelColor(xindex, yindex, "#ff000000");// object.value("color").toString());

        i++;
    }
    QGraphicsScene *graphic = new QGraphicsScene(this);
    graphic->addPixmap(QPixmap::fromImage(image));
    ui->heatmapView->setScene(graphic);
}

void TrendRealTimeDialog::resetPlot()
{
    xPoints.clear();
    yPoints.clear();
    replot();
}

void TrendRealTimeDialog::setRange(QJsonObject payload)
{
    QJsonArray heatmapMassRange = payload.value("heatmapMassRange").toArray();
    if(heatmapMassRange.size() == 2){
        heatmapMassL = 0;
        heatmapMassH = heatmapMassRange.last().toDouble();
    }

    QJsonArray heatmapMobilityRange = payload.value("heatmapMobilityRange").toArray();
    if(heatmapMobilityRange.size() == 2){
        heatmapMobilityL = 0;
        heatmapMobilityH = heatmapMobilityRange.last().toDouble();
    }

    QJsonArray massRange = payload.value("massRange").toArray();
    if(massRange.size() == 2){
        massL = 0;
        massH = massRange.last().toDouble();
    }

    QJsonArray massIntensityRange = payload.value("massIntensityRange").toArray();
    if(massIntensityRange.size() == 2){
        massIntensityL = 0;
        massIntensityH = massIntensityRange.last().toDouble() * 1.05;
    }

    QJsonArray mobilityRange = payload.value("mobilityRange").toArray();
    if(mobilityRange.size() == 2){
        mobilityL = 0;
        mobilityH = mobilityRange.last().toDouble();
    }

    QJsonArray mobilityIntensityRange = payload.value("mobilityIntensityRange").toArray();
    if(mobilityIntensityRange.size() == 2){
        mobilityIntensityL = 0;
        mobilityIntensityH = mobilityIntensityRange.last().toDouble() * 1.05;
    }

//        QJsonArray::Iterator i = ranges.begin();
//        while (i != ranges.end()) {
//            if(i->toArray().at())
//        }
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

    ui->mobilityRealTimePlot->xAxis->setLabel("Intensity");
    ui->mobilityRealTimePlot->xAxis->setRangeReversed(true);
    ui->mobilityRealTimePlot->yAxis->setLabel("arrive time");
    ui->mobilityRealTimePlot->addGraph(ui->mobilityRealTimePlot->yAxis, ui->mobilityRealTimePlot->xAxis);
    //ui->mobilityRealTimePlot->graph(0)->
}

void TrendRealTimeDialog::replot()
{
    ui->trendRealTimePlot->graph(0)->setData(xPoints, yPoints);
    ui->trendRealTimePlot->yAxis->rescale(true);
    ui->trendRealTimePlot->xAxis->rescale(true);
    ui->trendRealTimePlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 10));
    ui->trendRealTimePlot->replot();
}

double TrendRealTimeDialog::calibrate(double x)
{
    double slope = 0.3458;
    double intercept = 0.093;
    return qPow(slope * (x - intercept), 2);
}