#ifndef AUTOTREND_H
#define AUTOTREND_H

#include <QWidget>
#include <QStringListModel>
#include <QStateMachine>
#include "MEyeOn/Broker.h"
#include "MEyeOn/streamerclient.h"
#include "MEyeOn/trendrealtimedialog.h"

#include "ui_mips.h"
#include "mips.h"
#include <QHash>
#include <QHostInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QScriptEngine>


namespace Ui {
class AutoTrend;
}

class AutoTrend : public QWidget
{
    Q_OBJECT

public:
    explicit AutoTrend(QWidget *parent = nullptr);
    ~AutoTrend();

signals:
    void nextState();
    void doneAllStates();
    void abortTrend();

private slots:
//    void on_initDigitizerButton_clicked();

//    void on_startAcqButton_clicked();

//    void on_stopAcqButton_clicked();

    void on_removeRelationButton_clicked();

    void on_addRelationButton_clicked();

    void on_runTrendButton_clicked();

    void on_stopTrendButton_clicked();

    void on_testSBCButton_clicked();

    void readResult();

    void on_loadRelationButton_clicked();

    void on_saveRelationButton_clicked();

    void connectStreamer();


    void onMessageReady(QString message);

    void on_pushButton_clicked();

private:
    QScriptValue mips; // Actually configuration panel instead of mips
    QScriptEngine *engine;
    TrendRealTimeDialog* trendRealTimeDialog;
    StreamerClient* _streamerClient;
    QString _sbcIpAddress;
    QStateMachine* trendSM;
    Ui::AutoTrend *ui;
    Broker* _broker{nullptr};
    QStringListModel* relationModel;
    QStringListModel* leftValueModel;
    QStringListModel* rightValueModel;
    QStringList electrodes = {"Funnel.NIF IN", "Funnel.NIF OUT", "Funnel.NIF CL", "Funnel.SLIMvolBias"};
    QStringList relationList;
    QString fileFolder;


    QString trendName;
    int trendFrom = 0;
    int trendTo = 0;
    int trendStepSize = 1;
    int stepDuration = 1;
    int currentStep = 0;
    bool relationEnabled = false;
    bool toStopTrend = false;

    int currentParameter = 0;

    void initUI();
    void updateDCBias(QString name, double value);
    bool applyRelations(QString startWith, double startValue);
    void buildTrendSM();

    void setupBroker(bool connected);
};

#endif // AUTOTREND_H
