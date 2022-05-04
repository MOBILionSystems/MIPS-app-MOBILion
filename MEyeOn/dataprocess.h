#ifndef DATAPROCESS_H
#define DATAPROCESS_H

#include <QObject>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QPair>
#include <QVector>

class DataProcess : public QObject
{
    Q_OBJECT
public:
    explicit DataProcess(QObject *parent = nullptr);

    double sumProcess(QJsonArray data, bool newStep);
    double processMsFrameSpectrum(QVector<QPair<double, double>> frameSpectrum);
    double averageFrameData();
    bool isDoublesEqual(double a, double b);
    void reset();
    double usToMz(double x);
    double tofError(double uSecTOF);

signals:

private:
QVector<double> frameProcessedData;

};

#endif // DATAPROCESS_H
