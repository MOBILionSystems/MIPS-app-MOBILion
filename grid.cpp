//
// Grid control panel. Created for Gary Eiceman.
//
// Gordon Anderson
//
#include "grid.h"
#include "ui_grid.h"
#include "qcustomplot.h"
#include <QDebug>
#include <QKeyEvent>

#define ClockFreq 2625000

Grid::Grid(QWidget *parent, Comms *c, QStatusBar *statusbar) :
    QDialog(parent),
    ui(new Ui::Grid)
{
    ui->setupUi(this);

    comms = c;
    sb = statusbar;
    Updating = false;
    UpdateOff = false;
    this->setFixedSize(955,434);

    QObjectList widgetList = ui->frmGRID->children();
    widgetList += ui->gbGrid1RF->children();
    foreach(QObject *w, widgetList)
    {
        if(w->objectName().contains("leS"))
        {
            ((QLineEdit *)w)->setValidator(new QDoubleValidator);
            connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(Updated()));
        }
    }
    connect(ui->chkGRID1enable,SIGNAL(toggled(bool)),this,SLOT(GRID1enable()));
    connect(ui->chkGRID2enable,SIGNAL(toggled(bool)),this,SLOT(GRID2enable()));
    connect(ui->pbShutdown,SIGNAL(pressed()),this,SLOT(Shutdown()));
    connect(ui->pbTuneG1,SIGNAL(pressed()),this,SLOT(AutoTune()));
    connect(ui->pbRetuneG1,SIGNAL(pressed()),this,SLOT(AutoRetune()));
    connect(ui->rbModeAuto,SIGNAL(clicked(bool)),this,SLOT(ModeChange()));
    connect(ui->rbModeManual,SIGNAL(clicked(bool)),this,SLOT(ModeChange()));

    // Plot setup
    ui->Vplot->addGraph(); // blue line
    ui->Vplot->graph(0)->setPen(QPen(QColor(40, 110, 255)));
    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    ui->Vplot->xAxis->setTicker(timeTicker);
    ui->Vplot->axisRect()->setupFullAxesBox();
    ui->Vplot->yAxis->setRange(0, 5000);
    ui->Vplot->xAxis->setRangeLower(0);
    ui->Vplot->xAxis->setLabel("Time");
    ui->Vplot->yAxis->setLabel("Voltage");
    ui->Vplot->plotLayout()->insertRow(0);
    ui->Vplot->plotLayout()->addElement(0, 0, new QCPTextElement(ui->Vplot, "Grid 1 RF p-p voltage", QFont("sans", 12, QFont::Bold)));
    ui->leYmax->setText("5000");
    ui->leYmin->setText("0");
    connect(ui->leYmax,SIGNAL(editingFinished()),this,SLOT(SetYmax()));
    connect(ui->leYmax,SIGNAL(editingFinished()),this,SLOT(SetYmin()));
    connect(ui->pbResetPlot,SIGNAL(pressed()),this,SLOT(ResetPlot()));
    connect(ui->chkYauto,SIGNAL(toggled(bool)),this,SLOT(YautoScale()));
    time = QTime::currentTime();

    // Init the pulse sequence variables and controls
    Tp = 150;
    Ts1 = 50;
    Ts2 = 50;
    n = 10;
    for(int i=0; i<20; i++)
    {
      Tdn[i] = (i+1) * 5;
      Repeat[i] = 5;
    }
    SeqRepeat = 10;
    PulseSeqVarUpdate();

    connect(ui->leTp,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leTs1,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leTs2,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leN,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leTdn,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leM,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leFullRepeat,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->comboSelectN,SIGNAL(currentIndexChanged(int)),this,SLOT(PulseSeqNselect()));
    connect(ui->pbDownload,SIGNAL(pressed()),this,SLOT(Download()));
    connect(ui->pbShow,SIGNAL(pressed()),this,SLOT(ShowTable()));
    connect(ui->pbStart,SIGNAL(pressed()),this,SLOT(StartTable()));

    clipboard = QApplication::clipboard();
    connect(ui->pbPaste,SIGNAL(pressed()),this,SLOT(slotDataChanged()));
}

Grid::~Grid()
{
    delete ui;
}

void Grid::reject()
{
    emit DialogClosed();
}

void Grid::slotDataChanged(void)
{
    const QMimeData *mimeData = clipboard->mimeData();

    if (mimeData->hasText())
    {
        // Text data
        QString res = mimeData->text();
        QStringList reslist = res.split(QRegExp("[\r\n\t ,]+"), QString::SkipEmptyParts);
        // The count should be a multople of three
        ui->leN->setText(QString::number((int)(reslist.count()/3)));
        ui->leN->setModified(true);
        ui->leN->editingFinished();
        QApplication::processEvents();
        for(int i=0;i<(int)(reslist.count());i+=3)
        {
           if(reslist[i].toInt() != i/3+1) break;
           ui->comboSelectN->setCurrentIndex(i/3);
           QApplication::processEvents();
           ui->leTdn->setText(reslist[i+1]);
           ui->leTdn->setModified(true);
           ui->leTdn->editingFinished();
           QApplication::processEvents();
           ui->leM->setText(reslist[i+2]);
           ui->leM->setModified(true);
           ui->leM->editingFinished();
           QApplication::processEvents();
        }
    }
}

void Grid::GRID1enable(void)
{
    if(ui->chkGRID1enable->isChecked()) comms->SendCommand("SHVENA,1\n");
    else  comms->SendCommand("SHVDIS,1\n");
}

void Grid::GRID2enable(void)
{
    if(ui->chkGRID2enable->isChecked()) comms->SendCommand("SHVENA,2\n");
    else  comms->SendCommand("SHVDIS,2\n");
}

// Set all output level to zero
void Grid::Shutdown(void)
{
    comms->SendCommand("SRFMODE,1,MANUAL\n");
    ui->chkGRID1enable->setChecked(false);
    ui->chkGRID2enable->setChecked(false);
    ui->leSRFDRV_1->setText("0");
    ui->leSRFDRV_1->setModified(true);
    QMetaObject::invokeMethod(ui->leSRFDRV_1, "editingFinished");
}

void Grid::AutoTune(void)
{
   QMessageBox msgBox;

   ui->pbTuneG1->setDown(false);
   QString msg = "This function will tune the RF head attached to channel 1. ";
   msg += "Make sure the RF head is attached and connected to your system as needed. ";
   msg += "This process can take up to 3 minutes.\n";
   msgBox.setText(msg);
   msgBox.setInformativeText("Are you sure you want to continue?");
   msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
   msgBox.setDefaultButton(QMessageBox::Yes);
   int ret = msgBox.exec();
   if(ret == QMessageBox::No) return;
   if(!comms->SendCommand("TUNERFCH,1\n"))
   {
       QString msg = "Request failed!, could be a tune in process, only one channel ";
       msg += "can be tuned or retuned at a time. ";
       msgBox.setText(msg);
       msgBox.setInformativeText("");
       msgBox.setStandardButtons(QMessageBox::Ok);
       msgBox.exec();
   }
}

void Grid::AutoRetune(void)
{
   QMessageBox msgBox;

   ui->pbRetuneG1->setDown(false);
   QString msg = "This function will retune the RF head attached to channel. ";
   msg += "Make sure the RF head is attached and connected to your system as needed. ";
   msg += "This process can take up to 1 minute.\n";
   msgBox.setText(msg);
   msgBox.setInformativeText("Are you sure you want to continue?");
   msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
   msgBox.setDefaultButton(QMessageBox::Yes);
   int ret = msgBox.exec();
   if(ret == QMessageBox::No) return;
   if(!comms->SendCommand("RETUNERFCH,1\n"))
   {
       QString msg = "Request failed!, could be a tune in process, only one channel ";
       msg += "can be tuned or retuned at a time. ";
       msgBox.setText(msg);
       msgBox.setInformativeText("");
       msgBox.setStandardButtons(QMessageBox::Ok);
       msgBox.exec();
   }
}

void Grid::Updated(void)
{
    QObject* obj = sender();
    QString res;
    QMessageBox msgBox;
    static bool busy = false;

    if(Updating) return;
    if(!((QLineEdit *)obj)->isModified()) return;
    res = obj->objectName().mid(2).replace("_",",") + "," + ((QLineEdit *)obj)->text() + "\n";
    if(!comms->SendCommand(res.toStdString().c_str()))
    {
        if(!busy)
        {
           busy = true;
           QString msg = "Value rejected, likely out of range!";
           msgBox.setText(msg);
           msgBox.setInformativeText("");
           msgBox.setStandardButtons(QMessageBox::Ok);
           msgBox.exec();
        }
        busy = false;
        UpdateOff = false;
        ((QLineEdit *)obj)->setModified(false);
        return;
    }
    ((QLineEdit *)obj)->setModified(false);
    UpdateOff = false;
}

void Grid::Update(void)
{
    QString res;

    if(ui->tabGrid->tabText(ui->tabGrid->currentIndex()) != "Grid") return;
    using ::Comms;
    if(UpdateOff) return;
    Updating = true;
    QObjectList widgetList = ui->frmGRID->children();
    widgetList += ui->gbGrid1RF->children();
    foreach(QObject *w, widgetList)
    {
       if((w->objectName().startsWith("leS")) || (w->objectName().startsWith("leG")))
       {
            if(!((QLineEdit *)w)->hasFocus())
            {
               res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
               ((QLineEdit *)w)->setText(comms->SendMess(res));
            }
       }
    }
    res = comms->SendMess("GHVSTATUS,1\n");
    if(res == "ON") ui->chkGRID1enable->setChecked(true);
    else ui->chkGRID1enable->setChecked(false);
    res = comms->SendMess("GHVSTATUS,2\n");
    if(res == "ON") ui->chkGRID2enable->setChecked(true);
    else ui->chkGRID2enable->setChecked(false);
    // Read the mode, if error asume its manual mode
    res = comms->SendMess("GRFMODE,1\n");
    if(res == "AUTO")
    {
        ui->rbModeAuto->setChecked(true);
        ui->leSRFDRV_1->setEnabled(false);
        ui->leSRFVLT_1->setEnabled(true);
    }
    else
    {
        ui->rbModeManual->setChecked(true);
        ui->leSRFDRV_1->setEnabled(true);
        ui->leSRFVLT_1->setEnabled(false);
    }
    // Plot the voltage
    if(ui->chkEnablePlot->isChecked())
    {
        double key = time.elapsed()/1000.0;
        ui->Vplot->graph(0)->addData(key, ui->leGRFPPVP_1->text().toFloat());
        ui->Vplot->xAxis->setRangeUpper(key);
        if(ui->chkYauto->isChecked()) ui->Vplot->yAxis->rescale(true);
        ui->Vplot->replot();
    }
    // End of polot code
    Updating = false;
}

void Grid::SetYmax(void)
{
    ui->Vplot->yAxis->setRangeUpper(ui->leYmax->text().toFloat());
    ui->Vplot->replot();
}

void Grid::SetYmin(void)
{
    ui->Vplot->yAxis->setRangeLower(ui->leYmin->text().toFloat());
    ui->Vplot->replot();
}

void Grid::ResetPlot(void)
{
   time = QTime::currentTime();
   ui->Vplot->graph(0)->data()->clear();
   ui->Vplot->replot();
}

void Grid::YautoScale(void)
{
    if(ui->chkYauto->isChecked())
    {
        ui->Vplot->yAxis->rescale(true);
    }
    else
    {
        ui->Vplot->yAxis->rescale(false);
        ui->Vplot->yAxis->setRangeUpper(ui->leYmax->text().toFloat());
        ui->Vplot->yAxis->setRangeLower(ui->leYmin->text().toFloat());
    }
    ui->Vplot->replot();
}

void Grid::ModeChange(void)
{
    if(ui->rbModeAuto->isChecked())
    {
        comms->SendCommand("SRFMODE,1,AUTO\n");
        ui->leSRFDRV_1->setEnabled(false);
        ui->leSRFVLT_1->setEnabled(true);
    }
    if(ui->rbModeManual->isChecked())
    {
        comms->SendCommand("SRFMODE,1,MANUAL\n");
        ui->leSRFDRV_1->setEnabled(true);
        ui->leSRFVLT_1->setEnabled(false);
    }

}

void Grid::PulseSeqVarUpdate(void)
{
    ui->leTp->setText(QString::number(Tp));
    ui->leTs1->setText(QString::number(Ts1));
    ui->leTs2->setText(QString::number(Ts2));
    ui->leN->setText(QString::number(n));
    ui->comboSelectN->clear();
    for(int i=0; i<n; i++) ui->comboSelectN->addItem(QString::number(i+1));
    ui->leTdn->setText(QString::number(Tdn[0]));
    ui->leM->setText(QString::number(Repeat[0]));
    ui->leFullRepeat->setText(QString::number(SeqRepeat));
}

void Grid::PulseSeqVarChange(void)
{
    Tp = ui->leTp->text().toFloat();
    Ts1 = ui->leTs1->text().toFloat();
    Ts2 = ui->leTs2->text().toFloat();
    n = ui->leN->text().toInt();
    // Range testing
    if(n > 20) ui->leN->setText(QString::number(n = 20));
    if(n < 1) ui->leN->setText(QString::number(n = 1));
    if((Ts1 < 20) && (Ts1 != 0))ui->leTs1->setText(QString::number(Ts1 = 20));
    if((Ts2 < 20) && (Ts2 != 0)) ui->leTs2->setText(QString::number(Ts2 = 20));
    if(Tp < 1) ui->leTp->setText(QString::number(Tp = 1));
    Tdn[ui->comboSelectN->currentIndex()] = ui->leTdn->text().toFloat();
    Repeat[ui->comboSelectN->currentIndex()] = ui->leM->text().toFloat();
    SeqRepeat = ui->leFullRepeat->text().toInt();
    if(n != ui->comboSelectN->count())
    {
        ui->comboSelectN->clear();
        for(int i=0; i<n; i++)
        {
            ui->comboSelectN->addItem(QString::number(i+1));
        }
        ui->leTdn->setText(QString::number(Tdn[0]));
        ui->leM->setText(QString::number(Repeat[0]));
    }
}

void Grid::PulseSeqNselect(void)
{
    ui->leTdn->setText(QString::number(Tdn[ui->comboSelectN->currentIndex()]));
    ui->leM->setText(QString::number(Repeat[ui->comboSelectN->currentIndex()]));
}

void Grid::Download(void)
{
    QString res;

    ui->pbDownload->setDown(false);
    // Make sure system is in local mode
    comms->SendCommand("SMOD,LOC\n");
    // Enable tasks in table mode
    comms->SendCommand("TBLTSKENA,TRUE\n");
    // Set clock
    comms->SendCommand("STBLCLK," + QString::number(ClockFreq) + "\n");
    // Set trigger
    comms->SendCommand("STBLTRG,POS\n");
    // Send table
    comms->SendCommand(GenerateTable());
    // Put system in table mode
    comms->SendCommand("SMOD,TBL\n");
    comms->waitforline(100);
    sb->showMessage(comms->getline());
}

// MIPS digital outputs assignments:
//  A = Sync output
//  B = A, ADC trigger
//  C = Shutter 1
//  D = Shutter 2
// STBLDAT;0:[L:0,0:C:1,13:C:0:[A:5,0:A:1,13:A:0,5000:B:1,5013:B:0,13125:],0:[B:5,0:A:1,13:A:0,8000:B:1,8013:B:0,13125:],23:];
//
// Note! Logic 1 = shutter closed
QString Grid::GenerateTable(void)
{
   QString TableName;
   bool    SwapAD = false;

   if(ui->chkSwapAD->isChecked()) SwapAD = true;
   TableName = "A";
   // Initial table with sync pulse
   if(!SwapAD) Table = "STBLDAT;0:[L:" + QString::number((int)(SeqRepeat)) + "," + QString::number((int)(50 * ClockFreq / 1000000)) + ":A:1";
   else Table = "STBLDAT;0:[L:" + QString::number((int)(SeqRepeat)) + "," + QString::number((int)(50 * ClockFreq / 1000000)) + ":D:1";
   if(Ts1 == 0) Table                += ":C:0";    // Open shutter if not used
   if((Ts2 == 0) && (!SwapAD)) Table += ":D:0";    // Open shutter if not used
   if((Ts2 == 0) && (SwapAD))  Table += ":A:0";    // Open shutter if not used
   Table += ",";
   if(!SwapAD) Table += QString::number((int)(100 * ClockFreq / 1000000))  + ":A:0:";
   else Table += QString::number((int)(100 * ClockFreq / 1000000))  + ":D:0:";
   // Add each sequence to the table
   for(int i=0; i<n; i++)
   {
       if(i > 0) Table += ",0:";
       Table += "[" + TableName + ":" + QString::number(Repeat[i]) + ",0:B:1";
       if(Ts1 > 0) Table += ":C:0";
       if((ui->chkTs2eTs1->isChecked()) && (Ts1 > 0) && (!SwapAD)) Table += ":D:0";
       if((ui->chkTs2eTs1->isChecked()) && (Ts1 > 0) && (SwapAD))  Table += ":A:0";
       Table += ",";
       if(Ts1 == 0) Table += QString::number((int)(50 * ClockFreq / 1000000)) + ":B:0";
       else Table += QString::number((int)(Ts1 * ClockFreq / 1000000)) + ":B:0";
       if(Ts1 > 0) Table += ":C:1";
       if((ui->chkTs2eTs1->isChecked()) && (Ts1 > 0)) Table += ":D:1";
       Table += ",";
       if((Ts2 > 0) && (!SwapAD)) Table += QString::number((int)(Tdn[i] * ClockFreq / 1000)) + ":D:0";
       if((Ts2 > 0) && (SwapAD))  Table += QString::number((int)(Tdn[i] * ClockFreq / 1000)) + ":A:0";
       if((ui->chkTs1eTs2->isChecked()) && (Ts2 > 0)) Table += ":C:0";
       if(Ts2 > 0) Table += ",";
       if((Ts2 > 0) && (!SwapAD)) Table += QString::number((int)((Tdn[i] * ClockFreq / 1000) + Ts2 * ClockFreq / 1000000)) + ":D:1";
       if((Ts2 > 0) && (SwapAD))  Table += QString::number((int)((Tdn[i] * ClockFreq / 1000) + Ts2 * ClockFreq / 1000000)) + ":A:1";
       if((ui->chkTs1eTs2->isChecked()) && (Ts2 > 0)) Table += ":C:1";
       if(Ts2 > 0) Table += ",";
       Table += QString::number((int)(Tp * ClockFreq / 1000)) + ":]";
       TableName = TableName[0].unicode() + 1;
   }
   // Now Terminate the table command and its ready to send to MIPS!
   Table += "," + QString::number((int)(50 * ClockFreq / 1000000)) + ":];";
   return Table;
}

void Grid::ShowTable(void)
{
    QString table,dtable;

    ui->pbShow->setDown(false);
    table = GenerateTable();
    if(table.count()>60)
    {
        dtable = "";
        for(int i=0; i<table.count(); i+= 40)
        {
            dtable += table.mid(i,40);
            dtable += "\n";
        }
        table = dtable;
    }
    QMessageBox msgBox;
    msgBox.setText(table);
    msgBox.exec();
}

void Grid::StartTable(void)
{
    ui->pbStart->setDown(false);
    comms->SendCommand("TBLSTRT\n");
    comms->waitforline(100);
    sb->showMessage(comms->getline());
    comms->waitforline(500);
    sb->showMessage(sb->currentMessage() + " " + comms->getline());
    comms->waitforline(100);
    sb->showMessage(sb->currentMessage() + " " + comms->getline());
}

void Grid::Save(QString Filename)
{
    QString res;

    if(Filename == "") return;
    QFile file(Filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# Grid fragmentor settings, " + dateTime.toString() + "\n";
        // Save the pulse sequence generation variables
        stream << "Tp," << Tp << "\n";
        stream << "Ts1," << Ts1 << "\n";
        stream << "Ts2," << Ts2 << "\n";
        stream << "n," << n << "\n";
        stream << "Tdn";
        for(int i=0; i<20; i++) stream << "," << Tdn[i];
        stream << "\n";
        stream << "Repeat";
        for(int i=0; i<20; i++) stream << "," << Repeat[i];
        stream << "\n";
        stream << "SeqRepeat," << SeqRepeat << "\n";
        // Save the Grid parameters

        file.close();
        sb->showMessage("Settings saved to " + Filename,2000);
    }
}

void Grid::Load(QString Filename)
{
    QStringList resList;

    if(Filename == "") return;
    QFile file(Filename);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        // We're going to streaming the file
        // to the QString
        QTextStream stream(&file);
        QString line;
        do
        {
            line = stream.readLine();
            resList = line.split(",");
            if(resList.count() >= 2)
            {
                if(resList[0] == "Tp") Tp = resList[1].toFloat();
                if(resList[0] == "Ts1") Ts1 = resList[1].toFloat();
                if(resList[0] == "Ts2") Ts2 = resList[1].toFloat();
                if(resList[0] == "n") n = resList[1].toInt();
                if((resList[0] == "Tdn") && (resList.count() >= 21)) for(int i=0; i<20; i++) Tdn[i] = resList[i+1].toFloat();
                if((resList[0] == "Repeat") && (resList.count() >= 21)) for(int i=0; i<20; i++) Repeat[i] = resList[i+1].toFloat();
                if(resList[0] == "SeqRepeat") SeqRepeat = resList[1].toInt();
            }
        } while(!line.isNull());
        file.close();
        PulseSeqVarUpdate();
        sb->showMessage("Settings loaded from " + Filename,2000);
    }
}
