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
//
//  To do list:
//  1.) Refactor the code, here are some to dos:
//      a.) Split psePoint out to its own file
//      b.) Add the build table code to the psePoint class
//      c.) Create a class to communicate with the MIPS system

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

RingBuffer rb;

MIPS::MIPS(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MIPS)
{
    ui->setupUi(this);
    // Make the dialog fixed size.
    this->setFixedSize(this->size());

//  MIPS::setProperty("font", QFont("Times New Roman", 16));
    #if defined(Q_OS_MAC)
    QFont font = ui->lblMIPSconnectionNotes->font();
    font.setPointSize(21);
    ui->lblMIPSconnectionNotes->setFont(font);
    #endif

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

    ui->actionClear->setEnabled(true);
    ui->actionOpen->setEnabled(true);
    ui->actionSave->setEnabled(true);
    ui->actionHelp->setEnabled(true);
    ui->actionMIPS_commands->setEnabled(true);
    ui->actionProgram_MIPS->setEnabled(true);
    ui->actionSave_current_MIPS_firmware->setEnabled(true);
    ui->actionSet_bootloader_boot_flag->setEnabled(true);

  //   connect(app, SIGNAL(focusChanged(QWidget*, QWidget*)), this, SLOT(setWidgets(QWidget*, QWidget*)));    connect(ui->actionClear, SIGNAL(triggered()), console, SLOT(clear()));
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(loadSettings()));
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveSettings()));
    connect(ui->pbConfigure, SIGNAL(pressed()), settings, SLOT(show()));
    connect(ui->actionMIPS_commands, SIGNAL(triggered()), this, SLOT(MIPScommands()));
    connect(ui->actionHelp, SIGNAL(triggered()), this, SLOT(GeneralHelp()));
    connect(ui->tabMIPS,SIGNAL(currentChanged(int)),this,SLOT(tabSelected()));
    connect(ui->pbConnect,SIGNAL(pressed()),this,SLOT(MIPSconnect()));
    connect(ui->pbDisconnect,SIGNAL(pressed()),this,SLOT(MIPSdisconnect()));
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(pollLoop()));
    connect(ui->actionAbout,SIGNAL(triggered(bool)), this, SLOT(DisplayAboutMessage()));
    connect(ui->actionClear, SIGNAL(triggered()), this, SLOT(clearConsole()));
    // Sets the polling loop interval and starts the timer
    pollTimer->start(500);
}

MIPS::~MIPS()
{
    delete settings;
    delete ui;
}

void MIPS::MIPScommands(void)
{
    help->SetTitle("MIPS Commands");
    help->LoadHelpText(":/MIPScommands.txt");
    help->show();
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
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Pulse Sequence Generation")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Twave")
    {
        help->SetTitle("Twave Help");
        help->LoadHelpText(":/TWAVEhelp.txt");
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
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "System")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Data from File"),"",tr("Files (*.settings)"));
        console->Load(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Digital IO")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Files (*.settings)"));
        dio->Load(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "DCbias")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Files (*.settings)"));
        dcbias->Load(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "RFdriver")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Files (*.settings)"));
        rfdriver->Load(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Pulse Sequence Generation")
    {
        SeqGen->Load();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Twave")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Files (*.settings)"));
        twave->Load(fileName);
    }
    return;
}

void MIPS::saveSettings(void)
{
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "System")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Data File"),"",tr("Files (*.settings)"));
        console->Save(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Digital IO")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Files (*.settings)"));
        dio->Save(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "DCbias")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Files (*.settings)"));
        dcbias->Save(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "RFdriver")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Files (*.settings)"));
        rfdriver->Save(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Pulse Sequence Generation")
    {
        SeqGen->Save();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Twave")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Files (*.settings)"));
        twave->Save(fileName);
    }
    return;
}

void MIPS::pollLoop(void)
{
    QString res ="";
    //char c;

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
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "DCbias")
    {
         //UpdateDCbias();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "RFdriver")
    {
        //UpdateRFdriver();
    }
}

void MIPS::mousePressEvent(QMouseEvent * event)
{
//    qDebug() << "event";
//    return;
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

    ui->lblMIPSconfig->setText("MIPS: ");
    res = comms->SendMessage("GVER\n");
    ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + res);
    ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\nModules present:");

    res = comms->SendMessage("GCHAN,RF\n");
    if(res.contains("2")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   1 RF driver\n");
    if(res.contains("4")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   2 RF drivers\n");
    rfdriver->SetNumberOfChannels(res.toInt());

    res = comms->SendMessage("GCHAN,DCB\n");
    if(res.contains("8")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   1 DC bias (8 output channels)\n");
    if(res.contains("16")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   2 DC bias (16 output channels)\n");
    dcbias->SetNumberOfChannels(res.toInt());

    res = comms->SendMessage("GCHAN,TWAVE\n");
    if(res.contains("1")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   TWAVE\n");

    res = comms->SendMessage("GCHAN,FAIMS\n");
    if(res.contains("1")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   FAIMS\n");

    res = comms->SendMessage("GCHAN,ESI\n");
    //serial->write("GCHAN,ESI\n");
    if(res.contains("2")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   1 ESI\n");
    if(res.contains("4")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   2 ESI\n");
}

// Here when the Connect push button is pressed. This function makes a connection with MIPS.
// If a network name or IP is provided that connection is tried first.
// If the serial port of socket is connected then this function exits.
void MIPS::MIPSconnect(void)
{
    comms->p = settings->settings();
    comms->host = ui->leMIPSnetName->text();
    if(comms->ConnectToMIPS())
    {
       console->setEnabled(true);
       console->setLocalEchoEnabled(settings->settings().localEchoEnabled);
       ui->lblMIPSconnectionNotes->setHidden(true);
       MIPSsetup();
    }
}

void MIPS::MIPSdisconnect(void)
{
    comms->DisconnectFromMIPS();
    ui->lblMIPSconfig->setText("");
    ui->lblMIPSconnectionNotes->setHidden(false);
}

void MIPS::tabSelected()
{
    disconnect(comms, SIGNAL(DataReady()),0,0);
    disconnect(console, SIGNAL(getData(QByteArray)),0,0);
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "System")
    {
        settings->fillPortsInfo();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
    {
        connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
        connect(console, SIGNAL(getData(QByteArray)), this, SLOT(writeData(QByteArray)));
        console->resize(ui->Terminal);
        console->setEnabled(true);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Digital IO")
    {
        dio->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "DCbias")
    {
        dcbias->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "RFdriver")
    {
        rfdriver->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Pulse Sequence Generation")
    {
        SeqGen->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Twave")
    {
        twave->Update();
    }
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

void MIPS::setWidgets(QWidget* old, QWidget* now)
{
}

void MIPS::clearConsole(void)
{
    console->clear();
}
