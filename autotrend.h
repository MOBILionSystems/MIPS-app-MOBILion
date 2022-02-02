#ifndef AUTOTREND_H
#define AUTOTREND_H

#include <QWidget>

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
    void on_pushButton_clicked();

private:
    Ui::AutoTrend *ui;
};

#endif // AUTOTREND_H
