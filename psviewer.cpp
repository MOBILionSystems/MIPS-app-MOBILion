#include "psviewer.h"
#include "ui_psviewer.h"
#include "qcustomplot.h"

psviewer::psviewer(QList<psgPoint *> *P, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::psviewer)
{
    ui->setupUi(this);

    psg = P;
    plotnum = 0;
    // Find the minimum and maximum Y values from table
    minY = 0;
    maxY = 5;
    for(int i=0; i<psg->size(); i++)
    {
        for(int j=0; j<16; j++)
        {
            if((*psg)[i]->DCbias[j] < minY) minY = (*psg)[i]->DCbias[j];
            if((*psg)[i]->DCbias[j] > maxY) maxY = (*psg)[i]->DCbias[j];
        }
    }
    // Find the maximum number of clock counts
    minCycles = 0;
    maxCycles = (*psg)[psg->size()-1]->TimePoint;
    // Fill the plot selection combobox with all the channels, digital and analog
    ui->comboPlotItem->clear();
    ui->comboPlotItem->addItem("");
    for(int i=0; i<16; i++)
    {
        ui->comboPlotItem->addItem("CH " + QString::number(i+1));
    }

    // Setup the plot used to display the pulse sequence
    // give the axes some labels:
    ui->PSV->xAxis->setLabel("Clock cycles");
    ui->PSV->yAxis->setLabel("Voltage");
    // set axes ranges, so we see all data:
    ui->PSV->xAxis->setRange(minCycles, maxCycles);
    ui->PSV->yAxis->setRange(minY, maxY);
    ui->PSV->replot();
    // Look for loops in the sequence and label with brackets if found
    int BracketPos = 1;
    for(int i=0; i<psg->size(); i++)
    {
        if((*psg)[i]->Loop)
        {
            // Here with the closing side of the loop, now find the start
            for(int j=0; j<psg->size(); j++)
            {
                if((*psg)[i]->LoopName == (*psg)[j]->Name)
                {
                    // Here with the start and stop of a loop, draw the bracket
                    QCPItemBracket *bracket = new QCPItemBracket(ui->PSV);
                    bracket->left->setCoords((*psg)[j]->TimePoint, BracketPos);
                    bracket->right->setCoords((*psg)[i]->TimePoint, BracketPos);
                    // Add label with number of loops
                    QCPItemText *LoopText = new QCPItemText(ui->PSV);
                    LoopText->position->setParentAnchor(bracket->center);
                    LoopText->position->setCoords(0, -5);
                    LoopText->setPositionAlignment(Qt::AlignBottom|Qt::AlignHCenter);
                    if((*psg)[i]->LoopCount == 0) LoopText->setText("Loop forever");
                    else LoopText->setText("Loop " + QString::number((*psg)[i]->LoopCount) + " times");
                    LoopText->setFont(QFont(font().family(), 10));
                    BracketPos += 25;
                }
            }
        }
    }
    connect(ui->chkZoomX,SIGNAL(clicked(bool)),this,SLOT(SetZoom()));
    connect(ui->chkZoomY,SIGNAL(clicked(bool)),this,SLOT(SetZoom()));
    connect(ui->comboPlotItem,SIGNAL(currentIndexChanged(int)),this,SLOT(PlotSelectedItem()));

}

psviewer::~psviewer()
{
    delete ui;
}

void psviewer::PlotDCBchannel(int channel)
{
    QVector<double> x(maxCycles), y(maxCycles);
    for(int j=0; j<maxCycles; j++)
    {
        x[j] = j;
        y[j] = 0;
    }
    // Set the defined points in the vector, these are change points
    for(int i=0; i<psg->size(); i++)
    {
        for(int j=(*psg)[i]->TimePoint; j<maxCycles; j++)
        {
            y[j] = (*psg)[i]->DCbias[channel];
        }
    }
    ui->PSV->legend->setVisible(true);
    ui->PSV->addGraph();
    switch (plotnum)
    {
       case 0:
        ui->PSV->graph(plotnum)->setPen(QPen(Qt::red));
        break;
       case 1:
        ui->PSV->graph(plotnum)->setPen(QPen(Qt::blue));
        break;
       case 2:
        ui->PSV->graph(plotnum)->setPen(QPen(Qt::green));
        break;
       case 3:
        ui->PSV->graph(plotnum)->setPen(QPen(Qt::yellow));
        break;
       case 4:
        ui->PSV->graph(plotnum)->setPen(QPen(Qt::cyan));
        break;
       default:
        break;
    }
    ui->PSV->graph(plotnum)->setName("CH " + QString::number(channel +1));
    ui->PSV->graph(plotnum++)->setData(x, y);
    ui->PSV->replot();
}

void psviewer::SetZoom(void)
{
    if(ui->chkZoomX->isChecked() && ui->chkZoomY->isChecked())
    {
        ui->PSV->yAxis->axisRect()->setRangeZoom(Qt::Vertical | Qt::Horizontal);
        ui->PSV->yAxis->axisRect()->setRangeDrag(Qt::Vertical | Qt::Horizontal);
        ui->PSV->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    }
    else if(ui->chkZoomX->isChecked())
    {
        ui->PSV->yAxis->axisRect()->setRangeZoom(Qt::Horizontal);
        ui->PSV->yAxis->axisRect()->setRangeDrag(Qt::Horizontal);
        ui->PSV->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    }
    else if(ui->chkZoomY->isChecked())
    {
        ui->PSV->yAxis->axisRect()->setRangeZoom(Qt::Vertical);
        ui->PSV->yAxis->axisRect()->setRangeDrag(Qt::Vertical);
        ui->PSV->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    }
    else
    {
        ui->PSV->setInteractions(0);
    }
}

void psviewer::PlotSelectedItem(void)
{
    int index = ui->comboPlotItem->currentIndex();
    if(index == 0) return;
    index--;
    PlotDCBchannel(index);
}
