#include "autotrend.h"
#include "ui_autotrend.h"
#include <QDebug>

AutoTrend::AutoTrend(Ui::MIPS *w, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AutoTrend)
{
    mipsui = w;
    ui->setupUi(this);
    _broker = new Broker(this);

    relationModel = new QStringListModel(this);
    leftValueModel = new QStringListModel(this);
    rightValueModel = new QStringListModel(this);
    mathOperatorsModel = new QStringListModel(this);

    initUI();
}

AutoTrend::~AutoTrend()
{
    delete ui;
}

void AutoTrend::on_initDigitizerButton_clicked()
{
    _broker->initDigitizer();
}


void AutoTrend::on_startAcqButton_clicked()
{
    _broker->startAcquire();
}


void AutoTrend::on_stopAcqButton_clicked()
{
    _broker->stopAcquire();
}


void AutoTrend::on_removeRelationButton_clicked()
{
    relationModel->removeRows(ui->relationListView->currentIndex().row(), 1);
    relationList = relationModel->stringList();
}


void AutoTrend::on_addRelationButton_clicked()
{
    int leftIndex = ui->leftListView->currentIndex().row();
    int rightIndex = ui->rightListView->currentIndex().row();
    if( leftIndex < 0 || rightIndex < 0 || leftIndex == rightIndex) return;

    QString newRelation;
    newRelation = electrodes.at(leftIndex) + "=" + electrodes.at(rightIndex);

    int mathOperatorIndex = ui->mathListView->currentIndex().row();
    QString constString = ui->constInRelation->text();
    if(mathOperatorIndex > 0 && !constString.isEmpty()){
        newRelation += mathOperators.at(mathOperatorIndex) + constString;
    }
    relationList.append(newRelation);
    relationModel->setStringList(relationList);
}

void AutoTrend::initUI()
{
    relationModel->setStringList(relationList);
    leftValueModel->setStringList(electrodes);
    rightValueModel->setStringList(electrodes);
    mathOperatorsModel->setStringList(mathOperators);

    ui->leftListView->setModel(leftValueModel);
    ui->rightListView->setModel(rightValueModel);
    ui->mathListView->setModel(mathOperatorsModel);
    ui->trendComboBox->addItems(electrodes);
    ui->relationListView->setModel(relationModel);

    ui->constInRelation->setValidator(new QDoubleValidator(0, 1000, 3, this));
    ui->trendFrom->setValidator(new QIntValidator(0, 1000, this));
    ui->trendTo->setValidator(new QIntValidator(0, 1000, this));
    ui->trendStepSize->setValidator(new QIntValidator(0, 10000, this));
    ui->trendStepDuration->setValidator(new QIntValidator(0, 10000, this));
}


void AutoTrend::on_pushButton_6_clicked()
{
    QLineEdit *leDCB = mipsui->gbDCbias1->findChild<QLineEdit *>("leSDCB_1");
    leDCB->setText(QString::number(1234));
    leDCB->setModified(true);
    emit leDCB->editingFinished();
}

