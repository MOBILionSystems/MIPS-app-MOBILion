#include "autotrend.h"
#include "ui_autotrend.h"

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
