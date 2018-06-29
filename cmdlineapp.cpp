#include "cmdlineapp.h"
#include "ui_cmdlineapp.h"

cmdlineapp::cmdlineapp(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::cmdlineapp)
{
    ui->setupUi(this);
    ReadyMessage = "";
}

cmdlineapp::~cmdlineapp()
{
    delete ui;
}

void cmdlineapp::reject()
{
    emit DialogClosed();
    delete this;
}

void cmdlineapp::Execute(void)
{
    QStringList arguments;

    arguments << "-c";
    arguments << appPath;
#if defined(Q_OS_MAC)
    process.start("/bin/bash",arguments);
#else
    process.start(appPath);
#endif
    ui->txtTerm->appendPlainText("Application should start soon...\n");
    connect(&process,SIGNAL(readyReadStandardOutput()),this,SLOT(readProcessOutput()));
    connect(&process,SIGNAL(readyReadStandardError()),this,SLOT(readProcessOutput()));
    connect(&process,SIGNAL(finished(int)),this,SLOT(AppFinished()));
    connect(ui->leCommand,SIGNAL(editingFinished()),this,SLOT(readMessage()));
}

void cmdlineapp::readProcessOutput(void)
{
    QString mess;

    mess = process.readAllStandardOutput();
    if(ReadyMessage != "") if(mess.contains(ReadyMessage)) emit Ready();
    ui->txtTerm->moveCursor (QTextCursor::End);
    ui->txtTerm->insertPlainText (mess);
    ui->txtTerm->moveCursor (QTextCursor::End);
    ui->txtTerm->moveCursor (QTextCursor::End);
    ui->txtTerm->insertPlainText (process.readAllStandardError());
    ui->txtTerm->moveCursor (QTextCursor::End);
}

void cmdlineapp::readMessage(void)
{
    process.write(ui->leCommand->text().toStdString().c_str());
    process.write("\n");
    ui->txtTerm->moveCursor (QTextCursor::End);
    ui->txtTerm->insertPlainText (ui->leCommand->text() + "\n");
    ui->txtTerm->moveCursor (QTextCursor::End);
    ui->leCommand->setText("");
}

void cmdlineapp::AppFinished(void)
{
    ui->txtTerm->appendPlainText("App signaled its finished!");
    emit AppCompleted();
}
