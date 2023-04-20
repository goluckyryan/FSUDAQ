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

//^=======================================
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

//^=======================================
class RComboBox : public QComboBox{
  public : 
    RComboBox(QWidget * parent = nullptr): QComboBox(parent){
      setFocusPolicy(Qt::StrongFocus);
    }
  protected:
    void wheelEvent(QWheelEvent * event) override{ event->ignore(); }
};

//^====================================================
class Trace : public QChart{
public:
  explicit Trace(QGraphicsItem *parent = nullptr, Qt::WindowFlags wFlags = {})
    : QChart(QChart::ChartTypeCartesian, parent, wFlags){
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);
  }
  ~Trace(){}

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

class TraceView : public QChartView{
public:
  TraceView(QChart * chart, QWidget * parent = nullptr): QChartView(chart, parent){
    m_isTouching = false;
    this->setRubberBand(QChartView::RectangleRubberBand);

    m_coordinateLabel = new QLabel(this);
    m_coordinateLabel->setStyleSheet("QLabel { color : black; }");
    m_coordinateLabel->setVisible(false);
    m_coordinateLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    setMouseTracking(true);
  }

  void SetHRange(int min, int max) {
    this->hRangeMin = min;
    this->hRangeMax = max;
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
        chart()->axes(Qt::Vertical).first()->setRange(-(0x1FFF), 0x1FFF);
        //chart()->axes(Qt::Horizontal).first()->setRange(hRangeMin, hRangeMax);
        break;
      default: QGraphicsView::keyPressEvent(event); break;
    }
  }
  
private:
  bool m_isTouching;
  int hRangeMin;
  int hRangeMax;
  QLabel * m_coordinateLabel;
};



#endif