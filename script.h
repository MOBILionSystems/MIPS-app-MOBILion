#ifndef SCRIPT_H
#define SCRIPT_H

#include <QDialog>
#include <QFileInfo>
#include <QFileDialog>
#include <QTime>
#include <QTextStream>


namespace Ui {
class Script;
}

class Script : public QDialog
{
    Q_OBJECT

signals:
    void Shutdown(void);
    void Load(QString FileName);
public:
    explicit Script(QWidget *parent = 0);
    void Message(QString mess);
    ~Script();

private:
    Ui::Script *ui;
    bool AbortRequest;

private slots:
    void LoadScriptFile(void);
    void AbortScriptFile(void);
};

#endif // SCRIPT_H
