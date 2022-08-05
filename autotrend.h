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
    void nextForSingleShot();
    void runScript(QString s);

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

private:
   // if Autotrend is a dialog, scriptEngine will not work. The parent of AutoTrend need to be configuration
   // panel. So need to use signal and slot like runScript()
    QScriptValue mips; // Actually configuration panel instead of mips
    QScriptEngine *engine;
    TrendRealTimeDialog* trendRealTimeDialog;
    StreamerClient* _streamerClient;
    QString _sbcIpAddress;
    QString _streamerPort = "4001";
    QStateMachine* trendSM;
    Ui::AutoTrend *ui;
    Broker* _broker{nullptr};
    QStringListModel* relationModel;
    QStringListModel* leftValueModel;
    QStringListModel* rightValueModel;
    QStringList dcElectrodes = {"Funnel.NIF IN", "Funnel.NIF OUT", "Funnel.NIF CL", "Funnel.SLIMvolBias"};
    QStringList rfElectrodes = {"SLIM.SLIM Top.Drive", "SLIM.SLIM Bottom.Drive"};
    QStringList twElectrodes = {"SLIM.Seperation.Frequency", "SLIM.Seperation.Amplitude"};
    QStringList relationList;
    QString fileFolder;


    bool singleShot = false;
    QString trendName;
    int trendFrom = 0;
    int trendTo = 0;
    int trendStepSize = 1;
    int stepDuration = 1;
    int currentStep = 0;
    bool relationEnabled = false;
    bool toStopTrend = false;

    void initUI();
    void updateDCBias(QString name, double value);
    bool applyRelations(QString startWith, double startValue);
    void buildTrendSM();

    void setupBroker(bool connected);
};

#endif // AUTOTREND_H
