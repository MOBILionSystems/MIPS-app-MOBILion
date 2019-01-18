#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QDebug>
#include <QDateTime>

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
    void Save(QString fileName);
    void Load(QString fileName);
    void Log(QString Message);
    QString DataFilePath;
    QString MethodesPath;
    QString ScriptPath;
    QString LoadControlPanel;
    QString FileName;
    QString LogFile;
    bool AutoFileName;
    bool AutoConnect;
    int  MinMIPS;
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
