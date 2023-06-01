#ifndef CUSTOM_HIST_H
#define CUSTOM_HIST_H

#include "qcustomplot.h"

//^==============================================
//^==============================================
class Histogram1D : public QCustomPlot{
public:
  Histogram1D(QString title, QString xLabel, int xbin, double xmin, double xmax, QWidget * parent = nullptr) : QCustomPlot(parent){
    Rebin(xbin, xmin, xmax);

    xAxis->setLabel(xLabel);

    legend->setVisible(true);
    legend->setFont(QFont("Helvetica", 9));

    addGraph();
    graph(0)->setName(title);
    graph(0)->setPen(QPen(Qt::blue));
    graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20)));

    xAxis2->setVisible(true);
    xAxis2->setTickLabels(false);
    yAxis2->setVisible(true);
    yAxis2->setTickLabels(false);
    // make left and bottom axes always transfer their ranges to right and top axes:
    connect(xAxis, SIGNAL(rangeChanged(QCPRange)), xAxis2, SLOT(setRange(QCPRange)));
    connect(yAxis, SIGNAL(rangeChanged(QCPRange)), yAxis2, SLOT(setRange(QCPRange)));

    graph(0)->setData(xList, yList);

    //setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    setInteractions( QCP::iRangeDrag | QCP::iRangeZoom );

    rescaleAxes();
    yAxis->setRangeLower(0);
    yAxis->setRangeUpper(10);

    connect(this, &QCustomPlot::mouseMove, this, [=](QMouseEvent *event){
      double x = this->xAxis->pixelToCoord(event->pos().x());
      double bin = (x - xMin)/dX;
      double z = yList[2*qFloor(bin) + 1];

      QString coordinates = QString("Bin: %1, Value: %2").arg(qFloor(bin)).arg(z);
      QToolTip::showText(event->globalPosition().toPoint(), coordinates, this);
    });

    connect(this, &QCustomPlot::mousePress, this, [=](QMouseEvent * event){
      if (event->button() == Qt::RightButton) {
        QMenu *menu = new QMenu(this);
        menu->setAttribute(Qt::WA_DeleteOnClose);

        QAction * a1 = menu->addAction("UnZoom");
        //TODO QAction * a2 = menu->addAction("Fit Guass");

        QAction *selectedAction = menu->exec(event->globalPosition().toPoint());
        if( selectedAction == a1 ){
          xAxis->setRangeLower(xMin);
          xAxis->setRangeUpper(xMax);
          yAxis->setRangeLower(0);
          yAxis->setRangeUpper(yMax * 1.2 > 10 ? yMax * 1.2 : 10);
          replot();
        }
      }
    });

  }

  int    GetNBin() const {return xBin;}
  double GetXMin() const {return xMin;}
  double GetXMax() const {return xMax;}

  void UpdatePlot(){
    graph(0)->setData(xList, yList);
    xAxis->setRangeLower(xMin);
    xAxis->setRangeUpper(xMax);
    yAxis->setRangeLower(0);
    yAxis->setRangeUpper(yMax * 1.2 > 10 ? yMax * 1.2 : 10);
    replot();
  }

  void Clear(){
    for( int i = 0; i <= yList.count(); i++) yList[i] = 0;
    yMax = 0;
    UpdatePlot();
  }

  void Rebin(int xbin, double xmin, double xmax){
    xMin = xmin;
    xMax = xmax;
    xBin = xbin;

    dX = (xMax - xMin)/(xBin);

    xList.clear();
    yList.clear();

    for( int i = 0; i <= xBin; i ++ ){
      xList.append(xMin + i*dX-(dX)*0.000001); 
      xList.append(xMin + i*dX); 
      yList.append(0); 
      yList.append(0); 
    }

    yMax = 0;

    totalEntry = 0;
    underFlow = 0;
    overFlow = 0;
    
  }

  void Fill(double value){
    totalEntry ++;
    if( value < xMin ) underFlow ++;
    if( value > xMax ) overFlow ++;

    double bin = (value - xMin)/dX;
    int index1 = 2*qFloor(bin) + 1;
    int index2 = index1 + 1;

    if( 0 <= index1 && index1 <= 2*xBin) yList[index1] += 1;
    if( 0 <= index1 && index2 <= 2*xBin) yList[index2] += 1;

    if( yList[index1] > yMax ) yMax = yList[index1];
  }

  void Print(){
    for( int i = 0; i < xList.count(); i++){
      printf("%f  %f\n", xList[i], yList[i]);
    }
  }

private:
  double xMin, xMax, dX;
  int xBin;

  double yMax;

  int totalEntry;
  int underFlow;
  int overFlow;

  QVector<double> xList, yList;

};

//^==============================================
//^==============================================
class Histogram2D : public QCustomPlot{
public:
  Histogram2D(QString title, QString xLabel, QString yLabel, int xbin, double xmin, double xmax, int ybin, double ymin, double ymax, QWidget * parent = nullptr) : QCustomPlot(parent){
    xMin = xmin;
    xMax = xmax;
    yMin = ymin;
    yMax = ymax;
    xBin = xbin;
    yBin = ybin;

    axisRect()->setupFullAxesBox(true);
    xAxis->setLabel(xLabel);
    yAxis->setLabel(yLabel);

    colorMap = new QCPColorMap(xAxis, yAxis);
    colorMap->data()->setSize(xBin, yBin);
    colorMap->data()->setRange(QCPRange(xMin, xMax), QCPRange(yMin, yMax));
    colorMap->setInterpolate(false);

    QCPTextElement *titleEle = new QCPTextElement(this, title, QFont("sans", 12));
    plotLayout()->addElement(0, 0, titleEle);

    colorScale = new QCPColorScale(this);
    plotLayout()->addElement(0, 1, colorScale); 
    colorScale->setType(QCPAxis::atRight); 
    colorMap->setColorScale(colorScale);

    QCPColorGradient color;
    color.clearColorStops();
    color.setColorStopAt( 0,  QColor("white" ));
    color.setColorStopAt( 0.5,  QColor("blue"));
    color.setColorStopAt( 1,  QColor("yellow"));
    colorMap->setGradient(color);

    rescaleAxes();

    connect(this, &QCustomPlot::mouseMove, this, [=](QMouseEvent *event){
      double x = xAxis->pixelToCoord(event->pos().x());
      double y = yAxis->pixelToCoord(event->pos().y());
      int xI, yI;
      colorMap->data()->coordToCell(x, y, &xI, &yI);
      double z = colorMap->data()->cell(xI, yI);

      QString coordinates = QString("X: %1, Y: %2, Z: %3").arg(x).arg(y).arg(z);
      QToolTip::showText(event->globalPosition().toPoint(), coordinates, this);
    });


    connect(this, &QCustomPlot::mousePress, this, [=](QMouseEvent * event){
      if (event->button() == Qt::RightButton) {
        QMenu *menu = new QMenu(this);
        menu->setAttribute(Qt::WA_DeleteOnClose);

        QAction * a1 = menu->addAction("UnZoom");

        QAction *selectedAction = menu->exec(event->globalPosition().toPoint());

        if( selectedAction == a1 ){
          xAxis->setRangeLower(xMin);
          xAxis->setRangeUpper(xMax);
          yAxis->setRangeLower(yMin);
          yAxis->setRangeUpper(yMax);
          replot();
        }
      }
    });

  }

  void UpdatePlot(){
    // rescale the data dimension (color) such that all data points lie in the span visualized by the color gradient:
    colorMap->rescaleDataRange();
  
    // make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
    // QCPMarginGroup *marginGroup = new QCPMarginGroup(this);
    // this->axisRect()->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);
    // colorScale->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);
    
    // rescale the key (x) and value (y) axes so the whole color map is visible:
    rescaleAxes();

    replot();
  }

  void Fill(double x, double y){
    int xIndex, yIndex;
    colorMap->data()->coordToCell(x, y, &xIndex, &yIndex);
    //printf("%f,  %d  %d| %f, %d %d\n", x, xIndex, xBin, y, yIndex, yBin);
    if( xIndex < 0 || xBin < xIndex || yIndex < 0 || yBin < yIndex ) return;
    double value = colorMap->data()->cell(xIndex, yIndex);
    colorMap->data()->setCell(xIndex, yIndex, value + 1);
  }

private:
  double xMin, xMax, yMin, yMax;
  int xBin, yBin;

  QCPColorMap * colorMap;
  QCPColorScale *colorScale;

};

#endif