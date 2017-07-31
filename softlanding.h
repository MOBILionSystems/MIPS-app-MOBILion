#ifndef SOFTLANDING_H
#define SOFTLANDING_H

#include "comms.h"
#include <QDialog>
#include <QStatusBar>

namespace Ui {
class SoftLanding;
}

class SoftLanding : public QDialog
{
    Q_OBJECT

signals:
    void DialogClosed(void);

public:
    explicit SoftLanding(QWidget *parent = 0, Comms *c = NULL, QStatusBar *statusbar = NULL);
    ~SoftLanding();
    virtual void reject();
    bool ConfigurationCheck(void);
    void Save(QString Filename);
    void Load(QString Filename);
    void Update(void);

private:
    Ui::SoftLanding *ui;
    QStatusBar *sb;
    Comms *comms;
    bool Updating;
    bool UpdateOff;


private slots:
    void ESIenable(void);
    void AutoTune(void);
    void SLUpdated(void);
    void RQmode(void);
    void Shutdown(void);
    void ResetPowerSupply(void);
    void MassUnitsUpdated(void);

protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // SOFTLANDING_H
