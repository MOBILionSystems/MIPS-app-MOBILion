#include "dataprocess.h"

DataProcess::DataProcess(QObject *parent)
    : QObject{parent}
{

}

double DataProcess::sumProcess(QJsonArray dataPoints)
{
    double sum = 0;
    QJsonArray::Iterator i = dataPoints.begin();
    while (i != dataPoints.end()) {
        sum += i->toArray().at(1).toDouble();
        i++;
    }
    return sum;
}
