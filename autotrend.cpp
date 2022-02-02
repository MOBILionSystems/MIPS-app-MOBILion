#include "autotrend.h"
#include "ui_autotrend.h"
#include <QDebug>

AutoTrend::AutoTrend(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AutoTrend)
{
    ui->setupUi(this);
}

AutoTrend::~AutoTrend()
{
    delete ui;
}

void AutoTrend::on_pushButton_clicked()
{
    qDebug() << "autoTrend button clicked";
    if(!ui->twTrenVol->text().isEmpty()){
        qDebug() << "AutoTrend TW voltage: " << ui->twTrenVol->text();
    }
}

