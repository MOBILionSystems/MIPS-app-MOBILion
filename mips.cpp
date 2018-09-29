//
// MIPS
//
// This application is desiged to communicate with the MIPS system using a USB commection
// to the MIPS system. The MIPS system is controlled using a Arduino Due and the USB
// connection can be made to the native port or the programming port. If using the native
// port then the comm parameters are not important, if using the programming port set
// the baud rate to 115200, 8 bits, no parity, and 1 stop bit.
//
// This application support control and monitoring of most MIPS function for DCbias and
// digital IO function. Support is provided to create and edit pulse sequences.
//
// Note!
//  Opening the serial port on the native USB connection to the DUE and sentting the
//  data terminal ready line to false will erase the DUE flash.
//          serial->setDataTerminalReady(false);
//
// Gordon Anderson
//
//  Revision history:
//  1.0, July 6, 2015
//      1.) Initial release
//  1.1, July 8, 2015
//      1.) Added MIPS firmware download function. Works on PC and MAC
//      2.) Removed the 1200 baud rate and all rates below 9600.
//  1.2, July 12, 2015
//      1.) Fixed the bugs in the firmware download feature.
//      2.) Tested on MAC and PC
//      3.) Updated a lot of screen issues from PC to MAC. Most features tested.
//  1.3, November 30, 2015
//      1.) Added socket support for networked MIPS.
//  1.4, January 10, 2016
//      1.) Added MIPS firmware save and boot bit setting functions
//  1.5, February 17, 2016
//      1.) Fixed bug in DC bias page that caused control is focus to update with old value
//          when returning to page.
//      2.) Fixed but that did not allow all 16 values to be edited on DC bias page
//      3.) Fixed range of DC bias page to reflect offset value
//  1.6, June 27, 2016
//      1.) Started refactoring the code. Created the comms class and moved all communications to this class.
//      2.) Add the Twave class and tab.
//      3.) Still to do, create classes for each module and finish refactoring.
//  1.7, Sept 16, 2016
//      1.) Updated the help files
//      2.) Added new clock modes to PSG
//      3.) Added tool tip popup text
//  1.8, Oct 16, 2016
//      1.) Added fwr/rev to twave options
//      2.) Added Sweep functions to Twave tab
//      3.) Updated the MIPS commands help function
//      4.) Updated the Twave help data
//      5.) Add frequency control to DIO page
// 1.9, March 7, 2017
//      1.) Added the FAIMS tab and the CV parking capability
//      2.) Updated help text
//      3.) Updated MIPS command summary
// 1.10, April 9, 2017
//      1.) Added the Single funnel control panel
//      2.) Added the Soft landing control panel
//      3.) Allow DCbias mould to auto update
//      4.) Added filament tab
//      5.) Changed connect to only show supported tab after connecting
// 1.11, April 29, 2017
//      1.) Updated ARB diaplog to support 4 ARB modules. (not tested)
//      2.) Need to make ARB context sensitive help with the multi-pass table functions
// 1.12, May 9, 2017
//      1.) Updated the softlanding form. Add the mass units and a number of minor improvments
//          requested by the customer.
// 1.13, June 24, 2017
//      1.) Added support for multiple MIPS systems. This new farmwork will allow control
//          panels that communicate to multiple MIPS systems.
//      2.) Updated the serial port scanning
//      3.) Changed the disconnection procedure
// 1.14, July 24, 2017
//      1.) Added a number of host interface commands to allow sending and receiving files
//          to SD card as well as sending and receiving EEPROM module configureation data.
// 1.15, August 22, 2017
//      1.) Added the ramp up and down pulse generation to the sequence generator
//      2.) Fixed a few minor bugs
//      3.) Fixed bug in loop timepoint calcualtation for sequence generator
// 1.16, November 21, 2017
//      1.) Updated the grid fragmentor and added the plotting capability
//      2.) Added the softlanding control panel
// 1.17, November 25, 2017
//      1.) Added the pulse sequence generator to the gid control panel
// 1.18, December 12, 2017
//      1.) Added ADC tab and data collection capability
//      2.) Updated grid pse
// 1.19, December 27, 2017
//      1.) Updated the pulse sequence generator in the Grid control panel as follows:
//             - Enabled Ts1 set to zero to stop the pulse
//             - When pulses are stopped the outputs are now driven high
// 1.20, January 1, 2018
//      1.) Inverted the shutter signals in the Grid pulse sequence generator
//      2.) Added the Ts1=Ts2 and Ts2=Ts1 check boxes
// 1.21, February 18, 2018
//      1.) Added scripting capability to Purdue softlanding control panel
// 1.22, March 11, 2018
//      1.) Added the support for programming the Flash on the ARB
//      2.) Added up and down arrow control of values for the DB bias
//          and Twave values. Also added limits for the Twave values
//          specifically the pulse voltage
// 1.23, March 12, 2018
//      1.) Added shift+alt arrow to decrease increment by 10x.
//      2.) Fixed limit bug in Twave freq in arrow keys
// 1.24, May 12, 2018
//      1.) Added the group capability for DC bias voltages
// 1.25, August 2, 2018
//      1.) Added the user definable control panels
// 1.26, Sept 9,2018
//      1.) Added documentation and updated help
//      2.) Corrected PSG label errors and make the PSE box modal
//      3.) Added Java Scripting
//
#include "mips.h"
#include "ui_mips.h"
#include "console.h"
#include "settingsdialog.h"
#include "pse.h"
#include "ringbuffer.h"
#include "Comms.h"
#include "Twave.h"
#include "DCbias.h"
#include "DIO.h"
#include "RFdriver.h"
#include "PSG.h"
#include "Program.h"
#include "Help.h"
#include "ARB.h"
#include "FAIMS.h"
#include "Filament.h"
#include "singlefunnel.h"
#include "softlanding.h"
#include "softlanding2.h"
#include "grid.h"
#include "arbwaveformedit.h"
#include "adc.h"
#include "controlpanel.h"
#include "scriptingconsole.h"

#include <QMessageBox>
#include <QtSerialPort/QSerialPort>
#include <QTime>
#include <QThread>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QCursor>
#include <QDebug>
#include <QFileDialog>
#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QtNetwork/QTcpSocket>
#include <QInputDialog>

//RingBuffer rb;

MIPS::MIPS(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MIPS)
{
    ui->setupUi(this);
    ui->tabMIPS->setElideMode(Qt::ElideNone);
    ui->tabMIPS->setUsesScrollButtons(true);
    // Make the dialog fixed size.
    this->setFixedSize(this->size());

    ui->comboSystems->setVisible(false);
    ui->lblSystems->setVisible(false);

    #if defined(Q_OS_MAC)
    QFont font = ui->lblMIPSconnectionNotes->font();
    font.setPointSize(15);
    ui->lblMIPSconnectionNotes->setFont(font);
    #endif

    scriptconsole = NULL;
    qApp->installEventFilter(this);
    sf = NULL;
    sf_deleteRequest = false;
    sl = NULL;
    sl_deleteRequest = false;
    sl2 = NULL;
    sl2_deleteRequest = false;
    grid = NULL;
    grid_deleteRequest = false;
    cp = NULL;
    cp_deleteRequest = false;
    appPath = QApplication::applicationDirPath();
    pollTimer = new QTimer;
    settings = new SettingsDialog;
    comms  = new Comms(settings,"",ui->statusBar);
    console = new Console(ui->Terminal,ui,comms);
    console->setEnabled(false);
    twave  = new Twave(ui,comms);
    dcbias = new DCbias(ui,comms);
    dio = new DIO(ui,comms);
    rfdriver = new RFdriver(ui,comms);
    SeqGen = new PSG(ui,comms);
    pgm = new Program(ui, comms, console);
    help = new Help();
    arb = new ARB(ui, comms);
    faims = new FAIMS(ui, comms);
    filament = new Filament(ui, comms);
    adc = new ADC(ui, comms);

    RepeatMessage = "";
    ui->actionClear->setEnabled(true);
    ui->actionOpen->setEnabled(true);
    ui->actionSave->setEnabled(true);
    ui->actionHelp->setEnabled(true);
    ui->actionMIPS_commands->setEnabled(true);
    ui->actionProgram_MIPS->setEnabled(true);
    ui->actionSave_current_MIPS_firmware->setEnabled(true);
    ui->actionSet_bootloader_boot_flag->setEnabled(true);
    ui->actionSingle_Funnel->setEnabled(true);
    ui->actionSoft_landing->setEnabled(true);
    ui->actionSoft_landing_Purdue->setEnabled(true);
    ui->actionGrid->setEnabled(true);
    ui->actionMessage_repeat->setEnabled(true);
    ui->actionGet_file_from_MIPS->setEnabled(true);
    ui->actionSend_file_to_MIPS->setEnabled(true);
    ui->actionRead_EEPROM->setEnabled(true);
    ui->actionWrite_EEPROM->setEnabled(true);
    ui->actionRead_ARB_FLASH->setEnabled(true);
    ui->actionWrite_ARB_FLASH->setEnabled(true);
    ui->actionARB_upload->setEnabled(true);
    ui->actionSelect->setEnabled(true);
    ui->menuTerminal->setEnabled(false);
    ui->actionScripting->setEnabled(true);

    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(loadSettings()));
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveSettings()));
    connect(ui->pbConfigure, SIGNAL(pressed()), settings, SLOT(show()));
    connect(ui->actionMIPS_commands, SIGNAL(triggered()), this, SLOT(MIPScommands()));
    connect(ui->actionHelp, SIGNAL(triggered()), this, SLOT(GeneralHelp()));
    connect(ui->tabMIPS,SIGNAL(currentChanged(int)),this,SLOT(tabSelected()));
    connect(ui->pbConnect,SIGNAL(pressed()),this,SLOT(MIPSconnect()));
    connect(ui->pbSearchandConnect,SIGNAL(pressed()),this,SLOT(MIPSsearch()));
    connect(ui->pbDisconnect,SIGNAL(pressed()),this,SLOT(MIPSdisconnect()));
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(pollLoop()));
    connect(ui->actionAbout,SIGNAL(triggered(bool)), this, SLOT(DisplayAboutMessage()));
    connect(ui->actionClear, SIGNAL(triggered()), this, SLOT(clearConsole()));
    connect(ui->actionSingle_Funnel, SIGNAL(triggered()), this, SLOT(Single_Funnel()));
    connect(ui->actionSoft_landing, SIGNAL(triggered()), this, SLOT(Soft_Landing()));
    connect(ui->actionSoft_landing_Purdue, SIGNAL(triggered()), this, SLOT(Soft_Landing_Purdue()));
    connect(ui->actionGrid, SIGNAL(triggered()), this, SLOT(Grid_system()));
    connect(ui->actionMessage_repeat, SIGNAL(triggered()), this, SLOT(GetRepeatMessage()));
    connect(ui->actionGet_file_from_MIPS, SIGNAL(triggered()), this, SLOT(GetFileFromMIPS()));
    connect(ui->actionSend_file_to_MIPS, SIGNAL(triggered()), this, SLOT(PutFiletoMIPS()));
    connect(ui->actionRead_EEPROM, SIGNAL(triggered()), this, SLOT(ReadEEPROM()));
    connect(ui->actionWrite_EEPROM, SIGNAL(triggered()), this, SLOT(WriteEEPROM()));
    connect(ui->actionRead_ARB_FLASH, SIGNAL(triggered()), this, SLOT(ReadARBFLASH()));
    connect(ui->actionWrite_ARB_FLASH, SIGNAL(triggered()), this, SLOT(WriteARBFLASH()));
    connect(ui->actionARB_upload, SIGNAL(triggered()), this, SLOT(ARBupload()));
    connect(ui->actionSelect, SIGNAL(triggered()), this, SLOT(SelectCP()));
    connect(ui->actionScripting, SIGNAL(triggered()), this, SLOT(slotScripting()));

    ui->comboMIPSnetNames->installEventFilter(new DeleteHighlightedItemWhenShiftDelPressedEventFilter);
    // Sets the polling loop interval and starts the timer
    pollTimer->start(1000);
}

MIPS::~MIPS()
{
//    if(sl != NULL) delete sl;
//    if(sf != NULL) delete sf;
//    delete settings;
//    delete ui;
}

void MIPS::closeEvent(QCloseEvent *event)
{
    if(sl != NULL) delete sl;
    if(sl2 != NULL) delete sl2;
    if(sf != NULL) delete sf;
    if(grid != NULL) delete grid;
    if(cp != NULL) delete cp;
    delete settings;
    delete help;
    delete ui;
}

bool MIPS::eventFilter(QObject *obj, QEvent *event)
{
    if(SelectedTab == "DCbias") return dcbias->myEventFilter(obj, event);
    if(SelectedTab == "Twave") return twave->myEventFilter(obj, event);
  return QObject::eventFilter(obj, event);
}

bool DeleteHighlightedItemWhenShiftDelPressedEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        //qDebug() << keyEvent->key() << Qt::Key::Key_Delete <<Qt::Key::Key_Backspace;
        if ((keyEvent->key() == Qt::Key::Key_Backspace) && (keyEvent->modifiers() == Qt::ShiftModifier))
        {
            //qDebug() << "filter";
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

void MIPS::MIPScommands(void)
{
    help->SetTitle("MIPS Commands");
    help->LoadHelpText(":/MIPScommands.txt");
    help->show();
}

// Removes a tab by its name
void MIPS::RemoveTab(QString TabName)
{
   for(int i = 0; i < ui->tabMIPS->count(); i++)
   {
       if( ui->tabMIPS->tabText(i) == TabName)
       {
           ui->tabMIPS->removeTab(i);
           return;
       }
   }
}

void MIPS::AddTab(QString TabName)
{
    // If tab is present then do nothing and exit
    for(int i = 0; i < ui->tabMIPS->count(); i++)
    {
        if( ui->tabMIPS->tabText(i) == TabName)
        {
            return;
        }
    }
    if(TabName == "DCbias") ui->tabMIPS->insertTab(3,ui->DCbias, "DCbias");
    if(TabName == "RFdriver") ui->tabMIPS->insertTab(3,ui->RFdriver, "RFdriver");
    if(TabName == "Twave") ui->tabMIPS->insertTab(3,ui->Twave, "Twave");
    if(TabName == "ARB") ui->tabMIPS->insertTab(3,ui->ARB, "ARB");
    if(TabName == "FAIMS") ui->tabMIPS->insertTab(3,ui->FAIMS, "FAIMS");
    if(TabName == "Filament") ui->tabMIPS->insertTab(3,ui->Filament, "Filament");
}

void MIPS::GeneralHelp(void)
{
    help->SetTitle("MIPS Help");
    help->LoadHelpText(":/MIPShelp.txt");
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "System")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Digital IO")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "DCbias")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "RFdriver")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "FAIMS")
    {
        help->SetTitle("FAIMS help");
        help->LoadHelpText(":/FAIMShelp.txt");
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Pulse Sequence Generation")
    {
        help->SetTitle("Pulse Sequence Generation Help");
        help->LoadHelpText(":/PSGhelp.txt");
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "ARB")
    {
        help->SetTitle("ARB Help");
        help->LoadHelpText(":/ARBhelp.txt");
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Twave")
    {
        help->SetTitle("Twave Help");
        help->LoadHelpText(":/TWAVEhelp.txt");
    }
    if(sl != NULL)
    {
        help->SetTitle("Soft Landing Help");
        help->LoadHelpText(":/SoftLandingHelp.txt");
    }
    if(sl2 != NULL)
    {
        help->SetTitle("Soft Landing Help");
        help->LoadHelpText(":/SoftLandingHelp2.txt");
    }
    if(sf != NULL)
    {
        help->SetTitle("Single Funnel Help");
        help->LoadHelpText(":/SingleFunnelHelp.txt");
    }
    help->show();
    return;
}

void MIPS::DisplayAboutMessage(void)
{
    QMessageBox::information(
        this,
        tr("Application Named"),
        tr("MIPS interface application, written by Gordon Anderson. This application allows communications with the MIPS system supporting monitoring and control as well as pulse sequence generation.") );
}

void MIPS::loadSettings(void)
{
    if(sl != NULL)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        sl->Load(fileName);
        return;
    }
    if(sl2 != NULL)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        sl2->Load(fileName);
        return;
    }
    if(sf != NULL)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        sf->Load(fileName);
        return;
    }
    if(grid != NULL)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        grid->Load(fileName);
        return;
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "System")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Data from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        console->Load(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Digital IO")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        dio->Load(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "DCbias")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        dcbias->Load(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "RFdriver")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        rfdriver->Load(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "FAIMS")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        faims->Load(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Pulse Sequence Generation")
    {
        SeqGen->Load();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Twave")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        twave->Load(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "ARB")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        arb->Load(fileName);
    }
    return;
}

void MIPS::saveSettings(void)
{
    if(sl != NULL)
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        sl->Save(fileName);
        return;
    }
    if(sl2 != NULL)
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        sl2->Save(fileName);
        return;
    }
    if(sf != NULL)
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        sf->Save(fileName);
        return;
    }
    if(grid != NULL)
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        grid->Save(fileName);
        return;
    }
    if(cp != NULL)
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        cp->Save(fileName);
        return;
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "System")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Data File"),"",tr("Settings (*.settings);;All files (*.*)"));
        console->Save(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Digital IO")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        dio->Save(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "DCbias")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        dcbias->Save(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "RFdriver")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        rfdriver->Save(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "FAIMS")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        faims->Save(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Pulse Sequence Generation")
    {
        SeqGen->Save();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Twave")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        twave->Save(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "ARB")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        arb->Save(fileName);
    }
    return;
}

void MIPS::pollLoop(void)
{
    QString res ="";
    //char c;

    if(scriptconsole!=NULL) scriptconsole->UpdateStatus();
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Pulse Sequence Generation")
    {
        /*
        while(true)
        {
            c = rb.getch();
            if((int)c==0) return;
            ui->statusBar->showMessage(ui->statusBar->currentMessage() + c,2000);
        }
        */
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
    {
        if(RepeatMessage != "") comms->SendString(RepeatMessage.toStdString().c_str());
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "DCbias")
    {
         if(ui->chkDCBautoUpdate->isChecked())  dcbias->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "RFdriver")
    {
        //UpdateRFdriver();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Filament")
    {
        if(comms->isConnected()) filament->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "ARB")
    {
        if(comms->isConnected()) if(ui->chkARBautoUpdate->isChecked()) arb->Update();
    }
    if(comms->isConnected()) faims->PollLoop();
    if(sf_deleteRequest)
    {
      delete sf;
      sf = NULL;
      sf_deleteRequest = false;
    }
    if(sf != NULL) if(comms->isConnected()) sf->Update();

    if(sl_deleteRequest)
    {
      delete sl;
      sl = NULL;
      sl_deleteRequest = false;
    }
    if(sl != NULL) if(comms->isConnected()) sl->Update();

    if(sl2_deleteRequest)
    {
      delete sl2;
      sl2 = NULL;
      sl2_deleteRequest = false;
    }
    if(sl2 != NULL) if(comms->isConnected()) sl2->Update();

    if(grid_deleteRequest)
    {
      delete grid;
      grid = NULL;
      grid_deleteRequest = false;
    }
    if(grid != NULL) if(comms->isConnected()) grid->Update();

    if(cp_deleteRequest)
    {
      delete cp;
      cp = NULL;
      cp_deleteRequest = false;
    }
    //if(cp != NULL) if(comms->isConnected()) cp->Update();
    if(cp != NULL) cp->Update();
 }

void MIPS::mousePressEvent(QMouseEvent * event)
{
    QMainWindow::mousePressEvent(event);
}

void MIPS::resizeEvent(QResizeEvent* event)
{

   ui->tabMIPS->setFixedWidth(MIPS::width());
   #if defined(Q_OS_MAC)
    ui->tabMIPS->setFixedHeight(MIPS::height()-(ui->statusBar->height()));
   #else
    // Not sure why I need this 3x for a windows system??
    ui->tabMIPS->setFixedHeight(MIPS::height()-(ui->statusBar->height()*3));
   #endif

   console->resize(ui->Terminal);
   QMainWindow::resizeEvent(event);
}

void MIPS::MIPSsetup(void)
{
    QString res;

    pgm->comms = comms;
    ui->lblMIPSconfig->setText("MIPS: ");
    // Turn ECHO off
    comms->SendString("\n");
    comms->SendString("ECHO,FALSE\n");
    res = comms->SendMess("GVER\n");
    if(res=="") return;  // Exit if we timeout, no MIPS comms
    ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + res);
    ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\nModules present:");

    res = comms->SendMess("GCHAN,RF\n");
    if(res=="") return;
    if(res.contains("2")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   1 RF driver\n");
    if(res.contains("4")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   2 RF drivers\n");
    rfdriver->SetNumberOfChannels(res.toInt());
    if(res.contains("0")) RemoveTab("RFdriver");
    else AddTab("RFdriver");

    res = comms->SendMess("GCHAN,DCB\n");
    if(res=="") return;
    if(res.contains("8")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   1 DC bias (8 output channels)\n");
    if(res.contains("16")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   2 DC bias (16 output channels)\n");
    if(res.contains("24")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   3 DC bias (16 output channels)\n");
    if(res.contains("32")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   4 DC bias (16 output channels)\n");
    dcbias->SetNumberOfChannels(res.toInt());
    if(res.contains("0")) RemoveTab("DCbias");
    else AddTab("DCbias");

    res = comms->SendMess("GCHAN,TWAVE\n");
    if(res=="") return;
    if(res.contains("1")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   1 TWAVE module\n");
    if(res.contains("2")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   2 TWAVE modules\n");
    if(res.contains("0")) RemoveTab("Twave");
    else AddTab("Twave");

    res = comms->SendMess("GCHAN,ARB\n");
    if(res=="") return;
    if(res.contains("8")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() +  "\n   1 ARB module\n");
    if(res.contains("16")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   2 ARB modules\n");
    if(res.contains("24")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   3 ARB modules\n");
    if(res.contains("32")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   4 ARB modules\n");
    if(res.contains("0")) RemoveTab("ARB");
    else AddTab("ARB");
    arb->SetNumberOfChannels(res.toInt());

    res = comms->SendMess("GCHAN,FAIMS\n");
    if(res=="") return;
    if(res.contains("1")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   FAIMS module\n");
    if(res.contains("0")) RemoveTab("FAIMS");
    else AddTab("FAIMS");

    res = comms->SendMess("GCHAN,ESI\n");
    if(res=="") return;
    if(res.contains("1")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   1 ESI module, rev 3\n");
    if(res.contains("2")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   1 ESI module\n");
    if(res.contains("4")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   2 ESI modules\n");

    res = comms->SendMess("GCHAN,FIL\n");
    if(res=="") return;
    if(res.contains("1")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   1 Filament module\n");
    if(res.contains("2")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   2 Filament modules\n");
    if(res.contains("0")) RemoveTab("Filament");
    else AddTab("Filament");
}

// Here when the Connect push button is pressed. This function makes a connection with MIPS.
// If a network name or IP is provided that connection is tried first.
// If the serial port of socket is connected then this function exits.
void MIPS::MIPSconnect(void)
{
    comms->p = settings->settings();
    comms->host = ui->comboMIPSnetNames->currentText();
    if(comms->ConnectToMIPS())
    {
       console->setEnabled(true);
       console->setLocalEchoEnabled(settings->settings().localEchoEnabled);
       ui->lblMIPSconnectionNotes->setHidden(true);
       MIPSsetup();
    }
}

void MIPS::MIPSsearch(void)
{
    QMessageBox *msg = new QMessageBox();
    msg->setText("Searching for MIPS system...");
    msg->setStandardButtons(0);
    msg->show();
    QApplication::processEvents();
    QApplication::processEvents();
    QApplication::processEvents();

    settings->fillPortsParameters();
    settings->fillPortsInfo();
    FindMIPSandConnect();
    msg->hide();
}

void delay()
{
    QTime dieTime= QTime::currentTime().addSecs(1);
    while (QTime::currentTime() < dieTime)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void MIPS::FindAllMIPSsystems(void)
{
    Comms *cp;

    disconnect(ui->comboSystems, SIGNAL(currentIndexChanged(int)), 0, 0);
    Systems.clear();
    // If there are a defined list of net names or IP addresses
    // then use them and do not search for USB connected systems
    if(ui->comboMIPSnetNames->count() > 0)
    {
        for(int j=0;j<ui->comboMIPSnetNames->count();j++)
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            cp = new Comms(settings,"",ui->statusBar);
            cp->host = ui->comboMIPSnetNames->itemText(j);
            if(cp->isMIPS(settings->getPortName(j)))
            if(cp->ConnectToMIPS())
            {
               Systems << (cp);
               cp = new Comms(settings,"",ui->statusBar);
            }
        }
    }
    else
    {
        int i = settings->numberOfPorts();
        settings->fillPortsInfo();
        for(int j=0;j<i;j++)
        {
          ui->statusBar->showMessage("Trying: " + settings->getPortName(j));
          QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
          cp = new Comms(settings,"",ui->statusBar);
          if(cp->isMIPS(settings->getPortName(j)))
          {
            delay();
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            cp->host = "";
            cp->SendString("ECHO,FALSE\n");
            if(cp->ConnectToMIPS())
            {
               Systems << (cp);
               cp = new Comms(settings,"",ui->statusBar);
            }
          }
        }
    }
    ui->comboSystems->clear();
    for(int j=0;j<Systems.count();j++)
    {
        ui->comboSystems->addItem(Systems.at(j)->MIPSname);
    }
    if(ui->comboSystems->count() > 1)
    {
        ui->comboSystems->setVisible(true);
        ui->lblSystems->setVisible(true);
    }
}

void MIPS::FindMIPSandConnect(void)
{
  ui->pbSearchandConnect->setDown(false);
  QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
  FindAllMIPSsystems();
  if(ui->comboSystems->count() > 0)
  {
      delay();
      comms = Systems.at(0);
      console->setEnabled(true);
      console->setLocalEchoEnabled(settings->settings().localEchoEnabled);
      ui->lblMIPSconnectionNotes->setHidden(true);
      MIPSsetup();
      connect(ui->comboSystems, SIGNAL(currentIndexChanged(int)), this, SLOT(UpdateSystem()));
  }
  else ui->statusBar->showMessage(tr("Can't find MIPS system!"));
  return;

  int i = settings->numberOfPorts();
  for(int j=0;j<i;j++)
  {
    if(comms->isMIPS(settings->getPortName(j)))
    {
      delay();
      comms->host = ui->comboSystems->currentText();
      if(comms->ConnectToMIPS())
      {
        console->setEnabled(true);
        console->setLocalEchoEnabled(settings->settings().localEchoEnabled);
        ui->lblMIPSconnectionNotes->setHidden(true);
        MIPSsetup();
      }
      return;
    }
  }
  ui->statusBar->showMessage(tr("Can't find MIPS system!"));
}

void MIPS::MIPSdisconnect(void)
{
    comms->DisconnectFromMIPS();
    for(int j=0;j<Systems.count();j++)
    {
        Systems.at(j)->DisconnectFromMIPS();
    }
//    ui->comboSystems->clear();
    ui->comboSystems->setVisible(false);
    ui->lblSystems->setVisible(false);

    ui->lblMIPSconfig->setText("");
    ui->lblMIPSconnectionNotes->setHidden(false);
//    qApp->quit();
//    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
}

void MIPS::UpdateSystem(void)
{
   if(Systems.count() == 0) return;
   comms = Systems.at(ui->comboSystems->currentIndex());
   MIPSsetup();
}

void MIPS::tabSelected()
{
    static QString LastTab = "System";

    disconnect(comms, SIGNAL(DataReady()),0,0);
    disconnect(console, SIGNAL(getData(QByteArray)),0,0);
    ui->menuTerminal->setEnabled(false);
    if(LastTab == "Pulse Sequence Generation")
    {
        // Abort when exiting this tab
        comms->SendString("TBLABRT\n");
        delay();
        comms->rb.clear();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "System")
    {
        LastTab = "System";
        settings->fillPortsInfo();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "ADC")
    {
        LastTab = "ADC";
        adc->Update(true);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
    {
        ui->menuTerminal->setEnabled(true);
        LastTab = "Terminal";
        connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
        connect(console, SIGNAL(getData(QByteArray)), this, SLOT(writeData(QByteArray)));
        console->resize(ui->Terminal);
        console->setEnabled(true);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Digital IO")
    {
        ui->chkRemoteNav->setChecked(false);
        dio->comms = comms;
        LastTab = "Digital IO";
        dio->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "DCbias")
    {
        dcbias->comms = comms;
        LastTab = "DCbias";
        dcbias->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "RFdriver")
    {
        rfdriver->comms = comms;
        LastTab = "RFdriver";
        rfdriver->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Pulse Sequence Generation")
    {
        SeqGen->comms = comms;
        LastTab = "Pulse Sequence Generation";
        SeqGen->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Twave")
    {
        twave->comms = comms;
        LastTab = "Twave";
        twave->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "ARB")
    {
        arb->comms = comms;
        LastTab = "ARB";
        arb->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "FAIMS")
    {
        faims->comms = comms;
        LastTab = "FAIMS";
        faims->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Filament")
    {
        filament->comms = comms;
        LastTab = "Filament";
        filament->Update();
    }
    SelectedTab = LastTab;
}

void MIPS::writeData(const QByteArray &data)
{
    comms->writeData(data);
}

void MIPS::readData2Console(void)
{
    QByteArray data;

    data = comms->readall();
    console->putData(data);
}

void MIPS::CloseSingleFunnel(void)
{
   sf_deleteRequest = true;
}

void MIPS::Single_Funnel(void)
{
   if(sf != NULL) return;
   if(sl != NULL) return;
   if(sl2 != NULL) return;
   if(grid != NULL) return;
   if(cp != NULL) return;
   ui->tabMIPS->setCurrentIndex(0);
   sf = new SingleFunnel(0,comms,ui->statusBar);
   if(!sf->ConfigurationCheck())
   {
       delete sf;
       sf = NULL;
       return;
   }
   sf->show();
   connect(sf, SIGNAL(DialogClosed()), this, SLOT(CloseSingleFunnel()));
   sf->Update();
}

void MIPS::CloseSoftLanding(void)
{
   sl_deleteRequest = true;
}

void MIPS::Soft_Landing(void)
{
    if(sf != NULL) return;
    if(sl != NULL) return;
    if(sl2 != NULL) return;
    if(grid != NULL) return;
    if(cp != NULL) return;
    ui->tabMIPS->setCurrentIndex(0);
    sl = new SoftLanding(0,comms,ui->statusBar);
    if(!sl->ConfigurationCheck())
    {
        delete sl;
        sl = NULL;
        return;
    }
    sl->show();
    connect(sl, SIGNAL(DialogClosed()), this, SLOT(CloseSoftLanding()));
    sl->Update();
}

void MIPS::CloseSoftLanding2(void)
{
   sl2_deleteRequest = true;
}

void MIPS::Soft_Landing_Purdue(void)
{
    if(sf != NULL) return;
    if(sl != NULL) return;
    if(sl2 != NULL) return;
    if(grid != NULL) return;
    if(cp != NULL) return;
    ui->tabMIPS->setCurrentIndex(0);
    sl2 = new SoftLanding2(0,comms,ui->statusBar);
    if(!sl2->ConfigurationCheck())
    {
        delete sl2;
        sl2 = NULL;
        return;
    }
    sl2->show();
    connect(sl2, SIGNAL(DialogClosed()), this, SLOT(CloseSoftLanding2()));
    sl2->Update();
}

void MIPS::CloseGridSystem(void)
{
   grid_deleteRequest = true;
}

void MIPS::Grid_system(void)
{
    if(sf != NULL) return;
    if(sl != NULL) return;
    if(sl2 != NULL) return;
    if(grid != NULL) return;
    if(cp != NULL) return;
    ui->tabMIPS->setCurrentIndex(0);
    grid = new Grid(0,comms,ui->statusBar);
    grid->show();
    connect(grid, SIGNAL(DialogClosed()), this, SLOT(CloseGridSystem()));
    grid->Update();
}

void MIPS::setWidgets(QWidget* old, QWidget* now)
{
}

void MIPS::clearConsole(void)
{
    console->clear();
}

void MIPS::GetRepeatMessage(void)
{
    bool ok;

    QString text = QInputDialog::getText(0, "MIPS", "Enter Command to send every sec:", QLineEdit::Normal,"", &ok );
    if ( ok && !text.isEmpty() )
    {
        RepeatMessage =  text + "\n";
    } else
    {
        RepeatMessage = "";
    }
}

void MIPS::GetFileFromMIPS(void)
{
    bool ok;

    // Get file name from user
    QString text = QInputDialog::getText(0, "MIPS", "File name to copy from MIPS SD card:", QLineEdit::Normal,"", &ok );
    if ( ok && !text.isEmpty() )
    {
        // Let user pick detination location and file name
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to File"),"",tr("All files (*.*)"));
        if(fileName != "")
        {
            // If on terminal tab then turn off the slot for received chars
            if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal") disconnect(comms, SIGNAL(DataReady()),0,0);
            comms->GetMIPSfile(text,fileName);
            if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
            {
                connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
                readData2Console();
            }
        }
     }
}

void MIPS::PutFiletoMIPS(void)
{
    bool ok;

    // Let user pick source location and file name
    QString fileName = QFileDialog::getOpenFileName(this, tr("File to MIPS"),tr("All files (*)", 0));
    if(fileName == "") return;
    // If on terminal tab then turn off the slot for received chars
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal") disconnect(comms, SIGNAL(DataReady()),0,0);
    // Get file name from user
    QString text = QInputDialog::getText(0, "MIPS", "File name to copy to MIPS SD card:", QLineEdit::Normal,"", &ok );
    if ( ok && !text.isEmpty() )
    {
        comms->PutMIPSfile(text,fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
    {
        connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
        readData2Console();
    }
}

void MIPS::ReadEEPROM(void)
{
    bool ok;

    QMessageBox msgbox;
    msgbox.setText("MIPS Module EEPROM read function");
    msgbox.setInformativeText("Select the module A or B:");
    msgbox.addButton(tr("B"), QMessageBox::NoRole);
    msgbox.addButton(tr("A"), QMessageBox::NoRole);
    msgbox.addButton(tr("Cancel"), QMessageBox::NoRole);
    int brd = msgbox.exec();
    if(brd == 2) return;
    QString Address = QInputDialog::getText(this, "Read MIPS EEPROM", "Enter EEPROM address in hex:", QLineEdit::Normal,QString::null, &ok);
    if(ok && !Address.isEmpty())
    {
        // Here with the EEPROM module address and physical address
        // Selet a file to save the EEPROM contents
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to File"),"",tr("All files (*.*)"));
        if(fileName != "")
        {
            // Read the data from MIPS
            if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal") disconnect(comms, SIGNAL(DataReady()),0,0);
            if(brd == 1) comms->GetEEPROM(fileName,"A",Address.toInt(0,16));
            else comms->GetEEPROM(fileName,"B",Address.toInt(0,16));
            if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
            {
                connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
                readData2Console();
            }
        }
    }
}

void MIPS::WriteEEPROM(void)
{
    bool ok;

    QMessageBox msgbox;
    msgbox.setText("MIPS Module EEPROM write function. This function will allow you to reprogram the configuration memory of the selected module. Proceed with caution, you can render your system inoperable by entering invalid information.");
    msgbox.setInformativeText("Select the module A or B or cancel:");
    msgbox.addButton(tr("B"), QMessageBox::NoRole);
    msgbox.addButton(tr("A"), QMessageBox::NoRole);
    msgbox.addButton(tr("Cancel"), QMessageBox::NoRole);
    int brd = msgbox.exec();
    if(brd == 2) return;
    QString Address = QInputDialog::getText(this, "Write MIPS EEPROM", "Enter EEPROM address in hex:", QLineEdit::Normal,QString::null, &ok);
    if(ok && !Address.isEmpty())
    {
        // Here with the EEPROM module address and physical address
        // Selet a file to write to the EEPROM
        QString fileName = QFileDialog::getOpenFileName(this, tr("File to MIPS"),tr("All files (*)", 0));
        if(fileName != "")
        {
            // Read the data from MIPS
            if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal") disconnect(comms, SIGNAL(DataReady()),0,0);
            if(brd == 1) comms->PutEEPROM(fileName,"A",Address.toInt(0,16));
            else comms->PutEEPROM(fileName,"B",Address.toInt(0,16));
            if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
            {
                connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
                readData2Console();
            }
        }
    }
}

void MIPS::ReadARBFLASH(void)
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save to File"),"",tr("All files (*.*)"));
    if(fileName != "")
    {
        // Read the data from MIPS
        if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal") disconnect(comms, SIGNAL(DataReady()),0,0);
        comms->GetARBFLASH(fileName);
        if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
        {
            connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
            readData2Console();
        }
    }
}

void MIPS::WriteARBFLASH(void)
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("File to MIPS"),tr("All files (*)", 0));
    if(fileName != "")
    {
        // Read the data from MIPS
        if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal") disconnect(comms, SIGNAL(DataReady()),0,0);
        comms->PutARBFLASH(fileName);
        if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
        {
            connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
            readData2Console();
        }
    }
}

void MIPS::ARBupload(void)
{
    bool ok;

    QString Faddress = QInputDialog::getText(0, "ARB upload", "ARB Module FLASH write function. This function will allow\nyou to upload a file and place it in FLASH at the address\nyou select. Proceed with caution, you can render your\nsystem inoperable by entering invalid information.\n\nEnter FLASH address in hex or cancel:\nmover.bin at c0000\narb.bin at d0000", QLineEdit::Normal,"", &ok );
    if(ok)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("File to upload to ARB FLASH"),tr("All files (*)", 0));
        if(fileName != "")
        {
            if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal") disconnect(comms, SIGNAL(DataReady()),0,0);
            comms->ARBupload(Faddress, fileName);
            if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
            {
                connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
                readData2Console();
            }
        }
    }
}

void MIPS::CloseControlPanel(void)
{
    cp_deleteRequest = true;
    this->setWindowState(Qt::WindowMaximized);
}

void MIPS::SelectCP(void)
{
    if(sf   != NULL) return;
    if(sl   != NULL) return;
    if(sl2  != NULL) return;
    if(grid != NULL) return;
    if(cp   != NULL) return;
    ui->tabMIPS->setCurrentIndex(0);
    ControlPanel *c = new ControlPanel(0,Systems);
    if(!c->LoadedConfig) return;
    c->show();
    connect(c, SIGNAL(DialogClosed()), this, SLOT(CloseControlPanel()));
    c->Update();
    this->setWindowState(Qt::WindowMinimized);
    cp = c;
}

void MIPS::slotScripting(void)
{
    if(!scriptconsole) scriptconsole = new ScriptingConsole(this);
    scriptconsole->show();
}

// Returns a pointer to the comm port for the MIPS system defined my its name.
// Returns null if the MIPS system can't be found.
Comms* MIPS::FindCommPort(QString name, QList<Comms*> Systems)
{
   for(int i = 0; i < Systems.length(); i++)
   {
       if(Systems.at(i)->MIPSname == name) return(Systems.at(i));
   }
   return NULL;
}



bool MIPS::SendCommand(QString message)
{
   return comms->SendCommand(message);
}

QString MIPS::SendMess(QString message)
{
   return comms->SendMess(message);
}

bool MIPS::SendCommand(QString MIPSname, QString message)
{
    QApplication::processEvents();
    Comms *cp =  FindCommPort(MIPSname,Systems);
    if(cp==NULL) return false;
    return cp->SendCommand(message);
}

QString MIPS::SendMess(QString MIPSname, QString message)
{
    QApplication::processEvents();
    Comms *cp =  FindCommPort(MIPSname,Systems);
    if(cp==NULL) return "MIPS not found!";
    return cp->SendMess(message);
}

void MIPS::msDelay(int ms)
{
    QTime timer;

    timer.start();
    while(timer.elapsed() < ms) QApplication::processEvents();
}

void MIPS::statusMessage(QString message)
{
    QApplication::processEvents();
    ui->statusBar->showMessage(message);
    QApplication::processEvents();
}

void MIPS::popupMessage(QString message)
{
    QMessageBox msgBox;

    QApplication::processEvents();
    msgBox.setText(message);
    msgBox.setInformativeText("");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

bool MIPS::popupYesNoMessage(QString message)
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

QString MIPS::popupUserInput(QString title, QString message)
{
    bool ok;

    QApplication::processEvents();
    QString text = QInputDialog::getText(this, title, message, QLineEdit::Normal,QString::null, &ok);
    if(!ok) text="";
    QApplication::processEvents();
    return text;
}
