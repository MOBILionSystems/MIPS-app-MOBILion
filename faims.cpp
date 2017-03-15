#include "faims.h"
#include <QElapsedTimer>

QElapsedTimer eTimer;

int key;

FAIMS::FAIMS(Ui::MIPS *w, Comms *c)
{
    fui = w;
    comms = c;

    fui->comboFMtrig->clear();
    fui->comboFMtrig->addItem("None");
    fui->comboFMtrig->addItem("Q");
    fui->comboFMtrig->addItem("R");
    fui->comboFMtrig->addItem("S");
    fui->comboFMtrig->addItem("T");
    CVparkingTriggered = false;
    QObjectList widgetList = fui->gbFAIMS_DC->children();
    widgetList += fui->gbFAIMS_RF->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().startsWith("leS"))
       {
            ((QLineEdit *)w)->setValidator(new QDoubleValidator);
            connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(FAIMSUpdated()));
       }
    }
    connect(fui->chkFMenable,SIGNAL(toggled(bool)),this,SLOT(FAIMSenable()));
    connect(fui->pbFMstart,SIGNAL(pressed()),this,SLOT(FAIMSscan()));
    connect(fui->pbFMloadCSV,SIGNAL(pressed()),this,SLOT(FAIMSloadCSV()));
    eTimer.start();
}

void FAIMS::Save(QString Filename)
{

}

void FAIMS::Load(QString Filename)
{

}

void FAIMS::Update(void)
{
    QString res;

    QObjectList widgetList = fui->gbFAIMS_RF->children();
    widgetList += fui->gbFAIMS_DC->children();
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
}

void FAIMS::FAIMSUpdated(void)
{
   QObject* obj = sender();
   QString res;

   if(!((QLineEdit *)obj)->isModified()) return;
   res = obj->objectName().mid(2).replace("_",",") + "," + ((QLineEdit *)obj)->text() + "\n";
   comms->SendCommand(res.toStdString().c_str());
   ((QLineEdit *)obj)->setModified(false);
}

void FAIMS::FAIMSenable(void)
{
    if(fui->chkFMenable->isChecked()) comms->SendCommand("SFMENA,TRUE\n");
    else  comms->SendCommand("SFMENA,FALSE\n");
}

void FAIMS::FAIMSscan(void)
{
   fui->leFMcompound->setText("");
   fui->leFMcvVolts->setText("");
   fui->leFMbiasVolts->setText("");
   if((CVparkingTriggered) || (fui->pbFMstart->text() == "Stop"))
   {
       CVparkingTriggered = false;
       fui->pbFMstart->setText("Start");
       fui->leFMstatus->setText("Stopped");
   }
   else
   {
       // Turn off all trigger reporting, just in case it was on
       comms->SendCommand("RPT,Q,OFF\n");
       comms->SendCommand("RPT,R,OFF\n");
       comms->SendCommand("RPT,S,OFF\n");
       comms->SendCommand("RPT,T,OFF\n");
       fui->pbFMstart->setText("Stop");
       if(fui->comboFMtrig->currentText() == "None")
       {
           CVparkingTriggered = true;
           fui->leFMstatus->setText("Running");
           eTimer.restart();
       }
       else
       {
           // Issue the report command to MIPS for the selected channel
           // and set status to waiting for trigger
           fui->leFMstatus->setText("Waiting for trigger");
           comms->SendCommand("RPT," + fui->comboFMtrig->currentText() + ",RISING\n");
       }
       CurrentPoint = 0;
       if(!GetNextTarget(0))
       {
           CVparkingTriggered = false;
           fui->pbFMstart->setText("Start");
           fui->leFMstatus->setText("Finished");
       }
   }
}

int FAIMS::getHeaderIndex(QString Name)
{
    QStringList recordList;

    QStringList resList = Header.split(",");
    for(int i=0;i<resList.count();i++)
    {
        if(resList.at(i).startsWith(Name)) return(i);
    }
   return(-1);
}

QString FAIMS::getCSVtoken(QString Name, int index)
{
    QStringList recordList;

    QStringList resList = Header.split(",");
    for(int i=0;i<resList.count();i++)
    {
        if(resList.at(i).startsWith(Name))
        {
            recordList = Records.at(index).split(",");
            if(recordList.count() > i) return(recordList.at(i));
        }
    }
   return("");
}

bool startTime(const QString v1, const QString v2)
 {
    QStringList v1List;
    QStringList v2List;

    v1List = v1.split(",");
    v2List = v2.split(",");
    return (v1List.at(key).toFloat()-v1List.at(key+1).toFloat()/2) < (v2List.at(key).toFloat()-v2List.at(key+1).toFloat()/2);
 }

void FAIMS::FAIMSloadCSV(void)
{
    QStringList resList;

    QString fileName = QFileDialog::getOpenFileName(this, tr("Load CSV target file"),"",tr("CSV (*.csv);;All files (*.*)"));
    if(fileName == "") return;
    QFile file(fileName);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        // We're going to streaming the file
        // to the QString
        QTextStream stream(&file);

        QString line;
        Header = stream.readLine();     // Read the CSV header, assume its the first line in the file
        Records.clear();
        do
        {
            line = stream.readLine();
            if(line != "") Records.append(line);
        } while(!line.isNull());
        file.close();
        // Sort the records by the start time, start time = RT - (RT windows) / 2
        key = getHeaderIndex("Retention Time");
        qSort(Records.begin(),Records.end(),startTime);
        // For each string in record report the following from the CVS file:
        // Compound, Retention Time, RT Window, Compensation Voltage, Bias
        for(int i=0;i<Records.count();i++)
        {
            QString rec = getCSVtoken("Compound",i) + ",";
            rec += getCSVtoken("Retention Time",i) + ",";
            rec += getCSVtoken("RT Window",i) + ",";
            rec += getCSVtoken("Compensation Voltage",i) + ",";
            rec += getCSVtoken("Bias",i);
            fui->textCVparkList->append(rec);
        }
        fui->statusBar->showMessage("CSV target file read " + fileName,2000);
    }
}

bool FAIMS::GetNextTarget(float et)
{
    QStringList entries;

    if(CurrentPoint == 0)
    {
        QString text = fui->textCVparkList->toPlainText();
        Parks = text.split( "\n" );
    }
    while(1)
    {
        while(1)
        {
            if((CurrentPoint+1) > Parks.count()) return false;
            entries = Parks.at(CurrentPoint).split(",");
            if(entries.count() == 5) break;
            CurrentPoint++;
        }
        if((entries.at(1).toFloat() + entries.at(2).toFloat()/2) > et)
        {
            TargetCompound = entries.at(0);
            TargetRT = entries.at(1).toFloat();
            TargetWindow = entries.at(2).toFloat();
            TargetCV = entries.at(3).toFloat();
            TargetBias = entries.at(4).toFloat();
            CurrentPoint++;
            State = WAITING;
            return true;
        }
        else CurrentPoint++;
    }
}

void FAIMS::PollLoop(void)
{
   float CurrentMin;
   QString res;
   QStringList reslist;

   if(fui->leFMstatus->text() == "Waiting for trigger")
   {
       // If we received a trigger report then change status to running and
       // set the triggered flag
       // Looking for "DIC,x,RISING,time. where x is the input trigger
       // selected.
       res = comms->getline();
       if(res.startsWith("DIC,"))
       {
           reslist = res.split(",");
           if(reslist.count() == 4)
           {
               if((reslist.at(2) == "RISING") && (reslist.at(1) == fui->comboFMtrig->currentText()))
               {
                   CVparkingTriggered = true;
                   fui->leFMstatus->setText("Triggered");
                   eTimer.restart();
               }
           }
       }
       return;
   }
   // If this FAIMS tab is displayed then update the parameters
   if( fui->tabMIPS->tabText(fui->tabMIPS->currentIndex()) == "FAIMS")
   {
      Update();
   }
   if(!CVparkingTriggered) return;
   CurrentMin = (float)eTimer.elapsed()/(float)60000;
   fui->leFMelasped->setText(QString::number(CurrentMin, 'f', 1));
   switch(State)
   {
      case WAITING:
       if(CurrentMin >= TargetRT - TargetWindow/2)
       {
           fui->leFMcompound->setText(TargetCompound);
           fui->leFMcvVolts->setText(QString::number(TargetCV));
           fui->leFMbiasVolts->setText(QString::number(TargetBias));
           comms->SendCommand("SFMCV," + QString::number(TargetCV) + "\n");
           comms->SendCommand("SFMBIAS," + QString::number(TargetBias) + "\n");
           State = SCANNING;
       }
       break;
      case SCANNING:
       if(CurrentMin >= TargetRT + TargetWindow/2)
       {
           if(!GetNextTarget(CurrentMin))
           {
               CVparkingTriggered = false;
               fui->pbFMstart->setText("Start");
               fui->leFMstatus->setText("Finished");
           }
           if(CurrentMin < TargetRT - TargetWindow/2) fui->leFMcompound->setText("");
       }
       break;
      default:
        break;
   }
}
