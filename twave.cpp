//
// This class supports the Twave dialog options on the Twave tab.
//
#include "Twave.h"

namespace Ui {
class MIPS;
}

Twave::Twave(Ui::MIPS *w, Comms *c)
{
   tui = w;
   comms = c;

   NumChannels = 2;
   Compressor = true;
   QObjectList widgetList = tui->gbTwaveCH1->children();
   widgetList += tui->gbTwaveCH2->children();
   widgetList += tui->tabCompressor->children();
   widgetList += tui->gbTiming->children();
   widgetList += tui->gbTWsweepCH1->children();
   widgetList += tui->gbTWsweepCH2->children();
   foreach(QObject *w, widgetList)
   {
      if(w->objectName().contains("le"))
      {
           //((QLineEdit *)w)->setValidator(new QDoubleValidator);
           connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(Changed()));
      }
   }
   connect(tui->rbSTWCMODE_COMPRESS,SIGNAL(clicked(bool)),this,SLOT(rbModeCompress()));
   connect(tui->rbSTWCMODE_NORMAL,SIGNAL(clicked(bool)),this,SLOT(rbModeNormal()));
   connect(tui->rbSTWCSW_CLOSE,SIGNAL(clicked(bool)),this,SLOT(rbSwitchClose()));
   connect(tui->rbSTWCSW_OPEN,SIGNAL(clicked(bool)),this,SLOT(rbSwitchOpen()));
   connect(tui->pbTwaveUpdate,SIGNAL(pressed()),this,SLOT(pbUpdate()));
   connect(tui->pbTwaveForceTrigger,SIGNAL(pressed()),this,SLOT(pbForceTrigger()));
   connect(tui->rbSTWDIR_1_FWD,SIGNAL(clicked(bool)),this,SLOT(rbTW1fwd()));
   connect(tui->rbSTWDIR_1_REV,SIGNAL(clicked(bool)),this,SLOT(rbTW1rev()));
   connect(tui->rbSTWDIR_2_FWD,SIGNAL(clicked(bool)),this,SLOT(rbTW2fwd()));
   connect(tui->rbSTWDIR_2_REV,SIGNAL(clicked(bool)),this,SLOT(rbTW2rev()));
   // Buttons for sweep start and stop
   connect(tui->pbTWsweepCH1start,SIGNAL(pressed()),this,SLOT(pbTWsweepStart()));
   connect(tui->pbTWsweepCH2start,SIGNAL(pressed()),this,SLOT(pbTWsweepStart()));
   connect(tui->pbTWsweepStart,SIGNAL(pressed()),this,SLOT(pbTWsweepStart()));
   connect(tui->pbTWsweepCH1stop,SIGNAL(pressed()),this,SLOT(pbTWsweepStop()));
   connect(tui->pbTWsweepCH2stop,SIGNAL(pressed()),this,SLOT(pbTWsweepStop()));
   connect(tui->pbTWsweepStop,SIGNAL(pressed()),this,SLOT(pbTWsweepStop()));
   connect(tui->chkSweepExtTrig,SIGNAL(clicked(bool)),this,SLOT(SweepExtTrigger()));

}

void Twave::SweepExtTrigger(void)
{
    if(tui->chkSweepExtTrig->isChecked())
    {
        comms->SendCommand("SDTRIGDLY,0\n");
        comms->SendCommand("SDTRIGPRD,10000\n");
        comms->SendCommand("SDTRIGRPT,1\n");
        comms->SendCommand("SDTRIGMOD,SWEEP\n");
        comms->SendCommand("SDTRIGINP,R,POS\n");
        comms->SendCommand("SDTRIGENA,TRUE\n");
    }
    else
    {
        comms->SendCommand("SDTRIGENA,FALSE\n");
    }
}

// This function saves all the setable parameters to data file.
void Twave::Save(QString Filename)
{
    QString res;

    if(NumChannels <= 0) return;
    if(Filename == "") return;
    QFile file(Filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# DCbias settings, " + dateTime.toString() + "\n";

        QObjectList widgetList = tui->gbTwaveCH1->children();
        if(NumChannels > 1) widgetList += tui->gbTwaveCH2->children();
        if(Compressor)
        {
            widgetList += tui->tabCompressor->children();
            widgetList += tui->gbTWsweepCH1->children();
            widgetList += tui->gbTWsweepCH2->children();
            widgetList += tui->gbTiming->children();
            widgetList += tui->gbMode->children();
            widgetList += tui->gbSwitch->children();
        }
        foreach(QObject *w, widgetList)
        {
            if(w->objectName().mid(0,3) == "leS")
            {
                res = w->objectName() + "," + ((QLineEdit *)w)->text() + "\n";
                stream << res;
            }
            if(w->objectName().mid(0,3) == "rbS")
            {
                res = w->objectName() + ",";
                if(((QRadioButton *)w)->isChecked()) res += "true\n";
                else res += "false\n";
                stream << res;
            }
        }
        file.close();
        tui->statusBar->showMessage("Settings saved to " + Filename,2000);
    }
}

void Twave::Load(QString Filename)
{
    QStringList resList;

    if(NumChannels <= 0) return;
    if(Filename == "") return;
    QObjectList widgetList = tui->gbTwaveCH1->children();
    if(NumChannels >1) widgetList += tui->gbTwaveCH2->children();
    if(Compressor)
    {
        widgetList += tui->tabCompressor->children();
        widgetList += tui->gbTWsweepCH1->children();
        widgetList += tui->gbTWsweepCH2->children();
        widgetList += tui->gbTiming->children();
        widgetList += tui->gbMode->children();
        widgetList += tui->gbSwitch->children();
    }
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
                    if(w->objectName().mid(0,3) == "rbS")
                    {
                        if(w->objectName() == resList[0])
                        {
                            if(resList[1] == "true")
                            {
                                ((QRadioButton *)w)->setChecked(true);
                                QMetaObject::invokeMethod(w, "clicked");
                            }
                            else ((QRadioButton *)w)->setChecked(false);
                        }
                    }
                }

            }
        } while(!line.isNull());
        file.close();
        tui->statusBar->showMessage("Settings loaded from " + Filename,2000);
    }
}

void Twave::Update(void)
{
    QString res;

    switch(NumChannels)
    {
       case 0:
          tui->gbTwaveCH1->setEnabled(false);
          tui->gbTwaveCH2->setEnabled(false);
          tui->tabCompressor->setEnabled(false);
          tui->tabSweep->setEnabled(false);
          return;
       case 1:
          tui->gbTwaveCH1->setEnabled(true);
          tui->gbTwaveCH2->setEnabled(false);
          tui->tabCompressor->setEnabled(false);
          tui->tabSweep->setEnabled(true);
       case 2:
          tui->gbTwaveCH1->setEnabled(true);
          tui->gbTwaveCH2->setEnabled(true);
          if(Compressor) tui->tabCompressor->setEnabled(true);
          tui->tabSweep->setEnabled(true);
       default:
          break;
    }
    tui->tabMIPS->setEnabled(false);
    tui->statusBar->showMessage(tr("Updating Twave IO controls..."));
    QObjectList widgetList = tui->gbTwaveCH1->children();
    if(NumChannels > 1) widgetList += tui->gbTwaveCH2->children();
    widgetList += tui->gbTWsweepCH1->children();
    widgetList += tui->gbTWsweepCH2->children();
    if(Compressor)
    {
       widgetList += tui->tabCompressor->children();
       widgetList += tui->gbTiming->children();
    }
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("le"))
       {
            res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
            ((QLineEdit *)w)->setText(comms->SendMess(res));
            comms->rb.waitforline(1);
            res = comms->rb.getline();
       }
    }
    res = comms->SendMess("GTWDIR,1\n");
    if(res == "FWD") tui->rbSTWDIR_1_FWD->setChecked(true);
    if(res == "REV") tui->rbSTWDIR_1_REV->setChecked(true);
    res = comms->SendMess("GTWDIR,2\n");
    if(res == "FWD") tui->rbSTWDIR_2_FWD->setChecked(true);
    if(res == "REV") tui->rbSTWDIR_2_REV->setChecked(true);
    if(Compressor)
    {
        res = comms->SendMess("GTWCMODE\n");
        if(res == "Normal") tui->rbSTWCMODE_NORMAL->setChecked(true);
        if(res == "Compress") tui->rbSTWCMODE_COMPRESS->setChecked(true);
        res = comms->SendMess("GTWCSW\n");
        if(res == "Open") tui->rbSTWCSW_OPEN->setChecked(true);
        if(res == "Close") tui->rbSTWCSW_CLOSE->setChecked(true);
    }
    tui->tabMIPS->setEnabled(true);
    tui->statusBar->showMessage(tr(""));
}

void Twave::Changed(void)
{
    QObject* obj = sender();
    QString res;

    if(!((QLineEdit *)obj)->isModified()) return;
    res = obj->objectName().mid(2).replace("_",",") + "," + ((QLineEdit *)obj)->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    ((QLineEdit *)obj)->setModified(false);
}

void Twave::rbModeCompress(void)
{
    comms->SendCommand("STWCMODE,Compress\n");
}

void Twave::rbModeNormal(void)
{
    comms->SendCommand("STWCMODE,Normal\n");
}

void Twave::rbSwitchClose(void)
{
   comms->SendCommand("STWCSW,Close\n");
}

void Twave::rbSwitchOpen(void)
{
   comms->SendCommand("STWCSW,Open\n");
}

void Twave::rbTW1fwd(void)
{
   comms->SendCommand("STWDIR,1,FWD\n");
}

void Twave::rbTW1rev(void)
{
    comms->SendCommand("STWDIR,1,REV\n");
}

void Twave::rbTW2fwd(void)
{
    comms->SendCommand("STWDIR,2,FWD\n");
}

void Twave::rbTW2rev(void)
{
    comms->SendCommand("STWDIR,2,REV\n");
}

void Twave::pbUpdate(void)
{
    Update();
}

void Twave::pbForceTrigger(void)
{
   comms->SendCommand("TWCTRG\n");
}

void Twave::pbTWsweepStart(void)
{
   QObject *senderObj = sender();
   QString senderObjName = senderObj->objectName();

   if(senderObjName == "pbTWsweepCH1start")
   {
       comms->SendCommand("STWSGO,1\n");
   }
   if(senderObjName == "pbTWsweepCH2start")
   {
       comms->SendCommand("STWSGO,2\n");
   }
   if(senderObjName == "pbTWsweepStart")
   {
       comms->SendCommand("STWSGO,3\n");
   }
}

void Twave::pbTWsweepStop(void)
{
    QObject *senderObj = sender();
    QString senderObjName = senderObj->objectName();

    if(senderObjName == "pbTWsweepCH1stop")
    {
        comms->SendCommand("STWSHLT,1\n");
    }
    if(senderObjName == "pbTWsweepCH2stop")
    {
        comms->SendCommand("STWSHLT,2\n");
    }
    if(senderObjName == "pbTWsweepStop")
    {
        comms->SendCommand("STWSHLT,3\n");
    }
}
