#ifndef CustomWidgets_H
#define CustomWidgets_H

#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QWheelEvent>

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


#endif