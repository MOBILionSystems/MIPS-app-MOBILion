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
    double frameSpectrumProcess();
    bool isDoublesEqual(double a, double b);
    void reset();

signals:

private:
QVector<QPair<double, double>> frameSpectrum;

};

#endif // DATAPROCESS_H
