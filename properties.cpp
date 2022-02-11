#include "properties.h"
#include "ui_properties.h"

Properties::Properties(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Properties)
{
    ui->setupUi(this);
    // Set the application path used to save the properties file
    HomePath = QDir::homePath();
    ApplicationPath = QApplication::applicationFilePath();

    // Set some defaults incase we can't load the properties file
    ui->leDataFilePath->setText(QDir::currentPath());
    ui->leMethodesPath->setText(QDir::currentPath());
    ui->leScriptPath->setText(QDir::currentPath());
    ui->comboTCPIPlist->clear();
    // Try to load the default properties file that should be located in the
    // application home dir. Properties.mips
    if(~Load(ApplicationPath + "/Properties.mips")) Load(HomePath + "/Properties.mips");
    UpdateVars();
    connect(ui->pbDataFilePath, SIGNAL(pressed()), this, SLOT(slotDataFilePath()));
    connect(ui->pbMethodesPath, SIGNAL(pressed()), this, SLOT(slotMethodesPath()));
    connect(ui->pbScriptPath,   SIGNAL(pressed()), this, SLOT(slotScriptPath()));
    connect(ui->pbLoadControlPanel, SIGNAL(pressed()), this, SLOT(slotLoadControlPanel()));
    connect(ui->pbClear, SIGNAL(pressed()), this, SLOT(slotClear()));
    connect(ui->pbOK, SIGNAL(pressed()), this, SLOT(slotOK()));
    connect(ui->pbLogFile, SIGNAL(pressed()), this, SLOT(slotLogFile()));
}

Properties::~Properties()
{
    delete ui;
}

void Properties::Log(QString Message)
{
    // Exit if message is empty
    if(Message.isEmpty()) return;
    if(Message == "") return;
    // Exit if log file name is empty
    if(LogFile == "") return;
    // Open file for append and add message
    QFile file(LogFile);
    if(file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        QTextStream stream(&file);
        stream << Message.replace("\n","") + "," + QDateTime::currentDateTime().toString() + "\n";
        file.close();
    }
}

void Properties::UpdateVars(void)
{
    DataFilePath = ui->leDataFilePath->text();
    MethodesPath = ui->leMethodesPath->text();
    ScriptPath   = ui->leScriptPath->text();
    LoadControlPanel = ui->leControlPanel->text();
    FileName = ui->leFileName->text();
    LogFile = ui->leLogFile->text();
    MinMIPS = ui->leMinMIPS->text().toInt();
    UpdateSecs = ui->leUpdateSecs->text().toFloat();
    AMPSbaud = ui->leAMPSbaud->text();
    if(ui->chkSearchAMPS->isChecked()) SearchAMPS = true;
    else SearchAMPS = false;
    if(ui->chkAutoConnect->isChecked()) AutoConnect = true;
    else AutoConnect = false;
    if(ui->chkAutoRestore->isChecked()) AutoRestore = true;
    else AutoRestore = false;
    if(ui->chkAutoFileName->isChecked()) AutoFileName = true;
    else AutoFileName = false;
    MIPS_TCPIP.clear();
    for(int i = 0;i< ui->comboTCPIPlist->count();i++) MIPS_TCPIP.append(ui->comboTCPIPlist->itemText(i));
}

void Properties::slotDataFilePath(void)
{
    ui->pbDataFilePath->setDown(false);
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select data file path"),
                                                 ui->leDataFilePath->text(),
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
    ui->leDataFilePath->setText(dir);
}

void Properties::slotMethodesPath(void)
{
    ui->pbMethodesPath->setDown(false);
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select methode file path"),
                                                 ui->leMethodesPath->text(),
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
    ui->leMethodesPath->setText(dir);
}

void Properties::slotScriptPath(void)
{
    ui->pbScriptPath->setDown(false);
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select script file path"),
                                                 ui->leScriptPath->text(),
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
    ui->leScriptPath->setText(dir);
}

void Properties::slotLoadControlPanel(void)
{
    ui->pbLoadControlPanel->setDown(false);
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Configuration from File"),"",tr("cfg (*.cfg);;All files (*.*)"));
    ui->leControlPanel->setText(fileName);

}

void Properties::slotLogFile(void)
{
    ui->pbLogFile->setDown(false);
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setOptions(QFileDialog::DontConfirmOverwrite);
    QString fileName = dialog.getSaveFileName(this, tr("Select log file name"),"",tr("log (*.log);;All files (*.*)"),0, QFileDialog::DontConfirmOverwrite);
    ui->leLogFile->setText(fileName);
}

void Properties::slotClear(void)
{
    ui->comboTCPIPlist->clear();
}

void Properties::slotOK(void)
{
   UpdateVars();
   // Save all the current values to default proterties file in the app home
   // dir.
   if(~Save(ApplicationPath + "/Properties.mips")) Save(HomePath + "/Properties.mips");
   this->hide();
}

bool Properties::Save(QString fileName)
{
    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# MIPS properties data, " + dateTime.toString() + "\n";
        stream << "DataFilePath," + DataFilePath + "\n";
        stream << "MethodesPath," + MethodesPath + "\n";
        stream << "ScriptPath," + ScriptPath + "\n";
        stream << "LoadControlPanel," + LoadControlPanel + "\n";
        stream << "FileName," + FileName + "\n";
        stream << "MinMIPS," + QString::number(MinMIPS) + "\n";
        stream << "UpdateSecs," + QString::number(UpdateSecs) + "\n";
        stream << "LogFile," + LogFile + "\n";
        stream << "AMPSbaud," + AMPSbaud + "\n";
        if(SearchAMPS) stream << "SearchAMPS,TRUE\n";
        else stream << "SearchAMPS,FALSE\n";
        if(AutoFileName) stream << "AutoFileName,TRUE\n";
        else stream << "AutoFileName,FALSE\n";
        if(AutoConnect) stream << "AutoConnect,TRUE\n";
        else stream << "AutoConnect,FALSE\n";
        if(AutoRestore) stream << "AutoRestore,TRUE\n";
        else stream << "AutoRestore,FALSE\n";
        stream << "MIPS_TCPIP";
        for(int i=0; i<MIPS_TCPIP.count(); i++) stream << "," + MIPS_TCPIP[i];
        stream << "\n";
        file.close();
        return true;
    }
    return false;
}

bool Properties::Load(QString fileName)
{
    QFile file(fileName);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        // We're going to streaming the file
        // to a QString
        QTextStream stream(&file);
        QString line;
        do
        {
            line = stream.readLine();
            QStringList reslist = line.split(",");
            if((reslist.count() == 2) && (reslist[0] == "DataFilePath")) ui->leDataFilePath->setText(reslist[1]);
            else if((reslist.count() == 2) && (reslist[0] == "MethodesPath")) ui->leMethodesPath->setText(reslist[1]);
            else if((reslist.count() == 2) && (reslist[0] == "ScriptPath")) ui->leScriptPath->setText(reslist[1]);
            else if((reslist.count() == 2) && (reslist[0] == "LoadControlPanel")) ui->leControlPanel->setText(reslist[1]);
            else if((reslist.count() == 2) && (reslist[0] == "FileName")) ui->leFileName->setText(reslist[1]);
            else if((reslist.count() == 2) && (reslist[0] == "MinMIPS")) ui->leMinMIPS->setText(reslist[1]);
            else if((reslist.count() == 2) && (reslist[0] == "UpdateSecs")) ui->leUpdateSecs->setText(reslist[1]);
            else if((reslist.count() == 2) && (reslist[0] == "LogFile")) ui->leLogFile->setText(reslist[1]);
            else if((reslist.count() == 2) && (reslist[0] == "AMPSbaud")) ui->leAMPSbaud->setText(reslist[1]);
            else if((reslist.count() == 2) && (reslist[0] == "SearchAMPS") && (reslist[1] == "TRUE")) ui->chkSearchAMPS->setChecked(true);
            else if((reslist.count() == 2) && (reslist[0] == "SearchAMPS") && (reslist[1] == "FALSE")) ui->chkSearchAMPS->setChecked(false);
            else if((reslist.count() == 2) && (reslist[0] == "AutoFileName") && (reslist[1] == "TRUE")) ui->chkAutoFileName->setChecked(true);
            else if((reslist.count() == 2) && (reslist[0] == "AutoFileName") && (reslist[1] == "FALSE")) ui->chkAutoFileName->setChecked(false);
            else if((reslist.count() == 2) && (reslist[0] == "AutoConnect") && (reslist[1] == "TRUE")) ui->chkAutoConnect->setChecked(true);
            else if((reslist.count() == 2) && (reslist[0] == "AutoConnect") && (reslist[1] == "FALSE")) ui->chkAutoConnect->setChecked(false);
            else if((reslist.count() == 2) && (reslist[0] == "AutoRestore") && (reslist[1] == "TRUE")) ui->chkAutoRestore->setChecked(true);
            else if((reslist.count() == 2) && (reslist[0] == "AutoRestore") && (reslist[1] == "FALSE")) ui->chkAutoRestore->setChecked(false);
            else if((reslist.count() >= 1) && (reslist[0] == "MIPS_TCPIP"))
            {
                ui->comboTCPIPlist->clear();
                for(int i=1;i<reslist.count();i++) ui->comboTCPIPlist->addItem(reslist[i]);
            }
        } while(!line.isNull());
        file.close();
        return true;
    }
    return false;
}

