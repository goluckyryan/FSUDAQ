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
#include <QVector>
#include <QRandomGenerator>

#include "macro.h"
#include "ClassDigitizer.h"
#include "CustomThreads.h"
#include "CustomWidgets.h"
#include "Histogram1D.h"
#include "Histogram2D.h"


//^====================================================
//^====================================================
class SingleSpectra : public QMainWindow{
  Q_OBJECT

public:
  SingleSpectra(Digitizer ** digi, unsigned int nDigi, QString rawDataPath, QMainWindow * parent = nullptr);
  ~SingleSpectra();

  void ClearInternalDataCount();
  void SetFillHistograms(bool onOff) { fillHistograms = onOff;}
  bool IsFillHistograms() const {return fillHistograms;}

  void LoadSetting();
  void SaveSetting();

  void SetMaxFillTime(unsigned short milliSec) { maxFillTimeinMilliSec = milliSec;}
  unsigned short GetMaxFillTime() const {return maxFillTimeinMilliSec;};

  QVector<int> generateNonRepeatedCombination(int size);

public slots:
  void FillHistograms();
  void ChangeHistView();

private:

  Digitizer ** digi;
  unsigned short nDigi;

  Histogram1D * hist[MaxNDigitizer][MaxNChannels];
  Histogram2D * hist2D[MaxNDigitizer];

  bool histVisibility[MaxNDigitizer][MaxNChannels];
  bool hist2DVisibility[MaxNDigitizer];

  RComboBox * cbDivision;

  RComboBox * cbDigi;
  RComboBox * cbCh;

  QGroupBox * histBox;
  QGridLayout * histLayout;
  int oldBd, oldCh;

  int lastFilledIndex[MaxNDigitizer][MaxNChannels];
  int loopFilledIndex[MaxNDigitizer][MaxNChannels];

  bool fillHistograms;

  QString settingPath;

  unsigned short maxFillTimeinMilliSec;
  unsigned short maxFillTimePerDigi;

  bool isSignalSlotActive;

};
#endif