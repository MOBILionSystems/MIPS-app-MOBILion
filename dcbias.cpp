#include "dcbias.h"

namespace Ui {
class MIPS;
}

DCbias::DCbias(Ui::MIPS *w, Comms *c)
{
    dui = w;
    comms = c;

    Updating = false;
    UpdateOff = false;
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

bool DCbias::myEventFilter(QObject *obj, QEvent *event)
{
    QLineEdit *le;
    QString res;
    float delta = 0;

   if (obj->objectName().startsWith("leSDCB_") && (event->type() == QEvent::KeyPress))
   {
       if(Updating) return true;
       UpdateOff = true;
       le = (QLineEdit *)obj;
       QKeyEvent *key = static_cast<QKeyEvent *>(event);
       if(key->key() == 16777235) delta = 0.1;
       if(key->key() == 16777237) delta = -0.1;
       if((QApplication::queryKeyboardModifiers() & 0xA000000) != 0) delta *= 0.1;
       if((QApplication::queryKeyboardModifiers() & 0x2000000) != 0) delta *= 10;
       if((QApplication::queryKeyboardModifiers() & 0x8000000) != 0) delta *= 100;
       // Get range for this channel
       int ch = obj->objectName().mid(7).toInt();
       if(ch>24) ch = 25;
       else if(ch>16) ch = 17;
       else if(ch>8) ch = 9;
       else ch = 1;
       QLineEdit *lemin = dui->centralWidget->findChild<QLineEdit *>("leGDCMIN_" + QString::number(ch), Qt::FindChildrenRecursively);
       QLineEdit *lemax = dui->centralWidget->findChild<QLineEdit *>("leGDCMAX_" + QString::number(ch), Qt::FindChildrenRecursively);
       float min = lemin->text().toFloat();
       float max = lemax->text().toFloat();
       if((max-min) > 150) delta *= 10;
       if(delta != 0)
       {
          QString myString;
          myString.sprintf("%3.2f", le->text().toFloat() + delta);
          if(((le->text().toFloat() + delta) >= min) && ((le->text().toFloat() + delta) <= max))
          {
             le->setText(myString);
             le->setModified(true);
             le->editingFinished();
             return true;
          }
       }
   }
   return QObject::eventFilter(obj, event);
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

// This function will search for all matching groups and apply
// the change to all channels in the same group.
void DCbias::ApplyDelta(QString GrpName, float change)
{
    QString  res;

   // Look through all the groups for matches
    for(int i=1;i<=24;i++)
    {
        res = "leGRP" + QString::number(i);
        QLineEdit *leGR = dui->gbDCbias1->findChild<QLineEdit *>(res);
        if(leGR != NULL)
        {
            if(leGR->text() == GrpName)
            {
                // Here if the name matches, now build the line edit box name
                res = "leSDCB_" + QString::number(i);
                QLineEdit *leDCB = dui->gbDCbias1->findChild<QLineEdit *>(res);
                if(leDCB != NULL)
                {
                    if(!leDCB->hasFocus())
                    {
                        // Read its value and change
                        leDCB->setText(QString::number(leDCB->text().toFloat() - change));
                        leDCB->setModified(true);
                        leDCB->editingFinished();
                    }
                }
            }
        }
    }
}

// This function is called when a DCbias value is changed. This function will send the command to
// MIPS to apply the new voltage.
void DCbias::DCbiasUpdated(void)
{
   QObject*    obj = sender();
   QString     res,ans;
   QStringList resList;

   if(Updating) return;
   if(!((QLineEdit *)obj)->isModified()) return;
   // If this channel is part of a group then first read the current value to calculate the
   // change. Apply the change to all channels in this group.
   // When we get here the change has alreay been made so we need to read the
   // current value from MIPS and calculate the difference.
   if((obj->objectName().startsWith("leSDCB_")) & (((QLineEdit *)obj)->hasFocus()))
   {
       resList = obj->objectName().split("_");
       res = "leGRP" + resList[1];
       QLineEdit *leGR = dui->gbDCbias1->findChild<QLineEdit *>(res);
       if(leGR->text() != "")
       {
           // Here if this channel has a group label so we need to read the current
           // MIPS channel value to calculate the change
           res = "G" + obj->objectName().mid(3).replace("_",",") + "\n";
           qDebug() << res;
           ans = comms->SendMess(res);
           qDebug() << ans;
           // if(ans == "") ans="100";  // For testing
           if(ans != "")
           {
               float oldvalue =ans.toFloat();
               float change = oldvalue - ((QLineEdit *)obj)->text().toFloat();
               // Now change all the values with the same group name
               ApplyDelta(leGR->text(),change);
           }
       }
   }
   res = obj->objectName().mid(2).replace("_",",") + "," + ((QLineEdit *)obj)->text() + "\n";
   comms->SendCommand(res.toStdString().c_str());
   ((QLineEdit *)obj)->setModified(false);
   UpdateOff = false;
}

void DCbias::Update(void)
{
    QString res;

    if(UpdateOff) return;
    Updating = true;
//    dui->tabMIPS->setEnabled(false);
//    dui->statusBar->showMessage(tr("Updating DC bias controls..."));
     // Read the number of channels and enable the proper controls
    dui->leGCHAN_DCB->setText(QString::number(NumChannels));
    res = comms->SendMess("GDCPWR\n");
    if(res == "ON") dui->chkPowerEnable->setChecked(true);
    else  dui->chkPowerEnable->setChecked(false);
    QObjectList widgetList = dui->gbDCbias1->children();
    foreach(QObject *w, widgetList)
    {
       if((w->objectName().contains("le")) & (!w->objectName().contains("leGRP")))
       {
            if(!((QLineEdit *)w)->hasFocus())
            {
               res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
               ((QLineEdit *)w)->setText(comms->SendMess(res));
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
           if((w->objectName().contains("le")) & (!w->objectName().contains("leGRP")))
           {
               if(!((QLineEdit *)w)->hasFocus())
               {
                  res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
                  ((QLineEdit *)w)->setText(comms->SendMess(res));
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
                  ((QLineEdit *)w)->setText(comms->SendMess(res));
               }
           }
        }
        // Adjust range based on offet value
        dui->leGDCMIN_17->setText( QString::number(dui->leGDCMIN_17->text().toFloat()  + dui->leSDCBOF_17->text().toFloat()));
        dui->leGDCMAX_17->setText( QString::number(dui->leGDCMAX_17->text().toFloat()  + dui->leSDCBOF_17->text().toFloat()));
    }
//    dui->tabMIPS->setEnabled(true);
//    dui->statusBar->showMessage(tr(""));
    Updating = false;
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
