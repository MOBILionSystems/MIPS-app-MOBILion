#include "dio.h"

DIO::DIO(Ui::MIPS *w, Comms *c)
{
    dui = w;
    comms = c;

    dui->pbUPsmall->setText(QChar(0xB4, 0x25));
    dui->pbUPlarge->setText(QChar(0xB2, 0x25));
    dui->pbDOWNsmall->setText(QChar(0xBE, 0x25));
    dui->pbDOWNlarge->setText(QChar(0xBC, 0x25));
    QObjectList widgetList = dui->gbDigitalOut->children();
    widgetList += dui->gbDigitalIn->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("chk"))
       {
            connect(((QCheckBox *)w),SIGNAL(stateChanged(int)),this,SLOT(DOUpdated()));
       }
    }
    connect(dui->pbDIOupdate,SIGNAL(pressed()),this,SLOT(UpdateDIO()));
    connect(dui->pbTrigHigh,SIGNAL(pressed()),this,SLOT(TrigHigh()));
    connect(dui->pbTrigLow,SIGNAL(pressed()),this,SLOT(TrigLow()));
    connect(dui->pbTrigPulse,SIGNAL(pressed()),this,SLOT(TrigPulse()));
    connect(dui->pbRFgenerate,SIGNAL(pressed()),this,SLOT(RFgenerate()));
    connect(dui->leSFREQ,SIGNAL(editingFinished()),this,SLOT(SetFreq()));
    connect(dui->leSWIDTH,SIGNAL(editingFinished()),this,SLOT(SetWidth()));
    connect(dui->chkRemoteNav,SIGNAL(stateChanged(int)),this,SLOT(RemoteNavigation()));
    connect(dui->pbUPsmall,SIGNAL(pressed()),this,SLOT(RemoteNavSmallUP()));
    connect(dui->pbUPlarge,SIGNAL(pressed()),this,SLOT(RemoteNavLargeUP()));
    connect(dui->pbDOWNsmall,SIGNAL(pressed()),this,SLOT(RemoteNavSmallDown()));
    connect(dui->pbDOWNlarge,SIGNAL(pressed()),this,SLOT(RemoteNavLargeDown()));
    connect(dui->pbSelect,SIGNAL(pressed()),this,SLOT(RemoteNavSelect()));
}

void DIO::Update(void)
{
    QString res;

    dui->tabMIPS->setEnabled(false);
    dui->statusBar->showMessage(tr("Updating Digital IO controls..."));
    QObjectList widgetList = dui->gbDigitalOut->children();
    widgetList += dui->gbFreqGen->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("chk"))
       {
            res = "G" + w->objectName().mid(4).replace("_",",") + "\n";
            if(comms->SendMessage(res).toInt()==1) ((QCheckBox *)w)->setChecked(true);
            else ((QCheckBox *)w)->setChecked(false);
       }
       if(w->objectName().contains("le"))
       {
            res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
            ((QLineEdit *)w)->setText(comms->SendMessage(res));
       }
    }
    widgetList = dui->gbDigitalIn->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("chk"))
       {
            res = "G" + w->objectName().mid(4).replace("_",",") + "\n";
            if(comms->SendMessage(res).toInt()==1) ((QCheckBox *)w)->setChecked(true);
            else ((QCheckBox *)w)->setChecked(false);
       }
    }
    dui->tabMIPS->setEnabled(true);
    dui->statusBar->showMessage(tr(""));
}

void DIO::UpdateDIO(void)
{
    Update();
}

// Slot for Digital output check box selection
void DIO::DOUpdated(void)
{
    QObject* obj = sender();
    QString res;

    res = obj->objectName().mid(3).replace("_",",") + ",";
    if(((QCheckBox *)obj)->checkState()) res += "1\n";
    else res+= "0\n";
    comms->SendCommand(res.toStdString().c_str());
}

// Slot for Trigger high pushbutton
void DIO::TrigHigh(void)
{
    comms->SendCommand("TRIGOUT,HIGH\n");
}

// Slot for Trigger low pushbutton
void DIO::TrigLow(void)
{
    comms->SendCommand("TRIGOUT,LOW\n");
}

// Slot for Trigger pulse pushbutton
void DIO::TrigPulse(void)
{
    comms->SendCommand("TRIGOUT,PULSE\n");
}

// Slot for RF generate pushbutton
void DIO::RFgenerate(void)
{
    QString res;

    res = "BURST," + dui->Burst->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
}

void DIO::SetFreq(void)
{
    QString res;

    if(!(dui->leSFREQ->isModified())) return;
    res = dui->leSFREQ->objectName().mid(2).replace("_",",") + "," + dui->leSFREQ->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    dui->leSFREQ->setModified(false);
}

void DIO::SetWidth(void)
{
    QString res;

    if(!(dui->leSWIDTH->isModified())) return;
    res = dui->leSWIDTH->objectName().mid(2).replace("_",",") + "," + dui->leSWIDTH->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    dui->leSWIDTH->setModified(false);
}


void DIO::Save(QString Filename)
{
    QString res;

    if(Filename == "") return;
    QFile file(Filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# DIO settings, " + dateTime.toString() + "\n";
        QObjectList widgetList = dui->gbDigitalOut->children();
        foreach(QObject *w, widgetList)
        {
            if(w->objectName().mid(0,4) == "chkS")
            {
                res = w->objectName() + ",";
                if(((QCheckBox *)w)->checkState()) res += "true\n";
                else res += "false\n";
                stream << res;
            }
        }
        file.close();
        dui->statusBar->showMessage("Settings saved to " + Filename,2000);
    }
}

void DIO::Load(QString Filename)
{
    QStringList resList;

    if(Filename == "") return;
    QObjectList widgetList = dui->gbDigitalOut->children();
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
                    if(w->objectName().mid(0,4) == "chkS")
                    {
                        if(w->objectName() == resList[0])
                        {
                            if(resList[1] == "true")
                            {
                                ((QCheckBox *)w)->setChecked(true);
                            }
                            else ((QCheckBox *)w)->setChecked(false);
                        }
                    }
                }

            }
        } while(!line.isNull());
        file.close();
        dui->statusBar->showMessage("Settings loaded from " + Filename,2000);
    }
}

// Following slot function support remote navication of UI
void DIO::RemoteNavigation(void)
{
   if(dui->chkRemoteNav->isChecked())
   {
       if(comms->SendCommand("SSERIALNAV,TRUE\n"))
       {
           dui->pbUPsmall->setEnabled(true);
           dui->pbUPlarge->setEnabled(true);
           dui->pbDOWNsmall->setEnabled(true);
           dui->pbDOWNlarge->setEnabled(true);
           dui->pbSelect->setEnabled(true);
           return;
       }
   }
   dui->pbUPsmall->setEnabled(false);
   dui->pbUPlarge->setEnabled(false);
   dui->pbDOWNsmall->setEnabled(false);
   dui->pbDOWNlarge->setEnabled(false);
   dui->pbSelect->setEnabled(false);
}

void DIO::RemoteNavSmallUP(void)
{
    char str[2];

    str[0] = 30;
    str[1] = 0;
    comms->SendString(QString::fromStdString(str));
}

void DIO::RemoteNavLargeUP(void)
{
    char str[2];

    str[0] = 31;
    str[1] = 0;
    comms->SendString(QString::fromStdString(str));
}

void DIO::RemoteNavSmallDown(void)
{
    char str[2];

    str[0] = 28;
    str[1] = 0;
    comms->SendString(QString::fromStdString(str));
}

void DIO::RemoteNavLargeDown(void)
{
    char str[2];

    str[0] = 29;
    str[1] = 0;
    comms->SendString(QString::fromStdString(str));
}

void DIO::RemoteNavSelect(void)
{
   char str[2];

   str[0] = 9;
   str[1] = 0;
   comms->SendString(QString::fromStdString(str));
}
