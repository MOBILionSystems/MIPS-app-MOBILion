#include "dataprocess.h"

DataProcess::DataProcess(QObject *parent)
    : QObject{parent}
{

}

double DataProcess::process(QJsonArray dataPoints)
{
//    if(data.value("id").toString() == "STREAM_FRAME"){
//        QJsonObject payload = data.value("payload").toObject();
//        if(payload.value("chartType").toString() == "MASS"){

//        }
//    }
    double sum = 0;
    QJsonArray::Iterator i = dataPoints.begin();
    while (i != dataPoints.end()) {
        sum += i->toArray().at(1).toDouble();
        i++;
    }
    return sum;
}
