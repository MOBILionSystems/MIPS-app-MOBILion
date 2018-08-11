#ifndef SCRIPTINGCONSOLE_H
#define SCRIPTINGCONSOLE_H

#include <QDialog>
#include <QScriptEngine>

namespace Ui {
class ScriptingConsole;
}

class ScriptingConsole : public QDialog
{
    Q_OBJECT

public:
    explicit ScriptingConsole(QWidget *parent = 0);
    ~ScriptingConsole();

private:
    Ui::ScriptingConsole *ui;
    QScriptEngine *engine;

private slots:
    void on_pbEvaluate_clicked();
    void on_pbSave_clicked();
    void on_pbLoad_clicked();
};

#endif // SCRIPTINGCONSOLE_H
