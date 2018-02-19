#include "adc.h"

ADC::ADC(Ui::MIPS *w, Comms *c)
{
    ui = w;
    comms = c;

    ADCbuffer = NULL;
    ADCbufferSum = NULL;

    QObjectList widgetList = ui->gbADCdigitizer->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("leS"))
       {
           ((QLineEdit *)w)->setValidator(new QIntValidator);
           connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(ADCupdated()));
       }
    }

    connect(ui->pbADCinit,SIGNAL(pressed()),this,SLOT(ADCsetup()));
    connect(ui->pbADCtrigger,SIGNAL(pressed()),this,SLOT(ADCtrigger()));
    connect(ui->pbADCabort,SIGNAL(pressed()),this,SLOT(ADCabort()));
    connect(comms,SIGNAL(ADCvectorReady()),this,SLOT(ADCvectorReady()));
    connect(comms,SIGNAL(ADCrecordingDone()),this,SLOT(ADCrecordingDone()));
    connect(ui->chkADCzoomX,SIGNAL(clicked(bool)),this,SLOT(SetZoom()));
    connect(ui->chkADCzoomY,SIGNAL(clicked(bool)),this,SLOT(SetZoom()));

    // Setup the plot
    ui->ADCdata->xAxis->setLabel("Time, sec");
    ui->ADCdata->yAxis->setLabel("ADC counts");
    ui->ADCdata->addGraph();
}

void ADC::Update(bool UpdateSelected)
{
    QString res;

    QObjectList widgetList = ui->gbADCdigitizer->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("le"))
       {
            if((!((QLineEdit *)w)->hasFocus()) || (UpdateSelected))
            {
               res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
               ((QLineEdit *)w)->setText(comms->SendMess(res));
            }
       }
    }
}

void ADC::ADCupdated(void)
{
    QObject* obj = sender();
    QString res;

    if(!((QLineEdit *)obj)->isModified()) return;
    res = obj->objectName().mid(2).replace("_",",") + "," + ((QLineEdit *)obj)->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    ((QLineEdit *)obj)->setModified(false);
}

void ADC::ADCsetup(void)
{
    int NumSamples;

    NumSamples = ui->leSADCSAMPS->text().toInt();
    if(ADCbuffer != NULL) delete ADCbuffer;
    ADCbuffer = new quint16 [NumSamples];
    if(ADCbufferSum != NULL) delete ADCbufferSum;
    ADCbufferSum = new int [NumSamples];
    for(int i=0; i<NumSamples; i++) ADCbufferSum[i] = 0;
    NumScans = 0;
    comms->SendCommand("ADCINIT\n");
    comms->GetADCbuffer(ADCbuffer,NumSamples);
}

void ADC::ADCtrigger(void)
{
   comms->SendString("ADCTRIG\n");
}

void ADC::ADCabort(void)
{
   comms->SendString("ADCABORT\n");
}

void ADC::ADCrecordingDone(void)
{
    comms->ADCrelease();
}

void ADC::ADCvectorReady(void)
{
    int NumSamples;
    int Rate;

    NumScans++;
    NumSamples = ui->leSADCSAMPS->text().toInt();
    for(int i=0; i<NumSamples; i++) ADCbufferSum[i] += ADCbuffer[i];
    Rate = ui->leSADCRATE->text().toInt();
    QVector<double> x(NumSamples), y(NumSamples);
    // Here when the buffer has been filled
    for(int i=0; i<NumSamples; i++)
    {
       y[i] = ADCbufferSum[i] / NumScans;
       x[i] = (float)i / (float)Rate;
    }
    ui->ADCdata->graph(0)->setData(x, y);
    ui->ADCdata->yAxis->rescale(true);
    ui->ADCdata->xAxis->rescale(true);
    ui->ADCdata->replot();
}

void ADC::SetZoom(void)
{
    if(ui->chkADCzoomX->isChecked() && ui->chkADCzoomY->isChecked())
    {
        ui->ADCdata->yAxis->axisRect()->setRangeZoom(Qt::Vertical | Qt::Horizontal);
        ui->ADCdata->yAxis->axisRect()->setRangeDrag(Qt::Vertical | Qt::Horizontal);
        ui->ADCdata->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    }
    else if(ui->chkADCzoomX->isChecked())
    {
        ui->ADCdata->yAxis->axisRect()->setRangeZoom(Qt::Horizontal);
        ui->ADCdata->yAxis->axisRect()->setRangeDrag(Qt::Horizontal);
        ui->ADCdata->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    }
    else if(ui->chkADCzoomY->isChecked())
    {
        ui->ADCdata->yAxis->axisRect()->setRangeZoom(Qt::Vertical);
        ui->ADCdata->yAxis->axisRect()->setRangeDrag(Qt::Vertical);
        ui->ADCdata->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    }
    else
    {
        ui->ADCdata->setInteractions(0);
    }
}
