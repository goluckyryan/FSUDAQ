#ifndef CANVAS_H
#define CANVAS_H

#include <QMainWindow>
#include <QChart>
#include <QChartView>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLineSeries>
#include <QRubberBand>
#include <QMouseEvent>
#include <QGestureEvent>

#include "macro.h"
#include "ClassDigitizer.h"
#include "CustomThreads.h"
#include "CustomWidgets.h"


//^====================================================
//^====================================================
class Canvas : public QMainWindow{
  Q_OBJECT

public:
  Canvas(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr);
  ~Canvas();

public slots:
  void UpdateCanvas();

private:

  Digitizer ** digi;
  unsigned short nDigi;


};


#endif