#ifndef HELP_H
#define HELP_H

#include <QtCore/QtGlobal>
#include <QtSerialPort/QSerialPort>
#include <QStatusBar>
#include <QMessageBox>
#include <QObject>
#include <QTime>
#include <QApplication>
#include <QtNetwork/QTcpSocket>
#include <QDialog>
#include <QPlainTextEdit>
#include <QFontMetrics>

namespace Ui {
class Help;
}

class Help : public QDialog
{
    Q_OBJECT

public:
    explicit Help(QWidget *parent = 0);
    ~Help();
    virtual void resizeEvent(QResizeEvent* event);
    void LoadHelpText(QString FileName);
    void LoadStr(QString DisplayText);
    void SetTitle(QString title);

private:
    Ui::Help *ui;
};

#endif // HELP_H
