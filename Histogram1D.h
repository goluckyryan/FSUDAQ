#ifndef HISTOGRAM_1D_H
#define HISTOGRAM_1D_H

#include "qcustomplot.h"

#define MaxNHist 10

//^==============================================
//^==============================================
class Histogram1D : public QCustomPlot{
  Q_OBJECT
public:
  Histogram1D(QString title, QString xLabel, int xbin, double xmin, double xmax, QWidget * parent = nullptr) : QCustomPlot(parent){

    isLogY = false;

    for( int i = 0; i < 3; i ++) txt[i] = nullptr;
    nData = 1;
    Rebin(xbin, xmin, xmax);

    xAxis->setLabel(xLabel);

    legend->setVisible(true);
    QPen borderPen = legend->borderPen();
    borderPen.setWidth(0);
    borderPen.setColor(Qt::transparent);
    legend->setBorderPen(borderPen);
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

    graph(0)->setData(xList, yList[0]);

    //setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    //setInteractions( QCP::iRangeDrag | QCP::iRangeZoom );

    //setSelectionRectMode(QCP::SelectionRectMode::srmZoom);

    rescaleAxes();
    yAxis->setRangeLower(0);
    yAxis->setRangeUpper(10);

    for( int i = 0; i < 3; i ++){
      txt[i] = new QCPItemText(this);
      txt[i]->setPositionAlignment(Qt::AlignLeft);
      txt[i]->position->setType(QCPItemPosition::ptAxisRectRatio);
      txt[i]->position->setCoords(0.1, 0.1 + 0.1*i);;
      txt[i]->setFont(QFont("Helvetica", 9));
    }
    txt[0]->setText("Under Flow : 0");
    txt[1]->setText("Total Entry : 0");
    txt[2]->setText("Over Flow : 0");

    usingMenu = false;

    connect(this, &QCustomPlot::mouseMove, this, [=](QMouseEvent *event){
      double x = this->xAxis->pixelToCoord(event->pos().x());
      double bin = (x - xMin)/dX;
      double z = yList[0][2*qFloor(bin) + 1];

      QString coordinates = QString("Bin: %1, Value: %2").arg(qFloor(bin)).arg(z);
      QToolTip::showText(event->globalPosition().toPoint(), coordinates, this);
    });

    connect(this, &QCustomPlot::mousePress, this, [=](QMouseEvent * event){
      if (event->button() == Qt::LeftButton && !usingMenu){
        setSelectionRectMode(QCP::SelectionRectMode::srmZoom);
      }
      if (event->button() == Qt::RightButton) {
        usingMenu = true;
        setSelectionRectMode(QCP::SelectionRectMode::srmNone);

        QMenu menu(this);

        QAction * a1 = menu.addAction("UnZoom");
        QAction * a5 = menu.addAction("Set/UnSet Log-y");
        QAction * a2 = menu.addAction("Clear hist.");
        QAction * a3 = menu.addAction("Toggle Stat.");
        QAction * a4 = menu.addAction("Rebin (clear histogram)");
        //TODO fitGuass

        QAction *selectedAction = menu.exec(event->globalPosition().toPoint());
        if( selectedAction == a1 ){
          xAxis->setRangeLower(xMin);
          xAxis->setRangeUpper(xMax);
          yAxis->setRangeLower(0);
          yAxis->setRangeUpper(yMax * 1.2 > 10 ? yMax * 1.2 : 10);
          replot();
          usingMenu = false;
        }

        if( selectedAction == a2 ){
          Clear();
          usingMenu = false;
        }

        if( selectedAction == a3 ){
          for( int i = 0; i < 3; i++){
            txt[i]->setVisible( !txt[i]->visible());
          }
          replot();
          usingMenu = false;
        }
        if( selectedAction == a4 ){
          QDialog dialog(this);
          dialog.setWindowTitle("Rebin histogram");

          QFormLayout layout(&dialog);

          QLabel * info = new QLabel(&dialog);
          info->setStyleSheet("color:red;");
          info->setText("This will also clear histogram!!");
          layout.addRow(info);

          QStringList nameList = {"Num. Bin", "x-Min", "x-Max"};
          QLineEdit* lineEdit[3];

          for (int i = 0; i < 3; ++i) {
              lineEdit[i] = new QLineEdit(&dialog);
              layout.addRow(nameList[i] + " : ", lineEdit[i]);
          }
          lineEdit[0]->setText(QString::number(xBin));
          lineEdit[1]->setText(QString::number(xMin));
          lineEdit[2]->setText(QString::number(xMax));

          QLabel * msg = new QLabel(&dialog);
          msg->setStyleSheet("color:red;");
          layout.addRow(msg);

          QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
          layout.addRow(&buttonBox);

          double number[3];

          QObject::connect(&buttonBox, &QDialogButtonBox::accepted, [&]() {
              int OKcount = 0;
              bool conversionOk = true;
              for( int i = 0; i < 3; i++ ){
                number[i] = lineEdit[i]->text().toDouble(&conversionOk);
                if( conversionOk ){
                  OKcount++;
                }else{
                  msg->setText(nameList[i] + " is invalid.");
                  return;
                }
              }

              if( OKcount == 3 ) {
                if( number[2] > number[1] ) {
                  dialog.accept();
                }else{
                  msg->setText(nameList[2] + " is smaller than " + nameList[1]);
                }
              }
          });
          QObject::connect(&buttonBox, &QDialogButtonBox::rejected, [&]() { dialog.reject();});

          if( dialog.exec() == QDialog::Accepted ){
            Rebin((int)number[0], number[1], number[2]);
            emit ReBinned();
            UpdatePlot();
          }

        }
        if( selectedAction == a5 ){

          if( !isLogY ){
            this->yAxis->setScaleType(QCPAxis::stLogarithmic);
            isLogY = true;
          }else{
            this->yAxis->setScaleType(QCPAxis::stLinear);
            isLogY = false;
          }
          this->replot();
        }
      }
    });

  }

  int    GetNBin() const {return xBin;}
  double GetXMin() const {return xMin;}
  double GetXMax() const {return xMax;}

  void SetColor(QColor color, unsigned short ID = 0) {
    graph(ID)->setPen(QPen(color));
    QColor haha = color;
    haha.setAlpha(20);
    graph(ID)->setBrush(QBrush(haha));
  }
  void AddDataList(QString title, QColor color){
    nData ++;
    addGraph();
    graph(nData - 1)->setName(title);
    SetColor(color, nData-1);
    yList[nData-1] = yList[0];
  }

  void UpdatePlot(){
    for( int ID = 0 ; ID < nData; ID ++) graph(ID)->setData(xList, yList[ID]);
    xAxis->setRangeLower(xMin);
    xAxis->setRangeUpper(xMax);
    yAxis->setRangeLower(0);
    yAxis->setRangeUpper(yMax * 1.2 > 10 ? yMax * 1.2 : 10);
    replot();
  }

  void Clear(){
    for( int ID = 0 ; ID < nData; ID ++) {
      for( int i = 0; i <= yList[ID].count(); i++) yList[ID][i] = 0;
    }
    yMax = 0;
    txt[0]->setText("Under Flow : 0");
    txt[1]->setText("Total Entry : 0");
    txt[2]->setText("Over Flow : 0");
    totalEntry = 0;
    underFlow = 0;
    overFlow = 0;
    UpdatePlot();
  }
  
  void SetXTitle(QString xTitle) { xAxis->setLabel(xTitle); }

  void Rebin(int xbin, double xmin, double xmax){
    xMin = xmin;
    xMax = xmax;
    xBin = xbin;

    dX = (xMax - xMin)/(xBin);

    xList.clear();
    for( int i = 0 ; i < nData ; i ++) yList[i].clear();

    for( int i = 0; i <= xBin; i ++ ){
      xList.append(xMin + i*dX-(dX)*0.000001); 
      xList.append(xMin + i*dX); 
      for( int ID = 0 ; ID < nData; ID ++ ){
        yList[ID].append(0); 
        yList[ID].append(0); 
      }
    }

    yMax = 0;

    totalEntry = 0;
    underFlow = 0;
    overFlow = 0;

    if( txt[0] ) txt[0]->setText("Under Flow : 0");
    if( txt[1] ) txt[1]->setText("Total Entry : 0");
    if( txt[2] ) txt[2]->setText("Over Flow : 0");
  }

  void Fill(double value, unsigned int ID = 0){
    if( ID == 0 ){
      totalEntry ++;
      txt[1]->setText("Total Entry : "+ QString::number(totalEntry));

      if( value < xMin ) {
        underFlow ++;
        txt[0]->setText("Under Flow : "+ QString::number(underFlow));
        return;
      }
      if( value > xMax ) {
        overFlow ++;
        txt[2]->setText("Over Flow : "+ QString::number(overFlow));
        return;
      }
    }

    double bin = (value - xMin)/dX;
    int index1 = 2*qFloor(bin) + 1;
    int index2 = index1 + 1;

    if( 0 <= index1 && index1 <= 2*xBin) yList[ID][index1] += 1;
    if( 0 <= index1 && index2 <= 2*xBin) yList[ID][index2] += 1;

    if( yList[ID][index1] > yMax ) yMax = yList[ID][index1];
  }

  void Print(unsigned int ID = 0){
    for( int i = 0; i < xList.count(); i++){
      printf("%f  %f\n", xList[i], yList[ID][i]);
    }
  }

signals:
  void ReBinned(); //ONLY for right click rebin

private:
  double xMin, xMax, dX;
  int xBin;

  double yMax;

  int totalEntry;
  int underFlow;
  int overFlow;

  bool isLogY;

  unsigned short nData;
  QVector<double> xList;
  QVector<double> yList[MaxNHist];

  QCPItemText * txt[3];

  bool usingMenu;


};

#endif