//
// Controlpanel.cpp
//
// This file contains the classes used to create a user defined control panel. The control panels
// display MIPS controls for most features supported by MIPS. The user can define a background
// image and then place controls such as RF channels and DC bias channels at the locations the user
// defines on the background image.
//
// This feature also supports multiple MIPS systems connected to you PC at the same time. Use the
// "Find MIPS and Connect" button to find all connected MIPS system. The control panel configuration
// file uses the MIPS name to determine where the channels are located. Its important that you
// define a unique name for each MIPS system.
//
// The control panel also support the IFT (Ion Funnel Trap). This includes generation of an IFT
// trapping pulse sequence as well as calling an external application to acquire data from you
// detection system. Additionally a more generic timing generator control has been added.
//
// Feb 26, 2019, new control idea(s)
//  Implement a generic control with the following features:
//      - TYPE, defines the control type
//              - LineEdit, CheckBox, Button
//      - Set command, to set the MIPS value from LineEdit box
//      - Get command, to read the MIPS value to update the LineEdit box
//      - Readback command, if set add a readback box and this command will read the value
//  Syntax example:
//      Ccontrol,name,mips name,type,get command,set command,readback command,units,X,Y
//  This idea can be expanded to check boxes, radio buttons, etc.
//  This would allow a MIPS status page with control of system parameters.
//  The cool thing is this is not hard to implement and will support the FAIMSFB system.
//  For FAIMSFB I need to figureout how to do the plotting and heat maps. Generic plotting
//  is an idea I have thought about and need to mature this concept.
//  How can I support tabs or multiple control panel pages?
//      - Could have a control panel call another control panel basically a
//        child control panel. Somehow I would need to pass control, to the child.
//      - Could allow multiple control panels, this whould be pretty easy to do.
//      - Could have a master control panel and buttons to start other control panels,
//        how would methode save and load work in this case? (leading idea)
//      - Have a button that is tied to a control panel config file. load all the
//        control panels on start. when the button is pressed show the panel.
//
//  Add groupbox
//      GroupBox,name,width,hieght,X,Y  // Command to start group
//      GroupBoxEnd                     // Signals end of group box
//      Commands after group box is defined and before the end will be
//      placed in the group boc and there names will have the groupbox
//      name prepended delineated by a ".".
//
//  Add plotting capability
//      Could tie to scripting and popup a plot that is populated by a script. The script
//      could issue all the MIPS commands to scan and plot.
//
//  Buttons tied to scripts
//      This could allow plotting / scanning. You press the button and the script loads
//      and plays, the button name changes to abort. (leading idea)
//      Light up the button and if pressed again ask if the user would like to abort.
//      Add script commands for:
//          - Open file for save and load
//          - Write string to file
//          - Read string from file
//          - File CSV IO
//      Need to add script commands to allow generation of a plot and live feed of data.
//      Performance could be an issue here.
//      How to tie a script to a static plot on the control panel, via its name?
//
//  FAIMSFB specific notes:
//      A scan command could cause FAIMSFB to start scanning and send back the scan data
//      records. The scan data could be the scan point number followed by the specific
//      values, these would all be monitor or readback values. Plot(s) would be crreated
//      live. This could be a speed issues, maybe just send the electrometer data. The scan
//      data would not use a handshake, it would just be sent from MIPS.
//
//  Add script command for file selection dialog pop up. Return the selected
//  file name or empty string on cancel.
//      SelectFile(type,message,ext)
//
//  Add csv file support for scripting. Call a function with file name that will
//  read the full file and make a list of lists. The first first list is of each
//  line in the file. The second list is of the entries in each line. This will
//  be a list of QStringLists. Need the following functions:
//      ReadCSVfile(filename)
//      ReadCSVentry(int line, int entry)
//
//
// Gordon Anderson
//
#include "controlpanel.h"
#include "ui_controlpanel.h"
#include "cdirselectiondlg.h"
#include "scriptingconsole.h"

#include <QPixmap>
#include <math.h>
#include <QTextEdit>
#include <QTreeView>
#include <QUdpSocket>

// Set ProcessEvents to true to reduce the blocking during data collection.
#define ProcessEvents   false

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
    DCBgroups = NULL;
    statusBar = NULL;
    ARBcompressorButton = NULL;
    comp      = NULL;
    TC.clear();
    TextLabels.clear();
    RFchans.clear();
    RFCchans.clear();
    rfa.clear();
    ADCchans.clear();
    DACchans.clear();
    DCBchans.clear();
    DCBoffsets.clear();
    DCBenables.clear();
    DIOchannels.clear();
    ESIchans.clear();
    ARBchans.clear();
    Ccontrols.clear();
    Cpanels.clear();
    GroupBoxes.clear();
    ScripButtons.clear();
    devices.clear();
    plots.clear();
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
    if(properties != NULL) properties->Log("Control panel loaded: " + ControlPanelFile);
    MIPSnames.clear();
    for(int i=0;i<Systems.count();i++)
    {
       QString res = Systems[i]->MIPSname + ": " + Systems[i]->SendMess("GVER\n");
       // Add MIPS firmware version to log file
       if(properties != NULL) properties->Log(Systems[i]->MIPSname + ": " + Systems[i]->SendMess("GVER\n"));
       MIPSnames += "# " + res + "\n";
    }
    // Open UDP socket to send commands to reader app
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(QHostAddress("127.0.0.1"), 7755);
    // Read the configuration file and create the form as
    // well as all the controls.
    ui->lblBackground->setObjectName("");
    Container = ui->lblBackground;
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
            if((resList[0].toUpper() == "DEVICE") && (resList.length()==6))
            {
                devices.append(new Device(Container,resList[1],resList[2],resList[3],resList[4].toInt(),resList[5].toInt()));
                devices.last()->Show();
            }
            if((resList[0].toUpper() == "TEXTLABEL") && (resList.length()==5))
            {
                TextLabels.append(new TextLabel(Container,resList[1],resList[2].toInt(),resList[3].toInt(),resList[4].toInt()));
                TextLabels.last()->Show();
            }
            if((resList[0].toUpper() == "RFCHANNEL") && (resList.length()==6))
            {
                RFchans.append(new RFchannel(Container,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                RFchans.last()->Channel = resList[3].toInt();
                RFchans.last()->comms = FindCommPort(resList[2],Systems);
                RFchans.last()->Show();
            }
            if((resList[0].toUpper() == "RFCCHANNEL") && (resList.length()==6))
            {
                RFCchans.append(new RFCchannel(Container,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                RFCchans.last()->Channel = resList[3].toInt();
                RFCchans.last()->comms = FindCommPort(resList[2],Systems);
                RFCchans.last()->Show();
            }
            if((resList[0].toUpper() == "ADCCHANNEL") && (resList.length()>=6))
            {
                ADCchans.append(new ADCchannel(Container,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                ADCchans.last()->Channel = resList[3].toInt();
                ADCchans.last()->comms = FindCommPort(resList[2],Systems);
                if(resList.length()>=7)  ADCchans.last()->m = resList[6].toFloat();
                if(resList.length()>=8)  ADCchans.last()->b = resList[7].toFloat();
                if(resList.length()>=9)  ADCchans.last()->Units = resList[8];
                if(resList.length()>=10) ADCchans.last()->Format = resList[9];
                ADCchans.last()->Show();
            }
            if((resList[0].toUpper() == "DACCHANNEL") && (resList.length()>=6))
            {
                DACchans.append(new DACchannel(Container,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                DACchans.last()->Channel = resList[3].toInt();
                DACchans.last()->comms = FindCommPort(resList[2],Systems);
                if(resList.length()>=7)  DACchans.last()->m = resList[6].toFloat();
                if(resList.length()>=8)  DACchans.last()->b = resList[7].toFloat();
                if(resList.length()>=9)  DACchans.last()->Units = resList[8];
                if(resList.length()>=10) DACchans.last()->Format = resList[9];
                DACchans.last()->Show();
            }
            if((resList[0].toUpper() == "CPANEL") && (resList.length()==5))
            {
                Cpanels.append(new Cpanel(Container,resList[1],resList[2],resList[3].toInt(),resList[4].toInt(),Systems,properties));
                Cpanels.last()->Show();
            }
            if((resList[0].toUpper() == "CCONTROL") && (resList.length()>=10))
            {
                Ccontrols.append(new Ccontrol(Container,resList[1],resList[2],resList[3],resList[4],resList[5],resList[6],resList[7],resList[8].toInt(),resList[9].toInt()));
                if(resList.length()>=11) Ccontrols.last()->Dtype = resList[10];
                Ccontrols.last()->comms = FindCommPort(resList[2],Systems);
                Ccontrols.last()->Show();
            }
            if((resList[0].toUpper() == "DCBCHANNEL") && (resList.length()==6))
            {
                DCBchans.append(new DCBchannel(Container,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                DCBchans.last()->Channel = resList[3].toInt();
                DCBchans.last()->comms = FindCommPort(resList[2],Systems);
                DCBchans.last()->Show();
            }
            if((resList[0].toUpper() == "DCBOFFSET") && (resList.length()==6))
            {
                DCBoffsets.append(new DCBoffset(Container,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                DCBoffsets.last()->Channel = resList[3].toInt();
                DCBoffsets.last()->comms = FindCommPort(resList[2],Systems);
                DCBoffsets.last()->Show();
            }
            if((resList[0].toUpper() == "DCBENABLE") && (resList.length()==5))
            {
                DCBenables.append(new DCBenable(Container,resList[1],resList[2],resList[3].toInt(),resList[4].toInt()));
                DCBenables.last()->comms = FindCommPort(resList[2],Systems);
                DCBenables.last()->Show();
            }
            if((resList[0].toUpper() == "DIOCHANNEL") && (resList.length()==6))
            {
                DIOchannels.append(new DIOchannel(Container,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                DIOchannels.last()->Channel = resList[3];
                DIOchannels.last()->comms = FindCommPort(resList[2],Systems);
                DIOchannels.last()->Show();
            }
            if((resList[0].toUpper() == "ESICHANNEL") && (resList.length()==6))
            {
                ESIchans.append(new ESI(Container,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                ESIchans.last()->Channel = resList[3].toInt();
                ESIchans.last()->comms = FindCommPort(resList[2],Systems);
                ESIchans.last()->Show();
            }
            if((resList[0].toUpper() == "ARBCHANNEL") && (resList.length()==6))
            {
                ARBchans.append(new ARBchannel(Container,resList[1],resList[2],resList[4].toInt(),resList[5].toInt()));
                ARBchans.last()->Channel = resList[3].toInt();
                ARBchans.last()->comms = FindCommPort(resList[2],Systems);
                ARBchans.last()->Show();
            }
            if((resList[0].toUpper() == "RFAMP") && (resList.length()==6))
            {
                rfa.append(new RFamp(Container,resList[1],resList[2],resList[3].toInt()));
                rfa.last()->comms = FindCommPort(resList[2],Systems);
                QWidget *widget = new QWidget(this);
                widget->setGeometry(resList[4].toInt(),resList[5].toInt(),280,290);
                QVBoxLayout *layout = new QVBoxLayout(widget);
                layout->addWidget(rfa.last());
                rfa.last()->show();
            }
            if((resList[0].toUpper() == "SCRIPTBUTTON") && (resList.length()==5))
            {
                ScripButtons.append(new ScriptButton(Container, resList[1], resList[2], resList[3].toInt(), resList[4].toInt(), properties, statusBar));
                ScripButtons.last()->Show();
            }
            // GroupBox,name,width,hieght,X,Y  // Command to start group
            // GroupBoxEnd                     // Signals end of group box
            if((resList[0].toUpper() == "GROUPBOX") && (resList.length()==6))
            {
                GroupBoxes.append(new QGroupBox(Container));
                GroupBoxes.last()->setGeometry(resList[4].toInt(),resList[5].toInt(),resList[2].toInt(),resList[3].toInt());
                GroupBoxes.last()->setTitle(resList[1]);
                GroupBoxes.last()->setObjectName(resList[1]);
                Container = GroupBoxes.last();
            }
            if(resList[0].toUpper() == "GROUPBOXEND") Container = ui->lblBackground;
            if((resList[0].toUpper() == "TIMING") && (resList.length()==5))
            {
//                TC = new TimingControl(Container,resList[1],resList[2],resList[3].toInt(),resList[4].toInt());
                TC.append(new TimingControl(Container,resList[1],resList[2],resList[3].toInt(),resList[4].toInt()));
                TC.last()->comms = FindCommPort(resList[2],Systems);
                TC.last()->statusBar = statusBar;
                TC.last()->properties = properties;
                TC.last()->Show();
//                if(TC.last()->TG != NULL)
//                {
//                   foreach(DCBchannel *dcb, DCBchans) if(dcb->MIPSnm == TC.last()->MIPSnm) TC.last()->TG->AddSignal(dcb->Title, QString::number(dcb->Channel));
//                   foreach(DIOchannel *dio, DIOchannels) if(dio->MIPSnm == TC.last()->MIPSnm) TC.last()->TG->AddSignal(dio->Title, dio->Channel);
//                }
                connect(TC.last(),SIGNAL(dataAcquired(QString)),this,SLOT(slotDataAcquired(QString)));
            }
            if((resList[0].toUpper() == "EVENTCONTROL") && (resList.length()==5))
            {
                TC.last()->TG->EC.append(new EventControl(Container,resList[1],resList[2],resList[3].toInt(),resList[4].toInt()));
                TC.last()->TG->EC.last()->Show();
                connect(TC.last()->TG->EC.last(),SIGNAL(EventChanged(QString,QString)),TC.last(),SLOT(slotEventChanged(QString,QString)));
            }
            if((resList[0].toUpper() == "FILENAME") && (resList.length()==2)) if(TC.count() > 0) TC.last()->fileName = resList[1];
            if((resList[0].toUpper() == "IFT") && (resList.length()==5))
            {
                IFT = new IFTtiming(Container,resList[1],resList[2],resList[3].toInt(),resList[4].toInt());
                IFT->comms = FindCommPort(resList[2],Systems);
                IFT->statusBar = statusBar;
                IFT->properties = properties;
                IFT->Show();
                connect(IFT,SIGNAL(dataAcquired(QString)),this,SLOT(slotDataAcquired(QString)));
            }
            if((resList[0].toUpper() == "GRID1") && (resList.length()==2) && (IFT!=NULL))
            {
                IFT->Grid1 = NULL;
                for(int i=0; i< DCBchans.count(); i++) if(DCBchans[i]->Title == resList[1]) IFT->Grid1 = DCBchans[i];
            }
            if((resList[0].toUpper() == "GRID2") && (resList.length()==2) && (IFT!=NULL))
            {
                IFT->Grid2 = NULL;
                for(int i=0; i< DCBchans.count(); i++) if(DCBchans[i]->Title == resList[1]) IFT->Grid2 = DCBchans[i];
            }
            if((resList[0].toUpper() == "GRID3") && (resList.length()==2) && (IFT!=NULL))
            {
                IFT->Grid3 = NULL;
                for(int i=0; i< DCBchans.count(); i++) if(DCBchans[i]->Title == resList[1]) IFT->Grid3 = DCBchans[i];
            }
            if((resList[0].toUpper() == "ENABLE") && (resList.length()==2) && (IFT!=NULL)) IFT->Enable = resList[1];
            if((resList[0].toUpper() == "SHUTDOWN") && (resList.length()==4))
            {
                SD = new Shutdown(Container,resList[1],resList[2].toInt(),resList[3].toInt());
                SD->Show();
                connect(SD,SIGNAL(ShutdownSystem()),this,SLOT(pbSD()));
                connect(SD,SIGNAL(EnableSystem()),this,SLOT(pbSE()));
                SD->setFocus();
            }
            if((resList[0].toUpper() == "SAVELOAD") && (resList.length()==5))
            {
                SL = new SaveLoad(Container,resList[1],resList[2],resList[3].toInt(),resList[4].toInt());
                SL->Show();
                connect(SL,SIGNAL(Save()),this,SLOT(pbSave()));
                connect(SL,SIGNAL(Load()),this,SLOT(pbLoad()));
            }
            // COMPRESSOR,name,MIPS name,X,Y
            if((resList[0].toUpper() == "COMPRESSOR") && (resList.length()==5))
            {
                // Enable the ARB compressor button
                ARBcompressorButton = new QPushButton(resList[1],Container);
                ARBcompressorButton->setGeometry(resList[3].toInt(),resList[4].toInt(),150,32);
                ARBcompressorButton->setAutoDefault(false);
                ARBcompressorButton->setToolTip("Press this button to edit the compression options");
                comp = new Compressor(Container,resList[1],resList[2]);
                comp->comms = FindCommPort(resList[2],Systems);
                connect(ARBcompressorButton,SIGNAL(pressed()),this,SLOT(pbARBcompressor()));
            }
            if((resList[0].toUpper() == "MIPSCOMMS") && (resList.length()==3))
            {
                // Enable the MIPS communication button
                MIPScommsButton = new QPushButton("MIPS comms",Container);
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
                if(TC.count() > 0)
                {
                    TC.last()->AD->Acquire = line.mid(line.indexOf(",")+1);
                    #ifdef Q_OS_MAC
                    if(TC.last()->AD->Acquire .startsWith("~")) TC.last()->AD->Acquire  = QDir::homePath() + "/" + TC.last()->AD->Acquire .mid(2);
                    #endif
                }
            }
            if((resList[0].toUpper() == "SCRIPT") && (resList.length()==3))
            {
                // Use QTscrip and place a button at the defined location.
                scriptButton = new QPushButton("Scripting",Container);
                scriptButton->setGeometry(resList[1].toInt(),resList[2].toInt(),150,32);
                scriptButton->setAutoDefault(false);
                scriptButton->setToolTip("Press this button load or define a script");
                connect(scriptButton,SIGNAL(pressed()),this,SLOT(pbScript()));
                scriptconsole = new ScriptingConsole(this,properties);
            }
            if((resList[0].toUpper() == "DCBGROUPS") && (resList.length()==3))
            {
                DCBgroups = new DCBiasGroups(Container,resList[1].toInt(),resList[2].toInt());
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
    foreach(TimingControl *tc, TC)
    {
        foreach(DCBchannel *dcb, DCBchans) if(dcb->MIPSnm == tc->MIPSnm) tc->TG->AddSignal(dcb->Title, QString::number(dcb->Channel));
        foreach(DIOchannel *dio, DIOchannels) if(dio->MIPSnm == tc->MIPSnm) tc->TG->AddSignal(dio->Title, dio->Channel);
    }
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
    for(int i=0;i<devices.count();i++) delete devices[i];
    for(int i=0;i<Cpanels.count();i++) delete Cpanels[i]->cp;
    for(int i=0;i<ScripButtons.count();i++) delete ScripButtons[i];
    QApplication::processEvents();
    for(int i=0;i<plots.count();i++) delete plots[i];
    QApplication::processEvents();
    delete tcp;
    delete ui;
}

void ControlPanel::reject()
{
    QMessageBox msgBox;

    if(SystemIsShutdown)  // If its shutdown send warning
    {
        QString msg = "The system is currently shutdown, this means all voltages are disabled and ";
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
       for(i=0;i<RFCchans.count();i++)
       {
           vals = RFCchans[i]->Report().split(",");
           header += "," + vals[0] + ".DRV," + vals[0] + ".STP," + vals[0] + ".FREQ," + vals[0] + ".RF+," + vals[0] + ".RF-," + vals[0] + ".PWR";
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
       for(i=0;i<RFCchans.count();i++)
       {
           vals = RFCchans[i]->Report().split(",");
           record += "," + vals[1] + "," + vals[2] + "," + vals[3] + "," + vals[4] + "," + vals[5] + "," + vals[6];
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
   QMessageBox msgBox;
   int i,j,k;

   QApplication::processEvents();
   if(scriptconsole!=NULL) scriptconsole->UpdateStatus();
   if(UpdateStop) return;
   for(i=0;i<TC.count();i++) if(TC[i]->Downloading) return;
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
       for(i=0;i<Systems.count();i++)    Systems[i]->rb.clear();
       for(i=0;i<ESIchans.count();i++)   ESIchans[i]->Shutdown();
       for(i=0;i<DCBenables.count();i++) DCBenables[i]->Shutdown();
       for(i=0;i<RFchans.count();i++)    RFchans[i]->Shutdown();
       for(i=0;i<RFCchans.count();i++)   RFCchans[i]->Shutdown();
       for(i=0;i<Ccontrols.count();i++)  Ccontrols[i]->Shutdown();
       for(i=0;i<rfa.count();i++)        rfa[i]->Shutdown();
       for(i=0;i<ARBchans.count();i++)   ARBchans[i]->Shutdown();
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
       for(i=0;i<RFCchans.count();i++)   {RFCchans[i]->Restore(); msDelay(250);}
       for(i=0;i<Ccontrols.count();i++)  Ccontrols[i]->Restore();
       for(i=0;i<rfa.count();i++)        rfa[i]->Restore();
       for(i=0;i<ARBchans.count();i++)   ARBchans[i]->Restore();
       if(statusBar != NULL) statusBar->showMessage("System enabled, " + QDateTime().currentDateTime().toString());
       if(properties != NULL) properties->Log("System Restored");
       UpdateHoldOff = 1;
       return;
   }
   // Look for any broken connections and warn the user.
   // Also allow user to try and reconnect.
   QString res;
   res.clear();
   for(i=0;i<Systems.count();i++)
   {
       QApplication::processEvents();  // Not sure why this is needed?
       if(!Systems[i]->isConnected())
       {
           res += Systems[i]->MIPSname + ",";
       }
   }
   if(!res.isEmpty())
   {
       int ret;
       bool AutoR = false;
       if(properties != NULL) if(properties->AutoRestore) AutoR = true;
       if(!AutoR)
       {
           QString msg = "Connection(s) have been lost to " + res;
           msg += "would you like to try and reestablish the connection(s)";
           msg += "or exit this control panel.";
           msgBox.setText(msg);
           msgBox.setInformativeText("Select Yes to reestablish connection(s)?");
           msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
           msgBox.setDefaultButton(QMessageBox::Yes);
           ret = msgBox.exec();
       }
       else
       {
           if(statusBar != NULL) statusBar->showMessage("Attempting to reestablish connection(s).",2000);
           msDelay(2000);
           ret = QMessageBox::Yes;
       }
       if(ret == QMessageBox::Yes)
       {
           for(i=0;i<Systems.count();i++) if(!Systems[i]->isConnected()) Systems[i]->reopenPort();
       }
       else reject();
   }
   // For each MIPS system present if there are RF channels for the selected
   // MIPS system then read all values using the read all commands to speed up the process.
   for(i=0;i<Systems.count();i++)
   {
       for(j=0;j<RFchans.count();j++) if(RFchans[j]->comms == Systems[i])
       {
           // Read all the RF parameters
           QString RFallRes = Systems[i]->SendMess("GRFALL\n");
           if(ProcessEvents) QApplication::processEvents();
           QStringList RFallResList = RFallRes.split(",");
           if(RFallResList.count() < 1)
           {
               // Here with group read error so process one at a time
               for(k=0;k<RFchans.count();k++) if(RFchans[k]->comms == Systems[i]) { RFchans[k]->Update(); /*QApplication::processEvents();*/ }
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
                   if(ProcessEvents) QApplication::processEvents();
               }
           }
           break;
       }
   }
   for(i=0;i<Systems.count();i++)
   {
       for(j=0;j<RFCchans.count();j++) if(RFCchans[j]->comms == Systems[i])
       {
           // Read all the RF parameters
           QString RFallRes = Systems[i]->SendMess("GRFALL\n");
           if(ProcessEvents) QApplication::processEvents();
           QStringList RFallResList = RFallRes.split(",");
           if(RFallResList.count() < 1)
           {
               // Here with group read error so process one at a time
               for(k=0;k<RFCchans.count();k++) if(RFCchans[k]->comms == Systems[i]) { RFCchans[k]->Update(); /*QApplication::processEvents();*/ }
           }
           else
           {
               // build strings and update all channels that use this comm port
               for(k=0;k<RFCchans.count();k++) if(RFCchans[k]->comms == Systems[i])
               {
                   if(RFallResList.count() < (RFCchans[k]->Channel * 4)) RFCchans[k]->Update();
                   else RFCchans[k]->Update(RFallResList[(RFCchans[k]->Channel - 1) * 4 + 0] + "," + \
                                           RFallResList[(RFCchans[k]->Channel - 1) * 4 + 1] + "," + \
                                           RFallResList[(RFCchans[k]->Channel - 1) * 4 + 2] + "," + \
                                           RFallResList[(RFCchans[k]->Channel - 1) * 4 + 3]);
                   if(ProcessEvents) QApplication::processEvents();
               }
           }
           break;
       }
   }
   for(i=0;i<ADCchans.count();i++)    {ADCchans[i]->Update(); if(ProcessEvents) QApplication::processEvents();}
   for(i=0;i<DACchans.count();i++)    {DACchans[i]->Update(); if(ProcessEvents) QApplication::processEvents();}
   // For each MIPS system present if there are DCBchannels for the selected
   // MIPS system then read all setpoints and values using the read all
   // commands to speed up the process.
   for(i=0;i<Systems.count();i++)
   {
       for(j=0;j<DCBchans.count();j++) if(DCBchans[j]->comms == Systems[i])
       {
           // Read all the setpoints and readbacks and parse the strings
           QString VspRes = Systems[i]->SendMess("GDCBALL\n");
           if(ProcessEvents) QApplication::processEvents();
           QStringList VspResList = VspRes.split(",");
           QString VrbRes = Systems[i]->SendMess("GDCBALLV\n");
           if(ProcessEvents) QApplication::processEvents();
           QStringList VrbResList = VrbRes.split(",");
           if((VspResList.count() != VrbResList.count()) || (VspResList.count() < 1))
           {
               // Here with group read error so process one at a time
               for(k=0;k<DCBchans.count();k++) if(DCBchans[k]->comms == Systems[i]) {DCBchans[k]->Update(); /*QApplication::processEvents();*/}
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
   for(i=0;i<DCBoffsets.count();i++)  {DCBoffsets[i]->Update(); if(ProcessEvents) QApplication::processEvents();}
   for(i=0;i<DCBenables.count();i++)  {DCBenables[i]->Update(); if(ProcessEvents) QApplication::processEvents();}
   if(TC.count() > 0)
   {
       for(i=0;i<=TC.count();i++)
       {
           if(i==TC.count())
           {
              for(j=0;j<DIOchannels.count();j++) {DIOchannels[j]->Update(); if(ProcessEvents) QApplication::processEvents();}
              break;
           }
           if(TC[i]->TG->isTableMode()) break;
       }
   }
   else for(i=0;i<DIOchannels.count();i++) {DIOchannels[i]->Update(); if(ProcessEvents) QApplication::processEvents();}
   for(i=0;i<ESIchans.count();i++)         {ESIchans[i]->Update(); if(ProcessEvents) QApplication::processEvents();}
   for(i=0;i<ARBchans.count();i++)         {ARBchans[i]->Update(); if(ProcessEvents) QApplication::processEvents();}
   for(i=0;i<Ccontrols.count();i++)        {Ccontrols[i]->Update();if(ProcessEvents) QApplication::processEvents();}
   for(i=0;i<rfa.count();i++)              {rfa[i]->Update();      if(ProcessEvents) QApplication::processEvents();}
   for(i=0;i<devices.count();i++)          {devices[i]->Update();}
   for(i=0;i<Cpanels.count();i++)          Cpanels[i]->Update();
   for(i=0;i<TC.count();i++)               for(j=0;j<TC[i]->TG->EC.count();j++) TC[i]->TG->EC[j]->Update();
   if(comp!=NULL)                          {comp->Update();        if(ProcessEvents) QApplication::processEvents();}
   RequestUpdate = false;
   LogDataFile();
}

// Save methode function
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
        stream << MIPSnames;
        // Add uptime if MIPS systems are connected
        for(int i=0;i<Systems.count();i++)
        {
           QString res = Systems[i]->SendMess("UPTIME\n");
           if(res!="" && !res.contains("?")) stream << "# " + Systems[i]->MIPSname + ": " + res + "\n";
        }
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
        if(RFCchans.count() > 0)
        {
            stream << "RFC channels," + QString::number(RFCchans.count()) + "\n";
            for(int i=0;i<RFCchans.count();i++) stream << RFCchans[i]->Report() + "\n";
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
        if(Ccontrols.count() > 0)
        {
            stream << "Custom control channels," + QString::number(Ccontrols.count()) + "\n";
            for(int i=0; i<Ccontrols.count(); i++) stream << Ccontrols[i]->Report() + "\n";
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
        if(devices.count() > 0)
        {
            stream << "External devices," + QString::number(rfa.count()) + "\n";
            for(int i=0; i<devices.count(); i++) if(devices[i] != NULL) stream << devices[i]->Report();
            stream << "ExternalDevicesEnd\n";
        }
        if(IFT != NULL)
        {
            stream << "IFT parameters\n";
            stream << IFT->Report() + "\n";
        }
        if(TC.count() > 0)
        {
            stream << "Timing control parameters\n";
            for(int i=0; i<TC.count(); i++)
            {
                stream << TC[i]->TG->Report() + "\n";
                for(int j=0; j<TC[i]->TG->EC.count();j++) stream << TC[i]->TG->EC[j]->Report() + "\n";
            }
            stream << "TCparametersEnd\n";
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

    // Test is Filename ends in .settings, if not add .settings
    if(!Filename.toUpper().endsWith(".SETTINGS")) Filename += ".settings";
    if(properties != NULL) QDir().setCurrent(properties->MethodesPath);
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
                if(resList[0] == "RFC channels") for(int i=0;i<resList[1].toInt();i++)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     for(int j=0;j<RFCchans.count();j++) if(RFCchans[j]->SetValues(line)) break;
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
                if(resList[0] == "Custom control channels") for(int i=0;i<resList[1].toInt();i++)
                {
                     line = stream.readLine();
                     if(line.isNull()) break;
                     for(int j=0;j<Ccontrols.count();j++) if(Ccontrols[j]->SetValues(line)) break;
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
                    for(int i=0;i<TC.count();i++) TC[i]->TG->Events.clear();
                    while(true)
                    {
                        line = stream.readLine();
                        if(line.isNull()) break;
                        if(line.contains("TCparametersEnd")) break;
                        for(int i=0;i<TC.count();i++)
                        {
                            TC[i]->TG->SetValues(line);
                            for(int j=0;j<TC[i]->TG->EC.count();j++) TC[i]->TG->EC[j]->SetValues(line);
                        }
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
        udpSocket->writeDatagram(mess.toLocal8Bit(),QHostAddress("127.0.0.1"), 7755);
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
QString ControlPanel::GetLine(QString MIPSname)
{
   QApplication::processEvents();
   Comms *cp =  FindCommPort(MIPSname,Systems);
   if(cp==NULL) return "";
   cp->waitforline(500);
   return cp->getline();
}

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
   if(TC.count() > 0) TC.last()->AcquireData(filePath);
}

bool ControlPanel::isAcquiring(void)
{
  QApplication::processEvents();
  if(IFT != NULL) return(IFT->AD->Acquiring);
  if(TC.count() > 0) return(TC.last()->AD->Acquiring);
  return(false);
}

void ControlPanel::DismissAcquire(void)
{
  QApplication::processEvents();
  if(IFT != NULL)  IFT->AD->Dismiss();
  if(TC.count() > 0)  TC.last()->AD->Dismiss();
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
    UpdateStop = stop;
    return UpdateStop;
}

void ControlPanel::WaitForUpdate(void)
{
    RequestUpdate = true;
    while(RequestUpdate) QApplication::processEvents();
}

// This function is called with a command in the TCP server buffer.
// This function will process this command.
void ControlPanel::tcpCommand(void)
{
    QString cmd = tcp->readLine();
    QString resp = Command(cmd);
    tcp->sendMessage(resp);
}

// This function is called by the TCPserver code to process commands and
// also by the script engine.
QString ControlPanel::Command(QString cmd)
{
   int     i,j;
   QStringList resList;
   QString res;

   res.clear();
   // Process global commands first.
   // Load,Save,Shutdown,Restore
   if(cmd.toUpper() == "SHUTDOWN") { ShutdownFlag = true; return res; }
   if(cmd.toUpper() == "RESTORE")  { RestoreFlag  = true; return res; }
   if(cmd.toUpper().startsWith("LOAD"))
   {
       resList = cmd.split(",");
       if(resList.count()==2) Load(resList[1]);
       return res;
   }
   if(cmd.toUpper().startsWith("SAVE"))
   {
       resList = cmd.split(",");
       if(resList.count()==2) Save(resList[1]);
       return res;
   }
   // Process commands that will allow communications with the MIPS hardware directly
   if(cmd.trimmed().toUpper().startsWith("SENDMESSAGE"))
   {
       resList = cmd.split(",");
       if(resList.count() > 2)
       {
           QString arg;
           arg.clear();
           for(int i=2;i<resList.count();i++)
           {
               if(i>2) arg += ",";
               arg += resList[i];
           }
           //qDebug() << "SendMessage: " << resList[1] << "," << arg;
           return(SendMess(resList[1],arg + "\n"));
       }
       else return("?\n");
   }
   if(cmd.trimmed().toUpper().startsWith("SENDCOMMAND"))
   {
       resList = cmd.split(",");
       if(resList.count() > 2)
       {
           QString arg;
           arg.clear();
           for(int i=2;i<resList.count();i++)
           {
               if(i>2) arg += ",";
               arg += resList[i];
           }
           //qDebug() << "SendCommand: " << resList[1] << "," << arg;
           if(SendCommand(resList[1],arg + "\n")) return("\n");
           else return("?\n");
       }
       else return("?\n");
   }
   // Send the command string to all the controls for someone to process!
   for(i=0;i<DACchans.count();i++)    if((res = DACchans[i]->ProcessCommand(cmd)) != "?") return(res + "\n");
   for(i=0;i<ADCchans.count();i++)    if((res = ADCchans[i]->ProcessCommand(cmd)) != "?") return(res + "\n");
   for(i=0;i<DCBchans.count();i++)    if((res = DCBchans[i]->ProcessCommand(cmd)) != "?") return(res + "\n");
   for(i=0;i<RFchans.count();i++)     if((res = RFchans[i]->ProcessCommand(cmd)) != "?") return(res + "\n");
   for(i=0;i<RFCchans.count();i++)    if((res = RFCchans[i]->ProcessCommand(cmd)) != "?") return(res + "\n");
   for(i=0;i<DCBoffsets.count();i++)  if((res = DCBoffsets[i]->ProcessCommand(cmd)) != "?") return(res + "\n");
   for(i=0;i<DCBenables.count();i++)  if((res = DCBenables[i]->ProcessCommand(cmd)) != "?") return(res + "\n");
   for(i=0;i<DIOchannels.count();i++) if((res = DIOchannels[i]->ProcessCommand(cmd)) != "?") return(res + "\n");
   for(i=0;i<ESIchans.count();i++)    if((res = ESIchans[i]->ProcessCommand(cmd)) != "?") return(res + "\n");
   for(i=0;i<ARBchans.count();i++)    if((res = ARBchans[i]->ProcessCommand(cmd)) != "?") return(res + "\n");
   for(i=0;i<Ccontrols.count();i++)   if((res = Ccontrols[i]->ProcessCommand(cmd)) != "?") return(res + "\n");
   for(i=0;i<devices.count();i++)     if((res = devices[i]->ProcessCommand(cmd)) != "?") return(res + "\n");
   for(i=0;i<ScripButtons.count();i++) if((res = ScripButtons[i]->ProcessCommand(cmd)) != "?") return(res + "\n");
   for(i=0;i<TC.count();i++)
   {
       if((res = TC[i]->TG->ProcessCommand(cmd)) != "?") return(res + "\n");
       for(j=0;j<TC[i]->TG->EC.count();j++) if((res = TC[i]->TG->EC[j]->ProcessCommand(cmd)) != "?") return(res + "\n");
   }
   if(comp != NULL) if((res = comp->ProcessCommand(cmd)) != "?") return(res + "\n");
   return("?\n");
}

// This function allows the script system to popup a file selection dalog for file open
// or save.
QString ControlPanel::SelectFile(QString fType, QString Title, QString Ext)
{
    QString fileName = "";

    if(fType.toUpper() == "OPEN")
    {
        fileName = QFileDialog::getOpenFileName(this, Title,"",Ext + " (*." + Ext + ");;All files (*.*)");
    }
    if(fType.toUpper() == "SAVE")
    {
       fileName = QFileDialog::getSaveFileName(this, Title,"",Ext + " (*." + Ext + ");;All files (*.*)");
    }
    return fileName;
}

// Reads a CSV file into the CSVdata list and divides it into lines and elements.
int ControlPanel::ReadCSVfile(QString fileName, QString delimiter)
{
    CSVdata.clear();
    // Open the file for text read
    QFile file(fileName);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        // We're going to streaming the file to a QString
        QTextStream stream(&file);
        QString line;
        do
        {
            line = stream.readLine();
            //qDebug() << line;
            QStringList *qsl = new QStringList();
            *qsl = line.split(delimiter);
            CSVdata.append(qsl);
        } while(!line.isNull());
        file.close();
        return(CSVdata.count());
    }
    return -1;
}

// Returns the selected CSV line and entry in the selected line
QString ControlPanel::ReadCSVentry(int line, int entry)
{
    if(CSVdata.count() <= line) return("");
    if(CSVdata[line]->count() <= entry) return("");
    return (*CSVdata[line])[entry];
}

void ControlPanel::CreatePlot(QString Title, QString Yaxis, QString Xaxis, int NumPlots)
{
    plots.append(new Plot(0, Title, Yaxis, Xaxis, NumPlots));
    plots.last()->show();
    plots.last()->activateWindow();
    plots.last()->raise();
}

void ControlPanel::PlotCommand(QString cmd)
{
    plots.last()->PlotCommand(cmd);
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

    res.clear();
    if(p->objectName() != "") res = p->objectName() + ".";
    res += Title + "," + Vdac->text();
    return(res);
}

bool DACchannel::SetValues(QString strVals)
{
    QStringList resList;
    QString res;

    res.clear();
    if(p->objectName() != "") res = p->objectName() + ".";
    res += Title;
    if(!strVals.startsWith(res)) return false;
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
    QString res;

    res.clear();
    if(p->objectName() != "") res = p->objectName() + ".";
    res += Title;
    if(!cmd.startsWith(res)) return "?";
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

    res.clear();
    if(p->objectName() != "") res = p->objectName() + ".";
    res += Title + ",";
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
    QString res;

    res.clear();
    if(p->objectName() != "") res = p->objectName() + ".";
    res += Title;
    if(!strVals.startsWith(res)) return false;
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
    QString res;

    res.clear();
    if(p->objectName() != "") res = p->objectName() + ".";
    res += Title;
    if(!cmd.startsWith(res)) return "?";
    if(cmd == res) return ESIsp->text();
    if(cmd == res + ".readback") return ESIrb->text();
    if(cmd == res + ".ena")
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
       if(resList[0] == res + ".ena")
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

// **********************************************************************************************
// Ccontrol *************************************************************************************
// **********************************************************************************************
//  Implements a generic control with the following features:
//      - TYPE, defines the control type
//              - LineEdit, CheckBox, Button
//      - Set command
//              to set the MIPS value from LineEdit box
//              checked state command for the CheckBox
//              pressed command for Button
//      - Get command
//              to read the MIPS value to update the LineEdit box
//              unchecked state command for the CheckBox
//              not used for Button
//      - Readback command
//              if set add a readback box and this command will read the value for LineEdit
//              if CheckBox has read command then format is COMMAND_CHK_UNCHK
//              not used for Button
//  Syntax example:
//      Ccontrol,name,mips name,type,set command,get command,readback command,units,X,Y

Ccontrol::Ccontrol(QWidget *parent, QString name, QString MIPSname,QString Type, QString Gcmd, QString Scmd, QString RBcmd, QString Units, int x, int y)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    GetCmd = Gcmd.replace("_",",");
    SetCmd = Scmd.replace("_",",");
    ReadbackCmd = RBcmd.replace("_",",");
    UnitsText = Units;
    Ctype = Type;
    X      = x;
    Y      = y;
    comms  = NULL;
    isShutdown = false;
    Updating   = false;
    UpdateOff  = false;
    ShutdownValue.clear();
    Dtype = "Double";
//    qApp->installEventFilter(this);
}

void Ccontrol::Show(void)
{
    if(Ctype == "LineEdit")
    {
        frmCc = new QFrame(p);
        labels[0] = new QLabel(Title,frmCc); labels[0]->setGeometry(0,0,59,16);
        labels[1] = new QLabel(UnitsText,frmCc);
        // If Read back command or set command are empty then its only one lineEdit
        // boxe, else two.
        if(SetCmd.isEmpty() || ReadbackCmd.isEmpty())
        {
            // Only 1 line edit box
            frmCc->setGeometry(X,Y,171,21);
            if(SetCmd.isEmpty())
            {
                Vrb = new QLineEdit(frmCc);
                Vrb->setGeometry(70,0,70,21);
                Vrb->setReadOnly(true);
            }
            else if(ReadbackCmd.isEmpty())
            {
                Vsp = new QLineEdit(frmCc);
                Vsp->setGeometry(70,0,70,21);      
                if(Dtype.toUpper() == "DOUBLE") Vsp->setValidator(new QDoubleValidator);
                Vsp->setToolTip(MIPSnm + "," + SetCmd);
                connect(Vsp,SIGNAL(editingFinished()),this,SLOT(VspChange()));
            }
            labels[1]->setGeometry(150,0,21,16);
        }
        else
        {
            frmCc->setGeometry(X,Y,241,21);
            Vsp = new QLineEdit(frmCc); Vsp->setGeometry(70,0,70,21); //Vsp->setValidator(new QDoubleValidator);
            Vrb = new QLineEdit(frmCc); Vrb->setGeometry(140,0,70,21); Vrb->setReadOnly(true);
            labels[1]->setGeometry(220,0,21,16);
            Vsp->setToolTip(MIPSnm + "," + SetCmd);
            connect(Vsp,SIGNAL(editingFinished()),this,SLOT(VspChange()));
        }
    }
    if(Ctype == "CheckBox")
    {
        frmCc = new QFrame(p); frmCc->setGeometry(X,Y,241,21);
        chkBox = new QCheckBox(frmCc); chkBox->setGeometry(0,0,200,21);
        chkBox->setText(Title);
        connect(chkBox,SIGNAL(stateChanged(int)),this,SLOT(chkBoxStateChanged(int)));
    }
    if(Ctype == "Button")
    {
        pbButton = new QPushButton(Title,p);
        pbButton->setGeometry(X,Y,150,32);
        pbButton->setAutoDefault(false);
        connect(pbButton,SIGNAL(pressed()),this,SLOT(pbButtonPressed()));
    }
}

void Ccontrol::Update(void)
{
    QString res;

    if(comms == NULL) return;
    comms->rb.clear();
    if(Ctype == "LineEdit")
    {
        if(!GetCmd.isEmpty())
        {
            res = comms->SendMess(GetCmd + "\n");
            if(res == "") return;
            if(!Vsp->hasFocus()) Vsp->setText(res);
        }
        if(!ReadbackCmd.isEmpty())
        {
            res = comms->SendMess(ReadbackCmd + "\n");
            if(res == "") return;
            Vrb->setText(res);
        }
    }
    if(Ctype == "CheckBox")
    {
        // If there is a readback string then this is the update command. The
        // will contain the check and unchecked responses that are , delimited.
        if(!ReadbackCmd.isEmpty())
        {
            QStringList resList = ReadbackCmd.split(",");
            if(resList.count() >= 3)
            {
                if(resList.count() == 3) res =  comms->SendMess(resList[0] + "\n");
                if(resList.count() == 4) res =  comms->SendMess(resList[0] + "," + resList[1] + "\n");
                chkBox->setUpdatesEnabled(false);
                if(res == resList[resList.count() - 2]) chkBox->setChecked(true);
                else if(res == resList[resList.count() - 1]) chkBox->setChecked(false);
                chkBox->setUpdatesEnabled(true);
            }
        }
    }
}

QString Ccontrol::Report(void)
{
    QString res;

    res.clear();
    if(p->objectName() != "") res = p->objectName() + ".";
    res += Title;
    if(Ctype == "LineEdit")
    {
        if(!GetCmd.isEmpty() || !SetCmd.isEmpty())
        {
            res += "," + Vsp->text();
            if(isShutdown) res += "," + ActiveValue;
            else res += "," + Vsp->text();
        }
        if(!ReadbackCmd.isEmpty()) res += "," + Vrb->text();
        return(res);
    }
    if(Ctype == "CheckBox")
    {
        if(!ReadbackCmd.isEmpty())
        {
            QStringList resList = ReadbackCmd.split("_");
            if(resList.count() == 3)
            {
                if(chkBox->isChecked()) return resList[1];
                else return resList[2];
            }
        }
        if(chkBox->isChecked()) return(res + "," + "TRUE");
        else return(res + "," + "FALSE");
    }
    return("");
}

bool Ccontrol::SetValues(QString strVals)
{
    QStringList resList;
    QString res;

    res.clear();
    if(p->objectName() != "") res = p->objectName() + ".";
    res += Title;
    if(Ctype == "LineEdit")
    {
        if(!strVals.startsWith(res)) return false;
        resList = strVals.split(",");
        if(resList.count() < 2) return false;
        if(SetCmd.isEmpty()) return false;
        if(isShutdown) ActiveValue = resList[1];
        else
        {
            Vsp->setText(resList[1]);
            Vsp->setModified(true);
            Vsp->editingFinished();
        }
        return true;
    }
    if(Ctype == "CheckBox")
    {
        if(!strVals.startsWith(res)) return false;
        resList = strVals.split(",");
        if(resList.count() < 2) return false;
        if(!ReadbackCmd.isEmpty())
        {
            QStringList resCmd = ReadbackCmd.split(",");
            if(resCmd.count() == 3)
            {
                bool returnstate = true;
                chkBox->setUpdatesEnabled(false);
                if(resCmd[1] == resList[1]) chkBox->setChecked(true);
                else if(resCmd[2] == resList[1]) chkBox->setChecked(false);
                else returnstate = false;
                chkBox->setUpdatesEnabled(true);
                return returnstate;
            }
        }
        bool returnstate = true;
        chkBox->setUpdatesEnabled(false);
        if(resList[1] == "TRUE") chkBox->setChecked(true);
        else if(resList[1] == "FALSE") chkBox->setChecked(false);
        else returnstate = false;
        chkBox->setUpdatesEnabled(true);
        return returnstate;
    }
    return false;
 }

void Ccontrol::Shutdown(void)
{
    if(Ctype == "LineEdit")
    {
        if(ShutdownValue.isEmpty()) return;
        if(isShutdown) return;
        isShutdown = true;
        ActiveValue = Vsp->text();
        Vsp->setText(ShutdownValue);
        Vsp->setModified(true);
        Vsp->editingFinished();
    }
}

void Ccontrol::Restore(void)
{
    if(Ctype == "LineEdit")
    {
        if(!isShutdown) return;
        isShutdown = false;
        Vsp->setText(ActiveValue);
        Vsp->setModified(true);
        Vsp->editingFinished();
    }
}

// The following commands are processed:
// title            return the setpoint
// title=val        sets the setpoint
// title.readback   returns the readback voltage
// returns "?" if the command could not be processed
QString Ccontrol::ProcessCommand(QString cmd)
{
    QString res;

    res.clear();
    if(p->objectName() != "") res = p->objectName() + ".";
    res += Title;
    if(Ctype == "LineEdit")
    {
        if(!cmd.startsWith(res)) return "?";
        if(cmd == res)
        {
            if(!SetCmd.isEmpty() || !GetCmd.isEmpty()) return Vsp->text();
            if(!ReadbackCmd.isEmpty()) return Vrb->text();
        }
        if(cmd == res + ".readback")
        {
            if((!SetCmd.isEmpty() || !GetCmd.isEmpty()) && !ReadbackCmd.isEmpty()) return Vrb->text();
            return "?";
        }
        QStringList resList = cmd.split("=");
        if((resList.count()==2) && !SetCmd.isEmpty())
        {
           Vsp->setText(resList[1].trimmed());
           Vsp->setModified(true);
           Vsp->editingFinished();
           return "";
        }
        return "?";
    }
    if(Ctype == "CheckBox")
    {
        if(!cmd.startsWith(res)) return "?";
        if(cmd == res)
        {
            if(!ReadbackCmd.isEmpty())
            {
                QStringList resCmd = ReadbackCmd.split("_");
                if(resCmd.count() == 3)
                {
                    if(chkBox->isChecked()) return resCmd[1];
                    else return resCmd[2];
                }
            }
            if(chkBox->isChecked()) return "TRUE";
            else return "FALSE";
        }
        QStringList resList = cmd.split("=");
        if((resList.count()==2) && !SetCmd.isEmpty())
        {
            if(!ReadbackCmd.isEmpty())
            {
                QStringList resCmd = ReadbackCmd.split("_");
                if(resCmd.count() == 3)
                {
                    if(resCmd[1] == resList[1]) chkBox->setChecked(true);
                    else if(resCmd[2] == resList[1]) chkBox->setChecked(false);
                    else return "?";
                    return "";
                }
            }
            if(resList[1] == "TRUE") chkBox->setChecked(true);
            else if(resList[1] == "FALSE") chkBox->setChecked(false);
            else return "?";
            return "";
        }
    }
    if(Ctype == "Button")
    {
        if(!cmd.startsWith(res)) return "?";
        if(cmd == res) pbButtonPressed();
    }
    return "?";
}

void Ccontrol::VspChange(void)
{
   if(!Vsp->isModified()) return;
   QString res = SetCmd + "," + Vsp->text() + "\n";
   if(comms != NULL) comms->SendCommand(res.toStdString().c_str());
   Vsp->setModified(false);
}

void Ccontrol::pbButtonPressed(void)
{
    if(comms == NULL) return;
    pbButton->setDown(false);
    comms->SendCommand(SetCmd + "\n");
}

void Ccontrol::chkBoxStateChanged(int state)
{
    if(comms == NULL) return;
    if(state == Qt::Checked) comms->SendCommand(SetCmd + "\n");
    else if(state == Qt::Unchecked) comms->SendCommand(GetCmd + "\n");
}

// **********************************************************************************************
// Cpanel   *************************************************************************************
// **********************************************************************************************

Cpanel::Cpanel(QWidget *parent, QString name, QString CPfileName, int x, int y, QList<Comms*> S, Properties *prop)
{
    cp = new ControlPanel(0,CPfileName,S,prop);
    p      = parent;
    X      = x;
    Y      = y;
    Title = name;
    connect(cp,SIGNAL(DialogClosed()),this,SLOT(slotDialogClosed()));
}

void Cpanel::Show(void)
{
    pbButton = new QPushButton(Title,p);
    pbButton->setGeometry(X,Y,150,32);
    pbButton->setAutoDefault(false);
    connect(pbButton,SIGNAL(pressed()),this,SLOT(pbButtonPressed()));
}

void Cpanel::Update(void)
{
    if(cp->isVisible()) cp->Update();
}

void Cpanel::pbButtonPressed(void)
{
    pbButton->setDown(false);
    cp->show();
    cp->raise();
}

void Cpanel::slotDialogClosed(void)
{
    cp->hide();
}

