#include "dcbias.h"

namespace Ui {
class MIPS;
}

DCbias::DCbias(Ui::MIPS *w, Comms *c)
{
    dui = w;
    comms = c;

    SetNumberOfChannels(8);
    // DCbias page setup
    selectedLineEdit = NULL;
    QObjectList widgetList = dui->gbDCbias1->children();
    widgetList += dui->gbDCbias2->children();
    widgetList += dui->gbDCbias3->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("leSDCB"))
       {
            ((QLineEdit *)w)->setValidator(new QDoubleValidator);
           connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(DCbiasUpdated()));
       }
    }
    connect(dui->pbDCbiasUpdate,SIGNAL(pressed()),this,SLOT(UpdateDCbias()));
    connect(dui->chkPowerEnable,SIGNAL(toggled(bool)),this,SLOT(DCbiasPower()));
}

void DCbias::SetNumberOfChannels(int num)
{
    NumChannels = num;
    dui->gbDCbias1->setEnabled(false);
    dui->gbDCbias2->setEnabled(false);
    dui->gbDCbias3->setEnabled(false);
    if(NumChannels >= 8) dui->gbDCbias1->setEnabled(true);
    if(NumChannels > 8) dui->gbDCbias2->setEnabled(true);
    if(NumChannels > 16) dui->gbDCbias3->setEnabled(true);
}

void DCbias::DCbiasUpdated(void)
{
   QObject* obj = sender();
   QString res;

   if(!((QLineEdit *)obj)->isModified()) return;
   res = obj->objectName().mid(2).replace("_",",") + "," + ((QLineEdit *)obj)->text() + "\n";
   comms->SendCommand(res.toStdString().c_str());
   ((QLineEdit *)obj)->setModified(false);
}

void DCbias::Update(void)
{
    QString res;

//    dui->tabMIPS->setEnabled(false);
//    dui->statusBar->showMessage(tr("Updating DC bias controls..."));
     // Read the number of channels and enable the proper controls
    dui->leGCHAN_DCB->setText(QString::number(NumChannels));
    res = comms->SendMessage("GDCPWR\n");
    if(res == "ON") dui->chkPowerEnable->setChecked(true);
    else  dui->chkPowerEnable->setChecked(false);
    QObjectList widgetList = dui->gbDCbias1->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("le"))
       {
            if(!((QLineEdit *)w)->hasFocus())
            {
               res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
               ((QLineEdit *)w)->setText(comms->SendMessage(res));
            }
       }
    }
    // Adjust range based on offet value
    dui->leGDCMIN_1->setText( QString::number(dui->leGDCMIN_1->text().toFloat()  + dui->leSDCBOF_1->text().toFloat()));
    dui->leGDCMAX_1->setText( QString::number(dui->leGDCMAX_1->text().toFloat()  + dui->leSDCBOF_1->text().toFloat()));
    if(NumChannels > 8)
    {
        QObjectList widgetList = dui->gbDCbias2->children();
        foreach(QObject *w, widgetList)
        {
           if(w->objectName().contains("le"))
           {
               if(!((QLineEdit *)w)->hasFocus())
               {
                  res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
                  ((QLineEdit *)w)->setText(comms->SendMessage(res));
               }
           }
        }
        // Adjust range based on offet value
        dui->leGDCMIN_9->setText( QString::number(dui->leGDCMIN_9->text().toFloat()  + dui->leSDCBOF_9->text().toFloat()));
        dui->leGDCMAX_9->setText( QString::number(dui->leGDCMAX_9->text().toFloat()  + dui->leSDCBOF_9->text().toFloat()));
    }
    if(NumChannels > 16)
    {
        QObjectList widgetList = dui->gbDCbias3->children();
        foreach(QObject *w, widgetList)
        {
           if(w->objectName().contains("le"))
           {
               if(!((QLineEdit *)w)->hasFocus())
               {
                  res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
                  ((QLineEdit *)w)->setText(comms->SendMessage(res));
               }
           }
        }
        // Adjust range based on offet value
        dui->leGDCMIN_17->setText( QString::number(dui->leGDCMIN_17->text().toFloat()  + dui->leSDCBOF_17->text().toFloat()));
        dui->leGDCMAX_17->setText( QString::number(dui->leGDCMAX_17->text().toFloat()  + dui->leSDCBOF_17->text().toFloat()));
    }
//    dui->tabMIPS->setEnabled(true);
//    dui->statusBar->showMessage(tr(""));
}

void DCbias::DCbiasPower(void)
{
    if(dui->chkPowerEnable->isChecked()) comms->SendCommand("SDCPWR,ON\n");
    else  comms->SendCommand("SDCPWR,OFF\n");
}

void DCbias::UpdateDCbias(void)
{
    Update();
}

void DCbias::Save(QString Filename)
{
    QString res;

    if(NumChannels == 0) return;
    if(Filename == "") return;
    QFile file(Filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# DCbias settings, " + dateTime.toString() + "\n";
        QObjectList widgetList = dui->DCbias->children();
        widgetList += dui->gbDCbias1->children();
        if(NumChannels >8) widgetList += dui->gbDCbias2->children();
        foreach(QObject *w, widgetList)
        {
            if(w->objectName() == "chkPowerEnable")
            {
                stream << "chkPowerEnable,";
                if(dui->chkPowerEnable->isChecked()) stream << "ON\n";
                else stream << "OFF\n";
            }
            if(w->objectName().mid(0,3) == "leS")
            {
                res = w->objectName() + "," + ((QLineEdit *)w)->text() + "\n";
                stream << res;
            }
        }
        file.close();
        dui->statusBar->showMessage("Settings saved to " + Filename,2000);
    }
}

void DCbias::Load(QString Filename)
{
    QStringList resList;

    if(NumChannels == 0) return;
    if(Filename == "") return;
    QObjectList widgetList = dui->DCbias->children();
    widgetList += dui->gbDCbias1->children();
    if(NumChannels >8) widgetList += dui->gbDCbias2->children();
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
                    if(w->objectName() == "chkPowerEnable")
                    {
                        if(w->objectName() == resList[0])
                        {
                            if(resList[1] == "ON")
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
