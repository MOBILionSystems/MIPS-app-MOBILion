#ifndef GRID_H
#define GRID_H

#include "comms.h"
#include <QDialog>
#include <QStatusBar>

namespace Ui {
class Grid;
}

class Grid : public QDialog
{
    Q_OBJECT

signals:
    void DialogClosed(void);

public:
    explicit Grid(QWidget *parent = 0, Comms *c = NULL, QStatusBar *statusbar = NULL);
    ~Grid();
    virtual void reject();
//    bool ConfigurationCheck(void);
    void Save(QString Filename);
    void Load(QString Filename);
    void Update(void);
    void PulseSeqVarUpdate(void);
    QString GenerateTable(void);

private:
    Ui::Grid *ui;
    QStatusBar *sb;
    Comms *comms;
    bool Updating;
    bool UpdateOff;
    QTime time;
    // Pulse sequence variables
    float Tp;
    float Ts1;
    float Ts2;
    int   n;
    float Tdn[20];
    int   Repeat[20];
    int   SeqRepeat;
    QString Table;
    const QClipboard *clipboard;

private slots:
    void Updated(void);
    void GRID1enable(void);
    void GRID2enable(void);
    void Shutdown(void);
    void AutoTune(void);
    void AutoRetune(void);
    void SetYmax(void);
    void SetYmin(void);
    void ResetPlot(void);
    void YautoScale(void);
    void ModeChange(void);
    // Pluse sequence slots
    void PulseSeqVarChange(void);
    void PulseSeqNselect(void);
    void Download(void);
    void ShowTable(void);
    void StartTable(void);
    void slotDataChanged(void);
};

#endif // GRID_H
