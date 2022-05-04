#include "dataprocess.h"
#include <QtMath>

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
    double sum = 0;
    for(int i = 2; i < size; i++){
        if(frameSpectrum.at(i-1).second > frameSpectrum.at(i-2).second && frameSpectrum.at(i-1).second > frameSpectrum.at(i).second){
            if(frameSpectrum.at(i-1).second < 5000){
                i++;
                continue;
            }
            peaks.append(frameSpectrum.at(i-1));
            sum += frameSpectrum.at(i-1).second;
            i++;
        }
    }
    double mean = -1;
    if(peaks.size() > 0)
        mean = sum / peaks.size();

    double sumSquare = 0;
    for(auto p : peaks){
        sumSquare += qPow(p.second, 2);
    }

    double intensityAndBalance = qSqrt(sumSquare);

    if(mean > 0){
        double sumMeanDiffSquare = 0;
        for(auto p : peaks){
            sumMeanDiffSquare += qPow(p.second - mean, 2);
        }
        intensityAndBalance -= qSqrt(sumMeanDiffSquare);
    }else{
        qWarning() << "mean < 0 in frameSpectrumProcess calculation";
    }

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
    double slope = 0.3458234001095313;
    double intercept = 0.09326905279715753;


    double mass = qPow(slope * (x - intercept), 2);
    double error = tofError(x);
    return mass; // mass - mass * error / 1E+6;
}

double DataProcess::tofError(double uSecTOF)
{
    QVector<double> residual = {-229.24922242214916,16.236991249722326,-0.39444579353546055,0.0043708033832713265,-0.000022696140907354508,4.485435626557926e-8};
    double error = 0;
    for(int i = residual.size() - 1; i > -1; i--){
        error = error * uSecTOF + residual[i];
    }
    return error;
}
