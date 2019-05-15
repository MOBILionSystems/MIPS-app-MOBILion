#ifndef ARBWAVEFORMEDIT_H
#define ARBWAVEFORMEDIT_H

#include <QDialog>

namespace Ui {
class ARBwaveformEdit;
}

class ARBwaveformEdit : public QDialog
{
    Q_OBJECT

signals:
    void WaveformReady(void);

public:
    explicit ARBwaveformEdit(QWidget *parent, int ppp);
    ~ARBwaveformEdit();
    void SetWaveform(int *wf);
    void GetWaveform(int *wf);

private:
    Ui::ARBwaveformEdit *ui;
    int Waveform[32];
    int PPP;

private slots:
    void GenerateUpDown(void);
    void GenerateSine(void);
    void PlotData(void);
    void InvertData(void);
    void WaveformAccepted(void);
};

#endif // ARBWAVEFORMEDIT_H
