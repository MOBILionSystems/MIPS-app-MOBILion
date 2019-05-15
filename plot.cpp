// Plot.c
//
// Generic plot function called by the control panel script system.
// When the plot is created the following should be defined:
//  - Title
//  - Yaxis title
//  - Xaxis title
//  - Number of plot, 1 or 2 for now
// A generic PlotCommand function accepts plot command string to enable
// ploting data. All commands sent to the plot command function are
// appended to the comments.
//
// Add the following capability:
//  - Comment system, view and edit comments
//  - Add popup menu system that will have the following functions
//      - Save
//      - Load
//      - trace
//      - X axis zoom
//      - Y axis zoom
//      - Edit comments
//      - Display heatmap of scans
//  - Add strip chart option
//
#include "plot.h"
#include "ui_plot.h"

Plot::Plot(QWidget *parent, QString Title, QString Yaxis, QString Xaxis, int NumPlots) :
    QDialog(parent),
    ui(new Ui::Plot)
{
    ui->setupUi(this);
    PlotTitle = Title;
    Plot::setWindowTitle(Title);
    this->installEventFilter(this);
    ui->txtComments->setVisible(false);
    ui->pbCloseComments->setVisible(false);
    plotGraphs.clear();
    // Setup the statusbar
    statusBar = new QStatusBar(this);
    statusBar->setGeometry(0,this->height()-18,this->width(),18);
    statusBar->raise();
    // give the axes some labels:
    ui->Graph->xAxis->setLabel(Xaxis);
    ui->Graph->yAxis->setLabel(Yaxis);
    // create the plots
    for(int i=0; i<NumPlots; i++)
    {
        ui->Graph->addGraph();
        //if(i == 0) ui->Graph->graph(0)->setPen(QPen(Qt::blue));
        if(i == 1) ui->Graph->graph(1)->setPen(QPen(Qt::red));
    }
    ui->Graph->replot();
    connect(ui->Graph, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePressed(QMouseEvent*)));
    connect(ui->pbCloseComments, SIGNAL(pressed()), this, SLOT(slotCloseComments()));
    // Plot popup menu options
    SaveOption = new QAction("Save", this);
    connect(SaveOption, SIGNAL(triggered()), this, SLOT(slotSaveMenu()));
    LoadOption = new QAction("Load", this);
    connect(LoadOption, SIGNAL(triggered()), this, SLOT(slotLoadMenu()));
    CommentOption  = new QAction("Comments", this);
    connect(CommentOption, SIGNAL(triggered()), this, SLOT(slotCommentMenu()));

    XaxisZoomOption = new QAction("Xaxis zoom", this);
    XaxisZoomOption->setCheckable(true);
    connect(XaxisZoomOption, SIGNAL(triggered()), this, SLOT(slotXaxisZoomOption()));
    YaxisZoomOption = new QAction("Yaxis zoom", this);
    YaxisZoomOption->setCheckable(true);
    connect(YaxisZoomOption, SIGNAL(triggered()), this, SLOT(slotYaxisZoomOption()));
    TrackOption = new QAction("Track", this);
    TrackOption->setCheckable(true);
    connect(TrackOption, SIGNAL(triggered()), this, SLOT(slotTrackOption()));
    ClipboardOption = new QAction("Clipboard", this);
    connect(ClipboardOption, SIGNAL(triggered()), this, SLOT(slotClipBoard()));

}

void Plot::FreeAllData(void)
{
    for(int i=0;i<plotGraphs.count();i++)
    {
        for(int j=0;j<plotGraphs[i]->Vec.count();j++)
        {
            for(int k=0;k<plotGraphs[i]->Vec[j]->Y.count();k++)
            {
                delete plotGraphs[i]->Vec[j]->Y[k];
            }
        }
        plotGraphs[i]->Vec.clear();
    }
    plotGraphs.clear();
}

Plot::~Plot()
{
    FreeAllData();
    delete ui;
}

void Plot::resizeEvent(QResizeEvent *event)
{
    ui->Graph->setGeometry(0,0,this->width(),this->height()-18);
    statusBar->setGeometry(0,this->height()-18,this->width(),18);
    ui->txtComments->setGeometry(0,0,this->width()-ui->pbCloseComments->width(),this->height()-18);
    ui->pbCloseComments->setGeometry(this->width()-ui->pbCloseComments->width(),this->height()-18-ui->pbCloseComments->height(),ui->pbCloseComments->width(),ui->pbCloseComments->height());
    ui->Graph->replot();
    QWidget::resizeEvent(event);
}

bool Plot::eventFilter(QObject *obj, QEvent *event)
{

    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *key = static_cast<QKeyEvent *>(event);
        if(key->key() == 16777236)  // Right arrow
        {
            CurrentIndex++;
            if(CurrentIndex >= plotGraphs.count()) CurrentIndex = plotGraphs.count()-1;
            if(CurrentIndex >= 0)
            {
                PaintGraphs(plotGraphs[CurrentIndex]);
                Plot::setWindowTitle(PlotTitle + ", " + "number: " + QString::number(CurrentIndex+1));
                ui->Graph->replot();
            }
            return true;
        }
        if(key->key() == 16777234) // Left arrow
        {
            CurrentIndex--;
            if(CurrentIndex < 0) CurrentIndex = 0;
            if((CurrentIndex >= 0) && (plotGraphs.count() > 0))
            {
                PaintGraphs(plotGraphs[CurrentIndex]);
                Plot::setWindowTitle(PlotTitle + ", " + "number: " + QString::number(CurrentIndex+1));
                ui->Graph->replot();
            }
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

void Plot::mousePressed(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
       popupMenu = new QMenu(tr("Plot options"), this);
       popupMenu->addAction(SaveOption);
       popupMenu->addAction(LoadOption);
       popupMenu->addAction(XaxisZoomOption);
       popupMenu->addAction(YaxisZoomOption);
       popupMenu->addAction(TrackOption);
       popupMenu->addAction(ClipboardOption);
       popupMenu->addAction(CommentOption);
       popupMenu->exec(event->globalPos());
    }
}

// Save all the comments to a text file. The comments include
// all the plot commands.
void Plot::Save(QString filename)
{
    if(filename.isEmpty()) return;
    QFile file(filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        stream << ui->txtComments->toPlainText();
        file.close();
    }
}

void Plot::slotSaveMenu(void)
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save plot data","","Plot (*.plot);;All files (*.*)");
    Save(fileName);
}

void Plot::slotLoadMenu(void)
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load plot data","","Plot (*.plot);;All files (*.*)");
    Load(fileName);
}

void Plot::Load(QString filename)
{
    if(filename.isEmpty()) return;
    QFile file(filename);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        // We're going to streaming the file
        // to a QString
        QTextStream stream(&file);
        Comments = stream.readAll();
        ui->txtComments->clear();
        ui->txtComments->appendPlainText(Comments);
        // Clear and current data
        FreeAllData();
        file.close();
        // process the commands
        QStringList Lines = Comments.split("\n");
        for(int i=0;i<Lines.count();i++) PlotCommand(Lines[i],true);
    }
}

void Plot::slotCommentMenu(void)
{
    if(!ui->txtComments->isVisible())
    {
        ui->Graph->setVisible(false);
        ui->txtComments->setVisible(true);
        ui->pbCloseComments->setVisible(true);
    }
}

void Plot::slotCloseComments(void)
{
    ui->Graph->setVisible(true);
    ui->txtComments->setVisible(false);
    ui->pbCloseComments->setVisible(false);
}

void Plot::PaintGraphs(PlotGraph *pg)
{
    for(int i=0;i<ui->Graph->graphCount();i++) ui->Graph->graph(i)->data()->clear();
    // Add the data points from last plotGraphs
    for(int i=0;i<pg->Vec.count();i++)
    {
        for(int j=0;j<ui->Graph->graphCount();j++)
            ui->Graph->graph(j)->addData(pg->Vec[i]->X - pg->Vec[0]->X, *pg->Vec[i]->Y[j]/pg->NumScans);
    }
}

// This function accepts plot command strings. The following commands are
// supported:
// Clear
// Refresh
// Xrange, min, max
// Yrange, min, max
// NewGraph,numgraphs,points
// AddPoint,point num, X axis value, Graph1, Graph 2, ....
// Plot
// Save,filename
// Load,filename
void Plot::PlotCommand(QString cmd, bool PlotOnly)
{
    QStringList reslist = cmd.split(",");

    if(!PlotOnly)
    {
        ui->txtComments->appendPlainText(cmd);
        if(reslist[0].toUpper() == "SAVE") Save(reslist[1]);
        else if(reslist[0].toUpper() == "LOAD") Load(reslist[1]);
    }
    if(reslist[0].toUpper() == "REFRESH")
    {
        Plot::setWindowTitle(PlotTitle + ", " + "number: " + QString::number(CurrentIndex+1));
        ui->Graph->replot();
    }
    else if((cmd.toUpper().startsWith("XRANGE")) && (reslist.count()==3))
    {
        ui->Graph->xAxis->setRange(reslist[1].toDouble(), reslist[2].toDouble());
        ui->Graph->replot();
    }
    else if(cmd.toUpper().startsWith("YRANGE"))
    {
        if(reslist.count()==3)
        {
           ui->Graph->yAxis->setRange(reslist[1].toDouble(), reslist[2].toDouble());
           ui->Graph->replot();
        }
    }
    else if(cmd.toUpper() == "CLEAR")
    {
        for(int i=0;i<ui->Graph->graphCount();i++) ui->Graph->graph(i)->data()->clear();
    }
    else if((reslist[0].toUpper() == "NEWGRAPH")&&(reslist.count()==3))
    {
        PlotGraph *pg = new PlotGraph;
        plotGraphs.append(pg);
        plotGraphs.last()->NumScans = 0;
        for(int i=0;i<reslist[2].toInt();i++)
        {
            DataPoint *dp = new DataPoint;
            plotGraphs.last()->Vec.append(dp);
            plotGraphs.last()->Vec.last()->point = 0;
            plotGraphs.last()->Vec.last()->X = 0.0;
            for(int j=0;j<reslist[1].toInt();j++)
            {
                float *f = new float;
                plotGraphs.last()->Vec.last()->Y.append(f);
                *plotGraphs.last()->Vec.last()->Y.last() = 0.0;
            }
        }
    }
    else if((reslist[0].toUpper() == "ADDPOINT")&&(reslist.count()>=4))
    {
        if(reslist[1].toInt()-1 < plotGraphs.last()->Vec.count())
        {
           if(reslist[1].toInt()-1 == 0) plotGraphs.last()->NumScans++;
           plotGraphs.last()->Vec[reslist[1].toInt()-1]->X = reslist[2].toFloat();
           *plotGraphs.last()->Vec[reslist[1].toInt()-1]->Y[0] += reslist[3].toFloat();
           if(reslist.count()>4) *plotGraphs.last()->Vec[reslist[1].toInt()-1]->Y[1] += reslist[4].toFloat();
        }
    }
    else if(cmd.toUpper() == "PLOT")
    {
        CurrentIndex = plotGraphs.count() - 1;
        PaintGraphs(plotGraphs.last());
    }
    else if((reslist[0].toUpper() == "TITLE")&&(reslist.count()==2))
    {
        PlotTitle = reslist[1];
        Plot::setWindowTitle(PlotTitle);
    }
}

void Plot::slotXaxisZoomOption(void)
{
    if(XaxisZoomOption->isChecked()) XaxisZoomOption->setChecked(true);
    else XaxisZoomOption->setChecked(false);
    ZoomSelect();
}

void Plot::slotYaxisZoomOption(void)
{
    if(YaxisZoomOption->isChecked()) YaxisZoomOption->setChecked(true);
    else YaxisZoomOption->setChecked(false);
    ZoomSelect();
}

void Plot::ZoomSelect(void)
{
    if(XaxisZoomOption->isChecked() && YaxisZoomOption->isChecked())
    {
        ui->Graph->xAxis->axisRect()->setRangeZoom(Qt::Vertical | Qt::Horizontal);
        ui->Graph->xAxis->axisRect()->setRangeDrag(Qt::Vertical | Qt::Horizontal);
        ui->Graph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    }
    else if(XaxisZoomOption->isChecked())
    {
        ui->Graph->xAxis->axisRect()->setRangeZoom(Qt::Horizontal);
        ui->Graph->xAxis->axisRect()->setRangeDrag(Qt::Horizontal);
        ui->Graph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    }
    else if(YaxisZoomOption->isChecked())
    {
        ui->Graph->yAxis->axisRect()->setRangeZoom(Qt::Vertical);
        ui->Graph->yAxis->axisRect()->setRangeDrag(Qt::Vertical);
        ui->Graph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    }
    else ui->Graph->setInteractions(0);
}

void Plot::slotTrackOption(void)
{
    if(TrackOption->isChecked()) TrackOption->setChecked(true);
    else TrackOption->setChecked(false);
}

void Plot::slotClipBoard(void)
{
    // Set the clilpboard image
    QClipboard * clipboard = QApplication::clipboard();
    QPalette mypalette=this ->palette();
    mypalette.setColor(QPalette::Window,Qt::white);
    ui->Graph->setPalette(mypalette);
    QPixmap pixmap= QPixmap::grabWidget(ui->Graph);
    clipboard->setPixmap(pixmap);
}

