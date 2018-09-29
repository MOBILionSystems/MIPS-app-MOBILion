#include "psg.h"

class pseDialog;
class psgPoint;
class psviewer;

PSG::PSG(Ui::MIPS *w, Comms *c)
{
   pui = w;
   comms = c;

// Setup controls
   pui->comboClock->clear();
   pui->comboClock->addItem("Ext");
   pui->comboClock->addItem("ExtN");
   pui->comboClock->addItem("ExtS");
   pui->comboClock->addItem("42000000");
   pui->comboClock->addItem("10500000");
   pui->comboClock->addItem("2625000");
   pui->comboClock->addItem("656250");
   pui->comboClock->setCurrentIndex(6);
   pui->comboTrigger->clear();
   pui->comboTrigger->addItem("Software");
   pui->comboTrigger->addItem("Edge");
   pui->comboTrigger->addItem("Pos");
   pui->comboTrigger->addItem("Neg");
   pui->leSequenceNumber->setValidator(new QIntValidator);
   pui->leExternClock->setValidator(new QIntValidator);
   pui->leTimePoint->setValidator(new QIntValidator);
   pui->leValue->setValidator(new QIntValidator);
// Setup slots
   connect(pui->pbDownload, SIGNAL(pressed()), this, SLOT(on_pbDownload_pressed()));
   connect(pui->pbViewTable, SIGNAL(pressed()), this, SLOT(on_pbViewTable_pressed()));
   connect(pui->pbLoadFromFile, SIGNAL(pressed()), this, SLOT(on_pbLoadFromFile_pressed()));
   connect(pui->pbSaveToFile, SIGNAL(pressed()), this, SLOT(on_pbSaveToFile_pressed()));
   connect(pui->pbEditCurrent, SIGNAL(pressed()), this, SLOT(on_pbEditCurrent_pressed()));
   connect(pui->pbCreateNew, SIGNAL(pressed()), this, SLOT(on_pbCreateNew_pressed()));
   connect(pui->pbTrigger, SIGNAL(pressed()), this, SLOT(on_pbTrigger_pressed()));
   connect(pui->pbExitTableMode, SIGNAL(pressed()), this, SLOT(on_pbExitTableMode_pressed()));
   connect(pui->pbRead, SIGNAL(pressed()), this, SLOT(on_pbRead_pressed()));
   connect(pui->pbWrite, SIGNAL(pressed()), this, SLOT(on_pbWrite_pressed()));
   connect(pui->leSequenceNumber, SIGNAL(editingFinished()), this, SLOT(on_leSequenceNumber_textEdited()));
   connect(pui->chkAutoAdvance, SIGNAL(clicked()), this, SLOT(on_chkAutoAdvance_clicked()));
   connect(pui->pbVisPulseSequence, SIGNAL(pressed()), this, SLOT(on_pbVisPulseSequence_pressed()));
}

void PSG::Load(void)
{
    on_pbLoadFromFile_pressed();
}

void PSG::Save(void)
{
    on_pbSaveToFile_pressed();
}

int PSG::Referenced(QList<psgPoint*> P, int i)
{
    int j;

    for(j = 0; j < P.size(); j++)
    {
        if(P[j]->Loop)
        {
            if(P[j]->LoopName == P[i]->Name) return(P[j]->LoopCount);
        }
    }
    return - 1;
}

QString PSG::BuildTableCommand(QList<psgPoint*> P)
{
    QString sTable;
    QString TableName;
    int i,j,Count,TimePointOffset=0;
    bool NoChange;

    sTable = "";
    TableName = "A";
    for(i = 0;i<P.size();i++)
    {
        NoChange = true;
        // Assume non zero values need to be sent at time point 0
        if(i == 0)
        {
            // See if any time point loops to this location
            Count = Referenced(P, i);
            if(Count >= 0)
            {
                // Here if this time point is referenced so set it up
                sTable += "0:[" + TableName;
                sTable += ":" + QString::number(Count) + "," + QString::number(P[i]->TimePoint - TimePointOffset);
                TableName = TableName[0].unicode() + 1;
            }
            else sTable += QString::number(P[i]->TimePoint - TimePointOffset);
            for(j = 0; j < 16; j++)
            {
                if(P[0]->DCbias[j] != 0)
                {
                    sTable += ":" + QString::number(j + 1) + ":" + QString::number(P[0]->DCbias[j]);
                    NoChange = false;
                }
            }
            for(j = 0; j < 16;j++)
            {
                if(P[0]->DigitalO[j])
                {
                    sTable += ":" + QString((int)'A' + j) + ":1";
                    NoChange = false;
                }
            }
            // If nothing changed then update DO A, something has to be defined at this time point
            if(NoChange)
            {
                if(P[i]->DigitalO[0]) sTable += ":A:1";
                else sTable += ":A:0";
            }
        }
        else
        {
            // See if any time point loops to this location
            NoChange = true;
            Count = Referenced(P, i);
            if (Count >= 0)
            {
                // Here if this time point is referenced so set it up
                sTable += "," + QString::number(P[i]->TimePoint - TimePointOffset) + ":[" + TableName + ":";
                sTable += QString::number(Count) + ",0";
                TableName = TableName[0].unicode() + 1;
                TimePointOffset = P[i]->TimePoint;
            }
            else sTable += "," + QString::number(P[i]->TimePoint - TimePointOffset);
            for(j = 0; j < 16; j++)
            {
                if(P[i]->DCbias[j] != P[i - 1]->DCbias[j])
                {
                    QString textV;
                    textV.sprintf("%.3f", P[i]->DCbias[j]);
                    sTable += ":" + QString::number(j + 1) + ":" + textV;
                    NoChange = false;
                }
            }
            for(j = 0; j < 16; j++)
            {
                if(P[i]->DigitalO[j] != P[i - 1]->DigitalO[j])
                {
                    if (P[i]->DigitalO[j]) sTable += ":" + QString((int)'A' + j) + ":1";
                    else sTable += ":" + QString((int)'A' + j) + ":0";
                    NoChange = false;
                }
            }
            // If nothing changed then update DO A, something has to be defined at this time point
            if(NoChange)
            {
                if(P[i]->DigitalO[0]) sTable += ":A:1";
                else sTable += ":A:0";
            }
            if(P[i]->Loop)
            {
                sTable += "]";
                if(i > 0) TimePointOffset = P[i]->TimePoint;
            }
        }
    }
    sTable = "STBLDAT;" + sTable + ";";
    return sTable;
}

void PSG::Update(void)
{
    pui->tabMIPS->setEnabled(false);
    pui->statusBar->showMessage(tr("Updating Pulse Sequence Generation controls..."));

    if(comms->SendMess("GTBLADV\n").contains("ON")) pui->chkAutoAdvance->setChecked(true);
    else pui->chkAutoAdvance->setChecked(false);
    pui->leSequenceNumber->setText(comms->SendMess("GTBLNUM\n"));

    pui->tabMIPS->setEnabled(true);
    pui->statusBar->showMessage(tr(""));
}

void PSG::on_pbDownload_pressed(void)
{
    QString res;

    // Make sure a table is loaded
    pui->pbDownload->setDown(false);
    if(psg.size() == 0)
    {
        QMessageBox msgBox;
        msgBox.setText("There is no Pulse Sequence to download to MIPS!");
        msgBox.exec();
        return;
    }
    // Make sure system is in local mode
    comms->SendCommand("SMOD,LOC\n");
    // Set clock
    comms->SendCommand("STBLCLK," + pui->comboClock->currentText().toUpper() + "\n");
    // Set trigger
    res = pui->comboTrigger->currentText().toUpper();
    if(res == "SOFTWARE") res = "SW";
    comms->SendCommand("STBLTRG," + res + "\n");
    // Send table
    comms->SendCommand(BuildTableCommand(psg));
    // Put system in table mode
    comms->SendCommand("SMOD,TBL\n");
    comms->waitforline(100);
    pui->statusBar->showMessage(comms->getline());
}

void PSG::on_pbViewTable_pressed()
{
    QString table,dtable;
    QList<psgPoint> P;

    pui->pbViewTable->setDown(false);
    if(psg.size() == 0)
    {
        QMessageBox msgBox;
        msgBox.setText("There is no Pulse Sequence to view!");
        msgBox.exec();
        return;
    }

    table = BuildTableCommand(psg);
    if(table.count()>60)
    {
        dtable = "";
        for(int i=0; i<table.count(); i+= 60)
        {
            dtable += table.mid(i,60);
            dtable += "\n";
        }
        table = dtable;
    }
    QMessageBox msgBox;
    msgBox.setText(table);
    msgBox.exec();
}

void PSG::on_pbLoadFromFile_pressed()
{
    psgPoint *P;

    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Pulse Sequence File"),"",tr("Files (*.psg *.*)"));

    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);

    psg.clear();
    while(!in.atEnd())
    {
        P = new psgPoint;
        in >> *P;
        psg.push_back(P);
    }
    file.close();
    pui->pbLoadFromFile->setDown(false);
}

void PSG::on_pbCreateNew_pressed()
{
    psgPoint *point = new psgPoint;

    pui->pbCreateNew->setDown(false);
    if(psg.size() > 0)
    {
        QMessageBox msgBox;
        msgBox.setText("This will overwrite the current Pulse Sequence Table.");
        msgBox.setInformativeText("Do you want to contine?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if(ret == QMessageBox::No) return;
    }
    point->Name = "TP_1";
    psg.clear();
    psg.push_back(point);
    pse = new pseDialog(&psg);
    pse->exec();
}

void PSG::on_pbSaveToFile_pressed()
{
    QList<psgPoint*>::iterator it;

    pui->pbSaveToFile->setDown(false);
    if(psg.size() == 0)
    {
        QMessageBox msgBox;
        msgBox.setText("There is no Pulse Sequence to save!");
        msgBox.exec();
        return;
    }
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Pulse Sequence File"),"",tr("Files (*.psg)"));
    QFile file(fileName);
    file.open(QIODevice::WriteOnly);

    QDataStream out(&file);   // we will serialize the data into the file
    for(it = psg.begin(); it != psg.end(); ++it) out << **it;

    file.close();
    pui->pbSaveToFile->setDown(false);
}

void PSG::on_pbEditCurrent_pressed()
{
    pui->pbEditCurrent->setDown(false);
    if(psg.size() == 0)
    {
        QMessageBox msgBox;
        msgBox.setText("There is no Pulse Sequence to edit!");
        msgBox.exec();
        return;
    }
    pse = new pseDialog(&psg);
    pse->exec();
}

void PSG::on_leSequenceNumber_textEdited()
{
    comms->SendCommand("STBLNUM," + pui->leSequenceNumber->text() + "\n");
}

void PSG::on_chkAutoAdvance_clicked()
{
    if(pui->chkAutoAdvance->isChecked()) comms->SendCommand("STBLADV,ON\n");
    else comms->SendCommand("STBLADV,OFF\n");
}

void PSG::on_pbTrigger_pressed()
{
    comms->SendCommand("TBLSTRT\n");
    comms->waitforline(100);
    pui->statusBar->showMessage(comms->getline());
    comms->waitforline(500);
    pui->statusBar->showMessage(pui->statusBar->currentMessage() + " " + comms->getline());
    comms->waitforline(100);
    pui->statusBar->showMessage(pui->statusBar->currentMessage() + " " + comms->getline());
}

void PSG::on_pbExitTableMode_pressed()
{
    comms->SendCommand("TBLABORT\n");
    comms->SendCommand("SMOD,LOC\n");
}

void PSG::on_pbRead_pressed()
{
    pui->leValue->setText(comms->SendMess("GTBLVLT," + pui->leTimePoint->text() + "," + pui->leChannel->text() + "\n"));
}

void PSG::on_pbWrite_pressed()
{
    comms->SendCommand("STBLVLT," + pui->leTimePoint->text() + "," + pui->leChannel->text() + "," + pui->leValue->text() + "\n");
}

void PSG::on_pbVisPulseSequence_pressed()
{
    pui->pbVisPulseSequence->setDown(false);
    if(psg.size() == 0)
    {
        QMessageBox msgBox;
        msgBox.setText("There is no Pulse Sequence to visualize!");
        msgBox.exec();
        return;
    }
    psv = new psviewer(&psg);
    psv->show();
}

