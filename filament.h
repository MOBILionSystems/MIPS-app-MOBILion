#ifndef FILAMENT_H
#define FILAMENT_H

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
#include <QFileDialog>

class Filament: public QDialog
{
    Q_OBJECT

public:
    Filament(Ui::MIPS *w, Comms *c);
    void Update(void);

    Ui::MIPS *fui;
    Comms *comms;

private slots:
    void Filament1Enable(void);
    void Filament2Enable(void);
    void FilamentUpdated(void);
};

#endif // FILAMENT_H
