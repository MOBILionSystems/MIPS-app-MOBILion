#include "filament.h"

Filament::Filament(Ui::MIPS *w, Comms *c)
{
    fui = w;
    comms = c;

    QObjectList widgetList = fui->gpFilament1->children();
    widgetList += fui->gpFilament2->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().startsWith("leS"))
       {
            ((QLineEdit *)w)->setValidator(new QDoubleValidator);
           connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(FilamentUpdated()));
       }
    }
    connect(fui->chkEnableFilament1,SIGNAL(toggled(bool)),this,SLOT(Filament1Enable()));
    connect(fui->chkEnableFilament2,SIGNAL(toggled(bool)),this,SLOT(Filament2Enable()));
}

void Filament::Filament1Enable(void)
{
    if(fui->chkEnableFilament1->isChecked()) comms->SendCommand("SFLENA,1,ON\n");
    else  comms->SendCommand("SFLENA,1,OFF\n");
    //qDebug() << "Fil 1 ck";
}

void Filament::Filament2Enable(void)
{
    if(fui->chkEnableFilament2->isChecked()) comms->SendCommand("SFLENA,2,ON\n");
    else  comms->SendCommand("SFLENA,2,OFF\n");
    //qDebug() << "Fil 2 ck";
}

void Filament::FilamentUpdated(void)
{
    QObject* obj = sender();
    QString res;

    if(!((QLineEdit *)obj)->isModified()) return;
    res = obj->objectName().mid(2).replace("_",",") + "," + ((QLineEdit *)obj)->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    ((QLineEdit *)obj)->setModified(false);
}

void Filament::Update(void)
{
    QString res;

    // Update the enable check boxes
    res = comms->SendMess("GFLENA,1\n");
    if(res == "ON") fui->chkEnableFilament1->setChecked(true);
    else if(res == "OFF") fui->chkEnableFilament1->setChecked(false);
    // Update all the line edit boxes
    QObjectList widgetList = fui->gpFilament1->children();
    widgetList += fui->gpFilament2->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().startsWith("le"))
       {
            if(!((QLineEdit *)w)->hasFocus())
            {
               res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
               ((QLineEdit *)w)->setText(comms->SendMess(res));
            }
       }
    }
    res = comms->SendMess("GFLENA,2\n");
    if(res == "ON") fui->chkEnableFilament2->setChecked(true);
    else  if(res == "OFF") fui->chkEnableFilament2->setChecked(false);
}
