#ifndef SOFTLANDING2_H
#define SOFTLANDING2_H

#include "comms.h"
#include <QDialog>
#include <QStatusBar>

namespace Ui {
class SoftLanding2;
}

class SoftLanding2 : public QDialog
{
    Q_OBJECT

signals:
    void DialogClosed(void);

public:
    explicit SoftLanding2(QWidget *parent = 0, Comms *c = NULL, QStatusBar *statusbar = NULL);
    ~SoftLanding2();
    virtual void reject();
    bool ConfigurationCheck(void);
    void Save(QString Filename);
    QString Load(QString Filename);
    void Update(void);

private:
    Ui::SoftLanding2 *ui;
    QStatusBar *sb;
    Comms *comms;
    bool Updating;
    bool UpdateOff;
    bool AbortRequest;

private slots:
    void ESIenablePos(void);
    void ESIenableNeg(void);
    void AutoTune(void);
    void SLUpdated(void);
    void Shutdown(void);
    void ResetPowerSupply(void);
    void LoadScriptFile(void);
    void AbortScriptFile(void);

protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // SOFTLANDING2_H
