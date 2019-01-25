#ifndef SCRIPTINGCONSOLE_H
#define SCRIPTINGCONSOLE_H

#include <QDialog>
#include <QScriptEngine>

#include "properties.h"

namespace Ui {
class ScriptingConsole;
}

class ScriptingConsole : public QDialog
{
    Q_OBJECT

public:
    explicit ScriptingConsole(QWidget *parent = 0, Properties *prop = NULL);
    void UpdateStatus(void);
    void RunScript(void);
    ~ScriptingConsole();
    Properties  *properties;

protected:
    void paintEvent(QPaintEvent *event);

private:
    Ui::ScriptingConsole *ui;
    QScriptEngine *engine;

private slots:
    void on_pbEvaluate_clicked();
    void on_pbSave_clicked();
    void on_pbLoad_clicked();
    void on_pbAbort_clicked();
    void on_pbHelp_clicked();
};

#endif // SCRIPTINGCONSOLE_H
