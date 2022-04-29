#ifndef TRENDREALTIMEDIALOG_H
#define TRENDREALTIMEDIALOG_H

#include <QDialog>
#include "dataprocess.h"
#include <QGraphicsScene>

namespace Ui {
class TrendRealTimeDialog;
}

class TrendRealTimeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TrendRealTimeDialog(QWidget *parent = nullptr);
    ~TrendRealTimeDialog();
    void addPoint(double x, double y);
    void msPlot(QJsonArray dataPointsArray);
    void mobilityPlot(QJsonArray dataPointsArray);
    void heatmapPlot(QJsonArray dataPointsArray);
    void resetPlot();

    void setRange(QJsonObject payload);
    void startNewStep(double currentStep);

private:
    bool _newStep = false;
    double _currentStep = 0;
    QGraphicsScene* graphic_hm;
    DataProcess* dataProcess;
    Ui::TrendRealTimeDialog *ui;
    void initPlot();
    QVector<double> xPoints, yPoints;
    void replot();
    double calibrate(double x);

    double massL = 0;
    double massH = 0;
    double massIntensityL = 0;
    double massIntensityH = 0;
    double mobilityL = 0;
    double mobilityH = 0;
    double mobilityIntensityL = 0;
    double mobilityIntensityH = 0;
    double heatmapMassL = 0;
    double heatmapMassH = 0;
    double heatmapMobilityL = 0;
    double heatmapMobilityH = 0;
};

#endif // TRENDREALTIMEDIALOG_H
