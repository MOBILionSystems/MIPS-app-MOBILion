#ifndef AUTOTREND_H
#define AUTOTREND_H

#include <QWidget>
#include <QStringListModel>
#include <QStateMachine>
#include "MEyeOn/Broker.h"
#include "MEyeOn/streamerclient.h"
#include "MEyeOn/trendrealtimedialog.h"
#include "MEyeOn/MBI/mbifile.h"
#include "MEyeOn/dataprocess.h"
#include "MEyeOn/qtofaddonclient.h"

#include "ui_mips.h"
#include "mips.h"
#include <QHash>
#include <QHostInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QScriptEngine>
#include <QTimer>



namespace Ui {
class AutoTrend;
}

class AutoTrend : public QWidget
{
    Q_OBJECT

public:
    explicit AutoTrend(QWidget *parent = nullptr);
    ~AutoTrend();

    void updateScriptValue(QString v);

signals:
    void nextAcqState();        // signal for autotrend
    void nextConnectState();
    void nextMafState();        // signal for maf
    void doneMafState();
    void failMafState();
    void sbcFailed();
    void goFinishState();
    void doneAllStates();
    void abortTrend();
    void nextForSingleShot();   // signal for nonMAF signal shot
    void runCommand(QString s);
    void sendMess(QString toWhom, QString message);

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

    //void on_initvoltage_clicked();

    void on_dcRadioButton_toggled(bool checked);

    void on_rfRadioButton_toggled(bool checked);

    void on_twRadioButton_3_toggled(bool checked);

    void on_trendComboBox_currentTextChanged(const QString &arg1);

    void on_singleShotButton_clicked();

    void on_loadMsCalibrationButton_clicked();

    void onAcqAckNack(AckNack response);

    void onConfigureAckNack(AckNack response);

    void applyMafCeVoltage();

    void runTimingTable();

    void onCeVoltageReceived(bool success);

    void onTBTimerTimeout();

    void on_rtbCheckBox_stateChanged(int checkState);

    void on_rtbScansLineEdit_editingFinished();


private:
   // if Autotrend is a dialog, scriptEngine will not work. The parent of AutoTrend need to be configuration
   // panel. So need to use signal and slot like runScript()
    QScriptValue mips; // Actually configuration panel instead of mips
    QScriptEngine *engine;
    TrendRealTimeDialog* trendRealTimeDialog{};
    StreamerClient* _streamerClient{nullptr};
    QString _sbcIpAddress;
    QString _streamerPort = "4001";
    QStateMachine* trendSM;
    QStateMachine* sbcConnectSM;
    Ui::AutoTrend *ui;
    QProcess *ping;
    Broker* _broker{nullptr};
    QStringListModel* relationModel;
    QStringListModel* leftValueModel;
    QStringListModel* rightValueModel;
    QStringList dcElectrodes = {"Funnel.NIF IN", "Funnel.NIF OUT", "Funnel.NIF CL", "Funnel.SLIMvolBias"};
    QStringList rfElectrodes = {"SLIM.SLIM Top.Drive", "SLIM.SLIM Bottom.Drive"};
    QStringList twElectrodes = {"Seperation TW.Frequency", "Seperation TW.Amplitude"};
    QStringList polarities = {"Positive", "Negative"};
    QStringList ranges = {"None",
                          "6545(1700m/z)", "6545(3200m/z)", "6545(10000m/z)",
                          "6545XT(1700m/z)", "6545XT(3200m/z)", "6545XT(10000m/z)", "6545XT(30000m/z)",
                          "6546(1700m/z)", "6546(3200m/z)", "6546(10000m/z)",
                          "6230(1700m/z)", "6230(3200m/z)", "6230(20000m/z)"};
    QStringList recordSize = {"320000",
                             "232992", "320000", "569984",
                             "232992", "320000", "573984", "994976",
                             "206976", "284992", "508000",
                             "320000", "320000", "320000"}; // Todo update later
    QVector<double> periods_us = {163.5,
                              120, 163.5, 288.5,
                              120, 163.5, 290.5, 501,
                              107, 146, 257.5,
                              163.5, 163.5, 163.5}; // Todo update later
    QStringList relationList;
    QString fileFolder;
    QtofAddonClient* _qtofClient;
    QString cpResponse;


    int _mafTotalCycle = 1;
    int _mafCurrentCycle = 0;
    bool _maf = false;
    int _ceVol = 30;
    bool singleShot = false;
    QString trendName;
    int trendFrom = 0;
    int trendTo = 0;
    int trendStepSize = 1;
    int currentStep = 0;
    bool relationEnabled = false;
    bool toStopTrend = false;

    void initUI();
    void updateDCBias(QString name, double value);
    bool applyRelations(QString startWith, double startValue);
    void buildTrendSM();
    void buildSbcConnectSM();
    QTimer* tbMonitorTimer; // Timing table monitor timer

    bool test1s = true;
    void updateTrendCurrentValue(QString s = "");
};

#endif // AUTOTREND_H
