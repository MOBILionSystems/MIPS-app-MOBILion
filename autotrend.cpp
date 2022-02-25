#include "autotrend.h"
#include "ui_autotrend.h"
#include <QDebug>

AutoTrend::AutoTrend(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AutoTrend)
{
    ui->setupUi(this);
    _broker = new Broker();
}

AutoTrend::~AutoTrend()
{
    delete ui;
}

void AutoTrend::on_initDigitizerButton_clicked()
{
    qDebug() << "initDigitizer button clicked";
    if(!ui->twTrenVol->text().isEmpty()){
        qDebug() << "AutoTrend TW voltage: " << ui->twTrenVol->text();
    }
    _broker->initDigitizer();
}


void AutoTrend::on_startAcqButton_clicked()
{
    qDebug() << "startAcqButton button clicked";
    if(!ui->twTrenVol->text().isEmpty()){
        qDebug() << "AutoTrend TW voltage: " << ui->twTrenVol->text();
    }
    _broker->startAcquire();
}


void AutoTrend::on_stopAcqButton_clicked()
{
    qDebug() << "stopAcqButton button clicked";
    if(!ui->twTrenVol->text().isEmpty()){
        qDebug() << "AutoTrend TW voltage: " << ui->twTrenVol->text();
    }
    _broker->stopAcquire();
}

