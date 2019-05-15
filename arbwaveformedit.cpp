#include "arbwaveformedit.h"
#include "ui_arbwaveformedit.h"
#include "qcustomplot.h"

ARBwaveformEdit::ARBwaveformEdit(QWidget *parent, int ppp) :
    QDialog(parent),
    ui(new Ui::ARBwaveformEdit)
{
    ui->setupUi(this);
    this->setFixedSize(617,494);

    PPP = ppp;
    if(PPP < 8) PPP = 8;
    if(PPP >32) PPP = 32;

    connect(ui->pbGenUpDown,SIGNAL(pressed()),this,SLOT(GenerateUpDown()));
    connect(ui->pbGenSine,SIGNAL(pressed()),this,SLOT(GenerateSine()));
    connect(ui->pbPlot,SIGNAL(pressed()),this,SLOT(PlotData()));
    connect(ui->pbInvert,SIGNAL(pressed()),this,SLOT(InvertData()));
    connect(ui->buttonBox,SIGNAL(accepted()),this,SLOT(WaveformAccepted()));

    // Setup the plot used to display the waaveform
    ui->plot->addGraph();
    // give the axes some labels:
    ui->plot->xAxis->setLabel("index");
    ui->plot->yAxis->setLabel("amplitude");
    // set axes ranges, so we see all data:
    ui->plot->xAxis->setRange(0, PPP-1);
    ui->plot->yAxis->setRange(-105, 105);
    ui->plot->replot();
}

ARBwaveformEdit::~ARBwaveformEdit()
{
    delete ui;
}

void ARBwaveformEdit::SetWaveform(int *wf)
{
   int i;

   if(wf == NULL) return;
   ui->txtData->clear();
   for(i=0; i<PPP; i++)
   {
       Waveform[i] = wf[i];
       ui->txtData->appendPlainText(QString::number(Waveform[i]));
   }
   ui->txtData->moveCursor (QTextCursor::Start);
   QVector<double> x(PPP), y(PPP);
   for(i=0; i<PPP; i++)
   {
       x[i] = i;
       y[i] = Waveform[i];
   }
   ui->plot->graph(0)->setData(x, y);
   ui->plot->replot();
}

void ARBwaveformEdit::GetWaveform(int *wf)
{
   int i;

   for(i=0; i<32; i++) wf[i] = Waveform[i];
}

void ARBwaveformEdit::GenerateUpDown(void)
{
   int up,down,binsper,i;

   up = ui->leUp->text().toInt();
   down = ui->leDown->text().toInt();
   binsper = PPP/(up + down);
   // Fill waveform with data
   ui->txtData->clear();
   for(i=0; i<PPP; i++)
   {
       if(i < up * binsper) Waveform[i] = 100;
       else Waveform[i] = -100;
       ui->txtData->appendPlainText(QString::number(Waveform[i]));
   }
   ui->txtData->moveCursor (QTextCursor::Start);
   QVector<double> x(PPP), y(PPP);
   for(i=0; i<PPP; i++)
   {
       x[i] = i;
       y[i] = Waveform[i];
   }
   ui->plot->graph(0)->setData(x, y);
   ui->plot->replot();
}

void ARBwaveformEdit::GenerateSine(void)
{
   int i;

   // Fill waveform with data
   ui->txtData->clear();
   for(i=0; i<PPP; i++)
   {
       Waveform[i] = qSin(qDegreesToRadians((ui->leCycles->text().toFloat() * 360.0 * i)/PPP) + qDegreesToRadians(ui->lePhase->text().toFloat())) * 100.0;
       ui->txtData->appendPlainText(QString::number(Waveform[i]));
   }
   ui->txtData->moveCursor (QTextCursor::Start);
   QVector<double> x(PPP), y(PPP);
   for(i=0; i<PPP; i++)
   {
       x[i] = i;
       y[i] = Waveform[i];
   }
   ui->plot->graph(0)->setData(x, y);
   ui->plot->replot();
}

void ARBwaveformEdit::PlotData(void)
{
    int i;

    // Read the data from the text box into the waveform
    QString text = ui->txtData->toPlainText();
    QStringList valStrings = text.split( "\n" );
    // Write the data to the waveform buffer
    for(i=0; i<PPP; i++)
    {
        if(valStrings.count() > i)
        {
            Waveform[i] = valStrings[i].toFloat();
        }
        else Waveform[i] = 0;
    }
    QVector<double> x(PPP), y(PPP);
    for(i=0; i<PPP; i++)
    {
        x[i] = i;
        y[i] = Waveform[i];
    }
    ui->plot->graph(0)->setData(x, y);
    ui->plot->replot();
}

void ARBwaveformEdit::InvertData(void)
{
    int i;

    // Read the data from the text box into the waveform
    QString text = ui->txtData->toPlainText();
    QStringList valStrings = text.split( "\n" );
    // Write the data to the waveform buffer
    for(i=0; i<PPP; i++)
    {
        if(valStrings.count() > i)
        {
            Waveform[i] = -valStrings[i].toFloat();
        }
        else Waveform[i] = 0;
    }
    QVector<double> x(PPP), y(PPP);
    ui->txtData->clear();
    for(i=0; i<PPP; i++)
    {
        x[i] = i;
        y[i] = Waveform[i];
        ui->txtData->appendPlainText(QString::number(Waveform[i]));
    }
    ui->txtData->moveCursor (QTextCursor::Start);
    ui->plot->graph(0)->setData(x, y);
    ui->plot->replot();
}

void ARBwaveformEdit::WaveformAccepted(void)
{
    // Signal to the parent that data is avalible
    emit WaveformReady();
}
