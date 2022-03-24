#include "autotrend.h"
#include "ui_autotrend.h"
#include <QDebug>
#include <QQueue>
#include <QPair>
#include <QFinalState>
#include <QTimer>
#include <QDate>
#include <QTime>
#include <cstdlib>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

AutoTrend::AutoTrend(Ui::MIPS *w, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AutoTrend)
{
    trendSM = new QStateMachine(this);
    buildTrendSM();
    mipsui = w;
    ui->setupUi(this);

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

    electrodeLabelValueMap.insert("label", QPair<QString, int>("FunnelIn", 10));
    electrodeLabelValueMap.insert("label_2", QPair<QString, int>("FunnelOut", 10));
    electrodeLabelValueMap.insert("label_3", QPair<QString, int>("FunnelCL", 10));
    electrodeLabelValueMap.insert("label_4", QPair<QString, int>("SLIMBias", 10));
    electrodeLabelValueMap.insert("label_5", QPair<QString, int>("ExitCL", 10));
    electrodeLabelValueMap.insert("label_6", QPair<QString, int>("QuadBias", 10));

    initUI();

}

AutoTrend::~AutoTrend()
{
    delete ui;
    if(trendSM->isRunning()){
        qWarning() << "Trend state machine is still running when AutoTrend is destroied.";
    }
}

//void AutoTrend::on_initDigitizerButton_clicked()
//{
//    if(!_broker){
//        QMessageBox::warning(this, "No SBC", "Please connect to SBC first.");
//        return;
//    }
//    _broker->initDigitizer();
//}


//void AutoTrend::on_startAcqButton_clicked()
//{
//    if(!_broker){
//        QMessageBox::warning(this, "No SBC", "Please connect to SBC first.");
//        return;
//    }
//    _broker->startAcquire("20220228/b.mbi");
//}


//void AutoTrend::on_stopAcqButton_clicked()
//{
//    if(!_broker){
//        QMessageBox::warning(this, "No SBC", "Please connect to SBC first.");
//        return;
//    }
//    _broker->stopAcquire();
//}


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

    ui->constInRelation->setValidator(new QDoubleValidator(0, 10000, 3, this));
    ui->trendFrom->setValidator(new QIntValidator(-10000, 10000, this));
    ui->trendTo->setValidator(new QIntValidator(-10000, 10000, this));
    ui->trendStepSize->setValidator(new QIntValidator(-10000, 10000, this));
    ui->trendStepDuration->setValidator(new QIntValidator(0, 10000, this));

    connect(ui->sbcIPEdit, &QLineEdit::textChanged, this, [=](){
        QLineEdit *sbcIp = ui->sbcIPEdit;
        sbcIp->setStyleSheet("QLineEdit { background: rgb(255, 255, 255);}");
    });

}

void AutoTrend::updateDCBias(QString name, double value)
{
    QLineEdit *leDCB = mipsui->gbDCbias1->findChild<QLineEdit *>(name);
    leDCB->setText(QString::number(value));
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
            if(rightString.contains(currentName)){
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
                    QMessageBox::warning(this, "Loop in RelationShip", "Loop is detected in relationship and autotrend is stoped");
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
        toStopTrend = false;
        currentStep = trendFrom;
        relationEnabled = ui->relationRatioButton->isChecked();
        fileFolder = QDate::currentDate().toString("yyyyMMdd") + "/" + QTime::currentTime().toString("hhmmss") + trendName + "Trend";
        ui->trendProgressBar->setValue(0);
        mipsui->chkPowerEnable->setChecked(true);
        emit nextState();
    });
    initState->addTransition(this, &AutoTrend::nextState, updateTrendState);

    connect(updateTrendState, &QState::entered, this, [=](){
        if(electrodeChannelHash.contains(trendName))
            updateDCBias(electrodeChannelHash.value(trendName), currentStep);

        emit nextState();
    });
    updateTrendState->addTransition(this, &AutoTrend::nextState, applyRelationState);

    connect(applyRelationState, &QState::entered, this, [=](){
        if(relationEnabled){
            if(!applyRelations(trendName, currentStep)){
                emit abortTrend();
                return;
            }
        }
        emit nextState();
    });
    applyRelationState->addTransition(this, &AutoTrend::nextState, waitBeforeAcqState);
    applyRelationState->addTransition(this, &AutoTrend::abortTrend, finishState);

    connect(waitBeforeAcqState, &QState::entered, this, [=](){QTimer::singleShot(2000, this, [=](){emit nextState();});});
    waitBeforeAcqState->addTransition(this, &AutoTrend::nextState, startAcqState);

    connect(startAcqState, &QState::entered, this, [=](){_broker->startAcquire(fileFolder + "/" + trendName + QString::number(currentStep).remove('.') + ".mbi"); emit nextState();});
    startAcqState->addTransition(this, &AutoTrend::nextState, waitDuringAcqState);

    connect(waitDuringAcqState, &QState::entered, this, [=](){QTimer::singleShot(stepDuration, this, [=](){emit nextState();});});
    waitDuringAcqState->addTransition(this, &AutoTrend::nextState, stopAcqState);

    connect(stopAcqState, &QState::entered, this, [=](){_broker->stopAcquire(); emit nextState();});
    stopAcqState->addTransition(this, &AutoTrend::nextState, nextStepState);

    connect(nextStepState, &QState::entered, this, [=](){
        currentStep += trendStepSize;
        if(abs(trendTo - trendFrom) > 0)
            ui->trendProgressBar->setValue( (100 * abs(currentStep - trendFrom)) / (abs(trendTo - trendFrom) + abs(trendStepSize)));
        currentStep <= trendTo && !toStopTrend ? emit nextState() : emit doneAllStates();});
    nextStepState->addTransition(this, &AutoTrend::nextState, updateTrendState);
    nextStepState->addTransition(this, &AutoTrend::doneAllStates, finishState);

    connect(trendSM, &QStateMachine::finished, this, [=](){ui->trendProgressBar->setValue(100);});
}

void AutoTrend::setupBroker(bool connected)
{
    if(_broker && trendSM->isRunning()){
        QMessageBox::warning(this, "Warning", "Current SBC is running for Acquisition.");
        return;
    }
    if(connected){
        delete _broker;
        _broker = new Broker(_sbcIpAddress, this);
        _broker->initDigitizer();
    }else{
        _broker = nullptr;
    }
}


void AutoTrend::on_runTrendButton_clicked()
{
    if(!_broker){
        QMessageBox::warning(this, "No SBC", "Please connect to SBC first.");
        return;
    }
    if( ui->trendComboBox->currentText().isEmpty() ||
            ui->trendFrom->text().isEmpty() ||
            ui->trendTo->text().isEmpty() ||
            ui->trendStepSize->text().isEmpty() ||
            ui->trendStepDuration->text().isEmpty()){
        qWarning() << "Invalid setting for autotrend.";
        QMessageBox::warning(this, "Autotrend Failed", "Invalid settings for autotrend.");
        return;
    }

    trendName = ui->trendComboBox->currentText();
    trendFrom = ui->trendFrom->text().toInt();
    trendTo = ui->trendTo->text().toInt();
    trendStepSize = ui->trendStepSize->text().toInt();
    stepDuration = ui->trendStepDuration->text().toInt();
    trendSM->start();
}


void AutoTrend::on_stopTrendButton_clicked()
{
    if(!_broker){
        QMessageBox::warning(this, "No SBC", "Please connect to SBC first.");
        return;
    }
    toStopTrend = true;
}


void AutoTrend::on_initDCBiasButton_clicked()
{
    QMap<QString, QPair<QString, int>>::iterator i;
    for(i = electrodeLabelValueMap.begin(); i != electrodeLabelValueMap.end(); i++){
        QLabel* firstLabel = mipsui->gbDCbias1->findChild<QLabel *>(i.key());
        firstLabel->setText(i.value().first);

        QLineEdit *leDCB = mipsui->gbDCbias1->findChild<QLineEdit *>(electrodeChannelHash.value(i.value().first));
        leDCB->setText(QString::number(i.value().second));
        leDCB->setModified(true);
        emit leDCB->editingFinished();
    }
}

// https://www.qtcentre.org/threads/23723-check-IPAddress-existence
void AutoTrend::on_testSBCButton_clicked()
{
    setupBroker(false);

    _sbcIpAddress = ui->sbcIPEdit->text();
    QProcess *ping = new QProcess(this);
    connect(ping, SIGNAL(readyReadStandardOutput()), SLOT(readResult()));
    ping->start("ping.exe", QStringList() << ui->sbcIPEdit->text());
}

void AutoTrend::readResult()
{
    QProcess *ping = qobject_cast<QProcess *>(sender());
    if (!ping)
        return;
    QString res = ping->readAllStandardOutput();
    if (!res.contains('%'))
        return;

    const int percentLost = res.section('(', -1).section('%', 0, 0).toInt();
    QLineEdit *sbcIp = ui->sbcIPEdit;

    if (res.contains("unreachable") || percentLost == 100) {
        qDebug() << "host not found.";
        sbcIp->setStyleSheet("QLineEdit { background: rgb(255, 0, 0);}");
        setupBroker(false);
    } else {
        qDebug() << "host found." << res;
        sbcIp->setStyleSheet("QLineEdit { background: rgb(0, 255, 0);}");
        setupBroker(true);
    }
}


void AutoTrend::on_loadRelationButton_clicked()
{
    QString filter = "Text File (*.txt);; All File (*.*)";
    QString file_name = QFileDialog::getOpenFileName(this, "Open file", QDir::homePath(), filter);
    QFile file(file_name);

    if(!file.open(QFile::ReadOnly | QFile::Text)){
        return;
    }

    relationList.clear();
    relationList = QString(file.readAll()).split('\n');
    relationModel->setStringList(relationList);
    file.close();
}


void AutoTrend::on_saveRelationButton_clicked()
{
    QString filter = "Text File (*.txt);; All File (*.*)";
    QString file_name = QFileDialog::getSaveFileName(this, "Save file", QDir::homePath(), filter);
    QFile file(file_name);

    if(!file.open(QFile::WriteOnly | QFile::Text)){
        return;
    }

    QTextStream out(&file);
    out << relationList.join('\n');
    file.close();
}


void AutoTrend::on_pushButton_clicked()
{
    _streamerClient = new StreamerClient(this);
    _streamerClient->connectTo();
}




void AutoTrend::on_requestButton_clicked()
{
    _streamerClient->request("");
}


void AutoTrend::on_plotButton_clicked()
{
    trendRealTimeDialog = new TrendRealTimeDialog(this);
    trendRealTimeDialog->show();
}

