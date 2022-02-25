#ifndef AUTOTREND_H
#define AUTOTREND_H

#include <QWidget>
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

private:
    Ui::AutoTrend *ui;
    Broker* _broker;
};

#endif // AUTOTREND_H
