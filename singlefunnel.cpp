#include "singlefunnel.h"
#include "ui_singlefunnel.h"
#include <QCloseEvent>
#include <QDebug>

SingleFunnel::SingleFunnel(QWidget *parent, Comms *c, QStatusBar *statusbar) :
    QDialog(parent),
    ui(new Ui::SingleFunnel)
{
    ui->setupUi(this);

    comms = c;
    sb = statusbar;
    this->setFixedSize(599,441);

    QObjectList widgetList = ui->frmSF->children();
    foreach(QObject *w, widgetList)
    {
        if(w->objectName().contains("leS"))
        {
            ((QLineEdit *)w)->setValidator(new QDoubleValidator);
            connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(SFUpdated()));
        }
    }
    connect(ui->chkESIenable,SIGNAL(toggled(bool)),this,SLOT(ESIenable()));
    connect(ui->pbAutoTune,SIGNAL(pressed()),this,SLOT(AutoTune()));
    connect(ui->pbAutoRetune,SIGNAL(pressed()),this,SLOT(AutoRetune()));
}

void SingleFunnel::reject()
{
    emit DialogClosed();
}

SingleFunnel::~SingleFunnel()
{
    delete ui;
}

bool SingleFunnel::ConfigurationCheck(void)
{
    QString res;
    int     nRF,nDC,nESI;
    QMessageBox msgBox;

    // If we are connected to MIPS test to make sure we have the following modules:
    //  1 - RF drivers
    //  1 - DC bias modules
    //  1 - ESI module
    // Generate error message if not present
    if(comms->isConnected())
    {
       // Test for required modules
       res = comms->SendMessage("GCHAN,RF\n");
       nRF = res.toInt()/2;
       res = comms->SendMessage("GCHAN,DCB\n");
       nDC = res.toInt()/8;
       res = comms->SendMessage("GCHAN,ESI\n");
       nESI = res.toInt();
       if((nRF < 1) || (nDC < 1) || (nESI < 1))
       {
           QString msg = "Invalid configuration!\n";
           msg += "   Required 1 RF modules, found " + QString::number(nRF) + "\n";
           msg += "   Required 1 DCbias modules, found " + QString::number(nDC) + "\n";
           msg += "   Required 1 ESI module, found " + QString::number(nESI) + "\n";
           msgBox.setText(msg);
           msgBox.setInformativeText("Are you sure you want to continue?");
           msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
           msgBox.setDefaultButton(QMessageBox::No);
           int ret = msgBox.exec();
           if(ret == QMessageBox::No)
           {
               return false;
           }
       }
    }
    return true;
}

void SingleFunnel::Save(QString Filename)
{
    QString res;

    if(Filename == "") return;
    QFile file(Filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# Single Funnel System settings, " + dateTime.toString() + "\n";
        QObjectList widgetList = ui->frmSF->children();
        foreach(QObject *w, widgetList)
        {
            if(w->objectName().mid(0,3) == "leS")
            {
                res = w->objectName() + "," + ((QLineEdit *)w)->text() + "\n";
                stream << res;
            }
        }
        if(ui->chkESIenable->isChecked()) stream << "chkESIenable,TRUE\n";
        else stream << "chkESIenable,FALSE\n";
        file.close();
        sb->showMessage("Settings saved to " + Filename,2000);
    }
}

void SingleFunnel::Load(QString Filename)
{
    QStringList resList;

    if(Filename == "") return;
    QObjectList widgetList = ui->frmSF->children();
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
            if(resList.count() == 2)
            {
                foreach(QObject *w, widgetList)
                {
                    if(w->objectName().mid(0,3) == "leS")
                    {
                        if(resList[1] != "") if(w->objectName() == resList[0])
                        {
                            ((QLineEdit *)w)->setText(resList[1]);
                            ((QLineEdit *)w)->setModified(true);
                            QMetaObject::invokeMethod(w, "editingFinished");
                        }
                    }
                }
                if(resList[0] == "chkESIenable")
                {
                    if(resList[1] == "TRUE") ui->chkESIenable->setChecked(true);
                    else ui->chkESIenable->setChecked(false);
                    QMetaObject::invokeMethod(ui->chkESIenable, "toggled");
                }
            }
        } while(!line.isNull());
        file.close();
        sb->showMessage("Settings loaded from " + Filename,2000);
    }
}

void SingleFunnel::AutoTune(void)
{
   QMessageBox msgBox;

   ui->pbAutoTune->setDown(false);
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

void SingleFunnel::AutoRetune(void)
{
   QMessageBox msgBox;

   ui->pbAutoRetune->setDown(false);
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

void SingleFunnel::ESIenable(void)
{
    if(ui->chkESIenable->isChecked()) comms->SendCommand("SHVENA,1\n");
    else  comms->SendCommand("SHVDIS,1\n");
}

void SingleFunnel::SFUpdated(void)
{
    QObject* obj = sender();
    QString res;

    if(!((QLineEdit *)obj)->isModified()) return;
    res = obj->objectName().mid(2).replace("_",",") + "," + ((QLineEdit *)obj)->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    ((QLineEdit *)obj)->setModified(false);
}

void SingleFunnel::Update(void)
{
    QString res;

    QObjectList widgetList = ui->frmSF->children();
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
    if(res == "ON") ui->chkESIenable->setChecked(true);
    else ui->chkESIenable->setChecked(false);
}
