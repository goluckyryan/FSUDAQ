#include "CanvasClass.h"

#include <QValueAxis>
#include <QRandomGenerator>
#include <QGroupBox>
#include <QStandardItemModel>
#include <QLabel>

Canvas::Canvas(Digitizer ** digi, unsigned int nDigi, ReadDataThread ** readDataThread, QMainWindow * parent) : QMainWindow(parent){

  this->digi = digi;
  this->nDigi = nDigi;
  this->readDataThread = readDataThread;

  setWindowTitle("Canvas");
  setGeometry(0, 0, 1000, 800);  
  //setWindowFlags( this->windowFlags() & ~Qt::WindowCloseButtonHint );


  QVector<int> data = {1, 1, 2, 2, 3, 1, 2, 4, 5, 6, 5, 3, 4, 1, 3};
  double xMax = 5;
  double xMin = -1;
  double binSize = 0.5;

  // Calculate the number of bins based on the X-axis range and bin size
  int numBins = static_cast<int>((xMax - xMin) / binSize) + 1;

  // Create a bar series
  barSeries = new QBarSeries();

  // Create a histogram data array with the number of bins
  QVector<int> histogramData(numBins, 0);

  // Calculate the histogram bin counts
  for (double value : data) {
      int binIndex = static_cast<int>((value - xMin) / binSize);
      if (binIndex >= 0 && binIndex < numBins) {
          histogramData[binIndex]++;
      }
  }

  // Create bar sets and add them to the series
  for (int i = 0; i < numBins; ++i) {
      double binStart = xMin + i * binSize;
      double binEnd = binStart + binSize;
      QString binLabel = QString("%1-%2").arg(binStart).arg(binEnd);

      QBarSet *barSet = new QBarSet(binLabel);
      *barSet << histogramData[i];
      barSeries->append(barSet);
  }

  // Create the chart and set the series
  chart = new QChart();
  chart->addSeries(barSeries);

  // Create the X-axis category axis
  axisX = new QBarCategoryAxis();
  chart->setAxisX(axisX, barSeries);

  // Create category labels for the bins
  QStringList categories;
  for (int i = 0; i < numBins; ++i) {
      double binStart = xMin + i * binSize;
      double binEnd = binStart + binSize;
      QString binLabel = QString("%1-%2").arg(binStart).arg(binEnd);
      categories.append(binLabel);
      axisX->append(binLabel);
  }

  // Create a chart view and set the chart
  chartView = new QChartView(chart);
  chartView->setRenderHint(QPainter::Antialiasing);

  // Set the chart view as the main widget
  setCentralWidget(chartView);
}

Canvas::~Canvas(){

}