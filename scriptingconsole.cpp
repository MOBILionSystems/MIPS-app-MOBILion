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

ScriptingConsole::ScriptingConsole(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ScriptingConsole)
{
    ui->setupUi(this);

    this->setFixedSize(501,366);
    engine = new QScriptEngine(this);

   QScriptValue mips = engine->newQObject(parent);
   engine->globalObject().setProperty("mips",mips);
}

ScriptingConsole::~ScriptingConsole()
{
    delete ui;
}

void ScriptingConsole::on_pbEvaluate_clicked()
{
    ui->pbEvaluate->setDown(false);
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
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save script to file"),"",tr("scrpt (*.scrpt);;All files (*.*)"));
    if(fileName == "") return;
    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        stream << ui->txtScript->toPlainText();
        file.close();
    }
}

void ScriptingConsole::on_pbLoad_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load script from File"),"",tr("scrpt (*.scrpt);;All files (*.*)"));
    if(fileName == "") return;
    QFile file(fileName);
    ui->txtScript->clear();
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        QTextStream stream(&file);
        QString text = stream.readAll();
        ui->txtScript->append(text);
        file.close();
    }
}

void ScriptingConsole::on_pbAbort_clicked()
{
    engine->abortEvaluation();
}

void ScriptingConsole::on_pbHelp_clicked()
{
    Help *help = new Help();

    help->SetTitle("Script Help");
    help->LoadHelpText(":/scripthelp.txt");
    help->show();
}
