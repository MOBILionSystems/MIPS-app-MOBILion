#ifndef AUTOTREND_H
#define AUTOTREND_H

#include <QWidget>
#include <QStringListModel>
#include <QStateMachine>
#include "MEyeOn/Broker.h"

#include "ui_mips.h"
#include "mips.h"
#include <QHash>


namespace Ui {
class AutoTrend;
}

class AutoTrend : public QWidget
{
    Q_OBJECT

public:
    explicit AutoTrend(Ui::MIPS *w, QWidget *parent = nullptr);
    ~AutoTrend();

signals:
    void nextState();
    void doneAllStates();

private slots:
    void on_initDigitizerButton_clicked();

    void on_startAcqButton_clicked();

    void on_stopAcqButton_clicked();

    void on_removeRelationButton_clicked();

    void on_addRelationButton_clicked();

    void on_runTrendButton_clicked();

    void on_stopTrendButton_clicked();

private:
    QStateMachine* trendSM;
    Ui::MIPS *mipsui;
    Ui::AutoTrend *ui;
    Broker* _broker;
    QStringListModel* relationModel;
    QStringListModel* leftValueModel;
    QStringListModel* rightValueModel;
    QStringListModel* mathOperatorsModel;
    QStringList electrodes = {"FunnelIn", "FunnelOut", "FunnelCL", "SLIMBias", "ExitCL", "QuadBias"};
    QStringList mathOperators = {"None", "+", "-", "*"};
    QStringList relationList;
    QHash<QString, QString> electrodeChannelHash;


    QString trendName;
    int trendFrom = 0;
    int trendTo = 0;
    int trendStepSize = 1;
    int stepDuration = 1;
    int currentStep = 0;
    bool relationEnabled = false;

    void initUI();
    void updateDCBias(QString name, double value);
    bool applyRelations(QString startWith, double startValue);
    void buildTrendSM();
};

#endif // AUTOTREND_H
