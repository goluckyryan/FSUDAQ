#ifndef SPLITPOLEANLAYZER_H
#define SPLITPOLEANLAYZER_H

/*********************************************
 * This is online analyzer for Split-Pole at FSU
 * 
 * It is a template for other analyzer.
 * 
 * Any new analyzer add to added to FSUDAQ.cpp
 * 1) add include header
 * 2) in OpenAnalyzer(), change the new 
 * 
 * add the source file in FSUDAQ_Qt6.pro then compile
 * >qmake6 FSUDAQ_Qt6.pro
 * >make
 * 
 * ******************************************/
#include "SplitPoleHit.h"
#include "Analyser.h"

//^===========================================
//^===========================================
class SplitPole : public Analyzer{
  Q_OBJECT
public:
  SplitPole(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr): Analyzer(digi, nDigi, parent){

    SetUpdateTimeInSec(1.0);

    RedefineEventBuilder({0}); // only build for the 0-th digitizer, otherwise, it will build event accross all digitizers
    tick2ns = digi[0]->GetTick2ns();
    SetBackwardBuild(false, 100); // using normal building (acceding in time) or backward building, int the case of backward building, default events to be build is 100. 
    evtbder = GetEventBuilder();
    evtbder->SetTimeWindow(500);
    
    //========== use the influx from the Analyzer
    influx = new InfluxDB("https://fsunuc.physics.fsu.edu/influx/");
    dataBaseName = "testing"; 

    SetUpCanvas();
    leTarget->setText("12C");
    leBeam->setText("d");
    leRecoil->setText("p");
    sbBfield->setValue(0.76);
    sbAngle->setValue(20);
    sbEnergy->setValue(16);

    hit.CalConstants(leTarget->text().toStdString(), 
                     leBeam->text().toStdString(), 
                     leRecoil->text().toStdString(), sbEnergy->value(), sbAngle->value());

    hit.CalZoffset(sbBfield->value()); 

    FillConstants();

    hit.ClearData();

  }

  /// ~SplitPole(); // comment out = defalt destructor

  void SetUpCanvas();
  void FillConstants();

public slots:
  void UpdateHistograms();

private:

  MultiBuilder *evtbder;

  // declaie histograms
  Histogram2D * hPID;

  Histogram1D * h1;
  Histogram1D * h1g;
  Histogram1D * hMulti;

  int tick2ns;

  SplitPoleHit hit;

  RSpinBox * sbBfield;
  QLineEdit * leTarget;
  QLineEdit * leBeam;
  QLineEdit * leRecoil;
  RSpinBox * sbEnergy;
  RSpinBox * sbAngle;

  QCheckBox * chkRunAnalyzer;

  QLineEdit * leMassTablePath;
  QLineEdit * leQValue;
  QLineEdit * leGSRho;
  QLineEdit * leZoffset;

  RSpinBox * sbRhoOffset;
  RSpinBox * sbRhoScale;

};

inline void SplitPole::FillConstants(){
  leQValue->setText(QString::number(hit.GetQ0()));
  leGSRho->setText(QString::number(hit.GetRho0()*1000));
  leZoffset->setText(QString::number(hit.GetZoffset()));
}


inline void SplitPole::SetUpCanvas(){

  setGeometry(0, 0, 1600, 1000);  

  {//^====== magnet and reaction setting
    QGroupBox * box = new QGroupBox("Configuration", this);
    layout->addWidget(box, 0, 0);
    QGridLayout * boxLayout = new QGridLayout(box);
    boxLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    box->setLayout(boxLayout);
    
    QLabel * lbBfield = new QLabel("B-field [T] ", box);
    lbBfield->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbBfield, 0, 2);
    sbBfield = new RSpinBox(box);
    sbBfield->setDecimals(3);
    sbBfield->setSingleStep(0.05);
    boxLayout->addWidget(sbBfield, 0, 3);

    QLabel * lbTarget = new QLabel("Target ", box);
    lbTarget->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbTarget, 0, 0);
    leTarget = new QLineEdit(box);
    boxLayout->addWidget(leTarget, 0, 1);

    QLabel * lbBeam = new QLabel("Beam ", box);
    lbBeam->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbBeam, 1, 0);
    leBeam = new QLineEdit(box);
    boxLayout->addWidget(leBeam, 1, 1);

    QLabel * lbRecoil = new QLabel("Recoil ", box);
    lbRecoil->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbRecoil, 2, 0);
    leRecoil = new QLineEdit(box);
    boxLayout->addWidget(leRecoil, 2, 1);

    QLabel * lbEnergy = new QLabel("Beam Energy [MeV] ", box);
    lbEnergy->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbEnergy, 1, 2);
    sbEnergy = new RSpinBox(box);
    sbEnergy->setDecimals(3);
    sbEnergy->setSingleStep(1.0);
    boxLayout->addWidget(sbEnergy, 1, 3);

    QLabel * lbAngle = new QLabel("SPS Angle [Deg] ", box);
    lbAngle->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbAngle, 2, 2);
    sbAngle = new RSpinBox(box);
    sbAngle->setDecimals(3);
    sbAngle->setSingleStep(1.0);
    boxLayout->addWidget(sbAngle, 2, 3);

    boxLayout->setColumnStretch(0, 1);
    boxLayout->setColumnStretch(1, 2);
    boxLayout->setColumnStretch(2, 1);
    boxLayout->setColumnStretch(3, 2);

    connect(leTarget, &QLineEdit::returnPressed, this, [=](){
      hit.CalConstants(leTarget->text().toStdString(), 
                       leBeam->text().toStdString(), 
                       leRecoil->text().toStdString(), sbEnergy->value(), sbAngle->value() );
      hit.CalZoffset(sbBfield->value());  
      FillConstants();
    });
    
    connect(leBeam, &QLineEdit::returnPressed, this, [=](){
      hit.CalConstants(leTarget->text().toStdString(), 
                       leBeam->text().toStdString(), 
                       leRecoil->text().toStdString(), sbEnergy->value(), sbAngle->value());
      hit.CalZoffset(sbBfield->value());  
      FillConstants();
    });

    connect(leRecoil, &QLineEdit::returnPressed, this, [=](){
      hit.CalConstants(leTarget->text().toStdString(), 
                       leBeam->text().toStdString(), 
                       leRecoil->text().toStdString(), sbEnergy->value(), sbAngle->value());
      hit.CalZoffset(sbBfield->value());  
      FillConstants();
    });

    connect(sbBfield, &RSpinBox::returnPressed, this, [=](){
      hit.CalConstants(leTarget->text().toStdString(), 
                       leBeam->text().toStdString(), 
                       leRecoil->text().toStdString(), sbEnergy->value(), sbAngle->value());
      hit.CalZoffset(sbBfield->value());  
      FillConstants();
    });

    connect(sbAngle, &RSpinBox::returnPressed, this, [=](){
      hit.CalConstants(leTarget->text().toStdString(), 
                       leBeam->text().toStdString(), 
                       leRecoil->text().toStdString(), sbEnergy->value(), sbAngle->value());
      hit.CalZoffset(sbBfield->value());  
      FillConstants();
    });

    connect(sbEnergy, &RSpinBox::returnPressed, this, [=](){
      hit.CalConstants(leTarget->text().toStdString(), 
                       leBeam->text().toStdString(), 
                       leRecoil->text().toStdString(), sbEnergy->value(), sbAngle->value());
      hit.CalZoffset(sbBfield->value());  
      FillConstants();
    });

    chkRunAnalyzer = new QCheckBox("Run Analyzer", this);
    boxLayout->addWidget(chkRunAnalyzer, 4, 1);


    QFrame *separator = new QFrame(box);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    boxLayout->addWidget(separator, 5, 0, 1, 4);


    QLabel * lbMassTablePath = new QLabel("Mass Table Path : ", box);
    lbMassTablePath->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbMassTablePath, 6, 0);
    leMassTablePath = new QLineEdit(QString::fromStdString(massData),box);
    leMassTablePath->setReadOnly(true);
    boxLayout->addWidget(leMassTablePath, 6, 1, 1, 3);

    QLabel * lbQValue = new QLabel("Q-Value [MeV] ", box);
    lbQValue->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbQValue, 7, 0);
    leQValue = new QLineEdit(box);
    leQValue->setReadOnly(true);
    boxLayout->addWidget(leQValue, 7, 1);

    QLabel * lbGDRho = new QLabel("G.S. Rho [mm] ", box);
    lbGDRho->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbGDRho, 7, 2);
    leGSRho = new QLineEdit(box);
    leGSRho->setReadOnly(true);
    boxLayout->addWidget(leGSRho, 7, 3);

    QLabel * lbZoffset = new QLabel("Z-offset [mm] ", box);
    lbZoffset->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbZoffset, 8, 0);
    leZoffset = new QLineEdit(box);
    leZoffset->setReadOnly(true);
    boxLayout->addWidget(leZoffset, 8, 1);


    QFrame *separator1 = new QFrame(box);
    separator1->setFrameShape(QFrame::HLine);
    separator1->setFrameShadow(QFrame::Sunken);
    boxLayout->addWidget(separator1, 9, 0, 1, 4);


    QLabel * lbRhoOffset = new QLabel("Rho-offset [mm] ", box);
    lbRhoOffset->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbRhoOffset, 10, 0);
    sbRhoOffset = new RSpinBox(box);
    sbRhoOffset->setDecimals(2);
    sbRhoOffset->setSingleStep(1);
    sbRhoOffset->setValue(0);
    boxLayout->addWidget(sbRhoOffset, 10, 1);

    QLabel * lbRhoScale = new QLabel("Rho-Scaling ", box);
    lbRhoScale->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    boxLayout->addWidget(lbRhoScale, 10, 2);
    sbRhoScale = new RSpinBox(box);
    sbRhoScale->setDecimals(2);
    sbRhoScale->setSingleStep(0.01);
    sbRhoScale->setMinimum(0.5);
    sbRhoScale->setMaximum(1.5);
    sbRhoScale->setValue(1.0);
    boxLayout->addWidget(sbRhoScale, 10, 3);


    QFrame *separator2 = new QFrame(box);
    separator2->setFrameShape(QFrame::HLine);
    separator2->setFrameShadow(QFrame::Sunken);
    boxLayout->addWidget(separator2, 11, 0, 1, 4);

    QString chMapStr = "ScinR = " + QString::number(SPS::ChMap::ScinR);
    chMapStr += ", ScinL = " +      QString::number(SPS::ChMap::ScinL);
    chMapStr += ", dFR = " +        QString::number(SPS::ChMap::dFR);
    chMapStr += ", dFL = " +        QString::number(SPS::ChMap::dFL);
    chMapStr += ", dBR = " +        QString::number(SPS::ChMap::dBR);
    chMapStr += ", dBL = " +        QString::number(SPS::ChMap::dBL);
    chMapStr += ", Cathode = " +    QString::number(SPS::ChMap::Cathode);
    chMapStr += ", AnodeF = " +     QString::number(SPS::ChMap::AnodeF);
    chMapStr += ", AnodeB = " +     QString::number(SPS::ChMap::AnodeB);
    QLabel * chMapLabel = new QLabel(chMapStr, box);
    boxLayout->addWidget(chMapLabel, 12, 0, 1, 4);

  }

  //============ histograms
  hMulti = new Histogram1D("Multiplicity", "", 10, 0, 10, this);
  layout->addWidget(hMulti, 0, 1);  

  // the "this" make the histogram a child of the SplitPole class. When SplitPole destory, all childs destory as well.
  hPID = new Histogram2D("Split Pole PID", "Scin-L", "Anode-Font", 100, 0, 5000, 100, 0, 5000, this);
  //layout is inheriatge from Analyzer
  layout->addWidget(hPID, 1, 0, 2, 1);

  h1 = new Histogram1D("Spectrum", "x", 300, 30, 70, this);
  h1->SetColor(Qt::darkGreen);
  //h1->AddDataList("Test", Qt::red); // add another histogram in h1, Max Data List is 10
  layout->addWidget(h1, 1, 1);
  
  h1g = new Histogram1D("Spectrum (PID gated)", "Ex", 300, -2, 10, this);
  layout->addWidget(h1g, 2, 1);

  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(1, 1);

}

inline void SplitPole::UpdateHistograms(){

  if( this->isVisible() == false ) return;
  if( chkRunAnalyzer->isChecked() == false ) return;

  BuildEvents(); // call the event builder to build events

  //============ Get events, and do analysis
  long eventBuilt = evtbder->eventBuilt;
  if( eventBuilt == 0 ) return;

  //============ Get the cut list, if any
  QList<QPolygonF> cutList = hPID->GetCutList();
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

    hit.ClearData();

    for( int k = 0; k < (int) event.size(); k++ ){
      //event[k].Print();
      if( event[k].ch == SPS::ChMap::ScinR ) {hit.eSR = event[k].energy; hit.tSR = event[k].timestamp;}
      if( event[k].ch == SPS::ChMap::ScinL ) {hit.eSL = event[k].energy; hit.tSL = event[k].timestamp;}
      if( event[k].ch == SPS::ChMap::dFR )   {hit.eFR = event[k].energy; hit.tFR = event[k].timestamp;}
      if( event[k].ch == SPS::ChMap::dFL )   {hit.eFL = event[k].energy; hit.tFL = event[k].timestamp;}
      if( event[k].ch == SPS::ChMap::dBR )   {hit.eBL = event[k].energy; hit.tBL = event[k].timestamp;}
      if( event[k].ch == SPS::ChMap::dBL )   {hit.eBL = event[k].energy; hit.tBL = event[k].timestamp;}
      if( event[k].ch == SPS::ChMap::Cathode )   {hit.eCath = event[k].energy; hit.tCath = event[k].timestamp;}
      if( event[k].ch == SPS::ChMap::AnodeF )   {hit.eAF = event[k].energy; hit.tAF = event[k].timestamp;}
      if( event[k].ch == SPS::ChMap::AnodeB )   {hit.eAB = event[k].energy; hit.tAB = event[k].timestamp;}
    }

    hit.CalData();

    double pidX = hit.eSL;
    unsigned long long tPidX = hit.tSL;
    double pidY = hit.eAF;

    hPID->Fill(pidX, pidY); // x, y

    h1->Fill(hit.xAvg);
    //h1->Fill(hit.eSR, 1);

    //check events inside any Graphical cut and extract the rate, using tSR only
    for(int p = 0; p < cutList.count(); p++ ){ 
      if( cutList[p].isEmpty() ) continue;
      if( cutList[p].containsPoint(QPointF(pidX, pidY), Qt::OddEvenFill) ){
        if( tPidX < tMin[p] ) tMin[p] = tPidX;
        if( tPidX > tMax[p] ) tMax[p] = tPidX;
        count[p] ++;
        //printf(".... %d \n", count[p]);
        if( p == 0 ) {
          double xAvg = hit.xAvg * 10;
          double xAvgC = xAvg * sbRhoScale->value() + sbRhoOffset->value();
          h1g->Fill(hit.Rho2Ex(xAvgC/1000.));
        }
      }
    }
  }

  hPID->UpdatePlot();
  h1->UpdatePlot();
  hMulti->UpdatePlot();
  h1g->UpdatePlot();

  QList<QString> cutNameList = hPID->GetCutNameList();
  for( int p = 0; p < cutList.count(); p ++){
    if( cutList[p].isEmpty() ) continue;
    double dT = (tMax[p]-tMin[p]) * tick2ns / 1e9; // tick to sec
    double rate = count[p]*1.0/(dT);
    //printf("%llu %llu, %f %d\n", tMin[p], tMax[p], dT, count[p]);
    //printf("%10s | %d | %f Hz \n", cutNameList[p].toStdString().c_str(), count[p], rate);  
    
    influx->AddDataPoint("Cut,name=" + cutNameList[p].toStdString()+ " value=" + std::to_string(rate));
    influx->WriteData(dataBaseName);
    influx->ClearDataPointsBuffer();
  }

}


#endif