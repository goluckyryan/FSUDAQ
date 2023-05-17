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


#include <QWidget>
#include <QChart>
#include <QChartView>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>


//^====================================================
//^====================================================
class Canvas : public QMainWindow{
  Q_OBJECT

public:
  Canvas(Digitizer ** digi, unsigned int nDigi, ReadDataThread ** readDataThread, QMainWindow * parent = nullptr);
  ~Canvas();


private:

  Digitizer ** digi;
  unsigned short nDigi;
  ReadDataThread ** readDataThread;   

  QChartView *chartView;
  QChart *chart;
  QBarSeries *barSeries;
  QBarCategoryAxis *axisX;

};


#endif