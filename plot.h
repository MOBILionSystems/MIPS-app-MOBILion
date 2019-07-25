#ifndef PLOT_H
#define PLOT_H

#include <QDialog>
#include <QStatusBar>
#include <QMenu>
#include <QDebug>
#include <QKeyEvent>
#include "qcustomplot.h"

namespace Ui {
class Plot;
}

// This structure contains one set of data points for a graph
typedef struct
{
    int point;
    float X;
    QList<float *> Y;
} DataPoint;

// This structure contains one set of graphs
typedef struct
{
    int NumScans;           // Number of coadds, only Y values are coadded
    QList<DataPoint *> Vec;
} PlotGraph;

class Plot : public QDialog
{
    Q_OBJECT

public:
    explicit Plot(QWidget *parent, QString Title, QString Yaxis, QString Xaxis, int NumPlots);
    ~Plot();
    void PlotCommand(QString cmd, bool PlotOnly = false);
    void PaintGraphs(PlotGraph *pg);
    void Save(QString filename);
    void Load(QString filename);
    void FreeAllData(void);
    void ZoomSelect(void);
    QCPColorMap *colorMap1,*colorMap2;
    QCPColorScale *colorScale1,*colorScale2;
    QString PlotTitle;
    QString Comments;
    QString Label1;
    QString Label2;
    QString Scan;
    QStatusBar  *statusBar;

private:
    Ui::Plot *ui;
    int  CurrentIndex;
    float m,b;
    QList<PlotGraph *> plotGraphs;    // All graphs
    QMenu   *popupMenu;
    QAction *SaveOption;
    QAction *LoadOption;
    QAction *XaxisZoomOption;
    QAction *YaxisZoomOption;
    QAction *TrackOption;
    QAction *ClipboardOption;
    QAction *CommentOption;
    QAction *HeatOption;

protected:
    void resizeEvent(QResizeEvent *event); // override;

public slots:
    void mousePressed(QMouseEvent*);
    void mousePressedHM(QMouseEvent*);
    void slotSaveMenu(void);
    void slotLoadMenu(void);
    void slotCommentMenu(void);
    void slotCloseComments(void);
    void slotXaxisZoomOption(void);
    void slotYaxisZoomOption(void);
    void slotTrackOption(void);
    void slotClipBoard(void);
    void slotHeatMap(void);

protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // PLOT_H
