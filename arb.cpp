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

    Compressor = true;
    aui->comboSWFTYP_1->clear();
    aui->comboSWFTYP_1->addItem("SIN","SIN");
    aui->comboSWFTYP_1->addItem("RAMP","RAMP");
    aui->comboSWFTYP_1->addItem("TRI","TRI");
    aui->comboSWFTYP_1->addItem("PULSE","PULSE");
    aui->comboSWFTYP_1->addItem("ARB","ARB");
    aui->comboSWFTYP_2->clear();
    aui->comboSWFTYP_2->addItem("SIN","SIN");
    aui->comboSWFTYP_2->addItem("RAMP","RAMP");
    aui->comboSWFTYP_2->addItem("TRI","TRI");
    aui->comboSWFTYP_2->addItem("PULSE","PULSE");
    aui->comboSWFTYP_2->addItem("ARB","ARB");
    QObjectList widgetList = aui->gbARBmodule1->children();
    widgetList += aui->gbARBmodule2->children();
    widgetList += aui->gbARBtwave1->children();
    widgetList += aui->gbARBtwave2->children();
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
    connect(aui->tabARB,SIGNAL(currentChanged(int)),this,SLOT(ARBtabSelected()));
    connect(aui->comboSWFTYP_1,SIGNAL(currentIndexChanged(int)),this,SLOT(ARBtypeSelected()));
    connect(aui->rbSWFDIR_1_FWD,SIGNAL(clicked(bool)),this,SLOT(rbTW1fwd()));
    connect(aui->rbSWFDIR_1_REV,SIGNAL(clicked(bool)),this,SLOT(rbTW1rev()));
    connect(aui->comboSWFTYP_2,SIGNAL(currentIndexChanged(int)),this,SLOT(ARBtypeSelected2()));
    connect(aui->rbSWFDIR_2_FWD,SIGNAL(clicked(bool)),this,SLOT(rbTW2fwd()));
    connect(aui->rbSWFDIR_2_REV,SIGNAL(clicked(bool)),this,SLOT(rbTW2rev()));
    // Compressor controls
    connect(aui->rbSARBCMODE_COMPRESS,SIGNAL(clicked(bool)),this,SLOT(rbModeCompress()));
    connect(aui->rbSARBCMODE_NORMAL,SIGNAL(clicked(bool)),this,SLOT(rbModeNormal()));
    connect(aui->rbSARBCSW_CLOSE,SIGNAL(clicked(bool)),this,SLOT(rbSwitchClose()));
    connect(aui->rbSARBCSW_OPEN,SIGNAL(clicked(bool)),this,SLOT(rbSwitchOpen()));
    connect(aui->pbARBforceTrigger,SIGNAL(pressed()),this,SLOT(pbForceTrigger()));
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

void ARB::rbTW1fwd(void)
{
    comms->SendCommand("SWFDIR,1,FWD\n");
}

void ARB::rbTW1rev(void)
{
    comms->SendCommand("SWFDIR,1,REV\n");
}

void ARB::ARBtypeSelected(void)
{
    comms->SendCommand("SWFTYP,1," + aui->comboSWFTYP_1->currentText() + "\n");
}
void ARB::rbTW2fwd(void)
{
    comms->SendCommand("SWFDIR,2,FWD\n");
}

void ARB::rbTW2rev(void)
{
    comms->SendCommand("SWFDIR,2,REV\n");
}

void ARB::ARBtypeSelected2(void)
{
    comms->SendCommand("SWFTYP,2," + aui->comboSWFTYP_2->currentText() + "\n");
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

void ARB::ARBupdate(void)
{
    Update();
}

void ARB::Update(void)
{
    QString res;

    // Determine the number os ARB channels and exit if its zero
    res = comms->SendMessage("GCHAN,ARB\n");
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
    res = comms->SendMessage("GARBMODE,1\n");
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
               ((QLineEdit *)w)->setText(comms->SendMessage(res));
           }
        }
    }
    if(aui->tabARB->tabText(aui->tabARB->currentIndex()) == "Twave mode")
    {
        switch(NumChannels)
        {
           case 0:
              aui->gbARBtwave1->setEnabled(false);
              aui->gbARBtwave2->setEnabled(false);
              aui->gbARBcompressor->setEnabled(false);
              return;
           case 8:
              aui->gbARBtwave1->setEnabled(true);
              aui->gbARBtwave2->setEnabled(false);
              aui->tabCompressor->setEnabled(false);
           case 16:
              aui->gbARBtwave1->setEnabled(true);
              aui->gbARBtwave2->setEnabled(true);
              if(Compressor) aui->gbARBcompressor->setEnabled(true);
           default:
              break;
        }
        QObjectList widgetList = aui->gbARBtwave1->children();
        if(NumChannels > 8) widgetList += aui->gbARBtwave2->children();
        if(Compressor)
        {
           widgetList += aui->gbARBcompressor->children();
           widgetList += aui->gbARBtiming->children();
        }
        foreach(QObject *w, widgetList)
        {
           if(w->objectName().contains("le"))
           {
               res = "G" + w->objectName().mid(3).replace("_",",");
               if(res.right(1) == ",") res = res.left(res.length()-1);
               ((QLineEdit *)w)->setText(comms->SendMessage(res + "\n"));
           }
        }
        res = comms->SendMessage("GWFDIR,1\n");
        if(res == "FWD") aui->rbSWFDIR_1_FWD->setChecked(true);
        if(res == "REV") aui->rbSWFDIR_1_FWD->setChecked(true);
        res = comms->SendMessage("GWFTYP,1\n");
        int i = aui->comboSWFTYP_1->findData(res);
        aui->comboSWFTYP_1->setCurrentIndex(i);
        if(NumChannels > 8)
        {
            res = comms->SendMessage("GWFDIR,2\n");
            if(res == "FWD") aui->rbSWFDIR_2_FWD->setChecked(true);
            if(res == "REV") aui->rbSWFDIR_2_FWD->setChecked(true);
            res = comms->SendMessage("GWFTYP,2\n");
            i = aui->comboSWFTYP_2->findData(res);
            aui->comboSWFTYP_2->setCurrentIndex(i);
        }
        if(Compressor)
        {
            res = comms->SendMessage("GARBCMODE\n");
            if(res == "Normal") aui->rbSARBCMODE_NORMAL->setChecked(true);
            if(res == "Compress") aui->rbSARBCMODE_COMPRESS->setChecked(true);
            res = comms->SendMessage("GARBCSW\n");
            if(res == "Open") aui->rbSARBCSW_OPEN->setChecked(true);
            if(res == "Close") aui->rbSARBCSW_CLOSE->setChecked(true);
        }
    }
}

void ARB::ARBclearLog(void)
{
    LogString = "";
}

void ARB::ARBviewLog(void)
{
    LogedData->SetTitle("Log of commands sent to MIPS");
    LogedData->LoadString(LogString);
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
        QObjectList widgetList = aui->DCbias->children();
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
