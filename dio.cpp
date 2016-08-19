#include "dio.h"

DIO::DIO(Ui::MIPS *w, Comms *c)
{
    dui = w;
    comms = c;

    QObjectList widgetList = dui->gbDigitalOut->children();
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
}

void DIO::Update(void)
{
    QString res;

    dui->tabMIPS->setEnabled(false);
    dui->statusBar->showMessage(tr("Updating Digital IO controls..."));
    QObjectList widgetList = dui->gbDigitalOut->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("chk"))
       {
            res = "G" + w->objectName().mid(4).replace("_",",") + "\n";
            if(comms->SendMessage(res).toInt()==1) ((QCheckBox *)w)->setChecked(true);
            else ((QCheckBox *)w)->setChecked(false);
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

