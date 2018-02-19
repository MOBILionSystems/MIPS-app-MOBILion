#include "arb.h"
#include "help.h"

#include <QProcess>
#include <qthread.h>

namespace Ui {
class MIPS;
}

ARB::ARB(Ui::MIPS *w, Comms *c)
{
    aui = w;
    comms = c;

    LogedData = new Help();
//NumChannels = 32;
    Compressor = true;
    // Setup the module selection combo box
    aui->comboARBmodule->clear();
    for(int i=0;i<4;i++)
    {
        if(((i+1) * 8) <= NumChannels) aui->comboARBmodule->addItem(QString::number(i+1));
    }
    // Setup the waveform type combo box
    aui->comboSWFTYP->clear();
    aui->comboSWFTYP->addItem("SIN","SIN");
    aui->comboSWFTYP->addItem("RAMP","RAMP");
    aui->comboSWFTYP->addItem("TRI","TRI");
    aui->comboSWFTYP->addItem("PULSE","PULSE");
    aui->comboSWFTYP->addItem("ARB","ARB");
    QObjectList widgetList = aui->gbARBmodule1->children();
    widgetList += aui->gbARBmodule2->children();
    widgetList += aui->gbARBcompressor->children();
    widgetList += aui->gbARBtiming->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("leS"))
       {
            //((QLineEdit *)w)->setValidator(new QDoubleValidator);
            connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(ARBUpdated()));
       }
    }
    widgetList = aui->gbARBtwaveParms->children();
    widgetList += aui->gbDualOutput->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("leS"))
       {
            //((QLineEdit *)w)->setValidator(new QDoubleValidator);
            connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(ARBUpdatedParms()));
       }
    }
    connect(aui->pbSetChannel,SIGNAL(pressed()),this,SLOT(SetARBchannel()));
    connect(aui->pbSetRangeChannel,SIGNAL(pressed()),this,SLOT(SetARBchannelRange()));
    connect(aui->pbARBtrigger,SIGNAL(pressed()),this,SLOT(ARBtrigger()));
    connect(aui->pbSetChannel_2,SIGNAL(pressed()),this,SLOT(SetARBchannel_2()));
    connect(aui->pbSetRangeChannel_2,SIGNAL(pressed()),this,SLOT(SetARBchannelRange_2()));
    connect(aui->pbARBtrigger_2,SIGNAL(pressed()),this,SLOT(ARBtrigger_2()));
    connect(aui->pbARBviewLog,SIGNAL(pressed()),this,SLOT(ARBviewLog()));
    connect(aui->pbARBclearLog,SIGNAL(pressed()),this,SLOT(ARBclearLog()));
    connect(aui->pbARBupdate,SIGNAL(pressed()),this,SLOT(ARBupdate()));
    connect(aui->pbARBtwaveUpdate,SIGNAL(pressed()),this,SLOT(ARBupdate()));
    connect(aui->pbEditARBwf,SIGNAL(pressed()),this,SLOT(EditARBwaveform()));
    connect(aui->tabARB,SIGNAL(currentChanged(int)),this,SLOT(ARBtabSelected()));
    connect(aui->comboSWFTYP,SIGNAL(currentIndexChanged(int)),this,SLOT(ARBtypeSelected()));
    connect(aui->comboARBmodule,SIGNAL(currentIndexChanged(int)),this,SLOT(ARBmoduleSelected()));
    connect(aui->rbSWFDIR_FWD,SIGNAL(clicked(bool)),this,SLOT(rbTWfwd()));
    connect(aui->rbSWFDIR_REV,SIGNAL(clicked(bool)),this,SLOT(rbTWrev()));
    // Compressor controls
    connect(aui->rbSARBCMODE_COMPRESS,SIGNAL(clicked(bool)),this,SLOT(rbModeCompress()));
    connect(aui->rbSARBCMODE_NORMAL,SIGNAL(clicked(bool)),this,SLOT(rbModeNormal()));
    connect(aui->rbSARBCSW_CLOSE,SIGNAL(clicked(bool)),this,SLOT(rbSwitchClose()));
    connect(aui->rbSARBCSW_OPEN,SIGNAL(clicked(bool)),this,SLOT(rbSwitchOpen()));
    connect(aui->pbARBforceTrigger,SIGNAL(pressed()),this,SLOT(pbForceTrigger()));
}

void ARB::SetNumberOfChannels(int num)
{
    NumChannels = num;
    aui->comboARBmodule->clear();
    for(int i=0;i<4;i++)
    {
        if(((i+1) * 8) <= NumChannels) aui->comboARBmodule->addItem(QString::number(i+1));
    }
}

void ARB::pbForceTrigger(void)
{
   comms->SendCommand("TARBTRG\n");
}

void ARB::rbModeCompress(void)
{
    comms->SendCommand("SARBCMODE,Compress\n");
}

void ARB::rbModeNormal(void)
{
    comms->SendCommand("SARBCMODE,Normal\n");
}

void ARB::rbSwitchClose(void)
{
   comms->SendCommand("SARBCSW,Close\n");
}

void ARB::rbSwitchOpen(void)
{
   comms->SendCommand("SARBCSW,Open\n");
}

void ARB::rbTWfwd(void)
{
    comms->SendCommand("SWFDIR,1,FWD\n");
}

void ARB::rbTWrev(void)
{
    comms->SendCommand("SWFDIR,1,REV\n");
}

void ARB::ARBtypeSelected(void)
{
    comms->SendCommand("SWFTYP," + aui->comboARBmodule->currentText() + "," + aui->comboSWFTYP->currentText() + "\n");
}

void ARB::ARBtrigger(void)
{
    comms->SendCommand("SWFDIS,1\n");
    comms->SendCommand("SWFENA,1\n");
}
void ARB::ARBtrigger_2(void)
{
    comms->SendCommand("SWFDIS,2\n");
    comms->SendCommand("SWFENA,2\n");
}

void ARB::ARBtabSelected(void)
{
    if(aui->tabARB->tabText(aui->tabARB->currentIndex()) == "ARB mode")
    {
        if(NumChannels > 0) comms->SendCommand("SARBMODE,1,ARB\n");
        if(NumChannels > 8) comms->SendCommand("SARBMODE,2,ARB\n");
        QThread::msleep(2000);
     }
    if(aui->tabARB->tabText(aui->tabARB->currentIndex()) == "Twave mode")
    {
        if(NumChannels > 0) comms->SendCommand("SARBMODE,1,TWAVE\n");
        if(NumChannels > 8) comms->SendCommand("SARBMODE,2,TWAVE\n");
        QThread::msleep(2000);
    }
    Update();
}

void ARB::SetARBchannel(void)
{
    QString res;

    res = aui->leChan->text();
    if(res.toInt() == 0)
    {
        res = "SARBCHS,1," + aui->leLevel->text() + "\n";
        LogString += res;
        comms->SendCommand(res.toStdString().c_str());
        return;
    }
    res = "SARBCH,1," + aui->leChan->text() + "," + aui->leLevel->text() + "\n";
    LogString += res;
    comms->SendCommand(res.toStdString().c_str());
}
void ARB::SetARBchannel_2(void)
{
    QString res;

    res = aui->leChan->text();
    if(res.toInt() == 0)
    {
        res = "SARBCHS,2," + aui->leLevel_2->text() + "\n";
        LogString += res;
        comms->SendCommand(res.toStdString().c_str());
        return;
    }
    res = "SARBCH,2," + aui->leChan_2->text() + "," + aui->leLevel_2->text() + "\n";
    LogString += res;
    comms->SendCommand(res.toStdString().c_str());
}

void ARB::SetARBchannelRange(void)
{
    QString res;

    res = "SACHRNG,1," + aui->leRangeChan->text() + "," + aui->leRangeStart->text() + "," + aui->leRangeStop->text() + "," + aui->leRangeLevel->text() + "\n";
    LogString += res;
    comms->SendCommand(res.toStdString().c_str());
}
void ARB::SetARBchannelRange_2(void)
{
    QString res;

    res = "SACHRNG,2," + aui->leRangeChan_2->text() + "," + aui->leRangeStart_2->text() + "," + aui->leRangeStop_2->text() + "," + aui->leRangeLevel_2->text() + "\n";
    LogString += res;
    comms->SendCommand(res.toStdString().c_str());
}

void ARB::ARBmoduleSelected(void)
{
    QString res;

    QObjectList widgetList = aui->gbARBtwaveParms->children();
    QString chan = aui->comboARBmodule->currentText();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().startsWith("le"))
       {
           res = "G" + w->objectName().mid(3) + "," + chan;
           ((QLineEdit *)w)->setText(comms->SendMess(res + "\n"));
       }
    }
    res = comms->SendMess("GWFDIR," + chan +"\n");
    if(res == "FWD") aui->rbSWFDIR_FWD->setChecked(true);
    if(res == "REV") aui->rbSWFDIR_REV->setChecked(true);
    res = comms->SendMess("GWFTYP," + chan + "\n");
    int i = aui->comboSWFTYP->findData(res);
    aui->comboSWFTYP->setCurrentIndex(i);
}

void ARB::ARBUpdated(void)
{
   QObject* obj = sender();
   QString res;

   if(!((QLineEdit *)obj)->isModified()) return;
   res = obj->objectName().mid(2).replace("_",",");
   if(res.right(1) == ",") res = res.left(res.length()-1);
   res += "," + ((QLineEdit *)obj)->text() + "\n";
   comms->SendCommand(res.toStdString().c_str());
   ((QLineEdit *)obj)->setModified(false);
}

void ARB::ARBUpdatedParms(void)
{
   QString chan = aui->comboARBmodule->currentText();
   QObject* obj = sender();
   QString res;

   if(!((QLineEdit *)obj)->isModified()) return;
   res = obj->objectName().mid(2);
   res += "," + chan + "," + ((QLineEdit *)obj)->text() + "\n";
   comms->SendCommand(res.toStdString().c_str());
   ((QLineEdit *)obj)->setModified(false);
}

void ARB::ARBupdate(void)
{
    Update();
}

void ARB::Update(void)
{
    QString res;
    QObjectList widgetList;

    // Determine the number of ARB channels and exit if its zero
    res = comms->SendMess("GCHAN,ARB\n");
    NumChannels = res.toInt();
    if(NumChannels == 0)
    {
        aui->gbARBmodule1->setEnabled(false);
        aui->gbARBmodule2->setEnabled(false);
        return;
    }
    aui->gbARBmodule1->setEnabled(true);
    if(NumChannels > 8) aui->gbARBmodule2->setEnabled(true);
    // Get the first modules's mode and set the tab to that mode and
    // if there are two modules set second to match first
    res = comms->SendMess("GARBMODE,1\n");
    if(res == "ARB")
    {
        aui->tabARB->setCurrentIndex(1);
    }
    if(res == "TWAVE")
    {
        aui->tabARB->setCurrentIndex(0);
    }
    if(aui->tabARB->tabText(aui->tabARB->currentIndex()) == "ARB mode")
    {
        QObjectList widgetList = aui->gbARBmodule1->children();
        if(NumChannels > 8) widgetList += aui->gbARBmodule2->children();
        foreach(QObject *w, widgetList)
        {
           if(w->objectName().contains("le"))
           {
               res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
               ((QLineEdit *)w)->setText(comms->SendMess(res));
           }
        }
    }
    if(aui->tabARB->tabText(aui->tabARB->currentIndex()) == "Twave mode")
    {
        if(NumChannels >= 16)
        {
            if(Compressor) aui->gbARBcompressor->setEnabled(true);
            else aui->gbARBcompressor->setEnabled(false);
        }
        else aui->gbARBcompressor->setEnabled(false);
        if(Compressor)
        {
           widgetList = aui->gbARBcompressor->children();
           widgetList += aui->gbARBtiming->children();
           foreach(QObject *w, widgetList)
           {
              if(w->objectName().contains("le"))
              {
                  res = "G" + w->objectName().mid(3).replace("_",",");
                  if(res.right(1) == ",") res = res.left(res.length()-1);
                  ((QLineEdit *)w)->setText(comms->SendMess(res + "\n"));
              }
           }
        }
        // Check for dual output boards and individual offset control
        QString chan = aui->comboARBmodule->currentText();
        if(comms->SendMess("GARBOFFA," + chan + "\n").contains("?")) aui->gbDualOutput->setEnabled(false);
        else aui->gbDualOutput->setEnabled(true);
        widgetList = aui->gbARBtwaveParms->children();
        if(aui->gbDualOutput->isEnabled()) widgetList += aui->gbDualOutput->children();
        foreach(QObject *w, widgetList)
        {
           if(w->objectName().startsWith("le"))
           {
               res = "G" + w->objectName().mid(3) + "," + chan;
               ((QLineEdit *)w)->setText(comms->SendMess(res + "\n"));
           }
        }
        res = comms->SendMess("GWFDIR," + chan +"\n");
        if(res == "FWD") aui->rbSWFDIR_FWD->setChecked(true);
        if(res == "REV") aui->rbSWFDIR_REV->setChecked(true);
        res = comms->SendMess("GWFTYP," + chan + "\n");
        int i = aui->comboSWFTYP->findData(res);
        aui->comboSWFTYP->setCurrentIndex(i);
     }
     if(Compressor)
     {
         res = comms->SendMess("GARBCMODE\n");
         if(res == "Normal") aui->rbSARBCMODE_NORMAL->setChecked(true);
         if(res == "Compress") aui->rbSARBCMODE_COMPRESS->setChecked(true);
         res = comms->SendMess("GARBCSW\n");
         if(res == "Open") aui->rbSARBCSW_OPEN->setChecked(true);
         if(res == "Close") aui->rbSARBCSW_CLOSE->setChecked(true);
     }
}


void ARB::ARBclearLog(void)
{
    LogString = "";
}

void ARB::ARBviewLog(void)
{
    LogedData->SetTitle("Log of commands sent to MIPS");
    LogedData->LoadStr(LogString);
    LogedData->show();
}

void ARB::Save(QString Filename)
{
    QString res;

    if(Filename == "") return;
    QFile file(Filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# ARB settings, " + dateTime.toString() + "\n";
        // Loop thru all the channels and save ARB parameters
        QObjectList widgetList = aui->gbARBtwaveParms->children();
        for(int i=0;i<aui->comboARBmodule->count();i++)
        {
            aui->comboARBmodule->setCurrentIndex(i);
            // likely need to update the ARB parameters on the dialog here
            QApplication::processEvents();
            foreach(QObject *w, widgetList)
            {
                if(w->objectName().mid(0,3) == "leS")
                {
                    res = w->objectName().mid(2) + "," + aui->comboARBmodule->currentText() + "," + ((QLineEdit *)w)->text() + "\n";
                    stream << res;
                }
            }
            // Save the waveform direction and type
            if(aui->rbSWFDIR_FWD->isChecked()) res = "SWFDIR," + aui->comboARBmodule->currentText() + ",FWD\n";
            if(aui->rbSWFDIR_REV->isChecked()) res = "SWFDIR," + aui->comboARBmodule->currentText() + ",REV\n";
            stream << res;
            res = "SWFTYP," + aui->comboARBmodule->currentText() + "," + aui->comboSWFTYP->currentText() + "\n";
            stream << res;
        }
        widgetList = aui->gbARBcompressor->children();
        widgetList += aui->gbARBmodule1->children();
        if(NumChannels >8) widgetList += aui->gbARBmodule2->children();
        foreach(QObject *w, widgetList)
        {
            if(w->objectName().mid(0,3) == "leS")
            {
                res = w->objectName().mid(2).replace("_",",") + "," + ((QLineEdit *)w)->text() + "\n";
                stream << res;
            }
        }
        stream << "\n";
        stream << LogString;
        file.close();
        aui->statusBar->showMessage("Settings saved to " + Filename,2000);
    }
}

void ARB::Load(QString Filename)
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
            if(line.trimmed().mid(0,1) != "#") comms->SendCommand(line + "\n");
            QApplication::processEvents();
        } while(!line.isNull());
        file.close();
        Update();
        aui->statusBar->showMessage("Settings loaded from " + Filename,2000);
    }
}

void ARB::ReadWaveform(void)
{
    int Waveform[32];
    QString cmd;
    int i;

    // Read waveform
    ARBwfEdit->GetWaveform(Waveform);
    // Send waveform to MIPS
    cmd = "SWFARB,"  + aui->comboARBmodule->currentText();
    for(i=0; i<32; i++) cmd += "," + QString::number(Waveform[i]);
    cmd += "\n";
    if(!comms->SendCommand(cmd))
    {
        aui->statusBar->showMessage("Error sending waveform to MIPS",2000);
    }
}

void ARB::EditARBwaveform(void)
{
   int Waveform[32];
   int i;

   for(i=0; i<32; i++) Waveform[i] = i*4 - 64;
   // Read the ARB waveform from MIPS
   QString res = comms->SendMess("GWFARB," + aui->comboARBmodule->currentText() + "\n");
   if(res.contains("?"))
   {
       // Here if the message was NAKed
       aui->statusBar->showMessage("Error reading waveform from MIPS",2000);
       return;
   }
   QStringList Vals = res.split(",");
   for(i=0; i<32; i++)
   {
       if(i < Vals.count()) Waveform[i] = Vals[i].toInt();
       else Waveform[i] = 0;
   }
   ARBwfEdit = new ARBwaveformEdit;
   connect(ARBwfEdit, SIGNAL(WaveformReady()), this, SLOT(ReadWaveform()));
   ARBwfEdit->SetWaveform(Waveform);
   ARBwfEdit->show();
}
