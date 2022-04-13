#ifndef TRENDREALTIMEDIALOG_H
#define TRENDREALTIMEDIALOG_H

#include <QDialog>
#include "dataprocess.h"

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

private:
    DataProcess* dataProcess;
    Ui::TrendRealTimeDialog *ui;
    void initPlot();
    QVector<double> xPoints, yPoints;
    void replot();
    double calibrate(double x);

    double massL = 0;
    double massH = 0;
    double mobilityL = 0;
    double mobilityH = 0;
    double heatmapMassL = 0;
    double heatmapMassH = 0;
    double heatmapMobilityL = 0;
    double heatmapMobilityH = 0;
};

#endif // TRENDREALTIMEDIALOG_H
