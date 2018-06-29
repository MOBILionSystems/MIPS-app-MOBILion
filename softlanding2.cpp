#include "softlanding2.h"
#include "ui_softlanding2.h"
#include <QDebug>
#include <QKeyEvent>

SoftLanding2::SoftLanding2(QWidget *parent, Comms *c, QStatusBar *statusbar) :
    QDialog(parent),
    ui(new Ui::SoftLanding2)
{
    ui->setupUi(this);

    comms = c;
    sb = statusbar;
    Updating = false;
    UpdateOff = false;
    AbortRequest = false;
    this->setFixedSize(1051,831);

    qApp->installEventFilter(this);//in Dialog constructor
    QObjectList widgetList = ui->frmSL2->children();
    widgetList += ui->gbSLfunnel1->children();
    widgetList += ui->gbSLfunnel2->children();
    widgetList += ui->gbSLCQ->children();
    foreach(QObject *w, widgetList)
    {
        if(w->objectName().contains("leS"))
        {
            ((QLineEdit *)w)->setValidator(new QDoubleValidator);
            connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(SLUpdated()));
        }
    }
    connect(ui->chkESIenable,SIGNAL(toggled(bool)),this,SLOT(ESIenablePos()));
    connect(ui->chkESIenableNeg,SIGNAL(toggled(bool)),this,SLOT(ESIenableNeg()));
    connect(ui->pbTuneF1,SIGNAL(pressed()),this,SLOT(AutoTune()));
    connect(ui->pbTuneF2,SIGNAL(pressed()),this,SLOT(AutoTune()));
    connect(ui->pbTuneCQ,SIGNAL(pressed()),this,SLOT(AutoTune()));
    connect(ui->pbRetuneF1,SIGNAL(pressed()),this,SLOT(AutoTune()));
    connect(ui->pbRetuneF2,SIGNAL(pressed()),this,SLOT(AutoTune()));
    connect(ui->pbRetuneCQ,SIGNAL(pressed()),this,SLOT(AutoTune()));
    connect(ui->pbShutdown,SIGNAL(pressed()),this,SLOT(Shutdown()));
    connect(ui->pbResetPowerSupply,SIGNAL(pressed()),this,SLOT(ResetPowerSupply()));
    connect(ui->pbLoadScript,SIGNAL(pressed()),this,SLOT(LoadScriptFile()));
    connect(ui->pbAbort,SIGNAL(pressed()),this,SLOT(AbortScriptFile()));
}

void SoftLanding2::reject()
{
    emit DialogClosed();
}

SoftLanding2::~SoftLanding2()
{
    delete ui;
}

bool SoftLanding2::ConfigurationCheck(void)
{
    QString res;
    int     nRF,nDC,nESI,nDAC;
    QMessageBox msgBox;

    // If we are connected to MIPS test to make sure we have the following modules:
    //  2 - RF drivers
    //  3 - DC bias modules
    //  2 - ESI module
    //  1 - DAC module
    // Generate error message if not present
    if(comms->isConnected())
    {
       // Test for required modules
       res = comms->SendMessage("GCHAN,RF\n");
       nRF = res.toInt()/2;
       res = comms->SendMessage("GCHAN,DCB\n");
       nDC = res.toInt()/8;
       res = comms->SendMessage("GCHAN,ESI\n");
       nESI = res.toInt();
       res = comms->SendMessage("GCHAN,DAC\n");
       nDAC = res.toInt()/8;
       if((nRF < 2) || (nDC < 3) || (nESI < 2) || (nDAC < 1))
       {
           QString msg = "Invalid configuration!\n";
           msg += "   Required 2 RF modules, found " + QString::number(nRF) + "\n";
           msg += "   Required 3 DCbias modules, found " + QString::number(nDC) + "\n";
           msg += "   Required 2 ESI modules, found " + QString::number(nESI) + "\n";
           msg += "   Required 1 DAC module, found " + QString::number(nDAC) + "\n";
           msgBox.setText(msg);
           msgBox.setInformativeText("Are you sure you want to continue?");
           msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
           msgBox.setDefaultButton(QMessageBox::No);
           int ret = msgBox.exec();
           if(ret == QMessageBox::No)
           {
               return false;
           }
       }
    }
    return true;
}

bool SoftLanding2::eventFilter(QObject *obj, QEvent *event)
{
    QLineEdit *le;
    QString res;
    float delta = 0;

    if (((obj->objectName().startsWith("leS")) || (obj->objectName().startsWith("MassUnits"))) && (event->type() == QEvent::KeyPress))
    {
        if(Updating) return true;
        UpdateOff = true;
        le = (QLineEdit *)obj;
        QKeyEvent *key = static_cast<QKeyEvent *>(event);
//        qDebug() << "pressed"<< key->key();
//        qDebug() << QApplication::queryKeyboardModifiers();
        if(key->key() == 16777235) delta = 0.1;
        if(key->key() == 16777237) delta = -0.1;
        if((QApplication::queryKeyboardModifiers() & 0x2000000) != 0) delta *= 10;
        if((QApplication::queryKeyboardModifiers() & 0x8000000) != 0) delta *= 100;
        if(delta != 0)
        {
           //le->setText(QString::number(le->text().toFloat() + delta));
           QString myString;
           if(obj->objectName().startsWith("leSRFFRQ"))
           {
               myString.sprintf("%3.0f", le->text().toFloat() + delta*10);
           }
           else if(obj->objectName().startsWith("leSDAC"))
           {
               myString.sprintf("%3.3f", le->text().toFloat() + delta/100);
           }
           else
           {
               myString.sprintf("%3.2f", le->text().toFloat() + delta);
           }
           le->setText(myString);
           le->setModified(true);
           le->editingFinished();
           return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

void SoftLanding2::Save(QString Filename)
{
    QString res;

    if(Filename == "") return;
    QFile file(Filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# Soft Landing System settings, " + dateTime.toString() + "\n";
        QObjectList widgetList = ui->gbSLCQ->children();
        widgetList += ui->leSDCBOF_1;
        widgetList += ui->leSDCBOF_9;
        widgetList += ui->frmSL2->children();
        widgetList += ui->gbSLfunnel1->children();
        widgetList += ui->gbSLfunnel2->children();
        foreach(QObject *w, widgetList)
        {
            if(w->objectName().mid(0,3) == "leS")
            {
                res = w->objectName() + "," + ((QLineEdit *)w)->text() + "\n";
                stream << res;
            }
        }
        if(ui->chkESIenable->isChecked()) stream << "chkESIenable,TRUE\n";
        else stream << "chkESIenable,FALSE\n";
        if(ui->chkESIenableNeg->isChecked()) stream << "chkESIenableNeg,TRUE\n";
        else stream << "chkESIenableNeg,FALSE\n";
        file.close();
        sb->showMessage("Settings saved to " + Filename,2000);
    }
}

QString SoftLanding2::Load(QString Filename)
{
    QStringList resList;

    if(Filename == "") return("");
    QObjectList widgetList = ui->frmSL2->children();
    widgetList += ui->gbSLfunnel1->children();
    widgetList += ui->gbSLfunnel2->children();
    widgetList += ui->gbSLCQ->children();
    QFile file(Filename);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        // We're going to streaming the file
        // to the QString
        QTextStream stream(&file);
        QString line;
        do
        {
            line = stream.readLine();
            resList = line.split(",");
            if(resList.count() == 2)
            {
                foreach(QObject *w, widgetList)
                {
                    if(w->objectName().mid(0,3) == "leS")
                    {
                        if(resList[1] != "") if(w->objectName() == resList[0])
                        {
                            ((QLineEdit *)w)->setText(resList[1]);
                            ((QLineEdit *)w)->setModified(true);
                            QMetaObject::invokeMethod(w, "editingFinished");
                        }
                    }
                }
                if(resList[0] == "chkESIenable")
                {
                    if(resList[1] == "TRUE") ui->chkESIenable->setChecked(true);
                    else ui->chkESIenable->setChecked(false);
                    QMetaObject::invokeMethod(ui->chkESIenable, "toggled");
                }
                if(resList[0] == "chkESIenableNeg")
                {
                    if(resList[1] == "TRUE") ui->chkESIenableNeg->setChecked(true);
                    else ui->chkESIenableNeg->setChecked(false);
                    QMetaObject::invokeMethod(ui->chkESIenableNeg, "toggled");
                }
            }
        } while(!line.isNull());
        file.close();
        sb->showMessage("Settings loaded from " + Filename,2000);
        return("Settings loaded from " + Filename);
    }
    return("Can't open settings file!");
}

void SoftLanding2::ESIenablePos(void)
{
    if(ui->chkESIenable->isChecked()) comms->SendCommand("SHVENA,1\n");
    else  comms->SendCommand("SHVDIS,1\n");
}

void SoftLanding2::ESIenableNeg(void)
{
    if(ui->chkESIenableNeg->isChecked()) comms->SendCommand("SHVENA,2\n");
    else  comms->SendCommand("SHVDIS,2\n");
}

void SoftLanding2::AutoTune(void)
{
    QObject* obj = sender();
    int tch = 0, rtch = 0;
    bool stat;

    ((QPushButton *)(obj))->setDown(false);
    if(obj->objectName() == "pbTuneF1") tch = 1;
    else if(obj->objectName() == "pbTuneF2") tch = 2;
    else if(obj->objectName() == "pbTuneCQ") tch = 3;
    else if(obj->objectName() == "pbRetuneF1") rtch = 1;
    else if(obj->objectName() == "pbRetuneF2") rtch = 2;
    else if(obj->objectName() == "pbRetuneCQ") rtch = 3;
    QMessageBox msgBox;
    if(tch != 0)
    {
        QString msg = "This function will tune the RF head attached to channel " + QString::number(tch) + ". ";
        msg += "Make sure the RF head is attached and connected to your system as needed. ";
        msg += "This process can take up to 3 minutes.\n";
        msgBox.setText(msg);
        msgBox.setInformativeText("Are you sure you want to continue?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if(ret == QMessageBox::No) return;
    }
    if(rtch != 0)
    {
        QString msg = "This function will retune the RF head attached to channel " + QString::number(tch) + ". ";
        msg += "Make sure the RF head is attached and connected to your system as needed. ";
        msg += "This process can take up to 1 minute.\n";
        msgBox.setText(msg);
        msgBox.setInformativeText("Are you sure you want to continue?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if(ret == QMessageBox::No) return;
    }
    if(tch == 1) stat = comms->SendCommand("TUNERFCH,1\n");
    else if(tch == 2) stat = comms->SendCommand("TUNERFCH,2\n");
    else if(tch == 3) stat = comms->SendCommand("TUNERFCH,3\n");
    else if(rtch == 1) stat = comms->SendCommand("RETUNERFCH,1\n");
    else if(rtch == 2) stat = comms->SendCommand("RETUNERFCH,2\n");
    else if(rtch == 3) stat = comms->SendCommand("RETUNERFCH,3\n");
    if(!stat)
    {
        QString msg = "Request failed!, could be a tune in process, only one channel ";
        msg += "can be tuned or retuned at a time. ";
        msgBox.setText(msg);
        msgBox.setInformativeText("");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }
}

// Set all output level to zero
void SoftLanding2::Shutdown(void)
{
    QObjectList widgetList = ui->frmSL2->children();
    widgetList += ui->leSRFDRV_1;
    widgetList += ui->leSRFDRV_2;
    widgetList += ui->leSRFDRV_3;
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().startsWith("leS"))
       {
           ((QLineEdit *)w)->setText("0");
           ((QLineEdit *)w)->setModified(true);
           QMetaObject::invokeMethod(w, "editingFinished");
       }
    }
    ui->chkESIenable->setChecked(false);
    QMetaObject::invokeMethod(ui->chkESIenable, "toggled");
    ui->chkESIenableNeg->setChecked(false);
    QMetaObject::invokeMethod(ui->chkESIenableNeg, "toggled");
}

void SoftLanding2::ResetPowerSupply(void)
{
   comms->SendCommand("SDCPWR,ON\n");
}

void SoftLanding2::SLUpdated(void)
{
    QObject* obj = sender();
    QString res;
    QMessageBox msgBox;
    static bool busy = false;

    if(Updating) return;
    if(!((QLineEdit *)obj)->isModified()) return;
    res = obj->objectName().mid(2).replace("_",",") + "," + ((QLineEdit *)obj)->text() + "\n";
    if(!comms->SendCommand(res.toStdString().c_str()))
    {
        if(!busy)
        {
           busy = true;
           QString msg = "Value rejected, likely out of range!";
           msgBox.setText(msg);
           msgBox.setInformativeText("");
           msgBox.setStandardButtons(QMessageBox::Ok);
           msgBox.exec();
        }
        busy = false;
        UpdateOff = false;
        ((QLineEdit *)obj)->setModified(false);
        return;
    }
    ((QLineEdit *)obj)->setModified(false);
    UpdateOff = false;
}

void SoftLanding2::Update(void)
{
    QString res;

    if(UpdateOff) return;
    Updating = true;
    QObjectList widgetList = ui->frmSL2->children();
    widgetList += ui->gbSLfunnel1->children();
    widgetList += ui->gbSLfunnel2->children();
    widgetList += ui->gbSLCQ->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().startsWith("le"))
       {
           if(!w->objectName().startsWith("leMass"))
           {
               if(!((QLineEdit *)w)->hasFocus())
               {
                  res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
                  ((QLineEdit *)w)->setText(comms->SendMessage(res));
               }
           }
       }
    }
    res = comms->SendMessage("GHVSTATUS,1\n");
    if(res == "ON") ui->chkESIenable->setChecked(true);
    else ui->chkESIenable->setChecked(false);
    ui->lblSLtime->setText(QDateTime::currentDateTime().toString());
    Updating = false;
}

// Functions supporting script system

void SoftLanding2::LoadScriptFile(void)
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
                    Shutdown();
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
                    ui->pteScriptStatus->appendPlainText(Load(resList[1]));
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

void SoftLanding2::AbortScriptFile(void)
{
    ui->pbAbort->setDown(false);
    AbortRequest = true;
}

