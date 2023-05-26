#ifndef CustomWidgets_H
#define CustomWidgets_H

#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QWheelEvent>

#include <QChart>
#include <QChartView>
#include <QValueAxis>
#include <QRubberBand>
#include <QMouseEvent>
#include <QGestureEvent>
#include <QLineSeries>
#include <QAreaSeries>
#include <QValueAxis>

//^====================================================
class RSpinBox : public QDoubleSpinBox{
  Q_OBJECT
  public : 
    RSpinBox(QWidget * parent = nullptr, int decimal = 0): QDoubleSpinBox(parent){
      setFocusPolicy(Qt::StrongFocus);
      setDecimals(decimal);
    }

    void SetToolTip(double min = -1){
      if( min == -1 ){
        setToolTip("(" + QString::number(minimum()) + " - " + QString::number(maximum()) + ", " + QString::number(singleStep()) + ")");
      }else{
        setToolTip("(" + QString::number(min) + " - " + QString::number(maximum()) + ", " + QString::number(singleStep()) + ")");
      }
      setToolTipDuration(-1);
    }

  signals:
    void returnPressed();
  protected:
    void wheelEvent(QWheelEvent * event) override{ event->ignore(); }

    void keyPressEvent(QKeyEvent * event) override{
      if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit returnPressed();
      } else {
        QDoubleSpinBox::keyPressEvent(event);
      }
    }
};

//^====================================================
class RComboBox : public QComboBox{
  public : 
    RComboBox(QWidget * parent = nullptr): QComboBox(parent){
      setFocusPolicy(Qt::StrongFocus);
    }
  protected:
    void wheelEvent(QWheelEvent * event) override{ event->ignore(); }
};

//^====================================================
class RChart : public QChart{
public:
  explicit RChart(QGraphicsItem *parent = nullptr, Qt::WindowFlags wFlags = {})
    : QChart(QChart::ChartTypeCartesian, parent, wFlags){
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);
  }
  ~RChart(){}

protected:
  bool sceneEvent(QEvent *event){
    if (event->type() == QEvent::Gesture) return gestureEvent(static_cast<QGestureEvent *>(event));
    return QChart::event(event);
  }

private:
  bool gestureEvent(QGestureEvent *event){
    if (QGesture *gesture = event->gesture(Qt::PanGesture)) {
      QPanGesture *pan = static_cast<QPanGesture *>(gesture);
      QChart::scroll(-(pan->delta().x()), pan->delta().y());
    }

    if (QGesture *gesture = event->gesture(Qt::PinchGesture)) {
      QPinchGesture *pinch = static_cast<QPinchGesture *>(gesture);
      if (pinch->changeFlags() & QPinchGesture::ScaleFactorChanged) QChart::zoom(pinch->scaleFactor());
    }
    return true;
  }

private:

};

//^====================================================
class RChartView : public QChartView{
public:
  RChartView(QChart * chart, QWidget * parent = nullptr): QChartView(chart, parent){
    m_isTouching = false;
    this->setRubberBand(QChartView::RectangleRubberBand);

    m_coordinateLabel = new QLabel(this);
    m_coordinateLabel->setStyleSheet("QLabel { color : black; }");
    m_coordinateLabel->setVisible(false);
    m_coordinateLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    setMouseTracking(true);

    setRenderHints(QPainter::Antialiasing);

    vRangeMin = -(0x1FFF);
    vRangeMax = 0x1FFF;

    hRangeMin = 0;
    hRangeMax = 0;
  }

  void SetHRange(int min, int max) {
    this->hRangeMin = min;
    this->hRangeMax = max;
  }  
  void SetVRange(int min, int max) {
    this->vRangeMin = min;
    this->vRangeMax = max;
  }
protected:
  bool viewportEvent(QEvent *event) override{
    if (event->type() == QEvent::TouchBegin) {
      m_isTouching = true;
      chart()->setAnimationOptions(QChart::NoAnimation);
    }
    return QChartView::viewportEvent(event);
  }
  void mousePressEvent(QMouseEvent *event) override{
    if (m_isTouching) return;
    QChartView::mousePressEvent(event);
  }
  void mouseMoveEvent(QMouseEvent *event) override{
    QPointF chartPoint = this->chart()->mapToValue(event->pos());
    QString coordinateText = QString("x: %1, y: %2").arg(QString::number(chartPoint.x(), 'f', 0)).arg(QString::number(chartPoint.y(), 'f', 0));
    m_coordinateLabel->setText(coordinateText);
    m_coordinateLabel->move(event->pos() + QPoint(10, -10));
    m_coordinateLabel->setVisible(true);
    if (m_isTouching) return;
    QChartView::mouseMoveEvent(event);
  }
  void mouseReleaseEvent(QMouseEvent *event) override{
    if (m_isTouching)  m_isTouching = false;
    chart()->setAnimationOptions(QChart::SeriesAnimations);
    QChartView::mouseReleaseEvent(event);
  }
  void leaveEvent(QEvent *event) override {
    m_coordinateLabel->setVisible(false);
    QChartView::leaveEvent(event);
  }
  void keyPressEvent(QKeyEvent *event) override{
    switch (event->key()) {
      case Qt::Key_Plus:  chart()->zoomIn(); break;
      case Qt::Key_Minus: chart()->zoomOut(); break;
      case Qt::Key_Left:  chart()->scroll(-10, 0); break;
      case Qt::Key_Right: chart()->scroll(10, 0); break;
      case Qt::Key_Up: chart()->scroll(0, 10); break;
      case Qt::Key_Down: chart()->scroll(0, -10);  break;
      case Qt::Key_R : 
        //chart()->axes(Qt::Vertical).first()->setRange(-(0x1FFF), 0x1FFF);
        chart()->axes(Qt::Vertical).first()->setRange(vRangeMin, vRangeMax);
        if( hRangeMax != hRangeMin ) chart()->axes(Qt::Horizontal).first()->setRange(hRangeMin, hRangeMax);
        break;
      default: QGraphicsView::keyPressEvent(event); break;
    }
  }

private:
  bool m_isTouching;
  int hRangeMin;
  int hRangeMax;
  int vRangeMin;
  int vRangeMax;
  QLabel * m_coordinateLabel;
};

//^====================================================
class Histogram {
public:
  Histogram(QString title, double xMin, double xMax, int nBin){
  
    plot = new RChart();
    dataSeries = new QLineSeries();

    Rebin(xMin, xMax, nBin);

    maxBin = -1;
    maxBinValue = 0;

    //dataSeries->setPen(QPen(Qt::blue, 1));
    areaSeries = new QAreaSeries(dataSeries);
    areaSeries->setName(title);
    areaSeries->setBrush(Qt::blue);

    plot->addSeries(areaSeries);

    plot->setAnimationDuration(1); // msec
    plot->setAnimationOptions(QChart::NoAnimation);
    plot->createDefaultAxes();

    QValueAxis * xaxis = qobject_cast<QValueAxis*> (plot->axes(Qt::Horizontal).first());
    xaxis->setRange(xMin, xMax);
    xaxis->setTickCount( nBin +  1 > 11 ? 11 : nBin + 1);
    //xaxis->setLabelFormat("%.1f");
    //xaxis->setTitleText("Time [ns]");

    QValueAxis * yaxis = qobject_cast<QValueAxis*> (plot->axes(Qt::Vertical).first());
    yaxis->setRange(0, 10);

  }
  ~Histogram(){
    delete areaSeries;
    delete dataSeries;
    delete plot;
  }

  RChart * GetChart() { return plot;} 

  void Clear(){
    for( int i = 0; i <= nBin; i++) {
      dataSeries->replace(2*i, xMin + i * dX, 0);
      dataSeries->replace(2*i+1, xMin + i * dX, 0);
    }
  }

  void SetColor(Qt::GlobalColor color){ areaSeries->setBrush(color);}

  void Rebin(double xMin, double xMax, int nBin){
    dataSeries->clear();
    this->xMin = xMin;
    this->xMax = xMax;
    this->nBin = nBin;
    dX = (xMax-xMin)/nBin;
    for( int i = 0; i <= nBin; i++) {
      dataSeries->append(xMin + i * dX, 0 );
      dataSeries->append(xMin + i * dX, 0 );
    }
  }

  void Fill(double value){

    double bin = (value - xMin)/dX;
    if( bin < 0 || bin >= nBin ) return;

    int index1 = 2*qFloor(bin) + 1;
    int index2 = index1 + 1;

    QPointF point1 = dataSeries->at(index1);
    dataSeries->replace(index1, point1.x(), point1.y() + 1);

    QPointF point2 = dataSeries->at(index2);
    dataSeries->replace(index2, point2.x(), point2.y() + 1);

    if( point2.y() + 1 > maxBinValue ){
      maxBinValue = point2.y() + 1;
      maxBin = index2/2;
    }

    QValueAxis * yaxis = qobject_cast<QValueAxis*> (plot->axes(Qt::Vertical).first());
    yaxis->setRange(0, maxBinValue < 10 ? 10 : ((double)maxBinValue) * 1.2 );
    //yaxis->setTickInterval(1);
    //yaxis->setTickCount(10);
    //yaxis->setLabelFormat("%.0f");

  }

private:
  RChart * plot;
  QLineSeries * dataSeries;
  QAreaSeries * areaSeries;

  double dX, xMin, xMax;
  int nBin;

  int maxBin;
  int maxBinValue;

};

//^====================================================

#endif