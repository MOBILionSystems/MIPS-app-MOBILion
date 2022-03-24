#ifndef TRENDREALTIMEDIALOG_H
#define TRENDREALTIMEDIALOG_H

#include <QDialog>

namespace Ui {
class TrendRealTimeDialog;
}

class TrendRealTimeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TrendRealTimeDialog(QWidget *parent = nullptr);
    ~TrendRealTimeDialog();

private:
    Ui::TrendRealTimeDialog *ui;
    void initPlot();
};

#endif // TRENDREALTIMEDIALOG_H
