#include "mips.h"
#include <QApplication>
#include <QWidget>

int main(int argc, char *argv[])
{
    QString cpf;

    QApplication a(argc, argv);
    if(argc >= 1) cpf = argv[0];
    else cpf = "";
    MIPS w(0,"");
    w.setWindowIcon(QIcon(":/GAACElogo.ico"));
    //connect(&a, SIGNAL(focusChanged(QWidget*, QWidget*)), this, SLOT(setWidgets(QWidget*, QWidget*)));    connect(ui->actionClear, SIGNAL(triggered()), console, SLOT(clear()));
    QObject::connect(&a, SIGNAL(focusChanged(QWidget*,QWidget*)), &w, SLOT(setWidgets(QWidget*,QWidget*)));
    w.show();

    return a.exec();
}
