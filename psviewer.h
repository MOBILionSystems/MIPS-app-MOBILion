#ifndef PSVIEWER_H
#define PSVIEWER_H

#include "pse.h"

#include <QDialog>

namespace Ui {
class psviewer;
}

class psviewer : public QDialog
{
    Q_OBJECT

public:
    explicit psviewer(QList<psgPoint*> *P, QWidget *parent = 0);
    ~psviewer();
    void PlotDCBchannel(int channel);

private:
    Ui::psviewer *ui;
    QList<psgPoint*> *psg;
    float minY;
    float maxY;
    int   minCycles;
    int   maxCycles;
    int   plotnum;

private slots:
    void SetZoom(void);
    void PlotSelectedItem(void);
};

#endif // PSVIEWER_H
