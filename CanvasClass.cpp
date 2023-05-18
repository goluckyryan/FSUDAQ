#include "CanvasClass.h"

#include <QValueAxis>
#include <QRandomGenerator>
#include <QGroupBox>
#include <QStandardItemModel>
#include <QLabel>

Canvas::Canvas(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent) : QMainWindow(parent){

  this->digi = digi;
  this->nDigi = nDigi;

  setWindowTitle("Canvas");
  setGeometry(0, 0, 1000, 800);  
  //setWindowFlags( this->windowFlags() & ~Qt::WindowCloseButtonHint );

  QWidget * layoutWidget = new QWidget(this);
  setCentralWidget(layoutWidget);
  QVBoxLayout * layout = new QVBoxLayout(layoutWidget);
  layoutWidget->setLayout(layout);


  //========================
  QGroupBox * controlBox = new QGroupBox("Control", this);
  layout->addWidget(controlBox);
  QGridLayout * ctrlLayout = new QGridLayout(controlBox);
  controlBox->setLayout(ctrlLayout);

  QPushButton * bnClearHist = new QPushButton("Clear Hist.", this);
  ctrlLayout->addWidget(bnClearHist, 0, 0);

  //========================
  QGroupBox * histBox = new QGroupBox("Histgrams", this);
  layout->addWidget(histBox);
  QGridLayout * histLayout = new QGridLayout(histBox);
  histBox->setLayout(histLayout);

  double xMax = 4000;
  double xMin = 0;
  double nBin = 100;

  Histogram * hist = new Histogram("test", xMin, xMax, nBin);

  hist->Fill(1);
  hist->Fill(300);
  hist->Fill(350);

  TraceView * plotView = new TraceView(hist->GetTrace(), this);
  histLayout->addWidget(plotView, 0, 0);

}

Canvas::~Canvas(){

}

void Canvas::UpdateCanvas(){


  


}