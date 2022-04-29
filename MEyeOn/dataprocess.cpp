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
        sum = frameSpectrumProcess();
        frameSpectrum.clear();
        QJsonArray::Iterator i = dataPoints.begin();
        while (i != dataPoints.end()) {
            QJsonArray array = i->toArray();
            frameSpectrum.append(QPair<double, double>(array.at(0).toDouble(), array.at(1).toDouble()));
            i++;
        }
    }else{
        int vectorIndex = 0;
        QJsonArray::Iterator it = dataPoints.begin();
        while(vectorIndex < frameSpectrum.size() && it != dataPoints.end()){
            if(isDoublesEqual(it->toArray().at(0).toDouble(), frameSpectrum.at(vectorIndex).first)){
                frameSpectrum[vectorIndex].second += it->toArray().at(1).toDouble();
                vectorIndex++;
                it++;
            }else if(it->toArray().at(0).toDouble() < frameSpectrum.at(vectorIndex).first){
                qWarning() << QString("datapoints %1 and frameSpectrum %2 not matching").arg(it->toArray().at(0).toDouble()).arg(frameSpectrum.at(vectorIndex).first);
                frameSpectrum.insert(vectorIndex, QPair<double, double>(it->toArray().at(0).toDouble(), it->toArray().at(1).toDouble()));
                vectorIndex++;
                it++;
            }else{
                qWarning() << QString("datapoints %1 and frameSpectrum %2 not matching").arg(it->toArray().at(0).toDouble()).arg(frameSpectrum.at(vectorIndex).first);
                vectorIndex++;
            }
        }
        while (it != dataPoints.end()) {
            frameSpectrum.append(QPair<double, double>(it->toArray().at(0).toDouble(), it->toArray().at(1).toDouble()));
            it++;
        }
    }
    return sum;
}

double DataProcess::frameSpectrumProcess()
{
    if(frameSpectrum.isEmpty())
        return -1;
    int size = frameSpectrum.size();
    QVector<QPair<double,double>> peaks;
    double sum = 0;
    for(int i = 2; i < size; i++){
        if(frameSpectrum.at(i-1).second > frameSpectrum.at(i-2).second && frameSpectrum.at(i-1).second > frameSpectrum.at(i).second){
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

    return intensityAndBalance;
}

bool DataProcess::isDoublesEqual(double a, double b)
{
    return qAbs(a - b) < 0.01;
}

void DataProcess::reset()
{
    frameSpectrum.clear();
}
