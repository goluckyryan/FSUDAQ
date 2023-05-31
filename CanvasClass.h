#ifndef CANVAS_H
#define CANVAS_H

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
#include "CustomHistogram.h"


//^====================================================
//^====================================================
class Canvas : public QMainWindow{
  Q_OBJECT

public:
  Canvas(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr);
  ~Canvas();

public slots:
  void UpdateCanvas();
  void ChangeHistView();

private:

  Digitizer ** digi;
  unsigned short nDigi;

  Histogram1D * hist[MaxNDigitizer][MaxNChannels];

  RComboBox * cbDivision;

  RComboBox * cbDigi;
  RComboBox * cbCh;

  QGroupBox * histBox;
  QGridLayout * histLayout;
  int oldBd, oldCh;

};
#endif