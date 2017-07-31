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
//    void Save(QString Filename);
//    void Load(QString Filename);
    void Update(void);

private:
    Ui::Grid *ui;
    QStatusBar *sb;
    Comms *comms;
    bool Updating;
    bool UpdateOff;

private slots:
    void Updated(void);
    void GRID1enable(void);
    void GRID2enable(void);
    void Shutdown(void);
    void AutoTune(void);
    void AutoRetune(void);

};

#endif // GRID_H
