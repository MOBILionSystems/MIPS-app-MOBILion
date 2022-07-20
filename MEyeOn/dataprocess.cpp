#include "dataprocess.h"
#include <QtMath>

const QVector<double> DataProcess::RESIDULE_DEFAULT = {-229.24922242214916,16.236991249722326,-0.39444579353546055,0.0043708033832713265,-0.000022696140907354508,4.485435626557926e-8};
QString DataProcess::msCalibration;

DataProcess::DataProcess(QObject *parent)
    : QObject{parent}
{

}

double DataProcess::sumProcess(QJsonArray dataPoints, bool newStep)
{
    double sum = -1;
    if(newStep){
        sum = averageFrameData();
    }else{
        QVector<QPair<double, double>> frameSpectrum;
        QJsonArray::Iterator i = dataPoints.begin();
        while (i != dataPoints.end()) {
            QJsonArray array = i->toArray();
            frameSpectrum.append(QPair<double, double>(usToMz(array.at(0).toDouble()), array.at(1).toDouble()));
            i++;
        }

        double currentProcessed = processMsFrameSpectrum(frameSpectrum);
        if(currentProcessed > 0)
            frameProcessedData.append(currentProcessed);
    }
    return sum;
}

double DataProcess::processMsFrameSpectrum(QVector<QPair<double, double>> frameSpectrum)
{
    if(frameSpectrum.isEmpty())
        return -1;
    int size = frameSpectrum.size();
    QVector<QPair<double,double>> peaks;
    for(int i = 2; i < size; i++){
        if(frameSpectrum.at(i-1).second > frameSpectrum.at(i-2).second && frameSpectrum.at(i-1).second > frameSpectrum.at(i).second){
            peaks.append(frameSpectrum.at(i-1));
            i++;
        }
    }

    double sumSquare = 0;
    for(auto p : peaks){
        sumSquare += qPow(p.second, 2);
    }

    double intensityAndBalance = qSqrt(sumSquare);

    //qDebug() << peaks;

    return intensityAndBalance;
}

double DataProcess::averageFrameData()
{
    double sum = 0;
    if(frameProcessedData.isEmpty())
        return 0;

    for(auto &d : frameProcessedData)
        sum += d;

    return sum / frameProcessedData.size();
}

bool DataProcess::isDoublesEqual(double a, double b)
{
    return qAbs(a - b) < 0.01;
}

void DataProcess::reset()
{
    frameProcessedData.clear();
}

double DataProcess::usToMz(double x)
{
    /*
    * mass = ((t-t0)*k)^2  [has some error]
    * correctedMass = mass - Error(mass)
    */


    double mass = qPow(SLOPE_DEFAULT * (x - INTERCEPT_DEFAULT), 2);
    double error = tofError(x);
    return mass - mass * error / 1E+6;
}

double DataProcess::tofError(double uSecTOF)
{
    double error = 0;
    for(int i = RESIDULE_DEFAULT.size() - 1; i > -1; i--){
        error = error * uSecTOF + RESIDULE_DEFAULT[i];
    }
    return error;
}

QString DataProcess::getResidule()
{
    QStringList residuleSL;
    for(int i = 0; i < RESIDULE_DEFAULT.size(); i++){
        residuleSL.append(QString::number(RESIDULE_DEFAULT[i]));
    }
    return residuleSL.join(',');
}


bool DataProcess::isNonDefaultMsCalibrationAvailable()
{
    return !DataProcess::msCalibration.isEmpty();
}
