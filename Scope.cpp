#include "Scope.h"

#include <QValueAxis>
#include <QRandomGenerator>
#include <QGroupBox>
#include <QStandardItemModel>
#include <QLabel>


Scope::Scope(Digitizer ** digi, unsigned int nDigi, ReadDataThread ** readDataThread, QMainWindow * parent) : QMainWindow(parent){

  this->digi = digi;
  this->nDigi = nDigi;
  this->readDataThread = readDataThread;

  setWindowTitle("Scope");
  setGeometry(0, 0, 1000, 800);  
  setWindowFlags( this->windowFlags() & ~Qt::WindowCloseButtonHint );

  enableSignalSlot = false;

  plot = new Trace();
  for( int i = 0; i < MaxNumberOfTrace; i++) {
    dataTrace[i] = new QLineSeries();
    dataTrace[i]->setName("Trace " + QString::number(i));
    for(int j = 0; j < 100; j ++) dataTrace[i]->append(40*j, QRandomGenerator::global()->bounded(8000) + 8000);
    plot->addSeries(dataTrace[i]);
  }

  dataTrace[0]->setPen(QPen(Qt::red, 2));
  dataTrace[1]->setPen(QPen(Qt::blue, 2));
  dataTrace[2]->setPen(QPen(Qt::darkYellow, 1));
  dataTrace[3]->setPen(QPen(Qt::darkGreen, 1));

  plot->setAnimationDuration(1); // msec
  plot->setAnimationOptions(QChart::NoAnimation);
  plot->createDefaultAxes(); /// this must be after addSeries();
  /// this must be after createDefaultAxes();
  QValueAxis * yaxis = qobject_cast<QValueAxis*> (plot->axes(Qt::Vertical).first());
  QValueAxis * xaxis = qobject_cast<QValueAxis*> (plot->axes(Qt::Horizontal).first());
  yaxis->setTickCount(6);
  yaxis->setTickInterval(16384);
  yaxis->setRange(-16384, 65536);
  yaxis->setLabelFormat("%.0f");

  xaxis->setRange(0, 5000);
  xaxis->setTickCount(11);
  xaxis->setLabelFormat("%.0f");
  xaxis->setTitleText("Time [ns]");

  updateTraceThread = new UpdateTraceThread();
  //connect(updateTraceThread, &UpdateTraceThread::updateTrace, this, &Scope::UpdateScope);

  //*================================== UI
  int rowID = -1;

  QWidget * layoutWidget = new QWidget(this);
  setCentralWidget(layoutWidget);
  QGridLayout * layout = new QGridLayout(layoutWidget);
  layoutWidget->setLayout(layout);

  //-------------------- 
  rowID ++;
  cbScopeDigi = new RComboBox(this);
  cbScopeCh = new RComboBox(this);
  layout->addWidget(cbScopeDigi, rowID, 0);
  layout->addWidget(cbScopeCh, rowID, 1);


  //-------------------- Plot view
  rowID ++;
  TraceView * plotView = new TraceView(plot);
  plotView->setRenderHints(QPainter::Antialiasing);
  layout->addWidget(plotView, rowID, 0, 1, 6);

  //------------ close button
  rowID ++;
  bnScopeStart = new QPushButton("Start", this);
  layout->addWidget(bnScopeStart, rowID, 0);
  bnScopeStart->setEnabled(false);
  connect(bnScopeStart, &QPushButton::clicked, this, [=](){this->StartScope();});

  bnScopeStop = new QPushButton("Stop", this);
  layout->addWidget(bnScopeStop, rowID, 1);
  connect(bnScopeStop, &QPushButton::clicked, this, &Scope::StopScope);

  QLabel * lbTriggerRate = new QLabel("Trigger Rate [Hz] : ", this);
  lbTriggerRate->setAlignment(Qt::AlignCenter | Qt::AlignRight);
  layout->addWidget(lbTriggerRate, rowID, 2);

  leTriggerRate = new QLineEdit(this);
  leTriggerRate->setAlignment(Qt::AlignRight);
  leTriggerRate->setReadOnly(true);
  layout->addWidget(leTriggerRate, rowID, 3);

  QPushButton * bnClose = new QPushButton("Close", this);
  layout->addWidget(bnClose, rowID, 5);
  connect(bnClose, &QPushButton::clicked, this, &Scope::close);

}

Scope::~Scope(){

  updateTraceThread->Stop();
  updateTraceThread->quit();
  updateTraceThread->wait();
  delete updateTraceThread;
  for( int i = 0; i < MaxNumberOfTrace; i++) delete dataTrace[i];
  delete plot;
}

void Scope::StartScope(){


}

void Scope::StopScope(){

}

