#ifndef AUTOTREND_H
#define AUTOTREND_H

#include <QWidget>
#include <QStringListModel>
#include "MEyeOn/Broker.h"

namespace Ui {
class AutoTrend;
}

class AutoTrend : public QWidget
{
    Q_OBJECT

public:
    explicit AutoTrend(QWidget *parent = nullptr);
    ~AutoTrend();

private slots:
    void on_initDigitizerButton_clicked();

    void on_startAcqButton_clicked();

    void on_stopAcqButton_clicked();

    void on_removeRelationButton_clicked();

    void on_addRelationButton_clicked();

private:
    Ui::AutoTrend *ui;
    Broker* _broker;
    QStringListModel* relationModel;
    QStringListModel* leftValueModel;
    QStringListModel* rightValueModel;
    QStringListModel* mathOperatorsModel;
    QStringList electrodes = {"FunnelIn", "FunnelOut", "FunnelCL", "SLIMBias", "ExitCL", "QuadBias"};
    QStringList mathOperators = {"None", "+", "-", "*"};
    QStringList relationList;
};

#endif // AUTOTREND_H
