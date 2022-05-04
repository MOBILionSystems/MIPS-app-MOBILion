#include "autotrend.h"
#include "ui_autotrend.h"
#include "ui_controlpanel.h"
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
#include <QtMath>
#include <QRandomGenerator>

AutoTrend::AutoTrend(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AutoTrend)
{
    engine = new QScriptEngine(this);
    mips = engine->newQObject(parent);
    engine->globalObject().setProperty("mips", mips);
    trendRealTimeDialog = new TrendRealTimeDialog(this);
    trendSM = new QStateMachine(this);
    buildTrendSM();
    ui->setupUi(this);

    relationModel = new QStringListModel(this);
    leftValueModel = new QStringListModel(this);
    rightValueModel = new QStringListModel(this);

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
    newRelation = dcElectrodes.at(leftIndex) + "=" + dcElectrodes.at(rightIndex);

    QString leftValueString = engine->evaluate(QString("mips.Command(\"%1\")").arg(dcElectrodes.at(leftIndex))).toString().trimmed();
    QString rightValueString = engine->evaluate(QString("mips.Command(\"%1\")").arg(dcElectrodes.at(rightIndex))).toString().trimmed();

    if(leftValueString.isEmpty() || rightValueString.isEmpty()) return;
    if(leftValueString == rightValueString){
        relationList.append(newRelation);
        relationModel->setStringList(relationList);
        return;
    }

    bool ok_left, ok_right;
    double leftValue = leftValueString.toDouble(&ok_left);
    double rightValue = rightValueString.toDouble(&ok_right);

    if(!ok_left || !ok_right) return;

    double diff = leftValue - rightValue;
    if(diff > 0){
        newRelation += QString("+%1").arg(diff);
    }else{
        newRelation += QString("-%1").arg(diff * (-1));
    }
    relationList.append(newRelation);
    relationModel->setStringList(relationList);
}

void AutoTrend::initUI()
{
    relationModel->setStringList(relationList);
    leftValueModel->setStringList(dcElectrodes);
    rightValueModel->setStringList(dcElectrodes);

    ui->leftListView->setModel(leftValueModel);
    ui->rightListView->setModel(rightValueModel);
    ui->dcRadioButton->setChecked(true);
    ui->trendComboBox->addItems(dcElectrodes);
    ui->relationListView->setModel(relationModel);
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
    QString command = QString("mips.Command(\"%1=%2\")").arg(name).arg(value);
    engine->evaluate(command);
    if(name == ui->trendComboBox->currentText()){
        ui->trendCurrentValue->setText(QString::number(value));
    }
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
                }

                if(dcElectrodes.contains(leftString))
                    updateDCBias(leftString, currentValue);

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
        trendRealTimeDialog->show();
        trendRealTimeDialog->resetPlot();
        toStopTrend = false;
        currentStep = trendFrom;
        trendRealTimeDialog->startNewStep(currentStep);
        relationEnabled = ui->relationRatioButton->isChecked();
        fileFolder = QDate::currentDate().toString("yyyyMMdd") + "/" + QTime::currentTime().toString("hhmmss") + trendName + "Trend";
        ui->trendProgressBar->setValue(0);
        emit nextState();
    });
    initState->addTransition(this, &AutoTrend::nextState, updateTrendState);

    connect(updateTrendState, &QState::entered, this, [=](){
        updateDCBias(trendName, currentStep);

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

    connect(startAcqState, &QState::entered, this, [=](){
        _streamerClient->resetFrameIndex();
        _broker->startAcquire(fileFolder + "/" + trendName + QString::number(currentStep).remove('.') + ".mbi"); emit nextState();
    });
    startAcqState->addTransition(this, &AutoTrend::nextState, waitDuringAcqState);

    connect(waitDuringAcqState, &QState::entered, this, [=](){QTimer::singleShot(stepDuration, this, [=](){emit nextState();});});
    waitDuringAcqState->addTransition(this, &AutoTrend::nextState, stopAcqState);

    connect(stopAcqState, &QState::entered, this, [=](){_broker->stopAcquire(); QTimer::singleShot(3000, this, [=](){emit nextState();});}); // add delay for wifi communication and data processing
    stopAcqState->addTransition(this, &AutoTrend::nextState, nextStepState);

    connect(nextStepState, &QState::entered, this, [=](){
        currentStep += trendStepSize;
        trendRealTimeDialog->startNewStep(currentStep);
        if(abs(trendTo - trendFrom) > 0){
            ui->trendProgressBar->setValue( (100 * abs(currentStep - trendFrom)) / (abs(trendTo - trendFrom) + abs(trendStepSize)));
        }
        if(currentStep <= trendTo && !toStopTrend){
            emit nextState();
        }else{
            emit doneAllStates();
        }
    });
    nextStepState->addTransition(this, &AutoTrend::nextState, updateTrendState);
    nextStepState->addTransition(this, &AutoTrend::doneAllStates, finishState);

    connect(trendSM, &QStateMachine::finished, this, [=](){trendRealTimeDialog->wrapLastStep(); ui->trendProgressBar->setValue(100);});
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

        connectStreamer();
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
        sbcIp->setStyleSheet("QLineEdit { background: rgb(255, 0, 0);}");
        setupBroker(false);
    } else {
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


void AutoTrend::connectStreamer()
{
    _streamerClient = new StreamerClient(this);
    connect(_streamerClient, &StreamerClient::messageReady, this, &AutoTrend::onMessageReady);
    _streamerClient->connectTo(_sbcIpAddress + ":" + _streamerPort);
    QTimer::singleShot(1000, [=](){_streamerClient->request("");});
}


void AutoTrend::onMessageReady(QString message)
{
    QJsonDocument document = QJsonDocument::fromJson(message.toLocal8Bit());
    QJsonObject object = document.object();
    if(object.value("id").toString() == "STREAM_FRAME"){
        QJsonObject payload = object.value("payload").toObject();
        if(payload.value("chartType").toString() == "MASS"){
            QJsonArray dataPointsArray = payload.value("dataPoints").toArray();
            qDebug() << dataPointsArray;
            trendRealTimeDialog->msPlot(dataPointsArray);
        }else if(payload.value("chartType").toString() == "MOBILITY"){
            QJsonArray dataPointsArray = payload.value("dataPoints").toArray();
            trendRealTimeDialog->mobilityPlot(dataPointsArray);
        }else if(payload.value("chartType").toString() == "HEATMAP"){
            QJsonArray dataPointsArray = payload.value("dataPoints").toArray();
            trendRealTimeDialog->heatmapPlot(dataPointsArray);
        }
    }else if(object.value("id").toString() == "SPECTRUM_RANGES"){
        QJsonObject payload = object.value("payload").toObject();
        trendRealTimeDialog->setRange(payload);
    }
}


void AutoTrend::on_pushButton_clicked()
{
    int start = 1;
    for(auto &v : dcElectrodes){
        engine->evaluate(QString("mips.Command(\"%1=%2\")").arg(v).arg(start + QRandomGenerator::global()->generateDouble()));
        start++;
    }
}



void AutoTrend::on_dcRadioButton_toggled(bool checked)
{
    if(checked){
        ui->relationRatioButton->setEnabled(true);
        ui->trendComboBox->clear();
        ui->trendComboBox->addItems(dcElectrodes);
    }else{
        ui->relationRatioButton->setDisabled(true);
        ui->relationRatioButton->setChecked(false);
    }
}


void AutoTrend::on_rfRadioButton_toggled(bool checked)
{
    if(checked){
        ui->trendComboBox->clear();
        ui->trendComboBox->addItems(rfElectrodes);
    }
}


void AutoTrend::on_twRadioButton_3_toggled(bool checked)
{
    if(checked){
        ui->trendComboBox->clear();
        ui->trendComboBox->addItems(twElectrodes);
    }
}


void AutoTrend::on_trendComboBox_currentTextChanged(const QString &arg1)
{
    if(arg1.trimmed().isEmpty()){
        ui->trendCurrentValue->setText("None");
    }else{
        qDebug() << QString("mips.Command(\"%1\")").arg(arg1);
        QString v = engine->evaluate(QString("mips.Command(\"%1\")").arg(arg1)).toString().trimmed();
        qDebug() << v;
        if(v.isEmpty() || v == "?"){
            ui->trendCurrentValue->setText("None");
        }else{
            ui->trendCurrentValue->setText(v);
        }
    }
}


void AutoTrend::on_pushButton_2_clicked()
{
    connectStreamer();
    trendRealTimeDialog->show();
    trendRealTimeDialog->resetPlot();
    toStopTrend = false;
    currentStep = trendFrom;
    trendRealTimeDialog->startNewStep(currentStep);
}


void AutoTrend::on_pushButton_3_clicked()
{
    trendRealTimeDialog->wrapLastStep();
}

