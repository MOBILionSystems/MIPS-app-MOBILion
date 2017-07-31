#include "grid.h"
#include "ui_grid.h"
#include <QDebug>
#include <QKeyEvent>

Grid::Grid(QWidget *parent, Comms *c, QStatusBar *statusbar) :
    QDialog(parent),
    ui(new Ui::Grid)
{
    ui->setupUi(this);

    comms = c;
    sb = statusbar;
    Updating = false;
    UpdateOff = false;
    this->setFixedSize(496,411);

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
}

Grid::~Grid()
{
    delete ui;
}

void Grid::reject()
{
    emit DialogClosed();
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

    if(UpdateOff) return;
    Updating = true;
    QObjectList widgetList = ui->frmGRID->children();
    widgetList += ui->gbGrid1RF->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().startsWith("le"))
       {
            if(!((QLineEdit *)w)->hasFocus())
            {
               res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
               ((QLineEdit *)w)->setText(comms->SendMessage(res));
            }
       }
    }
    res = comms->SendMessage("GHVSTATUS,1\n");
    if(res == "ON") ui->chkGRID1enable->setChecked(true);
    else ui->chkGRID1enable->setChecked(false);
    res = comms->SendMessage("GHVSTATUS,2\n");
    if(res == "ON") ui->chkGRID2enable->setChecked(true);
    else ui->chkGRID2enable->setChecked(false);
    Updating = false;
}
