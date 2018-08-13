//
// Controlpanel.cpp
//
// This file contains the classes used to create a user defined control panel. The control panels
// display MIPS controls for most features supported by MIPS. The user can define a background
// image and then place controls such as RF channels and DC bias channels at the locations the user
// defines on the background image.
//
// This feature akso supports multiple MIPS systems connected to you PC at the same time. Use the
// "Find MIPS and Connect" button to find all connected MIPS system. The control panel configuration
// file used the MIPS name to determine where the channels are located. Its important that you
// define a unique name for each MIPS system.
//
// The control panel also support the IFT (Ion Funnel Trap). This includes generation of an IFT
// trapping pulse sequence as well as calling an external application to acquire data from you
// detection system.
//
// Gordon Anderson
//
#include "controlpanel.h"
#include "ui_controlpanel.h"
#include "CDirSelectionDlg.h"
#include "scriptingconsole.h"

#include <QPixmap>
#include <math.h>
#include <QTextEdit>
#include <QTreeView>

ControlPanel::ControlPanel(QWidget *parent, QList<Comms*> S) :
    QDialog(parent),
    ui(new Ui::ControlPanel)
{
    QStringList resList;

    ui->setupUi(this);
    Systems = S;

    // Init a number of variables
    mc = NULL;
    DCBgroups = NULL;
    statusBar      = NULL;
    numTextLabels  = 0;
    numRFchannels  = 0;
    numDCBchannels = 0;
    numDCBoffsets  = 0;
    numDCBenables  = 0;
    numESIchannels = 0;
    numDIOchannels = 0;
    numARBchannels = 0;
    UpdateHoldOff  = 0;
    UpdateStop     = false;
    ShutdownFlag   = false;
    RestoreFlag    = false;
    StartMIPScomms = false;
    SystemIsShutdown = false;
    scriptconsole = NULL;
    // Allow user to select the configuration file
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Configuration from File"),"",tr("cfg (*.cfg);;All files (*.*)"));
    QFile file(fileName);
    // Read the configuration file and create the form as
    // well as all the contorls.
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        QTextStream stream(&file);
        QString line;
        do
        {
            line = stream.readLine();
            if(line.trimmed().startsWith("#")) continue;
            resList = line.split(",");
            if((resList[0].toUpper() == "SIZE") && (resList.length()==3))
            {
                this->setFixedSize(resList[1].toInt(),resList[2].toInt());
                ui->lblBackground->setGeometry(0,0,resList[1].toInt(),resList[2].toInt());
                statusBar = new QStatusBar(this);
                statusBar->setGeometry(0,resList[2].toInt()-18,resList[1].toInt(),18);
                statusBar->showMessage("Control panel loaded: " + QDateTime().currentDateTime().toString());
                statusBar->raise();
            }
            if((resList[0].toUpper() == "IMAGE") && (resList.length()==2))
            {
                QPixmap img(resList[1]);
                ui->lblBackground->clear();
                ui->lblBackground->setPixmap(img);
            }
            if((resList[0].toUpper() == "TEXTLABELS") && (resList.length()==2))
            {
                numTextLabels = resList[1].toInt();
                TextLabels = new TextLabel * [numTextLabels];
                for(int i = 0; i < numTextLabels; i++)
                {
                    TextLabels[i] = NULL;
                    // Read each RFchannel
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "TEXTLABEL") && (resList.length()==5))
                    {
                        TextLabels[i] = new TextLabel(ui->lblBackground,resList[1],resList[2].toInt(),resList[3].toInt(),resList[4].toInt());
                        TextLabels[i]->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "RFCHANNELS") && (resList.length()==2))
            {
                numRFchannels = resList[1].toInt();
                RFchans = new RFchannel * [numRFchannels];
                for(int i = 0; i < numRFchannels; i++)
                {
                    RFchans[i] = NULL;
                    // Read each RFchannel
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "RFCHANNEL") && (resList.length()==6))
                    {
                        RFchans[i] = new RFchannel(ui->lblBackground,resList[1],resList[2],resList[4].toInt(),resList[5].toInt());
                        RFchans[i]->Channel = resList[3].toInt();
                        RFchans[i]->comms = FindCommPort(resList[2],Systems);
                        RFchans[i]->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "DCBCHANNELS") && (resList.length()==2))
            {
                numDCBchannels = resList[1].toInt();
                DCBchans = new DCBchannel * [numDCBchannels];
                for(int i = 0; i < numDCBchannels; i++)
                {
                    DCBchans[i] = NULL;
                    // Read each DCbias channel
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "DCBCHANNEL") && (resList.length()==6))
                    {
                        DCBchans[i] = new DCBchannel(ui->lblBackground,resList[1],resList[2],resList[4].toInt(),resList[5].toInt());
                        DCBchans[i]->Channel = resList[3].toInt();
                        DCBchans[i]->comms = FindCommPort(resList[2],Systems);
                        DCBchans[i]->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "DCBOFFSETS") && (resList.length()==2))
            {
                numDCBoffsets = resList[1].toInt();
                DCBoffsets = new DCBoffset * [numDCBoffsets];
                for(int i = 0; i < numDCBoffsets; i++)
                {
                    DCBoffsets[i] = NULL;
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "DCBOFFSET") && (resList.length()==6))
                    {
                        DCBoffsets[i] = new DCBoffset(ui->lblBackground,resList[1],resList[2],resList[4].toInt(),resList[5].toInt());
                        DCBoffsets[i]->Channel = resList[3].toInt();
                        DCBoffsets[i]->comms = FindCommPort(resList[2],Systems);
                        DCBoffsets[i]->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "DCBENABLES") && (resList.length()==2))
            {
                numDCBenables = resList[1].toInt();
                DCBenables = new DCBenable * [numDCBenables];
                for(int i = 0; i < numDCBenables; i++)
                {
                    DCBenables[i] = NULL;
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "DCBENABLE") && (resList.length()==5))
                    {
                        DCBenables[i] = new DCBenable(ui->lblBackground,resList[1],resList[2],resList[3].toInt(),resList[4].toInt());
                        DCBenables[i]->comms = FindCommPort(resList[2],Systems);
                        DCBenables[i]->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "DIOCHANNELS") && (resList.length()==2))
            {
                numDIOchannels = resList[1].toInt();
                DIOchannels = new DIOchannel * [numDIOchannels];
                for(int i = 0; i < numDIOchannels; i++)
                {
                    DIOchannels[i] = NULL;
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "DIOCHANNEL") && (resList.length()==6))
                    {
                        DIOchannels[i] = new DIOchannel(ui->lblBackground,resList[1],resList[2],resList[4].toInt(),resList[5].toInt());
                        DIOchannels[i]->Channel = resList[3];
                        DIOchannels[i]->comms = FindCommPort(resList[2],Systems);
                        DIOchannels[i]->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "ESICHANNELS") && (resList.length()==2))
            {
                numESIchannels = resList[1].toInt();
                ESIchans = new ESI * [numESIchannels];
                for(int i = 0; i < numESIchannels; i++)
                {
                    ESIchans[i] = NULL;
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "ESICHANNEL") && (resList.length()==6))
                    {
                        ESIchans[i] = new ESI(ui->lblBackground,resList[1],resList[2],resList[4].toInt(),resList[5].toInt());
                        ESIchans[i]->Channel = resList[3].toInt();
                        ESIchans[i]->comms = FindCommPort(resList[2],Systems);
                        ESIchans[i]->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "ARBCHANNELS") && (resList.length()==2))
            {
                numARBchannels = resList[1].toInt();
                ARBchans = new ARBchannel * [numARBchannels];
                for(int i = 0; i < numARBchannels; i++)
                {
                    ARBchans[i] = NULL;
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "ARBCHANNEL") && (resList.length()==6))
                    {
                        ARBchans[i] = new ARBchannel(ui->lblBackground,resList[1],resList[2],resList[4].toInt(),resList[5].toInt());
                        ARBchans[i]->Channel = resList[3].toInt();
                        ARBchans[i]->comms = FindCommPort(resList[2],Systems);
                        ARBchans[i]->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "IFT") && (resList.length()==5))
            {
                IFT = new IFTtiming(ui->lblBackground,resList[1],resList[2],resList[3].toInt(),resList[4].toInt());
                IFT->comms = FindCommPort(resList[2],Systems);
                IFT->statusBar = statusBar;
                IFT->Show();
                connect(IFT,SIGNAL(dataAcquired(QString)),this,SLOT(slotDataAcquired(QString)));
            }
            if((resList[0].toUpper() == "GRID1") && (resList.length()==2))
            {
                if(IFT!=NULL)
                {
                    IFT->Grid1 = NULL;
                    for(int i=0; i< numDCBchannels; i++) if(DCBchans[i]->Title == resList[1]) IFT->Grid1 = DCBchans[i];
                }
            }
            if((resList[0].toUpper() == "GRID2") && (resList.length()==2))
            {
                if(IFT!=NULL)
                {
                    IFT->Grid2 = NULL;
                    for(int i=0; i< numDCBchannels; i++) if(DCBchans[i]->Title == resList[1]) IFT->Grid2 = DCBchans[i];
                }
            }
            if((resList[0].toUpper() == "GRID3") && (resList.length()==2))
            {
                if(IFT!=NULL)
                {
                    IFT->Grid3 = NULL;
                    for(int i=0; i< numDCBchannels; i++) if(DCBchans[i]->Title == resList[1]) IFT->Grid3 = DCBchans[i];
                }
            }
            if((resList[0].toUpper() == "ENABLE") && (resList.length()==2))
            {
                if(IFT!=NULL)
                {
                    IFT->Enable = resList[1];
                }
            }
            if((resList[0].toUpper() == "SHUTDOWN") && (resList.length()==4))
            {
                SD = new Shutdown(ui->lblBackground,resList[1],resList[2].toInt(),resList[3].toInt());
                SD->Show();
                connect(SD,SIGNAL(ShutdownSystem()),this,SLOT(pbSD()));
                connect(SD,SIGNAL(EnableSystem()),this,SLOT(pbSE()));
                SD->setFocus();
            }
            if((resList[0].toUpper() == "SAVELOAD") && (resList.length()==5))
            {
                SL = new SaveLoad(ui->lblBackground,resList[1],resList[2],resList[3].toInt(),resList[4].toInt());
                SL->Show();
                connect(SL,SIGNAL(Save()),this,SLOT(pbSave()));
                connect(SL,SIGNAL(Load()),this,SLOT(pbLoad()));
            }
            if((resList[0].toUpper() == "MIPSCOMMS") && (resList.length()==3))
            {
                // Enable the MIPS communication button
                MIPScommsButton = new QPushButton("MIPS comms",ui->lblBackground);
                MIPScommsButton->setGeometry(resList[1].toInt(),resList[2].toInt(),150,32);
                MIPScommsButton->setAutoDefault(false);
                MIPScommsButton->setToolTip("Press this button to manually send commands to MIPS");
                connect(MIPScommsButton,SIGNAL(pressed()),this,SLOT(pbMIPScomms()));
            }
            if(resList[0].toUpper() == "ACQUIRE")
            {
                if(IFT!=NULL)
                {
                    IFT->Acquire = line.mid(line.indexOf(",")+1);
                }
            }
            if((resList[0].toUpper() == "SCRIPT") && (resList.length()==3))
            {
                // Use QTscrip and place a button at the defined location.
                ScriptButton = new QPushButton("Scripting",ui->lblBackground);
                ScriptButton->setGeometry(resList[1].toInt(),resList[2].toInt(),150,32);
                ScriptButton->setAutoDefault(false);
                ScriptButton->setToolTip("Press this button load or define a script");
                connect(ScriptButton,SIGNAL(pressed()),this,SLOT(pbScript()));
                scriptconsole = NULL;
                /* old scripting code
                QWidget *widget = new QWidget(this);
                widget->setGeometry(resList[1].toInt(),resList[2].toInt(),350,210);
                QVBoxLayout *layout = new QVBoxLayout(widget);
                script = new Script();
                layout->addWidget(script);
                // Connect to the slots for loading and shutdown
                connect(script,SIGNAL(Shutdown()),this,SLOT(scriptShutdown()));
                connect(script,SIGNAL(Load(QString)),this,SLOT(scriptLoad(QString)));
                */
            }
            if((resList[0].toUpper() == "DCBGROUPS") && (resList.length()==3))
            {
                DCBgroups = new DCBiasGroups(ui->lblBackground,resList[1].toInt(),resList[2].toInt());
                DCBgroups->Show();
                connect(DCBgroups,SIGNAL(disable()),this,SLOT(DCBgroupDisable()));
                connect(DCBgroups,SIGNAL(enable()),this,SLOT(DCBgroupEnable()));
            }
            if((resList[0].toUpper() == "INITPARMS") && (resList.length()==2))
            {
                // Load the file and apply the initialization MIPS commands
                InitMIPSsystems(resList[1]);
            }

        } while(!line.isNull());
    }
    file.close();
}

ControlPanel::~ControlPanel()
{
    delete ui;
}

void ControlPanel::reject()
{
    QMessageBox msgBox;

    if(SystemIsShutdown)  // If it shutdown send warning
    {
        QString msg = "The system is currently shutdown, this means all volatges are disabled and ";
        msg += "all RF drive levels are set to 0. Make sure you have saved all your settings ";
        msg += "because when the control panel is restarted you will lose the shutdown recover data.\n";
        msgBox.setText(msg);
        msgBox.setInformativeText("Are you sure you want to exit?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if(ret == QMessageBox::No) return;
    }
    emit DialogClosed();
}

void ControlPanel::InitMIPSsystems(QString initFilename)
{
    // Open the file and send commands to MIPS systems.
    QFile file(initFilename);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        QTextStream stream(&file);
        QString line;
        do
        {
            line = stream.readLine();
            if(line.trimmed().startsWith("#")) continue;
            QStringList resList = line.split(",");
            Comms *cp =  FindCommPort(resList[0],Systems);
            if(cp!=NULL) cp->SendString(line.mid(line.indexOf(resList[0]) + resList[0].count() + 1) + "\n");
        } while(!line.isNull());
        file.close();
    }
}

// Returns a pointer to the comm port for the MIPS system defined my its name.
// Returns null if the MIPS system can't be found.
Comms* ControlPanel::FindCommPort(QString name, QList<Comms*> Systems)
{
   for(int i = 0; i < Systems.length(); i++)
   {
       if(Systems.at(i)->MIPSname == name) return(Systems.at(i));
   }
   return NULL;
}

void ControlPanel::Update(void)
{
   int i;

   if(UpdateStop) return;
   if(StartMIPScomms)
   {
       mc = new MIPScomms(0,Systems);
       mc->show();
       UpdateStop = true;
       connect(mc, SIGNAL(DialogClosed()), this, SLOT(CloseMIPScomms()));
       StartMIPScomms = false;
       UpdateStop = true;
       return;
   }
   if(UpdateHoldOff > 0)
   {
       UpdateHoldOff--;
       return;
   }
   if(ShutdownFlag)
   {
       ShutdownFlag = false;
       SystemIsShutdown = true;
       UpdateHoldOff = 2;
       // Make sure all MIPS systems are in local mode
       for(int i=0;i<Systems.count();i++) Systems[i]->SendString("SMOD,LOC\n");
       for(int i=0;i<numESIchannels;i++) ESIchans[i]->Shutdown();
       for(int i=0;i<numDCBenables;i++)  DCBenables[i]->Shutdown();
       for(int i=0;i<numRFchannels;i++)  RFchans[i]->Shutdown();
       if(statusBar != NULL) statusBar->showMessage("System shutdown, " + QDateTime().currentDateTime().toString());
       return;
   }
   if(RestoreFlag)
   {
       RestoreFlag = false;
       SystemIsShutdown = false;
       UpdateHoldOff = 2;
       for(int i=0;i<numESIchannels;i++) ESIchans[i]->Restore();
       for(int i=0;i<numDCBenables;i++)  DCBenables[i]->Restore();
       for(int i=0;i<numRFchannels;i++)  RFchans[i]->Restore();
       if(statusBar != NULL) statusBar->showMessage("System enabled, " + QDateTime().currentDateTime().toString());
       return;
   }
   for(i=0;i<numRFchannels;i++)  if(RFchans[i] != NULL)     RFchans[i]->Update();
   for(i=0;i<numDCBchannels;i++) if(DCBchans[i] != NULL)    DCBchans[i]->Update();
   for(i=0;i<numDCBoffsets;i++)  if(DCBoffsets[i] != NULL)  DCBoffsets[i]->Update();
   for(i=0;i<numDCBenables;i++)  if(DCBenables[i] != NULL)  DCBenables[i]->Update();
   for(i=0;i<numDIOchannels;i++) if(DIOchannels[i] != NULL) DIOchannels[i]->Update();
   for(i=0;i<numESIchannels;i++) if(ESIchans[i] != NULL)    ESIchans[i]->Update();
   for(i=0;i<numARBchannels;i++) if(ARBchans[i] != NULL)    ARBchans[i]->Update();
}

QString ControlPanel::Save(QString Filename)
{
    QString res;

    UpdateHoldOff = 1000;
    if(Filename == "") return "No file defined!";
    QFile file(Filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# Control panel settings, " + dateTime.toString() + "\n";
        if(DCBgroups != NULL)
        {
            stream << "DC bias groups\n";
            stream << DCBgroups->Report() + "\n";
        }
        if(numRFchannels > 0)
        {
            stream << "RF channels," + QString::number(numRFchannels) + "\n";
            for(int i=0; i<numRFchannels; i++)
            {
                if(RFchans[i] != NULL) stream << RFchans[i]->Report() + "\n";
            }
        }
        if(numDCBchannels > 0)
        {
            stream << "DCB channels," + QString::number(numDCBchannels) + "\n";
            for(int i=0; i<numDCBchannels; i++)
            {
                if(DCBchans[i] != NULL) stream << DCBchans[i]->Report() + "\n";
            }
        }
        if(numDCBoffsets > 0)
        {
            stream << "DCB offsets," + QString::number(numDCBoffsets) + "\n";
            for(int i=0; i<numDCBoffsets; i++)
            {
                if(DCBoffsets[i] != NULL) stream << DCBoffsets[i]->Report() + "\n";
            }
        }
        if(numDCBenables > 0)
        {
            stream << "DCB enables," + QString::number(numDCBenables) + "\n";
            for(int i=0; i<numDCBenables; i++)
            {
                if(DCBenables[i] != NULL) stream << DCBenables[i]->Report() + "\n";
            }
        }
        if(numDIOchannels > 0)
        {
            stream << "DIO channels," + QString::number(numDIOchannels) + "\n";
            for(int i=0; i<numDIOchannels; i++)
            {
                if(DIOchannels[i] != NULL) stream << DIOchannels[i]->Report() + "\n";
            }
        }
        if(numESIchannels > 0)
        {
            stream << "ESI channels," + QString::number(numESIchannels) + "\n";
            for(int i=0; i<numESIchannels; i++)
            {
                if(ESIchans[i] != NULL) stream << ESIchans[i]->Report() + "\n";
            }
        }
        if(numARBchannels > 0)
        {
            stream << "ARB channels," + QString::number(numARBchannels) + "\n";
            for(int i=0; i<numARBchannels; i++)
            {
                if(ARBchans[i] != NULL) stream << ARBchans[i]->Report() + "\n";
            }
        }
        if(IFT != NULL)
        {
            stream << "IFT parameters\n";
            stream << IFT->Report() + "\n";
        }
        file.close();
        UpdateHoldOff = 5;
        return "Settings saved to " + Filename;
    }
    return "Can't open file!";
}

QString ControlPanel::Load(QString Filename)
{
    QStringList resList;

    UpdateHoldOff = 1000;
    if(Filename == "") return "No file defined!";
    QFile file(Filename);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        // We're going to streaming the file
        // to a QString
        QTextStream stream(&file);
        QString line;
        do
        {
            line = stream.readLine();
            resList = line.split(",");
            if(resList.count() >= 2)
            {
                if(resList[0] == "RF channels") for(int i=0;i<resList[1].toInt();i++)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     for(int j=0;j<numRFchannels;j++) if(RFchans[j]->SetValues(line)) break;
                }
                if(resList[0] == "DCB channels") for(int i=0;i<resList[1].toInt();i++)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     for(int j=0;j<numDCBchannels;j++) if(DCBchans[j]->SetValues(line)) break;
                }
                if(resList[0] == "DCB offsets") for(int i=0;i<resList[1].toInt();i++)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     for(int j=0;j<numDCBoffsets;j++) if(DCBoffsets[j]->SetValues(line)) break;
                }
                if(resList[0] == "DCB enables") for(int i=0;i<resList[1].toInt();i++)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     for(int j=0;j<numDCBenables;j++) if(DCBenables[j]->SetValues(line)) break;
                }
                if(resList[0] == "DIO channels") for(int i=0;i<resList[1].toInt();i++)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     for(int j=0;j<numDIOchannels;j++) if(DIOchannels[j]->SetValues(line)) break;
                }
                if(resList[0] == "ESI channels") for(int i=0;i<resList[1].toInt();i++)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     for(int j=0;j<numESIchannels;j++) if(ESIchans[j]->SetValues(line)) break;
                }
                if(resList[0] == "ARB channels") for(int i=0;i<resList[1].toInt();i++)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     for(int j=0;j<numARBchannels;j++) if(ARBchans[j]->SetValues(line)) break;
                }
            }
            else if(resList.count() == 1)
            {
                if(resList[0] == "IFT parameters")
                {
                    line = stream.readLine();
                    if(line.isNull()) break;
                    if(IFT != NULL) IFT->SetValues(line);
                }
                if(resList[0] == "DC bias groups")
                {
                    line = stream.readLine();
                    if(line.isNull()) break;
                    if(DCBgroups != NULL) DCBgroups->SetValues(line);
                }
            }
        } while(!line.isNull());
        UpdateHoldOff = 5;
        file.close();
        return "Settings loaded from " + Filename;
    }
    return "Can't open file!";
}

// Called after data collection, save method to the filepath passed
void ControlPanel::slotDataAcquired(QString filepath)
{
    if(filepath != "")
    {
        Save(filepath + "/Method.settings");
    }
}

// System shutdown
//   Disable ESI
//   Disable all DCbias modules
//   Disable all RF drives
void ControlPanel::pbSD(void)
{
    ShutdownFlag = true;
}

// System enable
void ControlPanel::pbSE(void)
{
    RestoreFlag = true;
}

void ControlPanel::pbSave(void)
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
    statusBar->showMessage(Save(fileName), 5000);
}

void ControlPanel::pbLoad(void)
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
    if(fileName == "") return;
    statusBar->showMessage(Load(fileName), 5000);
}

void ControlPanel::CloseMIPScomms(void)
{
    UpdateStop = false;
}

void ControlPanel::pbMIPScomms(void)
{
   StartMIPScomms = true;
}

void ControlPanel::scriptShutdown(void)
{
    ShutdownFlag = true;
}

void ControlPanel::scriptLoad(QString Filename)
{
    script->Message(Load(Filename));
}

void ControlPanel::DCBgroupDisable(void)
{
    for(int i=0;i<numDCBchannels;i++) if(DCBchans[i] != NULL) DCBchans[i]->LinkEnable = false;
}

DCBchannel * ControlPanel::FindDCBchannel(QString name)
{
    for(int i=0;i<numDCBchannels;i++) if(DCBchans[i]->Title.toUpper() == name.toUpper()) return DCBchans[i];
    return NULL;
}

void ControlPanel::DCBgroupEnable(void)
{
    DCBchannel            *dcb;
    QList<DCBchannel*>    DCBs;

    for(int i=0;i<DCBgroups->comboGroups->count();i++)
    {
        QStringList resList = DCBgroups->comboGroups->itemText(i).split(",");
        DCBs.clear();
        for(int j=0;j<resList.count();j++) if((dcb=FindDCBchannel(resList[j])) != NULL) DCBs.append(dcb);
        for(int j=0;j<numDCBchannels;j++)
        {
            if(DCBs.contains(DCBchans[j]))
            {
               DCBchans[j]->DCBs = DCBs;
               DCBchans[j]->LinkEnable = true;
            }
        }
    }
}

void ControlPanel::pbScript(void)
{
    if(!scriptconsole) scriptconsole = new ScriptingConsole(this);
    scriptconsole->show();
}

// The following functions are for the scripting system
bool ControlPanel::SendCommand(QString MIPSname, QString message)
{
   QApplication::processEvents();
   Comms *cp =  FindCommPort(MIPSname,Systems);
   if(cp==NULL) return false;
   return cp->SendCommand(message);
}

QString ControlPanel::SendMess(QString MIPSname, QString message)
{
    QApplication::processEvents();
    Comms *cp =  FindCommPort(MIPSname,Systems);
    if(cp==NULL) return "MIPS not found!";
    return cp->SendMess(message);
}

void ControlPanel::SystemEnable(void)
{
    QApplication::processEvents();
    RestoreFlag = true;
}

void ControlPanel::SystemShutdown(void)
{
    QApplication::processEvents();
    ShutdownFlag = true;
}

void ControlPanel::Acquire(QString filePath)
{
   QApplication::processEvents();
   if(IFT != NULL)
   {
       IFT->AcquireData(filePath);
   }
}

bool ControlPanel::isAcquiring(void)
{
  QApplication::processEvents();
  if(IFT != NULL) return(IFT->Acquiring);
  return(false);
}

void ControlPanel::DismissAcquire(void)
{
  QApplication::processEvents();
  if(IFT != NULL)  IFT->Dismiss();
  QApplication::processEvents();
  QApplication::processEvents();
}

void ControlPanel::msDelay(int ms)
{
    QTime timer;

    timer.start();
    while(timer.elapsed() < ms) QApplication::processEvents();
}

void ControlPanel::statusMessage(QString message)
{
    QApplication::processEvents();
    statusBar->showMessage(message);
    QApplication::processEvents();
}

void ControlPanel::popupMessage(QString message)
{
    QMessageBox msgBox;

    QApplication::processEvents();
    msgBox.setText(message);
    msgBox.setInformativeText("");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

bool ControlPanel::popupYesNoMessage(QString message)
{
    QMessageBox msgBox;

    QApplication::processEvents();
    msgBox.setText(message);
    msgBox.setInformativeText("Are you sure?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if(ret == QMessageBox::No) return false;
    return true;
}


// *************************************************************************************************
// Text box  ***************************************************************************************
// *************************************************************************************************

TextLabel::TextLabel(QWidget *parent, QString name, int s, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    Size   = s;
    X      = x;
    Y      = y;
}

void TextLabel::Show(void)
{
    label = new QLabel(Title,p); label->setGeometry(X,Y,1,1);
    QFont font = label->font();
    font.setPointSize(Size);
    label->setFont(font);
    label->adjustSize();
}

// *************************************************************************************************
// System shutdown and restore button  *************************************************************
// *************************************************************************************************

Shutdown::Shutdown(QWidget *parent, QString name, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    X      = x;
    Y      = y;
}

void Shutdown::Show(void)
{
    pbShutdown = new QPushButton("System shutdown",p);
    pbShutdown->setGeometry(X,Y,150,32);
    pbShutdown->setAutoDefault(false);
    connect(pbShutdown,SIGNAL(pressed()),this,SLOT(pbPressed()));
}

void Shutdown::pbPressed(void)
{
    if(pbShutdown->text() == "System shutdown")
    {
        pbShutdown->setText("System enable");
        emit ShutdownSystem();
    }
    else
    {
        pbShutdown->setText("System shutdown");
        emit EnableSystem();
    }
}

// *************************************************************************************************
// Save and load buttons  **************************************************************************
// *************************************************************************************************

SaveLoad::SaveLoad(QWidget *parent, QString nameSave, QString nameLoad, int x, int y) : QWidget(parent)
{
    p          = parent;
    TitleSave  = nameSave;
    TitleLoad  = nameLoad;
    X          = x;
    Y          = y;
}

void SaveLoad::Show(void)
{
    pbSave = new QPushButton(TitleSave,p);
    pbSave->setGeometry(X,Y,150,32);
    pbSave->setAutoDefault(false);
    pbLoad = new QPushButton(TitleLoad,p);
    pbLoad->setGeometry(X,Y+40,150,32);
    pbLoad->setAutoDefault(false);
    connect(pbSave,SIGNAL(pressed()),this,SLOT(pbSavePressed()));
    connect(pbLoad,SIGNAL(pressed()),this,SLOT(pbLoadPressed()));
}

void SaveLoad::pbSavePressed(void)
{
    pbSave->setDown(false);
    emit Save();
}

void SaveLoad::pbLoadPressed(void)
{
    pbLoad->setDown(false);
    emit Load();
}

// *************************************************************************************************
// RF driver channel  ******************************************************************************
// *************************************************************************************************

RFchannel::RFchannel(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    comms  = NULL;
    isShutdown = false;
    qApp->installEventFilter(this);
    Updating = false;
    UpdateOff = false;
}

void RFchannel::Show(void)
{
    // Make a group box
    gbRF = new QGroupBox(Title,p);
    gbRF->setGeometry(X,Y,200,180);
    gbRF->setToolTip(MIPSnm + " RF channel " + QString::number(Channel));
    // Build and place the RF parameters group box
    Drive = new QLineEdit(gbRF); Drive->setGeometry(60,22,91,21);  Drive->setValidator(new QDoubleValidator);
    Freq = new QLineEdit(gbRF);  Freq->setGeometry(60,46,91,21);   Freq->setValidator(new QIntValidator);
    RFP = new QLineEdit(gbRF);   RFP->setGeometry(60,70,91,21);    RFP->setReadOnly(true);
    RFN = new QLineEdit(gbRF);   RFN->setGeometry(60,94,91,21);    RFN->setReadOnly(true);
    Power = new QLineEdit(gbRF); Power->setGeometry(60,118,91,21); Power->setReadOnly(true);
    // Place push buttons
    Tune = new QPushButton("Tune",gbRF);     Tune->setGeometry(10,147,81,32);    Tune->setAutoDefault(false);
    Retune = new QPushButton("Retune",gbRF); Retune->setGeometry(102,147,81,32); Retune->setAutoDefault(false);
    // Add labels
    labels[0] = new QLabel("Drive",gbRF); labels[0]->setGeometry(10,26,59,16);
    labels[1] = new QLabel("Freq",gbRF);  labels[1]->setGeometry(10,48,59,16);
    labels[2] = new QLabel("RF+",gbRF);   labels[2]->setGeometry(10,73,59,16);
    labels[3] = new QLabel("RF-",gbRF);   labels[3]->setGeometry(10,96,59,16);
    labels[4] = new QLabel("Power",gbRF); labels[4]->setGeometry(10,118,59,16);

    labels[5] = new QLabel("%",gbRF);     labels[5]->setGeometry(159,26,31,21);
    labels[6] = new QLabel("Hz",gbRF);    labels[6]->setGeometry(159,48,31,21);
    labels[7] = new QLabel("Vp-p",gbRF);  labels[7]->setGeometry(159,73,31,21);
    labels[8] = new QLabel("Vp-p",gbRF);  labels[8]->setGeometry(159,96,31,21);
    labels[9] = new QLabel("W",gbRF);     labels[9]->setGeometry(159,118,31,21);
    // Connect to the event slots
    connect(Drive,SIGNAL(editingFinished()),this,SLOT(DriveChange()));
    connect(Freq,SIGNAL(editingFinished()),this,SLOT(FreqChange()));
    connect(Tune,SIGNAL(pressed()),this,SLOT(TunePressed()));
    connect(Retune,SIGNAL(pressed()),this,SLOT(RetunePressed()));
}

bool RFchannel::eventFilter(QObject *obj, QEvent *event)
{
    float delta = 0;

    if (((obj == Drive) || (obj == Freq)) && (event->type() == QEvent::KeyPress))
    {
        if(Updating) return true;
        UpdateOff = true;
        QKeyEvent *key = static_cast<QKeyEvent *>(event);
        if(key->key() == 16777235) delta = 0.1;
        if(key->key() == 16777237) delta = -0.1;
        if((QApplication::queryKeyboardModifiers() & 0x2000000) != 0) delta *= 10;
        if((QApplication::queryKeyboardModifiers() & 0x8000000) != 0) delta *= 100;
        if(delta != 0)
        {
           QString myString;
           if(obj == Drive)
           {
               myString.sprintf("%3.2f", Drive->text().toFloat() + delta);
               Drive->setText(myString);
               Drive->setModified(true);
               Drive->editingFinished();
               UpdateOff = false;
           }
           if(obj == Freq)
           {
               myString.sprintf("%5.0f", Freq->text().toFloat() + delta*1000);
               Freq->setText(myString);
               Freq->setModified(true);
               Freq->editingFinished();
               UpdateOff = false;
           }
           return true;
        }
    }
    UpdateOff = false;
    return QObject::eventFilter(obj, event);
}

// Returns a string with the following information
// Name,RFdrive,RFfreq,RFvolt+,RFvolt-,Power
QString RFchannel::Report(void)
{
    QString res;

    if(isShutdown) res = Title + "," + activeDrive;
    else res = Title + "," + Drive->text();
    res += "," + Freq->text() + "," + RFP->text() + "," + RFN->text() + "," + Power->text();
    return(res);
}

// Sets the RFchannel parameters if the name matches and we have atleast 3 total parms
bool RFchannel::SetValues(QString strVals)
{
    QStringList resList;

    if(!strVals.startsWith(Title)) return false;
    resList = strVals.split(",");
    if(resList.count() < 3) return false;
    Freq->setText(resList[2]);
    Freq->setModified(true);
    Freq->editingFinished();
    if(isShutdown)
    {
        activeDrive = resList[1];
    }
    else
    {
        Drive->setText(resList[1]);
        Drive->setModified(true);
        Drive->editingFinished();
    }
    return true;
 }

void RFchannel::Update(void)
{
    QString res;

    if(comms == NULL) return;
    if(UpdateOff) return;
    Updating = true;
    comms->rb.clear();
    res = "GRFDRV,"   + QString::number(Channel) + "\n";
    if(!Drive->hasFocus()) Drive->setText(comms->SendMess(res));
    res = "GRFFRQ,"   + QString::number(Channel) + "\n";
    if(!Freq->hasFocus()) Freq->setText(comms->SendMess(res));
    res = "GRFPPVP,"  + QString::number(Channel) + "\n";
    RFP->setText(comms->SendMess(res));
    res = "GRFPPVN,"  + QString::number(Channel) + "\n";
    RFN->setText(comms->SendMess(res));
    res = "GRFPWR,"   + QString::number(Channel) + "\n";
    Power->setText(comms->SendMess(res));
    Updating = false;
}

void RFchannel::DriveChange(void)
{
    if(comms == NULL) return;
    if(!Drive->isModified()) return;
    QString res = "SRFDRV," + QString::number(Channel) + "," + Drive->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    Drive->setModified(false);
}

void RFchannel::FreqChange(void)
{
    if(comms == NULL) return;
    if(!Freq->isModified()) return;
    QString res = "SRFFRQ," + QString::number(Channel) + "," + Freq->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    Freq->setModified(false);
}

void RFchannel::TunePressed(void)
{
    QMessageBox msgBox;

    Tune->setDown(false);
    QString msg = "This function will tune the RF head attached to " + MIPSnm + " channel " + QString::number(Channel) + ". ";
    msg += "Make sure the RF head is attached and connected to your system as needed. ";
    msg += "This process can take up to 3 minutes.\n";
    msgBox.setText(msg);
    msgBox.setInformativeText("Are you sure you want to continue?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if(ret == QMessageBox::No) return;
    QString res = "TUNERFCH," + QString::number(Channel) + "\n";
    comms->SendCommand(res);
}

void RFchannel::RetunePressed(void)
{
    QMessageBox msgBox;

    Retune->setDown(false);
    QString msg = "This function will retune the RF head attached to " + MIPSnm + " channel " + QString::number(Channel) + ". ";
    msg += "Make sure the RF head is attached and connected to your system as needed. ";
    msg += "This process can take up to 1 minute.\n";
    msgBox.setText(msg);
    msgBox.setInformativeText("Are you sure you want to continue?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if(ret == QMessageBox::No) return;
    QString res = "RETUNERFCH," + QString::number(Channel) + "\n";
    comms->SendCommand(res);
}

void RFchannel::Shutdown(void)
{
    if(isShutdown) return;
    isShutdown = true;
    activeDrive = Drive->text();
    Drive->setText("0");
    Drive->setModified(true);
    Drive->editingFinished();
}

void RFchannel::Restore(void)
{
    if(!isShutdown) return;
    isShutdown = false;
    Drive->setText(activeDrive);
    Drive->setModified(true);
    Drive->editingFinished();
}

// *************************************************************************************************
// DC bias channel  ********************************************************************************
// *************************************************************************************************

DCBchannel::DCBchannel(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    comms  = NULL;
    Updating = false;
    UpdateOff = false;
    qApp->installEventFilter(this);
    DCBs.clear();
    LinkEnable = false;
    CurrentVsp = 0;
}

void DCBchannel::Show(void)
{
    frmDCB = new QFrame(p); frmDCB->setGeometry(X,Y,241,21);
    Vsp = new QLineEdit(frmDCB); Vsp->setGeometry(70,0,70,21); Vsp->setValidator(new QDoubleValidator);
    Vrb = new QLineEdit(frmDCB); Vrb->setGeometry(140,0,70,21); Vrb->setReadOnly(true);
    labels[0] = new QLabel(Title,frmDCB); labels[0]->setGeometry(0,0,59,16);
    labels[1] = new QLabel("V",frmDCB);   labels[1]->setGeometry(220,0,21,16);
    connect(Vsp,SIGNAL(editingFinished()),this,SLOT(VspChange()));
    Vsp->setToolTip(MIPSnm + " channel " + QString::number(Channel));
}

bool DCBchannel::eventFilter(QObject *obj, QEvent *event)
{
    float delta = 0;

    if ((obj == Vsp) && (event->type() == QEvent::KeyPress))
    {
        if(Updating) return true;
        UpdateOff = true;
        QKeyEvent *key = static_cast<QKeyEvent *>(event);
        if(key->key() == 16777235) delta = 0.1;
        if(key->key() == 16777237) delta = -0.1;
        if((QApplication::queryKeyboardModifiers() & 0x2000000) != 0) delta *= 10;
        if((QApplication::queryKeyboardModifiers() & 0x8000000) != 0) delta *= 100;
        if(delta != 0)
        {
           QString myString;
           myString.sprintf("%3.2f", Vsp->text().toFloat() + delta);
           Vsp->setText(myString);
           Vsp->setModified(true);
           Vsp->editingFinished();
           UpdateOff = false;
           return true;
        }
    }
    UpdateOff = false;
    return QObject::eventFilter(obj, event);
}

QString DCBchannel::Report(void)
{
    QString res;

    res = Title + "," + Vsp->text() + "," + Vrb->text();
    return(res);
}

bool DCBchannel::SetValues(QString strVals)
{
    QStringList resList;

    if(!strVals.startsWith(Title)) return false;
    resList = strVals.split(",");
    if(resList.count() < 2) return false;
    Vsp->setText(resList[1]);
    CurrentVsp = Vsp->text().toFloat();
    Vsp->setModified(true);
    Vsp->editingFinished();
    return true;
 }

void DCBchannel::Update(void)
{
    QString res;

    if(comms == NULL) return;
    if(UpdateOff) return;
    Updating = true;
    comms->rb.clear();
    res = "GDCB,"  + QString::number(Channel) + "\n";
    if(!Vsp->hasFocus()) Vsp->setText(comms->SendMess(res));
    CurrentVsp = Vsp->text().toFloat();
    res = "GDCBV," + QString::number(Channel) + "\n";
    Vrb->setText(comms->SendMess(res));
    // Compare setpoint with readback and color the readback background
    // depending on the difference.
    // No data = white
    if((Vsp->text()=="") | (Vrb->text()==""))
    {
        Vrb->setStyleSheet("QLineEdit { background: rgb(255, 255, 255); }" );
        Updating = false;
        return;
    }
    float error = fabs(Vsp->text().toFloat() - Vrb->text().toFloat());
    if((error > 2.0) & (error > fabs(Vsp->text().toFloat()/100))) Vrb->setStyleSheet("QLineEdit { background: rgb(255, 204, 204); }" );
    else Vrb->setStyleSheet("QLineEdit { background: rgb(204, 255, 204); }" );
    //Vrb->setStyleSheet("QLineEdit { background: rgb(255, 204, 204); }" );  // light red
    //Vrb->setStyleSheet("QLineEdit { background: rgb(255, 255, 204); }" );  // light yellow
    //Vrb->setStyleSheet("QLineEdit { background: rgb(204, 255, 204); }" );  // light green
    Updating = false;
}

void DCBchannel::VspChange(void)
{
   //if(comms == NULL) return;
   if(!Vsp->isModified()) return;
   QString res = "SDCB," + QString::number(Channel) + "," + Vsp->text() + "\n";
   if(comms != NULL) comms->SendCommand(res.toStdString().c_str());
   // If this channel is part of a group calculate the delta and apply to all
   // other channels
   if((LinkEnable) && (DCBs.count()>0) && (CurrentVsp != Vsp->text().toFloat()))
   {
       float delta = CurrentVsp - Vsp->text().toFloat();
       foreach(DCBchannel * item, DCBs )
       {
           if(item != this)
           {
              item->CurrentVsp -= delta;
              item->Vsp->setText(QString::number(item->CurrentVsp,'f',2));
              item->CurrentVsp = item->Vsp->text().toFloat();
              item->Vsp->setModified(true);
              item->Vsp->editingFinished();
           }
       }
   }
   CurrentVsp = Vsp->text().toFloat();
   Vsp->setModified(false);
}

// *************************************************************************************************
// DC bias groups  *********************************************************************************
// *************************************************************************************************

DCBiasGroups::DCBiasGroups(QWidget *parent, int x, int y)
{
    p      = parent;
    X      = x;
    Y      = y;
}

void DCBiasGroups::Show(void)
{
    gbDCBgroups = new QGroupBox("Define DC bias channel groups",p);
    gbDCBgroups->setGeometry(X,Y,251,86);
    gbDCBgroups->setToolTip("DC bias groups are sets of DC bias channels that will track, so when you change one channel all other channels in the group will change by the same about.");
    comboGroups = new QComboBox(gbDCBgroups);
    comboGroups->setGeometry(5,25,236,26);
    comboGroups->setEditable(true);
    comboGroups->setToolTip("Define a group by entering a list of channel names seperated by a comma. You can define many groups.");
    DCBenaGroups = new QCheckBox(gbDCBgroups); DCBenaGroups->setGeometry(5,55,116,20);
    DCBenaGroups->setText("Enable groups");
    DCBenaGroups->setToolTip("Check enable to process the groups and apply.");
    comboGroups->installEventFilter(new DCbiasGroupsEventFilter);
    connect(DCBenaGroups,SIGNAL(clicked(bool)),this,SLOT(slotEnableChange()));
}

void DCBiasGroups::slotEnableChange(void)
{
    // If disable is selected then emit signal to disable;
    if(!DCBenaGroups->isChecked()) emit disable();
    // if emable is selected then emit signal to enable;
    if(DCBenaGroups->isChecked()) emit enable();
}

// One string is passed with all combobox entries. A semicolan seperates
// the entries.
bool DCBiasGroups::SetValues(QString strVals)
{
    DCBenaGroups->setChecked(false);
    comboGroups->clear();
    QStringList resList = strVals.split(";");
    for(int i=0;i<resList.count();i++)
    {
       comboGroups->addItem(resList[i]);
    }
    return true;
}

QString DCBiasGroups::Report(void)
{
    QString res = "";

    for(int i=0;i<comboGroups->count();i++)
    {
        if(res != "") res += ";";
        res += comboGroups->itemText(i);
    }
    return res;
}

bool DCbiasGroupsEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if ((keyEvent->key() == Qt::Key::Key_Backspace) && (keyEvent->modifiers() == Qt::ShiftModifier))
        {
            auto combobox = dynamic_cast<QComboBox *>(obj);
            if (combobox){
                combobox->removeItem(combobox->currentIndex());
                return true;
            }
        }
    }
    // standard event processing
    return QObject::eventFilter(obj, event);
}

// *************************************************************************************************
// DC bias offset  *********************************************************************************
// *************************************************************************************************

DCBoffset::DCBoffset(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    comms  = NULL;
}

void DCBoffset::Show(void)
{
    frmDCBO = new QFrame(p); frmDCBO->setGeometry(X,Y,170,21);
    Voff = new QLineEdit(frmDCBO); Voff->setGeometry(70,0,70,21); Voff->setValidator(new QDoubleValidator);
    labels[0] = new QLabel(Title,frmDCBO); labels[0]->setGeometry(0,0,59,16);
    labels[1] = new QLabel("V",frmDCBO);   labels[1]->setGeometry(150,0,21,16);
    Voff->setToolTip("Offset/range control " + MIPSnm);
    connect(Voff,SIGNAL(editingFinished()),this,SLOT(VoffChange()));
}

QString DCBoffset::Report(void)
{
    QString res;

    res = Title + "," + Voff->text();
    return(res);
}

bool DCBoffset::SetValues(QString strVals)
{
    QStringList resList;

    if(!strVals.startsWith(Title)) return false;
    resList = strVals.split(",");
    if(resList.count() < 2) return false;
    Voff->setText(resList[1]);
    Voff->setModified(true);
    Voff->editingFinished();
    return true;
 }

void DCBoffset::Update(void)
{
    QString res;

    if(comms == NULL) return;
    comms->rb.clear();
    res = "GDCBOF,"  + QString::number(Channel) + "\n";
    if(!Voff->hasFocus()) Voff->setText(comms->SendMess(res));
}

void DCBoffset::VoffChange(void)
{
   if(comms == NULL) return;
   if(!Voff->isModified()) return;
   QString res = "SDCBOF," + QString::number(Channel) + "," + Voff->text() + "\n";
   comms->SendCommand(res.toStdString().c_str());
   Voff->setModified(false);
}

// *************************************************************************************************
// DC bias enable  *********************************************************************************
// *************************************************************************************************

DCBenable::DCBenable(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    comms  = NULL;
    isShutdown = false;
}

void DCBenable::Show(void)
{
    frmDCBena = new QFrame(p); frmDCBena->setGeometry(X,Y,170,21);
    DCBena = new QCheckBox(frmDCBena); DCBena->setGeometry(0,0,170,21);
    DCBena->setText(Title);
    DCBena->setToolTip("Enables all DC bias channels on " + MIPSnm);
    connect(DCBena,SIGNAL(stateChanged(int)),this,SLOT(DCBenaChange()));
}

QString DCBenable::Report(void)
{
    QString res;

    res = Title + ",";
    if(isShutdown)
    {
        if(activeEnableState) res += "ON";
        else res += "OFF";
    }
    else
    {
        if(DCBena->isChecked()) res += "ON";
        else res += "OFF";
    }
    return(res);
}

bool DCBenable::SetValues(QString strVals)
{
    QStringList resList;

    if(!strVals.startsWith(Title)) return false;
    resList = strVals.split(",");
    if(resList.count() < 2) return false;
    if(isShutdown)
    {
        if(resList[1].contains("ON")) activeEnableState = true;
        else activeEnableState = false;
        return true;
    }
    if(resList[1].contains("ON")) DCBena->setChecked(true);
    else DCBena->setChecked(false);
    if(resList[1].contains("ON")) DCBena->stateChanged(1);
    else  DCBena->stateChanged(0);
    return true;
 }

void DCBenable::Update(void)
{
    QString res;

    if(comms == NULL) return;
    comms->rb.clear();
    res = comms->SendMess("GDCPWR\n");
    if(res.contains("ON")) DCBena->setChecked(true);
    if(res.contains("OFF")) DCBena->setChecked(false);
}

void DCBenable::DCBenaChange(void)
{
   QString res;

   if(comms == NULL) return;
   if(DCBena->checkState()) res ="SDCPWR,ON\n";
   else res ="SDCPWR,OFF\n";
   comms->SendCommand(res.toStdString().c_str());
}

void DCBenable::Shutdown(void)
{
    if(isShutdown) return;
    isShutdown = true;
    activeEnableState = DCBena->checkState();
    DCBena->setChecked(false);
    DCBena->stateChanged(0);
}

void DCBenable::Restore(void)
{
    if(!isShutdown) return;
    isShutdown = false;
    DCBena->setChecked(activeEnableState);
    DCBena->stateChanged(0);
}

// *************************************************************************************************
// DIO  ********************************************************************************************
// *************************************************************************************************

DIOchannel::DIOchannel(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    Channel = "A";
    comms  = NULL;
}

void DIOchannel::Show(void)
{
    frmDIO = new QFrame(p); frmDIO->setGeometry(X,Y,170,21);
    DIO = new QCheckBox(frmDIO); DIO->setGeometry(0,0,170,21);
    DIO->setText(Title);
    DIO->setToolTip(MIPSnm + " DIO channel " + Channel);
    connect(DIO,SIGNAL(stateChanged(int)),this,SLOT(DIOChange()));
}

QString DIOchannel::Report(void)
{
    QString res;

    res = Title + ",";
    if(DIO->isChecked()) res += "1";
    else res += "0";
    return(res);
}

bool DIOchannel::SetValues(QString strVals)
{
    QStringList resList;

    if(!strVals.startsWith(Title)) return false;
    resList = strVals.split(",");
    if(resList.count() < 2) return false;
    if(resList[1].contains("1")) DIO->setChecked(true);
    else DIO->setChecked(false);
    if(resList[1].contains("1")) DIO->stateChanged(1);
    else DIO->stateChanged(0);
    return true;
 }

void DIOchannel::Update(void)
{
    QString res;

    if(comms == NULL) return;
    comms->rb.clear();
    res = comms->SendMess("GDIO," + Channel + "\n");
    if(res.contains("1")) DIO->setChecked(true);
    if(res.contains("0")) DIO->setChecked(false);
}

void DIOchannel::DIOChange(void)
{
   QString res;

   if(comms == NULL) return;
   if(DIO->checkState()) res ="SDIO," + Channel + ",1\n";
   else  res ="SDIO," + Channel + ",0\n";
   comms->SendCommand(res.toStdString().c_str());
}


// **********************************************************************************************
// ESI ******************************************************************************************
// **********************************************************************************************

ESI::ESI(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    comms  = NULL;
    isShutdown = false;
}

void ESI::Show(void)
{
    frmESI = new QFrame(p); frmESI->setGeometry(X,Y,241,42);
    ESIsp = new QLineEdit(frmESI); ESIsp->setGeometry(70,0,70,21); ESIsp->setValidator(new QDoubleValidator);
    ESIrb = new QLineEdit(frmESI); ESIrb->setGeometry(140,0,70,21); ESIrb->setReadOnly(true);
    ESIena = new QCheckBox(frmESI); ESIena->setGeometry(70,22,70,21);
    ESIena->setText("Enable");
    labels[0] = new QLabel(Title,frmESI); labels[0]->setGeometry(0,0,59,16);
    labels[1] = new QLabel("V",frmESI);   labels[1]->setGeometry(220,0,21,16);
    ESIsp->setToolTip(MIPSnm + " ESI channel " + QString::number(Channel));
    connect(ESIsp,SIGNAL(editingFinished()),this,SLOT(ESIChange()));
    connect(ESIena,SIGNAL(stateChanged(int)),this,SLOT(ESIenaChange()));
}

QString ESI::Report(void)
{
    QString res;

    res = Title + ",";
    if(isShutdown)
    {
        if(activeEnableState) res += "ON,";
        else res += "OFF,";
    }
    else
    {
        if(ESIena->isChecked()) res += "ON,";
        else res += "OFF,";
    }
    res += ESIsp->text() + "," + ESIrb->text();
    return(res);
}

bool ESI::SetValues(QString strVals)
{
    QStringList resList;

    if(!strVals.startsWith(Title)) return false;
    resList = strVals.split(",");
    if(resList.count() < 3) return false;
    if(isShutdown)
    {
        if(resList[1].contains("ON")) activeEnableState = true;
        else  activeEnableState = false;
    }
    else
    {
        if(resList[1].contains("ON")) ESIena->setChecked(true);
        else ESIena->setChecked(false);
        if(resList[1].contains("ON")) ESIena->stateChanged(1);
        else ESIena->stateChanged(0);
    }
    ESIsp->setText(resList[2]);
    ESIsp->setModified(true);
    ESIsp->editingFinished();
    return true;
 }

void ESI::Update(void)
{
    QString res;

    if(comms == NULL) return;

    comms->rb.clear();
    res = "GHV,"  + QString::number(Channel) + "\n";
    if(!ESIsp->hasFocus()) ESIsp->setText(comms->SendMess(res));
    res = "GHVV," + QString::number(Channel) + "\n";
    ESIrb->setText(comms->SendMess(res));
    res = comms->SendMess("GHVSTATUS," + QString::number(Channel) + "\n");
    if(res.contains("ON")) ESIena->setChecked(true);
    if(res.contains("OFF")) ESIena->setChecked(false);
}

void ESI::ESIChange(void)
{
    if(comms == NULL) return;
    if(!ESIsp->isModified()) return;
    QString res = "SHV," + QString::number(Channel) + "," + ESIsp->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    ESIsp->setModified(false);
}

void ESI::ESIenaChange(void)
{
   QString res;

   if(comms == NULL) return;
   if(ESIena->checkState()) res ="SHVENA," + QString::number(Channel) + "\n";
   else res ="SHVDIS," + QString::number(Channel) + "\n";
   comms->SendCommand(res.toStdString().c_str());
}

void ESI::Shutdown(void)
{
    if(isShutdown) return;
    isShutdown = true;
    activeEnableState = ESIena->checkState();
    ESIena->setChecked(false);
    ESIena->stateChanged(0);
}

void ESI::Restore(void)
{
    if(!isShutdown) return;
    isShutdown = false;
    ESIena->setChecked(activeEnableState);
    ESIena->stateChanged(0);
}

// **********************************************************************************************
// ARB ******************************************************************************************
// **********************************************************************************************

ARBchannel::ARBchannel(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    comms  = NULL;
    statusBar = NULL;
}

void ARBchannel::Show(void)
{
    // Make a group box
    gbARB = new QGroupBox(Title,p);
    gbARB->setGeometry(X,Y,250,200);
    gbARB->setToolTip(MIPSnm + " ARB channel " + QString::number(Channel));
    // Place the controls on the group box
    leSWFREQ = new QLineEdit(gbARB);   leSWFREQ->setGeometry(100,22,91,21);  leSWFREQ->setValidator(new QIntValidator);
    leSWFVRNG = new QLineEdit(gbARB);  leSWFVRNG->setGeometry(100,46,91,21); leSWFVRNG->setValidator(new QDoubleValidator);
    leSWFVAUX = new QLineEdit(gbARB);  leSWFVAUX->setGeometry(100,70,91,21); leSWFVAUX->setValidator(new QDoubleValidator);
    leSWFVOFF = new QLineEdit(gbARB);  leSWFVOFF->setGeometry(100,94,91,21); leSWFVOFF->setValidator(new QDoubleValidator);
    Waveform = new QComboBox(gbARB); Waveform->setGeometry(100,118,91,21);
    Waveform->clear();
    Waveform->addItem("SIN","SIN");
    Waveform->addItem("RAMP","RAMP");
    Waveform->addItem("TRI","TRI");
    Waveform->addItem("PULSE","PULSE");
    Waveform->addItem("ARB","ARB");
    EditWaveform = new QPushButton("Edit",gbARB); EditWaveform->setGeometry(100,142,91,30); EditWaveform->setAutoDefault(false);
    SWFDIR_FWD = new QRadioButton("Forward",gbARB);  SWFDIR_FWD->setGeometry(20,166,91,21);
    SWFDIR_REV = new QRadioButton("Reverse",gbARB);  SWFDIR_REV->setGeometry(150,166,91,21);
    // Add labels
    labels[0] = new QLabel("Frequency",gbARB);     labels[0]->setGeometry(10,26,80,16);
    labels[1] = new QLabel("Amplitude",gbARB);     labels[1]->setGeometry(10,48,80,16);
    labels[2] = new QLabel("Aux output",gbARB);    labels[2]->setGeometry(10,73,80,16);
    labels[3] = new QLabel("Offset output",gbARB); labels[3]->setGeometry(10,96,80,16);
    labels[4] = new QLabel("Waveform",gbARB);      labels[4]->setGeometry(10,118,80,16);
    labels[5] = new QLabel("Hz",gbARB);   labels[5]->setGeometry(200,26,31,21);
    labels[6] = new QLabel("Vp-p",gbARB); labels[6]->setGeometry(200,48,31,21);
    labels[7] = new QLabel("V",gbARB);    labels[7]->setGeometry(200,73,31,21);
    labels[8] = new QLabel("V",gbARB);    labels[8]->setGeometry(200,96,31,21);
    // Connect to the event slots
    leSWFREQ->setObjectName("leSWFREQ");
    leSWFVRNG->setObjectName("leSWFVRNG");
    leSWFVAUX->setObjectName("leSWFVAUX");
    leSWFVOFF->setObjectName("leSWFVOFF");
    SWFDIR_FWD->setObjectName("SWFDIR_FWD");
    SWFDIR_REV->setObjectName("SWFDIR_REV");
    foreach(QObject *w, gbARB->children())
    {
       if(w->objectName().startsWith("le")) connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(leChange()));
    }
    connect(Waveform,SIGNAL(currentIndexChanged(int)),this,SLOT(wfChange()));
    connect(EditWaveform,SIGNAL(pressed()),this,SLOT(wfEdit()));
    connect(SWFDIR_FWD,SIGNAL(clicked(bool)),this,SLOT(rbChange()));
    connect(SWFDIR_REV,SIGNAL(clicked(bool)),this,SLOT(rbChange()));
}

QString ARBchannel::Report(void)
{
    QString res;

    res = Title + ",";
    res += leSWFREQ->text() + ",";
    res += leSWFVRNG->text() + ",";
    res += leSWFVAUX->text() + ",";
    res += leSWFVOFF->text() + ",";
    if(SWFDIR_FWD->isChecked()) res += "FWD,";
    else res += "REV,";
    res += Waveform->currentText();
    return res;
}

bool ARBchannel::SetValues(QString strVals)
{
    QStringList resList;

    if(!strVals.startsWith(Title)) return false;
    resList = strVals.split(",");
    if(resList.count() < 7) return false;
    leSWFREQ->setText(resList[1]);   leSWFREQ->setModified(true); leSWFREQ->editingFinished();
    leSWFVRNG->setText(resList[2]);  leSWFVRNG->setModified(true); leSWFVRNG->editingFinished();
    leSWFVAUX->setText(resList[3]);  leSWFVAUX->setModified(true); leSWFVAUX->editingFinished();
    leSWFVOFF->setText(resList[4]);  leSWFVOFF->setModified(true); leSWFVOFF->editingFinished();
    if(resList[5] == "ON")
    {
        SWFDIR_FWD->setChecked(true);
        SWFDIR_FWD->clicked(true);
    }
    else
    {
        SWFDIR_REV->setChecked(true);
        SWFDIR_REV->clicked(true);
    }
    int i = Waveform->findData(resList[6]);
    Waveform->setCurrentIndex(i);
    Waveform->currentIndexChanged(i);
    return true;
}

void ARBchannel::Update(void)
{
    QString res;

    if(comms==NULL) return;
    comms->rb.clear();
    // Update the line edit boxes
    foreach(QObject *w, gbARB->children())
    {
       if(w->objectName().startsWith("le"))
       {
           res = comms->SendMess("G" + w->objectName().mid(3) + "," + QString::number(Channel) + "\n");
           if(!((QLineEdit *)w)->hasFocus()) ((QLineEdit *)w)->setText(res);
       }
    }
    // Update the waveform selection box
    res = comms->SendMess("GWFTYP," + QString::number(Channel) + "\n");
    int i = Waveform->findData(res);
    Waveform->setCurrentIndex(i);
    // Update the dirction radio buttons
    res = comms->SendMess("GWFDIR," + QString::number(Channel) +"\n");
    if(res == "FWD") SWFDIR_FWD->setChecked(true);
    if(res == "REV") SWFDIR_REV->setChecked(true);
}

void ARBchannel::leChange(void)
{
    QObject* obj = sender();
    QString res;

    if(comms == NULL) return;
    if(!((QLineEdit *)obj)->isModified()) return;
    res = obj->objectName().mid(2) +"," + QString::number(Channel);
    res += "," + ((QLineEdit *)obj)->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    ((QLineEdit *)obj)->setModified(false);
}

void ARBchannel::rbChange(void)
{
    QString res = "SWFDIR,";

    if(comms == NULL) return;
    if(SWFDIR_FWD->isChecked()) res += QString::number(Channel) + ",FWD\n";
    if(SWFDIR_REV->isChecked()) res += QString::number(Channel) + ",REV\n";
    comms->SendCommand("SWFDIR,1,FWD\n");
}

void ARBchannel::wfChange(void)
{
    if(comms == NULL) return;
    comms->SendCommand("SWFTYP," + QString::number(Channel) + "," + Waveform->currentText() + "\n");
}

void ARBchannel::ReadWaveform(void)
{
    int Wform[32];
    QString cmd;
    int i;

    // Read waveform
    ARBwfEdit->GetWaveform(Wform);
    // Send waveform to MIPS
    cmd = "SWFARB,"  + Waveform->currentText();
    for(i=0; i<32; i++) cmd += "," + QString::number(Wform[i]);
    cmd += "\n";
    if(comms == NULL) return;
    if(!comms->SendCommand(cmd))
    {
        if(statusBar != NULL) statusBar->showMessage("Error sending waveform to MIPS",2000);
    }
}

void ARBchannel::wfEdit(void)
{
    QString res;
    int Wform[32];
    int i;

    for(i=0; i<32; i++) Wform[i] = i*4 - 64;
    // Read the ARB waveform from MIPS
    if(comms != NULL) res = comms->SendMess("GWFARB," + Waveform->currentText() + "\n");
    if(res.contains("?"))
    {
        // Here if the message was NAKed
        if(statusBar != NULL) statusBar->showMessage("Error reading waveform from MIPS",2000);
        return;
    }
    QStringList Vals = res.split(",");
    for(i=0; i<32; i++)
    {
        if(i < Vals.count()) Wform[i] = Vals[i].toInt();
        else Wform[i] = 0;
    }
    ARBwfEdit = new ARBwaveformEdit;
    connect(ARBwfEdit, SIGNAL(WaveformReady()), this, SLOT(ReadWaveform()));
    ARBwfEdit->SetWaveform(Wform);
    ARBwfEdit->show();
}

// **********************************************************************************************
// IFT timing generation ************************************************************************
// **********************************************************************************************
IFTtiming::IFTtiming(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    comms  = NULL;
    Enable = "";
    Acquire = "";
    Grid1 = Grid3 = Grid2 = NULL;
    statusBar = NULL;
    Acquiring = false;
}

void IFTtiming::Show(void)
{
    // Make a group box
    gbIFT = new QGroupBox(Title,p);
    gbIFT->setGeometry(X,Y,500,180);
    gbIFT->setToolTip(MIPSnm + " IFT timing generation");
    // Place the controls on the group box
    leFillTime = new QLineEdit(gbIFT);    leFillTime->setGeometry(95,25,51,21);   leFillTime->setValidator(new QIntValidator); leFillTime->setText("50");
    leTrapTime = new QLineEdit(gbIFT);    leTrapTime->setGeometry(95,50,51,21);   leTrapTime->setValidator(new QIntValidator); leTrapTime->setText("10");
    leReleaseTime = new QLineEdit(gbIFT); leReleaseTime->setGeometry(95,75,51,21);leReleaseTime->setValidator(new QIntValidator); leReleaseTime->setText("5");
    leGrid1FillV = new QLineEdit(gbIFT);  leGrid1FillV->setGeometry(250,25,51,21); leGrid1FillV->setValidator(new QDoubleValidator); leGrid1FillV->setText("20");
    leGrid2ReleaseV = new QLineEdit(gbIFT);  leGrid2ReleaseV->setGeometry(250,50,51,21); leGrid2ReleaseV->setValidator(new QDoubleValidator); leGrid2ReleaseV->setText("30");
    leGrid3ReleaseV = new QLineEdit(gbIFT);  leGrid3ReleaseV->setGeometry(250,75,51,21); leGrid3ReleaseV->setValidator(new QDoubleValidator); leGrid3ReleaseV->setText("40");
    FrameSize = new QLineEdit(gbIFT);  FrameSize->setGeometry(405,25,51,21); FrameSize->setValidator(new QIntValidator); FrameSize->setText("1000");
    Accumulations = new QLineEdit(gbIFT); Accumulations->setGeometry(405,47,51,21); Accumulations->setValidator(new QIntValidator); Accumulations->setText("10");
    Table = new QLineEdit(gbIFT); Table->setGeometry(50,150,441,21);
    ClockSource = new QComboBox(gbIFT); ClockSource->setGeometry(400,69,100,21);
    ClockSource->clear();
    ClockSource->addItem("Ext","Ext");
    ClockSource->addItem("ExtN","ExtN");
    ClockSource->addItem("ExtS","ExtS");
    ClockSource->addItem("42000000","42000000");
    ClockSource->addItem("10500000","10500000");
    ClockSource->addItem("2625000","2625000");
    ClockSource->addItem("656250","656250");
    TriggerSource = new QComboBox(gbIFT); TriggerSource->setGeometry(400,91,100,21);
    TriggerSource->clear();
    TriggerSource->addItem("Software","Software");
    TriggerSource->addItem("Edge","Edge");
    TriggerSource->addItem("Pos","Pos");
    TriggerSource->addItem("Neg","Neg");
    GenerateTable = new QPushButton("Generate",gbIFT); GenerateTable->setGeometry(30,118,113,32); GenerateTable->setAutoDefault(false);
    Download = new QPushButton("Download",gbIFT);  Download->setGeometry(140,118,113,32); Download->setAutoDefault(false);
    Trigger = new QPushButton("Trigger",gbIFT);  Trigger->setGeometry(250,118,113,32); Trigger->setAutoDefault(false);
    Abort = new QPushButton("Abort",gbIFT);  Abort->setGeometry(360,118,113,32); Abort->setAutoDefault(false);
    // Add labels
    labels[0] = new QLabel("Fill time",gbIFT);       labels[0]->setGeometry(10,25,81,16);
    labels[1] = new QLabel("Trap time",gbIFT);       labels[1]->setGeometry(10,50,81,16);
    labels[2] = new QLabel("Release time",gbIFT);    labels[2]->setGeometry(10,75,81,16);
    labels[3] = new QLabel("Grid1 fill V",gbIFT);    labels[3]->setGeometry(160,25,80,16);
    labels[4] = new QLabel("Grid2 rel V",gbIFT);     labels[4]->setGeometry(160,50,80,16);
    labels[5] = new QLabel("Grid3 rel V",gbIFT);     labels[5]->setGeometry(160,75,80,16);
    labels[6] = new QLabel("Frame length",gbIFT);    labels[6]->setGeometry(310,25,91,16);
    labels[7] = new QLabel("Accumulations",gbIFT);   labels[7]->setGeometry(310,47,91,16);
    labels[8] = new QLabel("Clock source",gbIFT);    labels[8]->setGeometry(310,69,91,16);
    labels[9] = new QLabel("Trigger source",gbIFT);  labels[9]->setGeometry(310,91,91,16);
    labels[10] =new QLabel("Table",gbIFT);           labels[10]->setGeometry(10,150,59,16);
    // Connect to the event slots
    connect(GenerateTable,SIGNAL(pressed()),this,SLOT(pbGenerate()));
    connect(Download,SIGNAL(pressed()),this,SLOT(pbDownload()));
    connect(Trigger,SIGNAL(pressed()),this,SLOT(pbTrigger()));
    connect(Abort,SIGNAL(pressed()),this,SLOT(pbAbort()));
    connect(leFillTime,SIGNAL(editingFinished()),this,SLOT(tblObsolite()));
    connect(leTrapTime,SIGNAL(editingFinished()),this,SLOT(tblObsolite()));
    connect(leReleaseTime,SIGNAL(editingFinished()),this,SLOT(tblObsolite()));
    connect(leGrid1FillV,SIGNAL(editingFinished()),this,SLOT(tblObsolite()));
    connect(leGrid2ReleaseV,SIGNAL(editingFinished()),this,SLOT(tblObsolite()));
    connect(leGrid3ReleaseV,SIGNAL(editingFinished()),this,SLOT(tblObsolite()));
    connect(FrameSize,SIGNAL(editingFinished()),this,SLOT(tblObsolite()));
    connect(Accumulations,SIGNAL(editingFinished()),this,SLOT(tblObsolite()));
    TableDownloaded = false;
    Table->raise();
}

// Generate table for IFT.
//
// 0:[A:accumulations],
void IFTtiming::pbGenerate(void)
{
    QString table,msg;
    QString Grid1V, Grid2V, Grid3V;
    QMessageBox msgBox;
    int     cclock = 0;

    // Load the resting grid voltages from the setting, set to null if the grid is not defined
    Grid1V = "";
    if(Grid1 != NULL)
    {
        if(Grid1->Vsp->text().contains("?")) Grid1V = "0";
        else if(Grid1->Vsp->text()=="") Grid1V = "0";
        else Grid1V = Grid1->Vsp->text();
    }
    Grid2V = "";
    if(Grid2 != NULL)
    {
        if(Grid2->Vsp->text().contains("?")) Grid2V = "0";
        else if(Grid2->Vsp->text()=="") Grid2V = "0";
        else Grid2V = Grid2->Vsp->text();
    }
    Grid3V = "";
    if(Grid3 != NULL)
    {
        if(Grid3->Vsp->text().contains("?")) Grid3V = "0";
        else if(Grid3->Vsp->text()=="") Grid3V = "0";
        else Grid3V = Grid3->Vsp->text();
    }
    if((Grid2 == NULL) && (Grid3 == NULL))
    {
        msg =  "You must define Grid 2 or Grid 3 in the configuration file to use";
        msg += "the IFT function!";
        msgBox.setText(msg);
        msgBox.setInformativeText("");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }
    // Build the table
    table =  "STBLDAT;0:[A:" + QString::number(Accumulations->text().toInt() + 1) + ",";
    // Add fill time if grid 1 is defined and fill time is > 0
    if((Grid1 != NULL) && (leFillTime->text().toInt() > 0))
    {
        table += QString::number(cclock) + ":" + QString::number(Grid1->Channel) + ":" + leGrid1FillV->text() + ",";
        cclock += leFillTime->text().toInt();
    }
    // If trap time is > 0 then lower grid 1 and wait
    if(leTrapTime->text().toInt() > 0)
    {
        if(Grid1V != "") table += QString::number(cclock) + ":" + QString::number(Grid1->Channel) + ":" + Grid1V + ",";
        cclock += leTrapTime->text().toInt();
    }
    table += QString::number(cclock);
    if(Grid1V != "") table += ":" + QString::number(Grid1->Channel) + ":" + Grid1V;
    if(Grid2 != NULL) table += ":" + QString::number(Grid2->Channel) + ":" + leGrid2ReleaseV->text();
    if(Grid3 != NULL)table += ":" + QString::number(Grid3->Channel) + ":" + leGrid3ReleaseV->text();
    if(Enable != "") table += ":" + Enable + ":1";
    table += "," + QString::number(cclock + leReleaseTime->text().toInt());
    if(Grid2 != NULL) table += ":" + QString::number(Grid2->Channel) + ":" + Grid2V;
    if(Grid3 != NULL) table += ":" + QString::number(Grid3->Channel) + ":" + Grid3V;
    cclock += FrameSize->text().toInt();
    table += "," + QString::number(cclock) + ":" + Enable + ":0];";
    Table->setText(table);
}

// Download table parameters to MIPS
void IFTtiming::pbDownload(void)
{
   if(comms == NULL) return;
   comms->SendCommand("SMOD,LOC\n");        // Make sure the system is in local mode
   comms->SendCommand("STBLREPLY,FALSE\n"); // Turn off any table messages from MIPS
   // Make sure a table has been generated
   if(Table->text() == "")
   {
       QMessageBox msgBox;
       msgBox.setText("There is no Pulse Sequence to download to MIPS!");
       msgBox.exec();
       return;
   }
   // Set clock
   comms->SendCommand("STBLCLK," + ClockSource->currentText().toUpper() + "\n");
   // Set trigger
   QString res = TriggerSource->currentText().toUpper();
   if(res == "SOFTWARE") res = "SW";
   comms->SendCommand("STBLTRG," + res + "\n");
   // Send table
   comms->SendCommand(Table->text());
   // Put system in table mode
//   if(comms->SendCommand("SMOD,TBL\n"))
//   {
//       if(statusBar != NULL) statusBar->showMessage("System mode changed to Table.", 5000);
       // We should now receive a table ready for trigger command so wait for it
//       comms->waitforline(1000);
//       while(comms->rb.numLines() > 0)
//       {
//           if(statusBar != NULL) statusBar->showMessage(comms->getline(),5000);
//       }
//   }
//   else if(statusBar != NULL) statusBar->showMessage("System mode change to Table rejected!", 5000);
   TableDownloaded = true;
}

void IFTtiming::slotAppFinished(void)
{
   // Send a signal that the data collection has finished.
   emit dataAcquired(filePath);
   Acquiring = false;
   if(comms == NULL) return;
   comms->SendCommand("SMOD,LOC\n");
   if(statusBar != NULL) statusBar->showMessage("Acquire app finished, returning to local mode.", 5000);
}

void IFTtiming::AppReady(void)
{
    if(comms == NULL) return;
    // Send table start command
    if(comms->SendCommand("TBLSTRT\n")) if(statusBar != NULL) statusBar->showMessage("Table trigger command accepted!", 5000);
    else if(statusBar != NULL) statusBar->showMessage("Table trigger command rejected!", 5000);
}

void IFTtiming::pbTrigger(void)
{
    AcquireData("");
}

void IFTtiming::AcquireData(QString path)
{
    // If the Acquire string is defined then we call the program defined
    // by The Acquire string. The program is called with optional arguments
    // as follows:
    // - Filename, this will result in a dialog box to appear
    //             allowing the user to select a filename to hold
    //             the data
    // - TOFscans, passes the total number of tof scans to acquire, this is
    //             the product of Frame size and accumulations
    // The acquire ap is expected to return "Ready" when ready to accept a trigger.
    filePath = "";
    // Make sure the system is in table mode
    if(comms != NULL) if(comms->SendCommand("SMOD,TBL\n"))
    {
        if(!TableDownloaded)
        {
            QString msg = "Pulse sequence table has not been downloaded to MIPS!";
            QMessageBox msgBox;
            msgBox.setText(msg);
            msgBox.setInformativeText("");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            return;
        }
        if(statusBar != NULL) statusBar->showMessage("System mode changed to Table.", 5000);
        // We should now receive a table ready for trigger command so wait for it
//        comms->waitforline(1000);
//        while(comms->rb.numLines() > 0)
//        {
//            if(statusBar != NULL) statusBar->showMessage(comms->getline(),5000);
//        }
    }
    if(Acquire != "")
    {
        QString cmd = "";
        QStringList resList = Acquire.split(",");
        cmd = Acquire;
        for(int i=0;i<resList.count();i++)
        {
            if(i==0) cmd = resList[0];
            if(resList[i].toUpper() == "TOFSCANS")
            {
                cmd += " -S" + QString::number(FrameSize->text().toInt() * Accumulations->text().toInt());
            }
            if(resList[i].toUpper() == "FILENAME")
            {
                CDirSelectionDlg *cds = new CDirSelectionDlg(QDir::currentPath());
                cds->setTitle("Select/enter folder to save data files");
                while(path == "")
                {
                    if(cds->exec() != 0)
                    {
                        QString selectedPath = cds->selectedPath();
                        // See if the directory is present
                        if(QDir(selectedPath).exists())
                        {
                            QString msg = "Selected folder exists, please define a unique folder to save data files.";
                            QMessageBox msgBox;
                            msgBox.setText(msg);
                            msgBox.setInformativeText("");
                            msgBox.setStandardButtons(QMessageBox::Ok);
                            msgBox.exec();
                        }
                        else
                        {
                            filePath = selectedPath;
                            break;
                        }
                    }
                    else
                    {
                       if(comms != NULL) comms->SendCommand("SMOD,LOC\n"); // Return to local mode
                       return;
                    }
                }
                // Create the folder and define the data storage path and file
                if(path != "")
                {
                    filePath = MakePathUnique(path);
                }
                QDir().mkdir(filePath);
                QDir().setCurrent(filePath);
                cmd += " " + filePath + "/" + "U1084A.data";
            }
        }
        cla = new cmdlineapp();
        cla->appPath = cmd;
        cla->show();
        cla->ReadyMessage = "Ready";
        cla->InputRequest = "? Y/N :";
        if(filePath != "") cla->fileName = filePath + "/Acquire.data";
        connect(cla,SIGNAL(Ready()),this,SLOT(AppReady()));
        connect(cla,SIGNAL(AppCompleted()),this,SLOT(slotAppFinished()));
        cla->Execute();
        Acquiring = true;
    }
    else
    {
        if(comms == NULL) return;
        // Send table start command
        if(comms->SendCommand("TBLSTRT\n")) if(statusBar != NULL) statusBar->showMessage("Table trigger command accepted!", 5000);
        else if(statusBar != NULL) statusBar->showMessage("Table trigger command rejected!", 5000);
    }
}

QString IFTtiming::MakePathUnique(QString path)
{
    int i, num;

    // Check if path exists, if it does then see if it ends in a number, if
    // it does then increment this number, if it does not append -0001 to the
    // name
    while(true)
    {
       if(QDir(path).exists())
       {
           for(i=0;i<path.length();i++) if(!path[path.length()-1-i].isDigit()) break;
           if(i == 0) path += "-0001";
           else
           {
               num = path.right(i).toInt();
               num++;
               path = path.left(path.length()-i) += QString("%1").arg(num,4,10,QLatin1Char('0'));
           }
       }
       else break;
    }
    // At this point path is unique
    return path;
}

// The function sends the table about command to MIPS. Returns ACK if ok, else returns NAK.
void IFTtiming::pbAbort(void)
{
    if(comms == NULL) return;
    // Send table abort command
    if(comms->SendCommand("TBLABRT\n")) if(statusBar != NULL) statusBar->showMessage("Table mode aborted!", 5000);
    else if(statusBar != NULL) statusBar->showMessage("Table mode abort error!", 5000);
}

QString IFTtiming::Report(void)
{
    QString res;

    res = Title + ",";
    res += leFillTime->text() + ",";
    res += leTrapTime->text() + ",";
    res += leReleaseTime->text() + ",";
    res += leGrid1FillV->text() + ",";
    res += leGrid2ReleaseV->text() + ",";
    res += leGrid3ReleaseV->text() + ",";
    res += Accumulations->text() + ",";
    res += FrameSize->text() + ",";
    res += ClockSource->currentText() + ",";
    res += TriggerSource->currentText() + ",";
    res += Table->text() + "\n";
    return res;
}

bool IFTtiming::SetValues(QString strVals)
{
    QStringList resList;

    if(!strVals.startsWith(Title)) return false;
    resList = strVals.split(",");
    if(resList.count() < 12) return false;
    leFillTime->setText(resList[1]);
    leTrapTime->setText(resList[2]);
    leReleaseTime->setText(resList[3]);
    leGrid1FillV->setText(resList[4]);
    leGrid2ReleaseV->setText(resList[5]);
    leGrid3ReleaseV->setText(resList[6]);
    Accumulations->setText(resList[7]);
    FrameSize->setText(resList[8]);
    int i = ClockSource->findData(resList[9]);
    ClockSource->setCurrentIndex(i);
    i = TriggerSource->findData(resList[10]);
    TriggerSource->setCurrentIndex(i);
    i = strVals.indexOf("STBLDAT");
    if(i > 0) Table->setText(strVals.mid(i));
    pbDownload();
    return true;
}

void IFTtiming::tblObsolite(void)
{
    TableDownloaded = false;
}

void IFTtiming::Dismiss(void)
{
    if(cla == NULL) return;
//    cla->deleteLater();
    delete cla;
    cla = NULL;
}
