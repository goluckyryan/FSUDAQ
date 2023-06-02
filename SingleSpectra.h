#ifndef SINGLE_SPECTR_H
#define SINGLE_SPECTR_H

#include <QMainWindow>
#include <QChart>
#include <QChartView>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QGridLayout>
#include <QGroupBox>

#include "macro.h"
#include "ClassDigitizer.h"
#include "CustomThreads.h"
#include "CustomWidgets.h"
#include "Histogram1D.h"


//^====================================================
//^====================================================
class SingleSpectra : public QMainWindow{
  Q_OBJECT

public:
  SingleSpectra(Digitizer ** digi, unsigned int nDigi, QString rawDataPath, QMainWindow * parent = nullptr);
  ~SingleSpectra();

  void SetFillHistograms(bool onOff) { fillHistograms = onOff;}
  bool IsFillHistograms() const {return fillHistograms;}

  void LoadSetting();
  void SaveSetting();

public slots:
  void FillHistograms();
  void ChangeHistView();
  void RebinHistogram();

private:

  Digitizer ** digi;
  unsigned short nDigi;

  Histogram1D * hist[MaxNDigitizer][MaxNChannels];

  RComboBox * cbDivision;

  RComboBox * cbDigi;
  RComboBox * cbCh;
  
  bool enableSignalSlot;

  RSpinBox * sbNBin;
  RSpinBox * sbXMin;
  RSpinBox * sbXMax;
  QPushButton * bnReBin;

  QGroupBox * histBox;
  QGridLayout * histLayout;
  int oldBd, oldCh;

  int lastFilledIndex[MaxNDigitizer][MaxNChannels];
  int loopFilledIndex[MaxNDigitizer][MaxNChannels];

  bool fillHistograms;

  QString rawDataPath;

};
#endif