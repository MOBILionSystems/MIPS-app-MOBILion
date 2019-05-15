#ifndef PLOT_H
#define PLOT_H

#include <QDialog>
#include <QStatusBar>
#include <QMenu>
#include <QDebug>
#include <QKeyEvent>

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
    QString PlotTitle;
    QString Comments;
    QStatusBar  *statusBar;

private:
    Ui::Plot *ui;
    int  CurrentIndex;
    QList<PlotGraph *> plotGraphs;    // All graphs
    QMenu   *popupMenu;
    QAction *SaveOption;
    QAction *LoadOption;
    QAction *XaxisZoomOption;
    QAction *YaxisZoomOption;
    QAction *TrackOption;
    QAction *ClipboardOption;
    QAction *CommentOption;

protected:
    void resizeEvent(QResizeEvent *event); // override;

public slots:
    void mousePressed(QMouseEvent*);
    void slotSaveMenu(void);
    void slotLoadMenu(void);
    void slotCommentMenu(void);
    void slotCloseComments(void);
    void slotXaxisZoomOption(void);
    void slotYaxisZoomOption(void);
    void slotTrackOption(void);
    void slotClipBoard(void);

protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // PLOT_H
