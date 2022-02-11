#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QDebug>
#include <QDateTime>
#include <QMessageBox>

namespace Ui {
class Properties;
}

class Properties : public QDialog
{
    Q_OBJECT

public:
    explicit Properties(QWidget *parent = 0);
    ~Properties();
    void UpdateVars(void);
    bool Save(QString fileName);
    bool Load(QString fileName);
    void Log(QString Message);
    QString ApplicationPath;
    QString HomePath;
    QString DataFilePath;
    QString MethodesPath;
    QString ScriptPath;
    QString LoadControlPanel;
    QString FileName;
    QString LogFile;
    QString AMPSbaud;
    bool SearchAMPS;
    bool AutoFileName;
    bool AutoConnect;
    bool AutoRestore;
    int  MinMIPS;
    float  UpdateSecs;
    QStringList MIPS_TCPIP;

private:
    Ui::Properties *ui;

private slots:
    void slotDataFilePath(void);
    void slotMethodesPath(void);
    void slotScriptPath(void);
    void slotLoadControlPanel(void);
    void slotClear(void);
    void slotOK(void);
    void slotLogFile(void);
};

#endif // PROPERTIES_H
