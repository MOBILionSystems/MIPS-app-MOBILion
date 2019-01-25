#include "scriptingconsole.h"
#include "ui_scriptingconsole.h"
#include "help.h"
#include <qscriptengine.h>
#include <QtCore/QtGlobal>
#include <QtSerialPort/QSerialPort>
#include <QStatusBar>
#include <QMessageBox>
#include <QObject>
#include <QApplication>
#include <QFileInfo>
#include <QFileDialog>
#include <QTextStream>

ScriptingConsole::ScriptingConsole(QWidget *parent, Properties *prop) :
    QDialog(parent),
    ui(new Ui::ScriptingConsole)
{
   ui->setupUi(this);

   properties = prop;

   this->setFixedSize(501,366);
   engine = new QScriptEngine(this);
   engine->setProcessEventsInterval(100);

   QScriptValue mips = engine->newQObject(parent);
   engine->globalObject().setProperty("mips",mips);
}

ScriptingConsole::~ScriptingConsole()
{
    delete ui;
}

void ScriptingConsole::paintEvent(QPaintEvent *event)
{
    UpdateStatus();
}

void ScriptingConsole::RunScript(void)
{
    if(engine->isEvaluating()) return;
    if(properties != NULL) properties->Log("Script executed");
    QString script = ui->txtScript->toPlainText();
    QScriptValue result = engine->evaluate(script);
    QString markup;
    if(result.isError())
        markup.append("<font color=\"red\">");
    markup.append(result.toString());
    if(result.isError())
        markup.append("</font>");
    ui->lblStatus->setText(markup);
}

void ScriptingConsole::on_pbEvaluate_clicked()
{
    ui->pbEvaluate->setDown(false);
    RunScript();
}

void ScriptingConsole::UpdateStatus(void)
{
    if(engine->isEvaluating())
    {
        ui->pbEvaluate->setText("Script is running!");
    }
    else
    {
        ui->pbEvaluate->setText("Execute");
    }
}

void ScriptingConsole::on_pbSave_clicked()
{
    QFileDialog fileDialog;

    if(properties != NULL) fileDialog.setDirectory(properties->ScriptPath);
    QString fileName = fileDialog.getSaveFileName(this, tr("Save script to file"),"",tr("scrpt (*.scrpt);;All files (*.*)"));
    if(fileName == "") return;
    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        stream << ui->txtScript->toPlainText();
        file.close();
        if(properties != NULL) properties->Log("Script saved: " + fileName);
    }
}

void ScriptingConsole::on_pbLoad_clicked()
{
    QFileDialog fileDialog;

    if(properties != NULL) fileDialog.setDirectory(properties->ScriptPath);
    QString fileName = fileDialog.getOpenFileName(this, tr("Load script from File"),"",tr("scrpt (*.scrpt);;All files (*.*)"));
    if(fileName == "") return;
    QFile file(fileName);
    ui->txtScript->clear();
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        QTextStream stream(&file);
        QString text = stream.readAll();
        ui->txtScript->append(text);
        file.close();
        if(properties != NULL) properties->Log("Script loaded: " + fileName);
    }
}

void ScriptingConsole::on_pbAbort_clicked()
{
    engine->abortEvaluation();
    if(properties != NULL) properties->Log("Script aborted");
}

void ScriptingConsole::on_pbHelp_clicked()
{
    Help *help = new Help();

    help->SetTitle("Script Help");
    help->LoadHelpText(":/scripthelp.txt");
    help->show();
}
