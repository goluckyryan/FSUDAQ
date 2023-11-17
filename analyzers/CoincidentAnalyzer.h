#ifndef COINCIDENTANLAYZER_H
#define COINCIDENTANLAYZER_H

#include "Analyser.h"

//^===========================================
class CoincidentAnalyzer : public Analyzer{
  Q_OBJECT
public:
  CoincidentAnalyzer(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr): Analyzer(digi, nDigi, parent){

    this->digi = digi;
    this->nDigi = nDigi;

    SetUpdateTimeInSec(1.0);

    //RedefineEventBuilder({0}); // only build for the 0-th digitizer, otherwise, it will build event accross all digitizers
    SetBackwardBuild(false, 100); // using normal building (acceding in time) or backward building, int the case of backward building, default events to be build is 100. 

    evtbder = GetEventBuilder();
    evtbder->SetTimeWindow(500);
    
    //========== use the influx from the Analyzer
    influx = new InfluxDB("https://fsunuc.physics.fsu.edu/influx/");
    dataBaseName = "testing"; 

    SetUpCanvas();

  }

  void SetUpCanvas();

public slots:
  void UpdateHistograms();

private:

  Digitizer ** digi;
  unsigned int nDigi;

  MultiBuilder *evtbder;

  // declaie histograms
  Histogram2D * h2D;
  Histogram1D * h1;
  Histogram1D * h1g;
  Histogram1D * hMulti;

  QCheckBox * chkRunAnalyzer;
  RSpinBox * sbUpdateTime;

  QCheckBox * chkBackWardBuilding;
  RSpinBox * sbBackwardCount;

  RSpinBox * sbBuildWindow;

  // data source for the h2D
  RComboBox * xDigi;
  RComboBox * xCh;
  RComboBox * yDigi;
  RComboBox * yCh;

  // data source for the h1
  RComboBox * aDigi;
  RComboBox * aCh;

};


inline void CoincidentAnalyzer::SetUpCanvas(){

  setGeometry(0, 0, 1600, 1000);  

  {//^====== magnet and reaction setting
    QGroupBox * box = new QGroupBox("Configuration", this);
    layout->addWidget(box, 0, 0);
    QGridLayout * boxLayout = new QGridLayout(box);
    boxLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    box->setLayout(boxLayout);

    chkRunAnalyzer = new QCheckBox("Run Analyzer", this);
    boxLayout->addWidget(chkRunAnalyzer, 0, 0);

    QLabel * lbUpdateTime = new QLabel("Update Period [s]", this);
    lbUpdateTime->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbUpdateTime, 0, 1);
    sbUpdateTime = new RSpinBox(this, 1);
    sbUpdateTime->setMinimum(0.1);
    sbUpdateTime->setMaximum(5);
    sbUpdateTime->setValue(1);
    boxLayout->addWidget(sbUpdateTime, 0, 2);

    connect(sbUpdateTime, &RSpinBox::valueChanged, this, [=](double sec){ SetUpdateTimeInSec(sec); });

    chkBackWardBuilding = new QCheckBox("Use Backward builder", this);
    boxLayout->addWidget(chkBackWardBuilding, 1, 0);

    QLabel * lbBKWindow = new QLabel("No. Backward Event", this);
    lbBKWindow->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbBKWindow, 1, 1);
    sbBackwardCount = new RSpinBox(this, 0);
    sbBackwardCount->setMinimum(1);
    sbBackwardCount->setMaximum(9999);
    sbBackwardCount->setValue(100);
    boxLayout->addWidget(sbBackwardCount, 1, 2);

    chkBackWardBuilding->setChecked(false);
    sbBackwardCount->setEnabled(false);

    connect(chkBackWardBuilding, &QCheckBox::stateChanged, this, [=](int status){
      SetBackwardBuild(status, sbBackwardCount->value());
      sbBackwardCount->setEnabled(status);
    });

    connect(sbBackwardCount, &RSpinBox::valueChanged, this, [=](double value){
      SetBackwardBuild(true, value);
    });

    QLabel * lbBuildWindow = new QLabel("Event Window [tick]", this);
    lbBuildWindow->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbBuildWindow, 2, 1);
    sbBuildWindow = new RSpinBox(this, 0);
    sbBuildWindow->setMinimum(1);
    sbBuildWindow->setMaximum(9999999999);
    boxLayout->addWidget(sbBuildWindow, 2, 2);

    connect(sbBuildWindow, &RSpinBox::valueChanged, this, [=](double value){
      evtbder->SetTimeWindow((int)value);
    });

    QFrame *separator = new QFrame(box);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    boxLayout->addWidget(separator, 3, 0, 1, 4);

    QLabel * lbXDigi = new QLabel("X-Digi", this);
    lbXDigi->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbXDigi, 4, 0);
    xDigi = new RComboBox(this);
    for(unsigned int i = 0; i < nDigi; i ++ ){
      xDigi->addItem("Digi-" +  QString::number(digi[i]->GetSerialNumber()), i);
    }
    boxLayout->addWidget(xDigi, 4, 1);

    QLabel * lbXCh = new QLabel("X-Ch", this);
    lbXCh->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbXCh, 4, 2);
    xCh = new RComboBox(this);
    for( int i = 0; i < digi[0]->GetNumInputCh(); i++) xCh->addItem("Ch-" + QString::number(i));
    boxLayout->addWidget(xCh, 4, 3);

    QLabel * lbYDigi = new QLabel("Y-Digi", this);
    lbYDigi->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbYDigi, 5, 0);
    yDigi = new RComboBox(this); 
    for(unsigned int i = 0; i < nDigi; i ++ ){
      yDigi->addItem("Digi-" +  QString::number(digi[i]->GetSerialNumber()), i);
    }
    boxLayout->addWidget(yDigi, 5, 1);

    QLabel * lbYCh = new QLabel("Y-Ch", this);
    lbYCh->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbYCh, 5, 2);
    yCh = new RComboBox(this);
    for( int i = 0; i < digi[0]->GetNumInputCh(); i++) yCh->addItem("Ch-" + QString::number(i));
    boxLayout->addWidget(yCh, 5, 3);

    QFrame *separator1 = new QFrame(box);
    separator1->setFrameShape(QFrame::HLine);
    separator1->setFrameShadow(QFrame::Sunken);
    boxLayout->addWidget(separator1, 6, 0, 1, 4);


    QLabel * lbaDigi = new QLabel("ID-Digi", this);
    lbaDigi->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbaDigi, 7, 0);
    aDigi = new RComboBox(this);
    for(unsigned int i = 0; i < nDigi; i ++ ){
      aDigi->addItem("Digi-" +  QString::number(digi[i]->GetSerialNumber()), i);
    }
    boxLayout->addWidget(yDigi, 7, 1);

    QLabel * lbaCh = new QLabel("1D-Ch", this);
    lbaCh->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbaCh, 7, 2);
    aCh = new RComboBox(this);
    for( int i = 0; i < digi[0]->GetNumInputCh(); i++) aCh->addItem("Ch-" + QString::number(i));
    boxLayout->addWidget(aCh, 7, 3);


  }

  //============ histograms
  hMulti = new Histogram1D("Multiplicity", "", 10, 0, 10, this);
  layout->addWidget(hMulti, 0, 1);  

  // the "this" make the histogram a child of the SplitPole class. When SplitPole destory, all childs destory as well.
  h2D = new Histogram2D("Coincident Plot", "XXX", "YYY", 100, 0, 5000, 100, 0, 5000, this);
  //layout is inheriatge from Analyzer
  layout->addWidget(h2D, 1, 0, 2, 1);

  h1 = new Histogram1D("1D Plot", "XXX", 300, 30, 70, this);
  h1->SetColor(Qt::darkGreen);
  h1->AddDataList("Test", Qt::red); // add another histogram in h1, Max Data List is 10
  layout->addWidget(h1, 1, 1);
  
  h1g = new Histogram1D("1D Plot (PID gated)", "XXX", 300, -2, 10, this);
  layout->addWidget(h1g, 2, 1);

  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(1, 1);

}

inline void CoincidentAnalyzer::UpdateHistograms(){

  if( this->isVisible() == false ) return;
  if( chkRunAnalyzer->isChecked() == false ) return;

  BuildEvents(); // call the event builder to build events

  //============ Get events, and do analysis
  long eventBuilt = evtbder->eventBuilt;
  if( eventBuilt == 0 ) return;

  //============ Get the cut list, if any
  QList<QPolygonF> cutList = h2D->GetCutList();
  const int nCut = cutList.count();
  unsigned long long tMin[nCut] = {0xFFFFFFFFFFFFFFFF}, tMax[nCut] = {0};
  unsigned int count[nCut]={0};

  //============ Processing data and fill histograms
  long eventIndex = evtbder->eventIndex;
  long eventStart = eventIndex - eventBuilt + 1;
  if(eventStart < 0 ) eventStart += MaxNEvent;
  
  for( long i = eventStart ; i <= eventIndex; i ++ ){
    std::vector<Hit> event = evtbder->events[i];
    //printf("-------------- %ld\n", i);

    hMulti->Fill((int) event.size());
    //if( event.size() < 9 ) return;
    if( event.size() == 0 ) return;


    for( int k = 0; k < (int) event.size(); k++ ){
      //event[k].Print();
    }

    //check events inside any Graphical cut and extract the rate, using tSR only
    for(int p = 0; p < cutList.count(); p++ ){ 
      if( cutList[p].isEmpty() ) continue;
      // if( cutList[p].containsPoint(QPointF(hit.eSL, hit.eSR), Qt::OddEvenFill) ){
      //   if( hit.tSR < tMin[p] ) tMin[p] = hit.tSR;
      //   if( hit.tSR > tMax[p] ) tMax[p] = hit.tSR;
      //   count[p] ++;
      //   //printf(".... %d \n", count[p]);
      //   if( p == 0 ) h1g->Fill(hit.eSR);
      // }
    }
  }

  h2D->UpdatePlot();
  h1->UpdatePlot();
  hMulti->UpdatePlot();
  h1g->UpdatePlot();

  QList<QString> cutNameList = h2D->GetCutNameList();
  for( int p = 0; p < cutList.count(); p ++){
    if( cutList[p].isEmpty() ) continue;
    // double dT = (tMax[p]-tMin[p]) * tick2ns / 1e9; // tick to sec
    // double rate = count[p]*1.0/(dT);
    //printf("%llu %llu, %f %d\n", tMin[p], tMax[p], dT, count[p]);
    //printf("%10s | %d | %f Hz \n", cutNameList[p].toStdString().c_str(), count[p], rate);  
    
    // influx->AddDataPoint("Cut,name=" + cutNameList[p].toStdString()+ " value=" + std::to_string(rate));
    // influx->WriteData(dataBaseName);
    // influx->ClearDataPointsBuffer();
  }

}


#endif