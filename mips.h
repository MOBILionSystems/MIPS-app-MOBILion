#ifndef MIPS_H
#define MIPS_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtCore/QtGlobal>
#include <QLineEdit>
#include <QTimer>
#include <QProcess>
#include <QtNetwork/QTcpSocket>


namespace Ui {
class MIPS;
}

class Console;
class SettingsDialog;
class pseDialog;
class psgPoint;
class Comms;
class Twave;
class DCbias;
class DIO;
class RFdriver;
class PSG;
class Program;
class Help;
class ARB;

class MIPS : public QMainWindow
{
    Q_OBJECT

public:
    explicit MIPS(QWidget *parent = 0);
    ~MIPS();
    virtual void resizeEvent(QResizeEvent* event);
    virtual void mousePressEvent(QMouseEvent * event);
    Ui::MIPS *ui;

private slots:
    void setWidgets(QWidget*, QWidget*);
    void MIPSconnect(void);
    void MIPSsetup(void);
    void MIPSdisconnect(void);
    void tabSelected();
    void writeData(const QByteArray &data);
    void readData2Console(void);
    void pollLoop(void);
    void loadSettings(void);
    void saveSettings(void);
    void DisplayAboutMessage(void);
    void clearConsole(void);
    void MIPScommands(void);
    void GeneralHelp(void);

private:
    Comms *comms;
    Twave *twave;
    DCbias *dcbias;
    DIO *dio;
    RFdriver *rfdriver;
    PSG *SeqGen;
    Program *pgm;
    Console *console;
    SettingsDialog *settings;
    Help *help;
    ARB *arb;
    QTimer *pollTimer;
    QString  appPath;
};

#endif // MIPS_H
