#ifndef HISTOGRAM_2D_H
#define HISTOGRAM_2D_H

#include "qcustomplot.h"

const QList<QColor> colorCycle = { QColor(Qt::red),
                                   QColor(Qt::blue), 
                                   QColor(Qt::darkGreen), 
                                   QColor(Qt::darkCyan), 
                                   QColor(Qt::darkYellow), 
                                   QColor(Qt::magenta), 
                                   QColor(Qt::darkMagenta),
                                   QColor(Qt::gray)}; 

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
    plotLayout()->insertRow(0);
    plotLayout()->addElement(0, 0, titleEle);

    colorScale = new QCPColorScale(this);
    plotLayout()->addElement(1, 1, colorScale); 
    colorScale->setType(QCPAxis::atRight); 
    colorMap->setColorScale(colorScale);

    QCPColorGradient color;
    color.clearColorStops();
    color.setColorStopAt( 0,  QColor("white" ));
    color.setColorStopAt( 0.5,  QColor("blue"));
    color.setColorStopAt( 1,  QColor("yellow"));
    colorMap->setGradient(color);

    cutList.clear();

    double xPosStart = 0.02;
    double xPosStep = 0.07;
    double yPosStart = 0.02;
    double yPosStep = 0.05;

    for( int i = 0; i < 3; i ++ ){
      for( int j = 0; j < 3; j ++ ){
        entry[i][j] = 0;
        box[i][j] = new QCPItemRect(this);

        box[i][j]->topLeft->setType(QCPItemPosition::ptAxisRectRatio);
        box[i][j]->topLeft->setCoords(xPosStart + xPosStep*i, yPosStart + yPosStep*j);
        box[i][j]->bottomRight->setType(QCPItemPosition::ptAxisRectRatio);
        box[i][j]->bottomRight->setCoords(xPosStart + xPosStep*(i+1), yPosStart + yPosStep*(j+1));

        txt[i][j] = new QCPItemText(this);
        txt[i][j]->setPositionAlignment(Qt::AlignLeft);
        txt[i][j]->position->setType(QCPItemPosition::ptAxisRectRatio);
        txt[i][j]->position->setCoords(xPosStart + xPosStep/2 + xPosStep*i, yPosStart + yPosStep*j);;
        txt[i][j]->setText("0");
        txt[i][j]->setFont(QFont("Helvetica", 9));

      }
    }


    rescaleAxes();

    usingMenu = false;
    isDrawCut = false;

    connect(this, &QCustomPlot::mouseMove, this, [=](QMouseEvent *event){
      double x = xAxis->pixelToCoord(event->pos().x());
      double y = yAxis->pixelToCoord(event->pos().y());
      int xI, yI;
      colorMap->data()->coordToCell(x, y, &xI, &yI);
      double z = colorMap->data()->cell(xI, yI);

      QString coordinates = QString("X: %1, Y: %2, Z: %3").arg(x).arg(y).arg(z);
      QToolTip::showText(event->globalPosition().toPoint(), coordinates, this);

      //TODO, when drawing cut, show dashhed line


    });

    connect(this, &QCustomPlot::mousePress, this, [=](QMouseEvent * event){
      if (event->button() == Qt::LeftButton && !usingMenu && !isDrawCut){
        setSelectionRectMode(QCP::SelectionRectMode::srmZoom);
      }
      if (event->button() == Qt::LeftButton && isDrawCut){

        double x = xAxis->pixelToCoord(event->pos().x());
        double y = yAxis->pixelToCoord(event->pos().y());

        tempCut.push_back(QPointF(x,y));

        DrawCut();
      }

      if (event->button() == Qt::RightButton) {
        usingMenu = true;
        setSelectionRectMode(QCP::SelectionRectMode::srmNone);

        QMenu *menu = new QMenu(this);
        menu->setAttribute(Qt::WA_DeleteOnClose);

        QAction * a1 = menu->addAction("UnZoom");
        QAction * a2 = menu->addAction("Clear hist.");
        QAction * a3 = menu->addAction("Toggle Stat.");
        QAction * a4 = menu->addAction("Create a Cut");
        for( int i = 0; i < cutList.size(); i++){
          menu->addAction("Delete Cut-" + QString::number(i));
        }

        QAction *selectedAction = menu->exec(event->globalPosition().toPoint());

        if( selectedAction == a1 ){
          xAxis->setRangeLower(xMin);
          xAxis->setRangeUpper(xMax);
          yAxis->setRangeLower(yMin);
          yAxis->setRangeUpper(yMax);
          replot();
          usingMenu = false;
        }

        if( selectedAction == a2 ) {
          Clear();
          usingMenu = false;
        }

        if( selectedAction == a3 ){
          for( int i = 0; i < 3; i ++ ){
            for( int j = 0; j < 3; j ++ ){
              box[i][j]->setVisible( !box[i][j]->visible());
              txt[i][j]->setVisible( !txt[i][j]->visible());
            }
          }
          replot();
          usingMenu = false;
        }

        if( selectedAction == a4 ){
          tempCut.clear();
          isDrawCut= true;
          usingMenu = false;
        }

        // if( selectedAction->text().contains("Delete Cut") ){
        //   qDebug() << "delete Cut";
        // }

      }
    });

    //connect( this, &QCustomPlot::mouseDoubleClick, this, [=](QMouseEvent *event){
    connect( this, &QCustomPlot::mouseDoubleClick, this, [=](){
      isDrawCut = false;
      tempCut.push_back(tempCut[0]);
      cutList.push_back(tempCut);
      DrawCut();
    });

    connect(this, &QCustomPlot::mouseRelease, this, [=](){


    });
  }

  void UpdatePlot(){
    colorMap->rescaleDataRange();
    rescaleAxes();
    replot();
  }

  void Clear(){
    colorMap->data()->clear();
    for( int i = 0; i < 3; i ++){
      for( int j = 0; j < 3; j ++){
        entry[i][j] = 0;
        txt[i][j]->setText("0");
      }
    }
    UpdatePlot();
  }

  void Fill(double x, double y){
    int xIndex, yIndex;
    colorMap->data()->coordToCell(x, y, &xIndex, &yIndex);
    //printf("%f,  %d  %d| %f, %d %d\n", x, xIndex, xBin, y, yIndex, yBin);

    int xk = 1, yk = 1;
    if( xIndex < 0 ) xk = 0;
    if( xIndex >= xBin ) xk = 2;
    if( yIndex < 0 ) yk = 0;
    if( yIndex >= yBin ) yk = 2;
    entry[xk][yk] ++;

    txt[xk][yk]->setText(QString::number(entry[xk][yk]));

    if( xk == 1 && yk == 1 ) {
      double value = colorMap->data()->cell(xIndex, yIndex);
      colorMap->data()->setCell(xIndex, yIndex, value + 1);
    }
  }

  void DrawCut(){

    //TODO how to delete cut from plot and from QList

    plottableCount();

    //TODO how to not double plot?

    if(tempCut.size() > 1) {
        QCPCurve *polygon = new QCPCurve(xAxis, yAxis);

        QPen pen(Qt::blue);
        pen.setWidth(1);
        polygon->setPen(pen);

        QVector<QCPCurveData> dataPoints;
        for (const QPointF& point : tempCut) {
            dataPoints.append(QCPCurveData(dataPoints.size(), point.x(), point.y()));
        }
        polygon->data()->set(dataPoints, false);
    }

    replot();
  }

  QList<QPolygonF> GetCutList() const{return cutList;}

private:
  double xMin, xMax, yMin, yMax;
  int xBin, yBin;

  QCPColorMap * colorMap;
  QCPColorScale *colorScale;

  bool usingMenu;

  int entry[3][3]; // overflow counter, entrt[1][1] is entry in the plot;

  QCPItemRect * box[3][3];
  QCPItemText * txt[3][3];
  
  QPolygonF tempCut;
  QList<QPolygonF> cutList;
  bool isDrawCut;

};

#endif