#ifndef SINGLEFUNNEL_H
#define SINGLEFUNNEL_H

#include "comms.h"
#include <QDialog>
#include <QStatusBar>

namespace Ui {
class SingleFunnel;
}

class SingleFunnel : public QDialog
{
    Q_OBJECT

signals:
    void DialogClosed(void);

public:
    explicit SingleFunnel(QWidget *parent = 0, Comms *c = NULL, QStatusBar *statusbar = NULL);
    ~SingleFunnel();
    virtual void reject();
    bool ConfigurationCheck(void);
    void Save(QString Filename);
    void Load(QString Filename);
    void Update(void);

    Comms *comms;

private:
    Ui::SingleFunnel *ui;
    QStatusBar *sb;

private slots:
    void SFUpdated(void);
    void ESIenable(void);
    void AutoTune(void);
    void AutoRetune(void);
};

#endif // SINGLEFUNNEL_H
