#include "script.h"
#include "ui_script.h"

Script::Script(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Script)
{
    ui->setupUi(this);

    connect(ui->pbLoadScript,SIGNAL(pressed()),this,SLOT(LoadScriptFile()));
    connect(ui->pbAbort,SIGNAL(pressed()),this,SLOT(AbortScriptFile()));
}

Script::~Script()
{
    delete ui;
}

void Script::Message(QString mess)
{
    ui->pteScriptStatus->appendPlainText(mess);
}

void Script::LoadScriptFile(void)
{
    QStringList resList;

    ui->pbLoadScript->setDown(false);
    ui->pbLoadScript->setEnabled(false);
    AbortRequest = false;
    ui->pbAbort->setEnabled(true);
    ui->pteScriptStatus->clear();
    // set black background
    QPalette CurPal = ui->gbScript->palette();
    QPalette pal = palette();
    pal.setColor(QPalette::Background, Qt::green);
    ui->gbScript->setAutoFillBackground(true);
    ui->gbScript->setPalette(pal);
    ui->gbScript->show();
    // Ask user for the script file and open
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Script from File"),"",tr("Scrpt (*.scrpt);;All files (*.*)"));
    ui->leScriptFile->setText(fileName);
    QFile file(fileName);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        // Read each line of the script and process.
        // If line starts with a # then its a comment.
        // The following are valid script commands:
        // LOAD,settingfile.ext
        // DELAY,mins
        // SHUTDOWN
        // We're going to streaming the file
        // to the QString
        QTextStream stream(&file);
        QString line;
        do
        {
            if(AbortRequest) break;
            line = stream.readLine();
            if(line.trimmed().startsWith("#")) continue;
            resList = line.split(",");
            if(resList.count() == 1)
            {
                if(resList[0] == "SHUTDOWN")
                {
                    emit Shutdown();
                    ui->pteScriptStatus->appendPlainText("Shutdown");
                }
            }
            else if(resList.count() == 2)
            {
                if(resList[0] == "DELAY")
                {
                    ui->pteScriptStatus->appendPlainText(QDateTime::currentDateTime().toString());
                    ui->pteScriptStatus->appendPlainText("Delaying for " + resList[1] + " mins");
                    QTime dieTime= QTime::currentTime().addSecs(resList[1].toFloat() * 60);
                    while (QTime::currentTime() < dieTime)
                    {
                        if(AbortRequest) break;
                        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
                    }
                }
                if(resList[0] == "LOAD")
                {
                    ui->pteScriptStatus->appendPlainText("LOAD," + resList[1]);
                    emit Load(resList[1]);
                }
            }
        } while(!line.isNull());
    }
    else
    {
       ui->pteScriptStatus->appendPlainText("Unable to open file!");
    }
    if(AbortRequest) ui->pteScriptStatus->appendPlainText("Aborted!");
    else ui->pteScriptStatus->appendPlainText("Done!");
    ui->pbLoadScript->setEnabled(true);
    ui->pbAbort->setEnabled(false);
    ui->gbScript->setAutoFillBackground(true);
    ui->gbScript->setPalette(CurPal);
    ui->gbScript->show();
}

void Script::AbortScriptFile(void)
{
    ui->pbAbort->setDown(false);
    AbortRequest = true;
}

