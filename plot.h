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

typedef struct
{
   int    np;
   float  h;
   QList<float> an;
} SGcoeff;

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
    void SavitzkyGolayFilter(int order, QList<float> Y, QList<float> *Yf);
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
    int  Filter;
    float m,b;
    QList<PlotGraph *> plotGraphs;    // All graphs
    QCPTextElement *plotFile = NULL;
    QMenu   *popupMenu;
    QAction *SaveOption;
    QAction *ExportOption;
    QAction *LoadOption;
    QAction *XaxisZoomOption;
    QAction *YaxisZoomOption;
    QAction *FilterOption;
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
    void slotExportMenu(void);
    void slotLoadMenu(void);
    void slotCommentMenu(void);
    void slotCloseComments(void);
    void slotXaxisZoomOption(void);
    void slotYaxisZoomOption(void);
    void slotFilterOption(void);
    void slotTrackOption(void);
    void slotClipBoard(void);
    void slotHeatMap(void);
    void mouseMove(QMouseEvent*event);

protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // PLOT_H
