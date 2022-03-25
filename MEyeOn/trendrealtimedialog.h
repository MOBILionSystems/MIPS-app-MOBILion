#ifndef TRENDREALTIMEDIALOG_H
#define TRENDREALTIMEDIALOG_H

#include <QDialog>

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
    void msPlot(QVector<double> xVector, QVector<double> yVector);
    void resetPlot();

private:
    Ui::TrendRealTimeDialog *ui;
    void initPlot();
    QVector<double> xPoints, yPoints;
    void replot();
};

#endif // TRENDREALTIMEDIALOG_H
