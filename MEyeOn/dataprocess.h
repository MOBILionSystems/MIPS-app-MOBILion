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
    double scanCountToMS(double s);
    double tofError(double uSecTOF);

    constexpr static double SLOPE_DEFAULT = 0.3458234001095313;
    constexpr static double INTERCEPT_DEFAULT = 0.09326905279715753;
    constexpr static int samplingRate = 2e3; // per us
    static double frm_dt_period_s; // period of trigger from QTOF
    static int recordSize; // for ms range calculation
    static const QVector<double> RESIDULE_DEFAULT;
    static QString getResidule();
    static void setDtPeriod_S(double dt);
    static void setRecordSize(int rs);
    static int getUsRange();

    static bool isNonDefaultMsCalibrationAvailable();
    static QString msCalibration;

signals:

private:
QVector<double> frameProcessedData;


};

#endif // DATAPROCESS_H
