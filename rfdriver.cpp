#include "rfdriver.h"

RFdriver::RFdriver(Ui::MIPS *w, Comms *c)
{
    rui = w;
    comms = c;

    SetNumberOfChannels(2);
    connect(rui->pbUpdateRF,SIGNAL(pressed()),this,SLOT(UpdateRFdriver()));
    connect(rui->leSRFFRQ,SIGNAL(editingFinished()),this,SLOT(leSRFFRQ_editingFinished()));
    connect(rui->leSRFDRV,SIGNAL(editingFinished()),this,SLOT(leSRFDRV_editingFinished()));
    rui->leSRFFRQ->setValidator(new QIntValidator);
    rui->leSRFDRV->setValidator(new QDoubleValidator);
}

void RFdriver::SetNumberOfChannels(int num)
{
    disconnect(rui->comboRFchan, SIGNAL(currentTextChanged(QString)),0,0);
    NumChannels = num;
    rui->comboRFchan->clear();
    for(int i=0;i<NumChannels;i++)
    {
        rui->comboRFchan->addItem(QString::number(i+1));
    }
    connect(rui->comboRFchan,SIGNAL(currentTextChanged(QString)),this,SLOT(UpdateRFdriver()));
}

void RFdriver::Update(void)
{
    QString res;

    rui->tabMIPS->setEnabled(false);
    rui->statusBar->showMessage(tr("Updating RF driver controls..."));
    rui->leSRFFRQ->setText(comms->SendMessage("GRFFRQ," + rui->comboRFchan->currentText() + "\n"));
    rui->leSRFDRV->setText(comms->SendMessage("GRFDRV," + rui->comboRFchan->currentText() + "\n"));
    rui->leGRFPPVP->setText(comms->SendMessage("GRFPPVP," + rui->comboRFchan->currentText() + "\n"));
    rui->leGRFPPVN->setText(comms->SendMessage("GRFPPVN," + rui->comboRFchan->currentText() + "\n"));
    rui->leGRFPWR->setText(comms->SendMessage("GRFPWR," + rui->comboRFchan->currentText() + "\n"));
    rui->tabMIPS->setEnabled(true);
    rui->statusBar->showMessage(tr(""));
}

// Select each channel from the tab to populate the group box
// and then save the group box parameters.
void RFdriver::Save(QString Filename)
{
    int i;
    int SelectedChannel = rui->comboRFchan->currentIndex();
    QString res;

    if(NumChannels == 0) return;
    if(Filename == "") return;
    QFile file(Filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# RFdriver settings, " + dateTime.toString() + "\n";
        QObjectList widgetList = rui->gbRFparms->children();
        // Loop through all the channels
        for(i=0;i<NumChannels;i++)
        {
           rui->comboRFchan->setCurrentIndex(i);
           // Save the parameters to the file
           foreach(QObject *w, widgetList)
           {
               if(w->objectName().mid(0,3) == "leS")
               {
                   res = w->objectName() + "," + rui->comboRFchan->currentText() + "," + ((QLineEdit *)w)->text() + "\n";
                   stream << res;
               }
           }
        }
        rui->comboRFchan->setCurrentIndex(SelectedChannel);
        file.close();
        rui->statusBar->showMessage("Settings saved to " + Filename,2000);
    }
}

// Load all the parameters from the file, for the selected channel
// write to UI and update MIPS, for the other channels just update MIPS.
void RFdriver::Load(QString Filename)
{
    int SelectedChannel = rui->comboRFchan->currentIndex();
    QString res;
    QStringList resList;

    if(NumChannels == 0) return;
    if(Filename == "") return;
    QFile file(Filename);
    QObjectList widgetList = rui->gbRFparms->children();
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        // We're going to streaming the file to the QString
        QTextStream stream(&file);
        QString line;
        do
        {
            line = stream.readLine();
            resList = line.split(",");
            if(resList.count() == 3)
            {
                if(rui->comboRFchan->currentIndex() != (resList[1].toInt()-1)) rui->comboRFchan->setCurrentIndex(resList[1].toInt()-1);
                foreach(QObject *w, widgetList)
                {
                    if(w->objectName().mid(0,3) == "leS")
                    {
                        if(resList[2] != "") if(w->objectName() == resList[0])
                        {
                            ((QLineEdit *)w)->setText(resList[2]);
                            ((QLineEdit *)w)->setModified(true);
                            QMetaObject::invokeMethod(w, "editingFinished");
                        }
                    }
                }
            }
        } while(!line.isNull());
        rui->comboRFchan->setCurrentIndex(SelectedChannel);
        file.close();
        rui->statusBar->showMessage("Settings loaded from " + Filename,2000);
    }
}
void RFdriver::UpdateRFdriver(void)
{
    Update();
}

void RFdriver::leSRFFRQ_editingFinished()
{
    if(!rui->leSRFFRQ->isModified()) return;
    comms->SendCommand("SRFFRQ," + rui->comboRFchan->currentText() + "," + rui->leSRFFRQ->text() + "\n");
    rui->leSRFFRQ->setModified(false);
}

void RFdriver::leSRFDRV_editingFinished()
{
    if(!rui->leSRFDRV->isModified()) return;
    comms->SendCommand("SRFDRV," + rui->comboRFchan->currentText() + "," + rui->leSRFDRV->text() + "\n");
    rui->leSRFDRV->setModified(false);
}
