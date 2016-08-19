#ifndef TWAVE
#define TWAVE

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

class Twave : public QDialog
{
     Q_OBJECT

public:
    Twave(Ui::MIPS *w, Comms *c);
    void Update(void);
    void Save(QString Filename);
    void Load(QString Filename);

    Ui::MIPS *tui;
    Comms *comms;
    int NumChannels;
    bool Compressor;

private slots:
    void Changed(void);
    void rbModeCompress(void);
    void rbModeNormal(void);
    void rbSwitchClose(void);
    void rbSwitchOpen(void);
    void pbUpdate(void);
    void pbForceTrigger(void);
};

#endif // TWAVE

