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
#include <QUdpSocket>

ControlPanel::ControlPanel(QWidget *parent, QString CPfileName, QList<Comms*> S, Properties *prop) :
    QDialog(parent),
    ui(new Ui::ControlPanel)
{
    QStringList resList;

    ui->setupUi(this);
    ControlPanel::setWindowTitle("Custom control panel, right click for options");
    Systems = S;

    // Init a number of variables
    HelpFile.clear();
    LoadedConfig = false;
    mc        = NULL;
    IFT       = NULL;
    TC        = NULL;
    DCBgroups = NULL;
    statusBar = NULL;
    ARBcompressorButton = NULL;
    comp      = NULL;
    TextLabels.clear();
    RFchans.clear();
    rfa.clear();
    ADCchans.clear();
    DACchans.clear();
    DCBchans.clear();
    DCBoffsets.clear();
    DCBenables.clear();
    DIOchannels.clear();
    ESIchans.clear();
    ARBchans.clear();
    UpdateHoldOff  = 0;
    UpdateStop     = false;
    ShutdownFlag   = false;
    RestoreFlag    = false;
    StartMIPScomms = false;
    SystemIsShutdown = false;
    scriptconsole = NULL;
    tcp = NULL;
    properties = prop;
    help = new Help();
    LogFile.clear();
    // Allow user to select the configuration file
    QString fileName;
    if(CPfileName == "") fileName = QFileDialog::getOpenFileName(this, tr("Load Configuration from File"),"",tr("cfg (*.cfg);;All files (*.*)"));
    else fileName = CPfileName;
    if((fileName == "") || (fileName.isEmpty())) return;
    QFile file(fileName);
    ControlPanelFile = fileName;
    if(properties != NULL)
    {
        properties->Log("Control panel loaded: " + ControlPanelFile);
        for(int i=0;i<Systems.count();i++)
        {
            // Add MIPS firmware version to log file
            properties->Log(Systems[i]->MIPSname + ": " + Systems[i]->SendMess("GVER\n"));
        }
    }
    // Open UDP socket to send commands to reader app
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(QHostAddress::LocalHost, 7755);
    // Read the configuration file and create the form as
    // well as all the controls.
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
                connect(statusBar,SIGNAL(messageChanged(QString)),this,SLOT(slotLogStatusBarMessage(QString)));
            }
            if((resList[0].toUpper() == "IMAGE") && (resList.length()==2))
            {
                #ifdef Q_OS_MAC
                if(resList[1].startsWith("~")) resList[1] = QDir::homePath() + "/" + resList[1].mid(2);
                #endif
                QPixmap img(resList[1]);
                ui->lblBackground->clear();
                ui->lblBackground->setPixmap(img);
            }
            if((resList[0].toUpper() == "TEXTLABELS") && (resList.length()==2))
            {
                int j = resList[1].toInt();
                for(int i = 0; i < j; i++)
                {
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "TEXTLABEL") && (resList.length()==5))
                    {
                        TextLabels.append(new TextLabel(ui->lblBackground,resList[1],resList[2].toInt(),resList[3].toInt(),resList[4].toInt()));
                        TextLabels.last()->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "RFCHANNELS") && (resList.length()==2))
            {
                int j = resList[1].toInt();
                for(int i = 0; i < j; i++)
                {
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "RFCHANNEL") && (resList.length()==6))
                    {
                        RFchans.append(new RFchannel(ui->lblBackground,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                        RFchans.last()->Channel = resList[3].toInt();
                        RFchans.last()->comms = FindCommPort(resList[2],Systems);
                        RFchans.last()->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "ADCCHANNELS") && (resList.length()==2))
            {
                int j = resList[1].toInt();
                for(int i = 0; i < j; i++)
                {
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "ADCCHANNEL") && (resList.length()>=6))
                    {
                        ADCchans.append(new ADCchannel(ui->lblBackground,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                        ADCchans.last()->Channel = resList[3].toInt();
                        ADCchans.last()->comms = FindCommPort(resList[2],Systems);
                        if(resList.length()>=7)  ADCchans.last()->m = resList[6].toFloat();
                        if(resList.length()>=8)  ADCchans.last()->b = resList[7].toFloat();
                        if(resList.length()>=9)  ADCchans.last()->Units = resList[8];
                        if(resList.length()>=10) ADCchans.last()->Format = resList[9];
                        ADCchans.last()->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "DACCHANNELS") && (resList.length()==2))
            {
                int j = resList[1].toInt();
                for(int i = 0; i < j; i++)
                {
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "DACCHANNEL") && (resList.length()>=6))
                    {
                        DACchans.append(new DACchannel(ui->lblBackground,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                        DACchans.last()->Channel = resList[3].toInt();
                        DACchans.last()->comms = FindCommPort(resList[2],Systems);
                        if(resList.length()>=7)  DACchans.last()->m = resList[6].toFloat();
                        if(resList.length()>=8)  DACchans.last()->b = resList[7].toFloat();
                        if(resList.length()>=9)  DACchans.last()->Units = resList[8];
                        if(resList.length()>=10) DACchans.last()->Format = resList[9];
                        DACchans.last()->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "DCBCHANNELS") && (resList.length()==2))
            {
                int j = resList[1].toInt();
                for(int i = 0; i < j; i++)
                {
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "DCBCHANNEL") && (resList.length()==6))
                    {
                        DCBchans.append(new DCBchannel(ui->lblBackground,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                        DCBchans.last()->Channel = resList[3].toInt();
                        DCBchans.last()->comms = FindCommPort(resList[2],Systems);
                        DCBchans.last()->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "DCBOFFSETS") && (resList.length()==2))
            {
                int j = resList[1].toInt();
                for(int i = 0; i < j; i++)
                {
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "DCBOFFSET") && (resList.length()==6))
                    {
                        DCBoffsets.append(new DCBoffset(ui->lblBackground,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                        DCBoffsets.last()->Channel = resList[3].toInt();
                        DCBoffsets.last()->comms = FindCommPort(resList[2],Systems);
                        DCBoffsets.last()->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "DCBENABLES") && (resList.length()==2))
            {
                int j = resList[1].toInt();
                for(int i = 0; i < j; i++)
                {
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "DCBENABLE") && (resList.length()==5))
                    {
                        DCBenables.append(new DCBenable(ui->lblBackground,resList[1],resList[2],resList[3].toInt(),resList[4].toInt()));
                        DCBenables.last()->comms = FindCommPort(resList[2],Systems);
                        DCBenables.last()->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "DIOCHANNELS") && (resList.length()==2))
            {
                int j = resList[1].toInt();
                for(int i = 0; i < j; i++)
                {
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "DIOCHANNEL") && (resList.length()==6))
                    {
                        DIOchannels.append(new DIOchannel(ui->lblBackground,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                        DIOchannels.last()->Channel = resList[3];
                        DIOchannels.last()->comms = FindCommPort(resList[2],Systems);
                        DIOchannels.last()->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "ESICHANNELS") && (resList.length()==2))
            {
                int j = resList[1].toInt();
                for(int i = 0; i < j; i++)
                {
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "ESICHANNEL") && (resList.length()==6))
                    {
                        ESIchans.append(new ESI(ui->lblBackground,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                        ESIchans.last()->Channel = resList[3].toInt();
                        ESIchans.last()->comms = FindCommPort(resList[2],Systems);
                        ESIchans.last()->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "ARBCHANNELS") && (resList.length()==2))
            {
                int j = resList[1].toInt();
                for(int i = 0; i < j; i++)
                {
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "ARBCHANNEL") && (resList.length()==6))
                    {
                        ARBchans.append(new ARBchannel(ui->lblBackground,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                        ARBchans.last()->Channel = resList[3].toInt();
                        ARBchans.last()->comms = FindCommPort(resList[2],Systems);
                        ARBchans.last()->Show();
                    }
                }
            }
            if((resList[0].toUpper() == "RFAMPCHANNELS") && (resList.length()==2))
            {
                int j = resList[1].toInt();
                for(int i = 0; i < j; i++)
                {
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    resList = line.split(",");
                    if((resList[0].toUpper() == "RFAMP") && (resList.length()==6))
                    {
                        rfa.append(new RFamp(ui->lblBackground,resList[1],resList[2],resList[3].toInt()));
                        rfa.last()->comms = FindCommPort(resList[2],Systems);
                        QWidget *widget = new QWidget(this);
                        widget->setGeometry(resList[4].toInt(),resList[5].toInt(),280,290);
                        QVBoxLayout *layout = new QVBoxLayout(widget);
                        layout->addWidget(rfa.last());
                        rfa.last()->show();
                    }

                }
            }
            if((resList[0].toUpper() == "TIMING") && (resList.length()==5))
            {
                TC = new TimingControl(ui->lblBackground,resList[1],resList[2],resList[3].toInt(),resList[4].toInt());
                TC->comms = FindCommPort(resList[2],Systems);
                TC->statusBar = statusBar;
                TC->properties = properties;
                TC->Show();
                if(TC->TG != NULL)
                {
                   foreach(DCBchannel *dcb, DCBchans) if(dcb->MIPSnm == TC->MIPSnm) TC->TG->AddSignal(dcb->Title, QString::number(dcb->Channel));
                   foreach(DIOchannel *dio, DIOchannels) if(dio->MIPSnm == TC->MIPSnm) TC->TG->AddSignal(dio->Title, dio->Channel);
                }
                connect(TC,SIGNAL(dataAcquired(QString)),this,SLOT(slotDataAcquired(QString)));
            }
            if((resList[0].toUpper() == "IFT") && (resList.length()==5))
            {
                IFT = new IFTtiming(ui->lblBackground,resList[1],resList[2],resList[3].toInt(),resList[4].toInt());
                IFT->comms = FindCommPort(resList[2],Systems);
                IFT->statusBar = statusBar;
                IFT->properties = properties;
                IFT->Show();
                connect(IFT,SIGNAL(dataAcquired(QString)),this,SLOT(slotDataAcquired(QString)));
            }
            if((resList[0].toUpper() == "GRID1") && (resList.length()==2))
            {
                if(IFT!=NULL)
                {
                    IFT->Grid1 = NULL;
                    for(int i=0; i< DCBchans.count(); i++) if(DCBchans[i]->Title == resList[1]) IFT->Grid1 = DCBchans[i];
                }
            }
            if((resList[0].toUpper() == "GRID2") && (resList.length()==2))
            {
                if(IFT!=NULL)
                {
                    IFT->Grid2 = NULL;
                    for(int i=0; i< DCBchans.count(); i++) if(DCBchans[i]->Title == resList[1]) IFT->Grid2 = DCBchans[i];
                }
            }
            if((resList[0].toUpper() == "GRID3") && (resList.length()==2))
            {
                if(IFT!=NULL)
                {
                    IFT->Grid3 = NULL;
                    for(int i=0; i< DCBchans.count(); i++) if(DCBchans[i]->Title == resList[1]) IFT->Grid3 = DCBchans[i];
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
            // COMPRESSOR,name,MIPS name,X,Y
            if((resList[0].toUpper() == "COMPRESSOR") && (resList.length()==5))
            {
                // Enable the ARB compressor button
                ARBcompressorButton = new QPushButton(resList[1],ui->lblBackground);
                ARBcompressorButton->setGeometry(resList[3].toInt(),resList[4].toInt(),150,32);
                ARBcompressorButton->setAutoDefault(false);
                ARBcompressorButton->setToolTip("Press this button to edit the compression options");
                comp = new Compressor(ui->lblBackground,resList[1],resList[2]);
                comp->comms = FindCommPort(resList[2],Systems);
                connect(ARBcompressorButton,SIGNAL(pressed()),this,SLOT(pbARBcompressor()));
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
                    IFT->AD->Acquire = line.mid(line.indexOf(",")+1);
                    #ifdef Q_OS_MAC
                    if(IFT->AD->Acquire .startsWith("~")) IFT->AD->Acquire  = QDir::homePath() + "/" + IFT->AD->Acquire .mid(2);
                    #endif
                }
                if(TC!=NULL)
                {
                    TC->AD->Acquire = line.mid(line.indexOf(",")+1);
                    #ifdef Q_OS_MAC
                    if(TC->AD->Acquire .startsWith("~")) TC->AD->Acquire  = QDir::homePath() + "/" + TC->AD->Acquire .mid(2);
                    #endif
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
                scriptconsole = new ScriptingConsole(this,properties);
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
                #ifdef Q_OS_MAC
                if(resList[1].startsWith("~")) resList[1] = QDir::homePath() + "/" + resList[1].mid(2);
                #endif
                InitMIPSsystems(resList[1]);
            }
            if((resList[0].toUpper() == "HELP") && (resList.length()==2))
            {
                HelpFile = resList[1];
                #ifdef Q_OS_MAC
                if(HelpFile.startsWith("~")) HelpFile = QDir::homePath() + "/" + HelpFile.mid(2);
                #endif
            }
            if((resList[0].toUpper() == "TCPSERVER") && (resList.length()==2))
            {
                tcp = new TCPserver();
                tcp->port = resList[1].toInt();
                tcp->statusbar = statusBar;
                tcp->listen();
                connect(tcp,SIGNAL(lineReady()),this,SLOT(tcpCommand()));
            }
        } while(!line.isNull());
    }
    file.close();
    LoadedConfig = true;
    connect(ui->lblBackground, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(popupHelp(QPoint)));
    // Popup options menu actions
    GeneralHelp = new QAction("General help", this);
    connect(GeneralHelp, SIGNAL(triggered()), this, SLOT(slotGeneralHelp()));
    MIPScommands = new QAction("MIPS commands", this);
    connect(MIPScommands, SIGNAL(triggered()), this, SLOT(slotMIPScommands()));
    ScriptHelp  = new QAction("Script help", this);
    connect(ScriptHelp, SIGNAL(triggered()), this, SLOT(slotScriptHelp()));
    if(!HelpFile.isEmpty())
    {
        ThisHelp = new QAction("This control panel help", this);
        connect(ThisHelp, SIGNAL(triggered()), this, SLOT(slotThisControlPanelHelp()));
    }
    OpenLogFile = new QAction("Open data log", this);
    connect(OpenLogFile, SIGNAL(triggered()), this, SLOT(slotOpenLogFile()));
    CloseLogFile = new QAction("Close data log", this);
    connect(CloseLogFile, SIGNAL(triggered()), this, SLOT(slotCloseLogFile()));
}

ControlPanel::~ControlPanel()
{
    if(properties != NULL) properties->Log("Control panel unloading");
    delete tcp;
    delete ui;
}

void ControlPanel::reject()
{
    QMessageBox msgBox;

    if(SystemIsShutdown)  // If its shutdown send warning
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

void ControlPanel::popupHelp(QPoint qp)
{
    contextMenu2Dplot = new QMenu(tr("Help options"), this);
    contextMenu2Dplot->addAction(GeneralHelp);
    contextMenu2Dplot->addAction(MIPScommands);
    contextMenu2Dplot->addAction(ScriptHelp);
    if(!HelpFile.isEmpty()) contextMenu2Dplot->addAction(ThisHelp);
    if(LogFile == "") contextMenu2Dplot->addAction(OpenLogFile);
    if(LogFile != "") contextMenu2Dplot->addAction(CloseLogFile);
    contextMenu2Dplot->exec(qp);
}

void ControlPanel::slotGeneralHelp(void)
{
    help->SetTitle("MIPS Help");
    help->LoadHelpText(":/MIPShelp.txt");
    help->show();
}

void ControlPanel::slotMIPScommands(void)
{
    help->SetTitle("MIPS Commands");
    help->LoadHelpText(":/MIPScommands.txt");
    help->show();
}

void ControlPanel::slotScriptHelp(void)
{
    help->SetTitle("Script help");
    help->LoadHelpText(":/scripthelp.txt");
    help->show();
}

void ControlPanel::slotThisControlPanelHelp(void)
{
    qDebug() << HelpFile;
    help->SetTitle("This Control panel help");
    help->LoadHelpText(HelpFile);
    help->show();
}

void ControlPanel::slotLogStatusBarMessage(QString statusMess)
{
    if(statusMess == "") return;
    if(statusMess.isEmpty()) return;
    if(properties != NULL) properties->Log("CP StatusBar: " + statusMess);
}

// This menu option allows the user to define a data log file name and
// define the minimum time between samples. After the file is opened a header
// is written with the variable names and the time the logging started.
// The data file is CSV with wach record having a time stamp in seconds from
// the time the file was opened.
void ControlPanel::slotOpenLogFile(void)
{
    QFileDialog fileDialog;
    QMessageBox msgBox;
    bool ok;

    QString msg = "This option will open a data log file allowing you to define the ";
    msg += "minimum time between samples. The data file in CSV with a header holding ";
    msg += "the value names. Most control parameter values are saved.\n";
    msgBox.setText(msg);
    msgBox.setInformativeText("Are you sure you want to continue?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if(ret == QMessageBox::No) return;
    // Ask for file name
    QString dataFile = fileDialog.getSaveFileName(this, tr("Save data to log File"),"",tr("Log (*.log);;All files (*.*)"));
    if(dataFile == "") return;
    // Ask for time between samples
    QString res = QInputDialog::getText(this, "Log file sample period", "Enter the minimum time in seconds between samples.", QLineEdit::Normal,"10", &ok);
    // Initalize the logging parameters
    LogStartTime = 0;
    LogPeriod = res.toInt();
    if(LogPeriod <= 0) LogPeriod = 1;
    LogFile = dataFile;
    statusBar->showMessage("Data log file opened: " + dataFile);
    ControlPanel::setWindowTitle("Custom control panel, right click for options, data log file open: " + dataFile);
}

void ControlPanel::slotCloseLogFile(void)
{
    LogFile.clear();
    statusBar->showMessage("Data log file closed!");
    ControlPanel::setWindowTitle("Custom control panel, right click for options");
}

void ControlPanel::LogDataFile(void)
{
   QStringList vals;
   QString     header;
   QString     record;
   int         i;
   QDateTime   qt;
   static uint NextSampleTime;

   if(LogFile.isEmpty()) return;
   header.clear();
   record.clear();
   if(LogStartTime == 0)
   {
       LogStartTime = qt.currentDateTime().toTime_t();
       NextSampleTime = LogStartTime;
       // Write the file header
       header = QDateTime().currentDateTime().toString() + "\n";
       // Build the CSV header record
       header += "Seconds";
       for(i=0;i<DCBchans.count();i++)
       {
           vals = DCBchans[i]->Report().split(",");
           header += "," + vals[0] + ".SP," + vals[0] + ".RP";
       }
       for(i=0;i<RFchans.count();i++)
       {
           vals = RFchans[i]->Report().split(",");
           header += "," + vals[0] + ".DRV," + vals[0] + ".FREQ," + vals[0] + ".RF+," + vals[0] + ".RF-," + vals[0] + ".PWR";
       }
       header += "\n";
   }
   if(qt.currentDateTime().toTime_t() >= NextSampleTime)
   {
       while(NextSampleTime <= qt.currentDateTime().toTime_t()) NextSampleTime += LogPeriod;
       record = QString::number(qt.currentDateTime().toTime_t() - LogStartTime);
       // Build the data string to write to the log file
       for(i=0;i<DCBchans.count();i++)
       {
           vals = DCBchans[i]->Report().split(",");
           record += "," + vals[1] + "," + vals[2];
       }
       for(i=0;i<RFchans.count();i++)
       {
           vals = RFchans[i]->Report().split(",");
           record += "," + vals[1] + "," + vals[2] + "," + vals[3] + "," + vals[4] + "," + vals[5];
       }
       record += "\n";
   }
   if(!header.isEmpty() || !record.isEmpty())
   {
       // Open file for append and save log results
       QFile file(LogFile);
       if(file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
       {
           QTextStream stream(&file);
           if(!header.isEmpty()) stream << header;
           if(!record.isEmpty()) stream << record;
           file.close();
       }
   }
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
   for(int i = 0; i < Systems.length(); i++) if(Systems.at(i)->MIPSname == name) return(Systems.at(i));
   return NULL;
}

void ControlPanel::Update(void)
{
   int i,j,k;

   if(scriptconsole!=NULL) scriptconsole->UpdateStatus();
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
       UpdateHoldOff = 1000;
       // Make sure all MIPS systems are in local mode
       for(i=0;i<Systems.count();i++)    Systems[i]->SendString("SMOD,LOC\n");
       msDelay(100);
       for(i=0;i<Systems.count();i++) Systems[i]->rb.clear();

       for(i=0;i<ESIchans.count();i++)   ESIchans[i]->Shutdown();
       for(i=0;i<DCBenables.count();i++) DCBenables[i]->Shutdown();
       for(i=0;i<RFchans.count();i++)    RFchans[i]->Shutdown();
       for(i=0;i<rfa.count();i++)        rfa[i]->Shutdown();
       if(statusBar != NULL) statusBar->showMessage("System shutdown, " + QDateTime().currentDateTime().toString());
       if(properties != NULL) properties->Log("System Shutdown");
       UpdateHoldOff = 1;
       return;
   }
   if(RestoreFlag)
   {
       RestoreFlag = false;
       SystemIsShutdown = false;
       UpdateHoldOff = 1000;
       for(i=0;i<ESIchans.count();i++)   ESIchans[i]->Restore();
       msDelay(100);
       for(i=0;i<DCBenables.count();i++) {DCBenables[i]->Restore(); msDelay(250);}
       for(i=0;i<RFchans.count();i++)    {RFchans[i]->Restore(); msDelay(250);}
       for(i=0;i<rfa.count();i++)        rfa[i]->Restore();
       if(statusBar != NULL) statusBar->showMessage("System enabled, " + QDateTime().currentDateTime().toString());
       if(properties != NULL) properties->Log("System Restored");
       UpdateHoldOff = 1;
       return;
   }
   // For each MIPS system present if there are RF channels for the selected
   // MIPS system then read all values using the read all commands to speed up the process.
   for(i=0;i<Systems.count();i++)
   {
       for(j=0;j<RFchans.count();j++) if(RFchans[j]->comms == Systems[i])
       {
           // Read all the RF parameters
           QString RFallRes = Systems[i]->SendMess("GRFALL\n");
           QApplication::processEvents();
           QStringList RFallResList = RFallRes.split(",");
           if(RFallResList.count() < 1)
           {
               // Here with group read error so process one at a time
               for(k=0;k<RFchans.count();k++) if(RFchans[k]->comms == Systems[i]) { RFchans[k]->Update(); QApplication::processEvents(); }
           }
           else
           {
               // build strings and update all channels that use this comm port
               for(k=0;k<RFchans.count();k++) if(RFchans[k]->comms == Systems[i])
               {
                   if(RFallResList.count() < (RFchans[k]->Channel * 4)) RFchans[k]->Update();
                   else RFchans[k]->Update(RFallResList[(RFchans[k]->Channel - 1) * 4 + 0] + "," + \
                                           RFallResList[(RFchans[k]->Channel - 1) * 4 + 1] + "," + \
                                           RFallResList[(RFchans[k]->Channel - 1) * 4 + 2] + "," + \
                                           RFallResList[(RFchans[k]->Channel - 1) * 4 + 3]);
                   QApplication::processEvents();
               }
           }
           break;
       }
   }
   for(i=0;i<ADCchans.count();i++)    {ADCchans[i]->Update(); QApplication::processEvents();}
   for(i=0;i<DACchans.count();i++)    {DACchans[i]->Update(); QApplication::processEvents();}
   // For each MIPS system present if there are DCBchannels for the selected
   // MIPS system then read all setpoints and values using the read all
   // commands to speed up the process.
   for(i=0;i<Systems.count();i++)
   {
       for(j=0;j<DCBchans.count();j++) if(DCBchans[j]->comms == Systems[i])
       {
           // Read all the setpoints and readbacks and parse the strings
           QString VspRes = Systems[i]->SendMess("GDCBALL\n");
           QApplication::processEvents();
           QStringList VspResList = VspRes.split(",");
           QString VrbRes = Systems[i]->SendMess("GDCBALLV\n");
           QApplication::processEvents();
           QStringList VrbResList = VrbRes.split(",");
           if((VspResList.count() != VrbResList.count()) || (VspResList.count() < 1))
           {
               // Here with group read error so process one at a time
               for(k=0;k<DCBchans.count();k++) if(DCBchans[k]->comms == Systems[i]) {DCBchans[k]->Update(); QApplication::processEvents();}
           }
           else
           {
               // build strings and update all channels that use this comm port
               for(k=0;k<DCBchans.count();k++) if(DCBchans[k]->comms == Systems[i])
               {
                   if(VspResList.count() < (DCBchans[k]->Channel)) DCBchans[k]->Update();
                   else DCBchans[k]->Update(VspResList[DCBchans[k]->Channel - 1] + "," + VrbResList[DCBchans[k]->Channel - 1]);
               }
           }
           break;  //??
       }
   }
   for(i=0;i<DCBoffsets.count();i++)  {DCBoffsets[i]->Update(); QApplication::processEvents();}
   for(i=0;i<DCBenables.count();i++)  {DCBenables[i]->Update(); QApplication::processEvents();}
   if(TC != NULL)
   {
       if(!TC->TG->isTableMode()) for(i=0;i<DIOchannels.count();i++) {DIOchannels[i]->Update(); QApplication::processEvents();}
   }
   else for(i=0;i<DIOchannels.count();i++) {DIOchannels[i]->Update(); QApplication::processEvents();}
   for(i=0;i<ESIchans.count();i++)    {ESIchans[i]->Update(); QApplication::processEvents();}
   for(i=0;i<ARBchans.count();i++)    {ARBchans[i]->Update(); QApplication::processEvents();}
   for(i=0;i<rfa.count();i++)         {rfa[i]->Update(); QApplication::processEvents();}
   if(comp!=NULL)                     {comp->Update(); QApplication::processEvents();}
   LogDataFile();
}

QString ControlPanel::Save(QString Filename)
{
    QString res;

    #ifdef Q_OS_MAC
    if(Filename.startsWith("~")) Filename = QDir::homePath() + "/" + Filename.mid(2);
    #endif
    if(Filename == "") return "No file defined!";
    UpdateHoldOff = 1000;
    if(properties != NULL) properties->Log("Save method file: " + Filename);
    QFile file(Filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# Control panel settings, " + dateTime.toString() + "\n";
        stream << "# " + Version + "\n";
        stream << "# Control panel file, " + ControlPanelFile + "\n";
        if(DCBoffsets.count() > 0)
        {
            stream << "DCB offsets," + QString::number(DCBoffsets.count()) + "\n";
            for(int i=0; i<DCBoffsets.count(); i++) if(DCBoffsets[i] != NULL) stream << DCBoffsets[i]->Report() + "\n";
        }
        if(DCBgroups != NULL)
        {
            stream << "DC bias groups\n";
            stream << DCBgroups->Report() + "\n";
        }
        if(RFchans.count() > 0)
        {
            stream << "RF channels," + QString::number(RFchans.count()) + "\n";
            for(int i=0;i<RFchans.count();i++) stream << RFchans[i]->Report() + "\n";
        }
        if(DCBchans.count() > 0)
        {
            stream << "DCB channels," + QString::number(DCBchans.count()) + "\n";
            for(int i=0; i<DCBchans.count(); i++) stream << DCBchans[i]->Report() + "\n";
        }
        if(DACchans.count() > 0)
        {
            stream << "DAC channels," + QString::number(DACchans.count()) + "\n";
            for(int i=0; i<DACchans.count(); i++) stream << DACchans[i]->Report() + "\n";
        }
        if(DCBenables.count() > 0)
        {
            stream << "DCB enables," + QString::number(DCBenables.count()) + "\n";
            for(int i=0; i<DCBenables.count(); i++) stream << DCBenables[i]->Report() + "\n";
        }
        if(DIOchannels.count() > 0)
        {
            stream << "DIO channels," + QString::number(DIOchannels.count()) + "\n";
            for(int i=0; i<DIOchannels.count(); i++) stream << DIOchannels[i]->Report() + "\n";
        }
        if(ESIchans.count() > 0)
        {
            stream << "ESI channels," + QString::number(ESIchans.count()) + "\n";
            for(int i=0; i<ESIchans.count(); i++) stream << ESIchans[i]->Report() + "\n";
        }
        if(ARBchans.count() > 0)
        {
            stream << "ARB channels," + QString::number(ARBchans.count()) + "\n";
            for(int i=0; i<ARBchans.count(); i++) stream << ARBchans[i]->Report() + "\n";
        }
        if(comp != NULL)
        {
            stream << "Compression parameters\n";
            stream << comp->Report() + "\n";
            stream << "CompressionEnd\n";
        }
        if(rfa.count() > 0)
        {
            stream << "RFamp channels," + QString::number(rfa.count()) + "\n";
            for(int i=0; i<rfa.count(); i++) if(rfa[i] != NULL) stream << rfa[i]->Report();
            stream << "RFampEnd\n";
        }
        if(IFT != NULL)
        {
            stream << "IFT parameters\n";
            stream << IFT->Report() + "\n";
        }
        if(TC != NULL)
        {
            stream << "Timing control parameters\n";
            stream << TC->TG->Report() + "\n";
        }
        file.close();
        UpdateHoldOff = 1;
        return "Settings saved to " + Filename;
    }
    return "Can't open file!";
}

QString ControlPanel::Load(QString Filename)
{
    QStringList resList;

    #ifdef Q_OS_MAC
    if(Filename.startsWith("~")) Filename = QDir::homePath() + "/" + Filename.mid(2);
    #endif
    if(Filename == "") return "No file defined!";
    UpdateHoldOff = 1000;
    QFile file(Filename);
    if(properties != NULL) properties->Log("Load method file: " + Filename);
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
                     for(int j=0;j<RFchans.count();j++) if(RFchans[j]->SetValues(line)) break;
                }
                if(resList[0] == "DCB channels") for(int i=0;i<resList[1].toInt();i++)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     for(int j=0;j<DCBchans.count();j++)if(DCBchans[j]->SetValues(line)) break;
                }
                if(resList[0] == "DAC channels") for(int i=0;i<resList[1].toInt();i++)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     for(int j=0;j<DACchans.count();j++) if(DACchans[j]->SetValues(line)) break;
                }
                if(resList[0] == "DCB offsets") for(int i=0;i<resList[1].toInt();i++)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     for(int j=0;j<DCBoffsets.count();j++) if(DCBoffsets[j]->SetValues(line)) break;
                }
                if(resList[0] == "DCB enables") for(int i=0;i<resList[1].toInt();i++)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     for(int j=0;j<DCBenables.count();j++) if(DCBenables[j]->SetValues(line)) break;
                }
                if(resList[0] == "DIO channels") for(int i=0;i<resList[1].toInt();i++)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     for(int j=0;j<DIOchannels.count();j++) if(DIOchannels[j]->SetValues(line)) break;
                }
                if(resList[0] == "ESI channels") for(int i=0;i<resList[1].toInt();i++)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     for(int j=0;j<ESIchans.count();j++) if(ESIchans[j]->SetValues(line)) break;
                }
                if(resList[0] == "ARB channels") for(int i=0;i<resList[1].toInt();i++)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     for(int j=0;j<ARBchans.count();j++) if(ARBchans[j]->SetValues(line)) break;
                }
                if(resList[0] == "RFamp channels") while(true)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     if(line.contains("RFampEnd")) break;
                     for(int j=0;j<rfa.count();j++) if(rfa[j]->SetValues(line)) break;
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
                if(resList[0] == "Timing control parameters")
                {
                    TC->TG->Events.clear();
                    while(true)
                    {
                        line = stream.readLine();
                        if(line.isNull()) break;
                        if(line.contains("TCparametersEnd")) break;
                        TC->TG->SetValues(line);
                    }
                }
                if(resList[0] == "Compression parameters")
                {
                    while(true)
                    {
                        line = stream.readLine();
                        if(line.isNull()) break;
                        if(line.contains("CompressionEnd")) break;
                        comp->SetValues(line);
                    }
                }
            }
        } while(!line.isNull());
        UpdateHoldOff = 1;
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
        if(properties != NULL) properties->Log("Data acquired: " + filepath);
        Save(filepath + "/Method.settings");
        // Send UDP message to allow reader to open the data file
        QString mess = "Load,"+filepath+"/U1084A.data";
        udpSocket->writeDatagram(mess.toLocal8Bit(),QHostAddress::LocalHost, 7755);
    }
    // Dismiss the acquire box
//    if(IFT != NULL)  IFT->AD->Dismiss();
//    if(TC != NULL)  TC->AD->Dismiss();
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
    QFileDialog fileDialog;

    if(properties != NULL) fileDialog.setDirectory(properties->MethodesPath);
    QString fileName = fileDialog.getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
    if(fileName == "") return;
    statusBar->showMessage(Save(fileName), 5000);
}

void ControlPanel::pbLoad(void)
{
    QFileDialog fileDialog;

    if(properties != NULL) fileDialog.setDirectory(properties->MethodesPath);
    QString fileName = fileDialog.getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
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

void ControlPanel::pbARBcompressor(void)
{
    ARBcompressorButton->setDown(false);
    comp->show();
    comp->raise();
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
    for(int i=0;i<DCBchans.count();i++) DCBchans[i]->LinkEnable = false;
}

DCBchannel * ControlPanel::FindDCBchannel(QString name)
{
    for(int i=0;i<DCBchans.count();i++) if(DCBchans[i]->Title.toUpper() == name.toUpper()) return DCBchans[i];
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
        for(int j=0;j<DCBchans.count();j++)
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
    if(scriptconsole == NULL) return;
    scriptconsole->show();
    scriptconsole->raise();
    scriptconsole->repaint();
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
   filePath.replace("\\","/");
   if(isAcquiring())
   {
       // If here the system is in the acquire state so create the
       // empty folder and log error
       if(properties->AutoFileName) filePath = MakePathUnique(filePath);
       QDir().mkdir(filePath);
       statusBar->showMessage("Already acquiring, request ignored but attempted folder creation!");
       return;
   }
   if(IFT != NULL) IFT->AcquireData(filePath);
   if(TC != NULL) TC->AcquireData(filePath);
}

bool ControlPanel::isAcquiring(void)
{
  QApplication::processEvents();
  if(IFT != NULL) return(IFT->AD->Acquiring);
  if(TC != NULL) return(TC->AD->Acquiring);
  return(false);
}

void ControlPanel::DismissAcquire(void)
{
  QApplication::processEvents();
  if(IFT != NULL)  IFT->AD->Dismiss();
  if(TC != NULL)  TC->AD->Dismiss();
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

QString ControlPanel::popupUserInput(QString title, QString message)
{
    bool ok;

    QApplication::processEvents();
    QString text = QInputDialog::getText(this, title, message, QLineEdit::Normal,QString::null, &ok);
    if(!ok) text="";
    QApplication::processEvents();
    return text;
}

bool ControlPanel::UpdateHalted(bool stop)
{
    bool orginalState = UpdateStop;
    UpdateStop = stop;
    return UpdateStop;
}

// This function is called with a command in the TCP server buffer.
// This function will process this command.
void ControlPanel::tcpCommand(void)
{
   int     i;
   QStringList resList;
   QString res;
   QString cmd = tcp->readLine();

   // Process global commands first.
   // Load,Save,Shutdown,Restore
   if(cmd.toUpper() == "SHUTDOWN") { ShutdownFlag = true; return; }
   if(cmd.toUpper() == "RESTORE") { RestoreFlag = true; return; }
   if(cmd.toUpper().startsWith("LOAD"))
   {
       resList = cmd.split(",");
       if(resList.count()==2) Load(resList[1]);
       return;
   }
   if(cmd.toUpper().startsWith("SAVE"))
   {
       resList = cmd.split(",");
       if(resList.count()==2) Save(resList[1]);
       return;
   }
   // Send the command string to all the controls for someone to process!
   for(i=0;i<DACchans.count();i++)    if((res = DACchans[i]->ProcessCommand(cmd)) != "?") {tcp->sendMessage(res + "\n"); return;}
   for(i=0;i<ADCchans.count();i++)    if((res = ADCchans[i]->ProcessCommand(cmd)) != "?") {tcp->sendMessage(res + "\n"); return;}
   for(i=0;i<DCBchans.count();i++)    if((res = DCBchans[i]->ProcessCommand(cmd)) != "?") {tcp->sendMessage(res + "\n"); return;}
   for(i=0;i<RFchans.count();i++)     if((res = RFchans[i]->ProcessCommand(cmd)) != "?") {tcp->sendMessage(res + "\n"); return;}
   for(i=0;i<DCBoffsets.count();i++)  if((res = DCBoffsets[i]->ProcessCommand(cmd)) != "?") {tcp->sendMessage(res + "\n"); return;}
   for(i=0;i<DCBenables.count();i++)  if((res = DCBenables[i]->ProcessCommand(cmd)) != "?") {tcp->sendMessage(res + "\n"); return;}
   for(i=0;i<DIOchannels.count();i++) if((res = DIOchannels[i]->ProcessCommand(cmd)) != "?") {tcp->sendMessage(res + "\n"); return;}
   for(i=0;i<ESIchans.count();i++)    if((res = ESIchans[i]->ProcessCommand(cmd)) != "?") {tcp->sendMessage(res + "\n"); return;}
   for(i=0;i<ARBchans.count();i++)    if((res = ARBchans[i]->ProcessCommand(cmd)) != "?") {tcp->sendMessage(res + "\n"); return;}
   if(TC != NULL) if((res = TC->TG->ProcessCommand(cmd)) != "?") {tcp->sendMessage(res + "\n"); return;}
   if(comp != NULL) if((res = comp->ProcessCommand(cmd)) != "?") {tcp->sendMessage(res + "\n"); return;}
   tcp->sendMessage("?\n");
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
// DAC channels    *********************************************************************************
// *************************************************************************************************

DACchannel::DACchannel(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    comms  = NULL;
    Units = "V";
    m = 1;
    b = 0;
    Format = "%.3f";
}

void DACchannel::Show(void)
{
    frmDAC = new QFrame(p); frmDAC->setGeometry(X,Y,180,21);
    Vdac = new QLineEdit(frmDAC); Vdac->setGeometry(70,0,70,21); Vdac->setValidator(new QDoubleValidator);
    labels[0] = new QLabel(Title,frmDAC); labels[0]->setGeometry(0,0,59,16);
    labels[1] = new QLabel(Units,frmDAC); labels[1]->setGeometry(150,0,31,16);
    Vdac->setToolTip("DAC output CH" +  QString::number(Channel) + ", "  + MIPSnm);
    connect(Vdac,SIGNAL(editingFinished()),this,SLOT(VdacChange()));
}

QString DACchannel::Report(void)
{
    QString res;

    res = Title + "," + Vdac->text();
    return(res);
}

bool DACchannel::SetValues(QString strVals)
{
    QStringList resList;

    if(!strVals.startsWith(Title)) return false;
    resList = strVals.split(",");
    if(resList.count() < 2) return false;
    Vdac->setText(resList[1]);
    Vdac->setModified(true);
    Vdac->editingFinished();
    return true;
}

// The following commands are processed:
// title            return the offset value
// title=val        sets the offset value
// returns "?" if the command could not be processed
QString DACchannel::ProcessCommand(QString cmd)
{
    if(!cmd.startsWith(Title)) return "?";
    if(cmd == Title) return Vdac->text();
    QStringList resList = cmd.split("=");
    if(resList.count()==2)
    {
       Vdac->setText(resList[1]);
       Vdac->setModified(true);
       Vdac->editingFinished();
       return "";
    }
    return "?";
}

// display = m*readValue + b
void DACchannel::Update(void)
{
    QString res;

    if(comms == NULL) return;
    comms->rb.clear();
    res = "GDACV,CH"  + QString::number(Channel) + "\n";
    res = comms->SendMess(res);
    if(res == "") return;
    res.sprintf(Format.toStdString().c_str(),res.toFloat() * m + b);
    if(!Vdac->hasFocus()) Vdac->setText(res);
}

// writeValue = (display - b)/m
void DACchannel::VdacChange(void)
{
   QString val;

   if(comms == NULL) return;
   if(!Vdac->isModified()) return;
   val.sprintf("%.3f",(Vdac->text().toFloat() - b)/m);
   QString res = "SDACV,CH" + QString::number(Channel) + "," + val + "\n";
   comms->SendCommand(res.toStdString().c_str());
   Vdac->setModified(false);
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

// title        setpoint
// title=val    setpoint
// title.readback
// title.ena
// title.ena= ON or OFF
QString ESI::ProcessCommand(QString cmd)
{
    if(!cmd.startsWith(Title)) return "?";
    if(cmd == Title) return ESIsp->text();
    if(cmd == Title + ".readback") return ESIrb->text();
    if(cmd == Title + ".ena")
    {
        if(ESIena->isChecked()) return "ON";
        return "OFF";
    }
    QStringList resList = cmd.split("=");
    if(resList.count()==2)
    {
       if(resList[0] == Title)
       {
           ESIsp->setText(resList[1]);
           ESIsp->setModified(true);
           ESIsp->editingFinished();
           return "";
       }
       if(resList[0] == Title + ".ena")
       {
           if(resList[1] == "ON") ESIena->setChecked(true);
           else if(resList[1] == "OFF") ESIena->setChecked(false);
           else return "?";
           if(resList[1] == "ON") ESIena->stateChanged(1);
           else  ESIena->stateChanged(0);
           return "";
       }
    }
    return "?";
}

void ESI::Update(void)
{
    QString res;

    if(comms == NULL) return;

    comms->rb.clear();
    res = "GHV,"  + QString::number(Channel) + "\n";
    res = comms->SendMess(res);
    if(res == "") return;
    if(!ESIsp->hasFocus()) ESIsp->setText(res);
    res = "GHVV," + QString::number(Channel) + "\n";
    res = comms->SendMess(res);
    if(res=="") return;
    ESIrb->setText(res);
    res = comms->SendMess("GHVSTATUS," + QString::number(Channel) + "\n");
    bool oldState = ESIena->blockSignals(true);
    if(res.contains("ON")) ESIena->setChecked(true);
    if(res.contains("OFF")) ESIena->setChecked(false);
    ESIena->blockSignals(oldState);
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
