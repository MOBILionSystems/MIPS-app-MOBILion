#include "autotrend.h"
#include "ui_autotrend.h"
#include <QDebug>
#include <QQueue>
#include <QPair>
#include <QFinalState>
#include <QTimer>

AutoTrend::AutoTrend(Ui::MIPS *w, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AutoTrend)
{
    trendSM = new QStateMachine(this);
    buildTrendSM();
    mipsui = w;
    ui->setupUi(this);
    _broker = new Broker(this);

    relationModel = new QStringListModel(this);
    leftValueModel = new QStringListModel(this);
    rightValueModel = new QStringListModel(this);
    mathOperatorsModel = new QStringListModel(this);

    electrodeChannelHash.insert("FunnelIn", "leSDCB_1");
    electrodeChannelHash.insert("FunnelOut", "leSDCB_2");
    electrodeChannelHash.insert("FunnelCL", "leSDCB_3");
    electrodeChannelHash.insert("SLIMBias", "leSDCB_4");
    electrodeChannelHash.insert("ExitCL", "leSDCB_5");
    electrodeChannelHash.insert("QuadBias", "leSDCB_6");

    initUI();
}

AutoTrend::~AutoTrend()
{
    delete ui;
    if(trendSM->isRunning()){
        qWarning() << "Trend state machine is still running when AutoTrend is destroied.";
    }
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

void AutoTrend::updateDCBias(QString name, double value)
{
    QLineEdit *leDCB = mipsui->gbDCbias1->findChild<QLineEdit *>(name); // name "leSDCB_1"
    leDCB->setText(QString::number(value)); // value 1234
    leDCB->setModified(true);
    emit leDCB->editingFinished();
}

bool AutoTrend::applyRelations(QString startWith, double startValue)
{
    int appliedRelations = 0;
    int totalRelationsInList = relationList.size();
    QString currentName;
    double currentValue = 0;
    QQueue<QPair<QString, double>> calQueue;
    calQueue.enqueue(QPair<QString, double>(startWith, startValue));
    while(calQueue.size() > 0){
        QPair<QString, double> current = calQueue.dequeue();
        currentName = current.first;
        currentValue = current.second;
        for(auto& current : relationList){
            QStringList mathList = current.split("=", Qt::SkipEmptyParts);
            if(mathList.size() != 2) continue;
            QString leftString = mathList.at(0);
            QString rightString = mathList.at(1);
            if(rightString.contains(currentName)){ // Assume NameX=NameY+3
                qDebug() << "applying " << current;
                if(rightString.contains('+')){
                    QStringList rightList = rightString.split('+');
                    if(rightList.size() == 2){
                        currentValue += rightList.at(1).toDouble();
                    }
                }else if(rightString.contains('-')){
                    QStringList rightList = rightString.split('-');
                    if(rightList.size() == 2){
                        currentValue -= rightList.at(1).toDouble();
                    }
                }else if(rightString.contains('*')){
                    QStringList rightList = rightString.split('-');
                    if(rightList.size() == 2){
                        currentValue *= rightList.at(1).toDouble();
                    }
                }

                if(electrodeChannelHash.contains(leftString))
                    updateDCBias(electrodeChannelHash.value(leftString), currentValue);

                calQueue.enqueue(QPair<QString, double>(leftString, currentValue));
                appliedRelations++;
                if(appliedRelations > totalRelationsInList){
                    qWarning() << "Loop detected in Relations";
                    return false;
                }
            }
        }
    }
    if(appliedRelations < totalRelationsInList){
        qWarning() << "Some Relations were not applied";
        return true;
    }
    return true;
}

void AutoTrend::buildTrendSM()
{
    QState* initState = new QState(trendSM);
    QState* updateTrendState = new QState(trendSM);
    QState* applyRelationState = new QState(trendSM);
    QState* waitBeforeAcqState = new QState(trendSM);
    QState* startAcqState = new QState(trendSM);
    QState* waitDuringAcqState = new QState(trendSM);
    QState* stopAcqState = new QState(trendSM);
    QState* nextStepState = new QState(trendSM);
    QFinalState* finishState = new QFinalState(trendSM);

    trendSM->setInitialState(initState);
    connect(initState, &QState::entered, this, [=](){
        trendName = ui->trendComboBox->currentText();
        trendFrom = ui->trendFrom->text().toInt();
        trendTo = ui->trendTo->text().toInt();
        trendStepSize = ui->trendStepSize->text().toInt();
        stepDuration = ui->trendStepDuration->text().toInt();
        currentStep = trendFrom;
        relationEnabled = ui->relationRatioButton->isChecked();
        emit nextState();
    });
    initState->addTransition(this, &AutoTrend::nextState, updateTrendState);

    connect(updateTrendState, &QState::entered, this, [=](){
        if(electrodeChannelHash.contains(trendName))
            updateDCBias(electrodeChannelHash.value(trendName), currentStep);

        emit nextState();
    });
    updateTrendState->addTransition(this, &AutoTrend::nextState, applyRelationState);

    connect(applyRelationState, &QState::entered, this, [=](){if(relationEnabled) applyRelations(trendName, currentStep); emit nextState();});
    applyRelationState->addTransition(this, &AutoTrend::nextState, waitBeforeAcqState);

    connect(waitBeforeAcqState, &QState::entered, this, [=](){QTimer::singleShot(2000, this, [=](){emit nextState();});});
    waitBeforeAcqState->addTransition(this, &AutoTrend::nextState, startAcqState);

    connect(startAcqState, &QState::entered, this, [=](){_broker->startAcquire(); emit nextState();});
    startAcqState->addTransition(this, &AutoTrend::nextState, waitDuringAcqState);

    connect(waitDuringAcqState, &QState::entered, this, [=](){QTimer::singleShot(stepDuration, this, [=](){emit nextState();});});
    waitDuringAcqState->addTransition(this, &AutoTrend::nextState, stopAcqState);

    connect(stopAcqState, &QState::entered, this, [=](){_broker->stopAcquire(); emit nextState();});
    stopAcqState->addTransition(this, &AutoTrend::nextState, nextStepState);

    connect(nextStepState, &QState::entered, this, [=](){currentStep += trendStepSize; currentStep <= trendTo ? emit nextState() : emit doneAllStates();});
    nextStepState->addTransition(this, &AutoTrend::nextState, updateTrendState);
    nextStepState->addTransition(this, &AutoTrend::doneAllStates, finishState);

    connect(trendSM, &QStateMachine::finished, this, [](){qDebug() << "Trend State Machine finished";});
}


void AutoTrend::on_runTrendButton_clicked()
{
    trendSM->start();
}


void AutoTrend::on_stopTrendButton_clicked()
{

}

