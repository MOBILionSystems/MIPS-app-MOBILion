#ifndef DCBIAS
#define DCBIAS

#include "ui_mips.h"
#include "mips.h"
#include "comms.h"

#include <QtCore/QtGlobal>
#include <QtSerialPort/QSerialPort>
#include <QStatusBar>
#include <QMessageBox>
#include <QObject>
#include <QApplication>
#include <QFileInfo>

class DCbias : public QDialog
{
     Q_OBJECT

public:
    DCbias(Ui::MIPS *w, Comms *c);
    void SetNumberOfChannels(int num);
    void Update(void);
    void Save(QString Filename);
    void Load(QString Filename);
    bool myEventFilter(QObject *obj, QEvent *event);
    void ApplyDelta(QString GrpName, float change);

    Ui::MIPS *dui;
    Comms *comms;
    int NumChannels;
    QLineEdit *selectedLineEdit;

private:
    bool Updating;
    bool UpdateOff;

private slots:
    void DCbiasUpdated(void);
    void DCbiasPower(void);
    void UpdateDCbias(void);
};

#endif // DCBIAS

