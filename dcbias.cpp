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
    dui->dialDCbias->setMinimum(-250);
    dui->dialDCbias->setMaximum(250);
    dui->dialDCbias->setValue(0);
    selectedLineEdit = NULL;
    QObjectList widgetList = dui->gbDCbias1->children();
    widgetList += dui->gbDCbias2->children();
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
    connect(dui->dialDCbias,SIGNAL(valueChanged(int)),this,SLOT(DCbiasdial()));
    connect(dui->comboDialCH,SIGNAL(currentIndexChanged(int)),this,SLOT(CHselected()));
}

void DCbias::SetNumberOfChannels(int num)
{
    NumChannels = num;
    dui->gbDCbias1->setEnabled(false);
    dui->gbDCbias2->setEnabled(false);
    if(NumChannels >= 8) dui->gbDCbias1->setEnabled(true);
    if(NumChannels > 8) dui->gbDCbias2->setEnabled(true);
    dui->comboDialCH->clear();
    dui->comboDialCH->addItem("None");
    for(int i=0;i<NumChannels;i++)
    {
        dui->comboDialCH->addItem("CH " + QString::number(i+1));
    }
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

void DCbias::CHselected(void)
{
    if(dui->comboDialCH->currentText() == "None") selectedLineEdit = NULL;
    if(dui->comboDialCH->currentText() == "CH 1") selectedLineEdit = dui->leSDCB_1;
    if(dui->comboDialCH->currentText() == "CH 2") selectedLineEdit = dui->leSDCB_2;
    if(dui->comboDialCH->currentText() == "CH 3") selectedLineEdit = dui->leSDCB_3;
    if(dui->comboDialCH->currentText() == "CH 4") selectedLineEdit = dui->leSDCB_4;
    if(dui->comboDialCH->currentText() == "CH 5") selectedLineEdit = dui->leSDCB_5;
    if(dui->comboDialCH->currentText() == "CH 6") selectedLineEdit = dui->leSDCB_6;
    if(dui->comboDialCH->currentText() == "CH 7") selectedLineEdit = dui->leSDCB_7;
    if(dui->comboDialCH->currentText() == "CH 8") selectedLineEdit = dui->leSDCB_8;
    if(dui->comboDialCH->currentText() == "CH 9") selectedLineEdit = dui->leSDCB_9;
    if(dui->comboDialCH->currentText() == "CH 10") selectedLineEdit = dui->leSDCB_10;
    if(dui->comboDialCH->currentText() == "CH 11") selectedLineEdit = dui->leSDCB_11;
    if(dui->comboDialCH->currentText() == "CH 12") selectedLineEdit = dui->leSDCB_12;
    if(dui->comboDialCH->currentText() == "CH 13") selectedLineEdit = dui->leSDCB_13;
    if(dui->comboDialCH->currentText() == "CH 14") selectedLineEdit = dui->leSDCB_14;
    if(dui->comboDialCH->currentText() == "CH 15") selectedLineEdit = dui->leSDCB_15;
    if(dui->comboDialCH->currentText() == "CH 16") selectedLineEdit = dui->leSDCB_16;
    if(selectedLineEdit != NULL)
    {
        if(dui->comboDialCH->currentText().mid(3).toInt() <= 8)
        {
            dui->dialDCbias->setMinimum((int)(dui->leGDCMIN_1->text().toFloat()));
            dui->dialDCbias->setMaximum((int)(dui->leGDCMAX_1->text().toFloat()));
        }
        else
        {
            dui->dialDCbias->setMinimum((int)(dui->leGDCMIN_9->text().toFloat()));
            dui->dialDCbias->setMaximum((int)(dui->leGDCMAX_9->text().toFloat()));
        }
        dui->dialDCbias->setValue((int)(selectedLineEdit->text().toFloat()));
//        qDebug() << selectedLineEdit->text().toInt();
    }
}

void DCbias::DCbiasdial(void)
{
   static bool inuse = false;

   if(inuse) return;
   if(selectedLineEdit == NULL) return;
   if(QString::number(dui->dialDCbias->value()) == selectedLineEdit->text()) return;
   inuse = true;
//   qDebug() << QString::number(dui->dialDCbias->value());
//   qDebug() << selectedLineEdit->text();
   selectedLineEdit->setText(QString::number(dui->dialDCbias->value()));
   comms->SendCommand("SDCB," + dui->comboDialCH->currentText().mid(3) + "," + selectedLineEdit->text() + "\n");
//   qDebug() << "SDCB," + dui->comboDialCH->currentText().mid(3) + "," + selectedLineEdit->text();
   inuse = false;
}

void DCbias::Update(void)
{
    QString res;

    dui->tabMIPS->setEnabled(false);
    dui->statusBar->showMessage(tr("Updating DC bias controls..."));
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
 //           if(!((QLineEdit *)w)->hasFocus())
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
                res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
                ((QLineEdit *)w)->setText(comms->SendMessage(res));
           }
        }
        // Adjust range based on offet value
        dui->leGDCMIN_9->setText( QString::number(dui->leGDCMIN_9->text().toFloat()  + dui->leSDCBOF_9->text().toFloat()));
        dui->leGDCMAX_9->setText( QString::number(dui->leGDCMAX_9->text().toFloat()  + dui->leSDCBOF_9->text().toFloat()));
    }
    dui->tabMIPS->setEnabled(true);
    dui->statusBar->showMessage(tr(""));
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
