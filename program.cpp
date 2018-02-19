#include "program.h"

Program::Program(Ui::MIPS *w, Comms *c, Console *con)
{
    pui = w;
    comms = c;
    console = con;

    appPath = QApplication::applicationDirPath();
    connect(pui->actionProgram_MIPS, SIGNAL(triggered()), this, SLOT(programMIPS()));
    connect(pui->actionSave_current_MIPS_firmware, SIGNAL(triggered()), this, SLOT(saveMIPSfirmware()));
    connect(pui->actionSet_bootloader_boot_flag, SIGNAL(triggered()), this, SLOT(setBootloaderBootBit()));
}

// This function is called by the MIPS firmware save or write function. The cmd
// string passed contains the programmer invocation command.
void Program::executeProgrammerCommand(QString cmd)
{
    QString pcheck;

    // Select the Terminal tab and clear the display
    console->clear();
    // Make sure the app is connected to MIPS
    if(!comms->serial->isOpen())
    {
        console->putData("This application is not connected to MIPS!\n");
        return;
    }
    // Make sure we can locate the programmer tool...
    #if defined(Q_OS_MAC)
        pcheck = appPath + "/bossac";
    #else
        pcheck = appPath + "/bossac.exe";
    #endif
    QFileInfo checkFile(pcheck);
    if (!checkFile.exists() || !checkFile.isFile())
    {
        console->putData("Can't find the programmer!\n");
        console->putData(cmd.toStdString().c_str());
        return;
    }
    // Enable MIPS's bootloader
    console->putData("MIPS bootloader enabled!\n");
    // qDebug() << "Bootloader enabled";
    comms->closeSerialPort();
    QThread::sleep(1);
    while(comms->serial->isOpen()) QApplication::processEvents();
    QThread::sleep(1);
    comms->serial->setBaudRate(QSerialPort::Baud1200);
    QApplication::processEvents();
    comms->serial->open(QIODevice::ReadWrite);
    comms->serial->setDataTerminalReady(false);
    QThread::sleep(1);
    comms->serial->close();
    QApplication::processEvents();
    QThread::sleep(5);
    // Perform selected bootloader function and restart MIPS
    QApplication::processEvents();
    console->putData(cmd.toStdString().c_str());
    console->putData("\n");
    QApplication::processEvents();
    QStringList arguments;
    arguments << "-c";
    arguments << cmd;
//  arguments << " -h";
    #if defined(Q_OS_MAC)
        process.start("/bin/bash",arguments);
    #else
        process.start(cmd);
    #endif
    console->putData("Operation should start soon...\n");
    connect(&process,SIGNAL(readyReadStandardOutput()),this,SLOT(readProcessOutput()));
    connect(&process,SIGNAL(readyReadStandardError()),this,SLOT(readProcessOutput()));
}

void Program::setBootloaderBootBit(void)
{
    QString cmd;
    QString str;

    // Pop up a warning message and make sure the user wants to proceed
    QMessageBox msgBox;
    QString msg = "This function will attemp to set the bootloader boot flag in the MIPS system. ";
    msg += "This function is provided as part of an error recovery process and should not normally be necessary. ";
    msg += "If the boot flag is set on an erased DUE the results are unpredictable.\n";
    msgBox.setText(msg);
    msgBox.setInformativeText("Are you sure you want to contine?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if(ret == QMessageBox::No) return;
    // Select the Terminal tab and clear the display
    pui->tabMIPS->setCurrentIndex(1);
    // Make sure MIPS is in ready state
    msg = "Unplug any RF drive heads from MIPS before you proceed. This includes unplugging the FAIMS RF deck as well. ";
    msg += "It is assumed that you have already established communications with the MIPS system. ";
    msg += "If the connection is not establised this function will exit with no action.";
    msgBox.setText(msg);
    msgBox.setInformativeText("");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    // Set boot bit
    cmd = appPath + "/bossac -b -R";
    executeProgrammerCommand(cmd);
}

// This function is called from a menu selection to save the current MIPS firmware
// to a file.
void Program::saveMIPSfirmware(void)
{
    QString cmd;
    QString str;

    // Pop up a warning message and make sure the user wants to proceed
    QMessageBox msgBox;
    QString msg = "This will read the current MIPS firmware and save to a file. ";
    msg += "You should save to a file with the .bin extension and indicate the current version.\n";
    msgBox.setText(msg);
    msgBox.setInformativeText("Are you sure you want to contine?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if(ret == QMessageBox::No) return;
    // Select the Terminal tab and clear the display
    pui->tabMIPS->setCurrentIndex(1);
    // Select the binary file we are going to save MIPS firmware to
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save MIPS firmware .bin file"),"",tr("Files (*.bin *.*)"));
    if(fileName == "") return;
    // Make sure MIPS is in ready state
    msg = "Unplug any RF drive heads from MIPS before you proceed. This includes unplugging the FAIMS RF deck as well. ";
    msg += "It is assumed that you have already established communications with the MIPS system. ";
    msg += "If the connection is not establised this function will exit with no action.";
    msgBox.setText(msg);
    msgBox.setInformativeText("");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    // Save current MIPS firmware
    cmd = appPath + "/bossac -r -b " + fileName + " -R";
    executeProgrammerCommand(cmd);
}

// This function allows the user to download a new versoin of the MIPS firware.
void Program::programMIPS(void)
{
    QString cmd;
    QString str;

    // Pop up a warning message and make sure the user wants to proceed
    QMessageBox msgBox;
    QString msg = "This will erase the MIPS firmware and attemp to load a new version. ";
    msg += "Make sure you have a new MIPS binary file to load, it should have a .bin extension.\n";
    msg += "The MIPS firmware will be erased so if your bin file is invalid or fails to program, MIPS will be rendered useless!\n";
    msgBox.setText(msg);
    msgBox.setInformativeText("Are you sure you want to contine?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if(ret == QMessageBox::No) return;
    // Select the Terminal tab and clear the display
    pui->tabMIPS->setCurrentIndex(1);
    // Select the binary file we are going to load to MIPS
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load MIPS firmware .bin file"),"",tr("Files (*.bin *.*)"));
    if(fileName == "") return;
    // Make sure MIPS is in ready state
    msg = "Unplug any RF drive heads from MIPS before you proceed. This includes unplugging the FAIMS RF deck as well. ";
    msg += "It is assumed that you have already established communications with the MIPS system. ";
    msg += "If the connection is not establised this function will exit with no action.";
    msgBox.setText(msg);
    msgBox.setInformativeText("");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    // Program MIPS
    cmd = appPath + "/bossac -e -w -v -b " + fileName + " -R";
    executeProgrammerCommand(cmd);
}

void Program::readProcessOutput(void)
{
    console->putData(process.readAllStandardOutput());
    console->putData(process.readAllStandardError());
}
