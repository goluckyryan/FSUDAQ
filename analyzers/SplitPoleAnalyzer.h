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
#include "Analyser.h"
#include "Isotope.h"

#include <QRandomGenerator>

#include <cmath>
#include <random>

// static double randZeroToOne() {
//     return static_cast<double>(rand()) / RAND_MAX;
// }

// // Box-Muller transform to generate random Gaussian numbers
// static double generateGaussian(double mean, double stddev) {
//     static bool hasSpare = false;
//     static double spare;
//     if (hasSpare) {
//         hasSpare = false;
//         return mean + stddev * spare;
//     } else {
//         double u, v, s;
//         do {
//             u = 2.0 * randZeroToOne() - 1.0;
//             v = 2.0 * randZeroToOne() - 1.0;
//             s = u * u + v * v;
//         } while (s >= 1.0 || s == 0.0);

//         s = std::sqrt(-2.0 * std::log(s) / s);
//         spare = v * s;
//         hasSpare = true;
//         return mean + stddev * u * s;
//     }
// }

namespace SPS{
  namespace ChMap{

    const short ScinR = 0;
    const short ScinL = 1;
    const short dFR = 9;
    const short dFL = 8;
    const short dBR = 10;
    const short dBL = 11;
    const short Cathode = 7;
    const short AnodeF = 13;
    const short AnodeB = 15;

  };

  const double c = 299.792458; // mm/ns
  const double pi = M_PI;
  const double deg2rad = pi/180.;

}

class SplitPoleHit{

public:
  SplitPoleHit(){
    Clear();
  }

  unsigned int eSR; unsigned long long tSR;
  unsigned int eSL; unsigned long long tSL;
  unsigned int eFR; unsigned long long tFR;
  unsigned int eFL; unsigned long long tFL;
  unsigned int eBR; unsigned long long tBR;
  unsigned int eBL; unsigned long long tBL;
  unsigned int eCath; unsigned long long tCath;
  unsigned int eAF; unsigned long long tAF;
  unsigned int eAB; unsigned long long tAB;

  float eSAvg;
  float x1, x2, theta;
  float xAvg;

  void CalZoffset(QString targetStr, QString beamStr, QString recoilStr, double bfieldT, double angleDeg, double energyMeV){

    target.SetIsoByName(targetStr.toStdString());
    beam.SetIsoByName(beamStr.toStdString());
    recoil.SetIsoByName(recoilStr.toStdString());
    // target.SetIso(12, 6);
    // beam.SetIso(2,1);
    // recoil.SetIso(1,1);

    Bfield = bfieldT; // Tesla
    angleDegree = angleDeg; // degree
    beamKE = energyMeV; // MeV

    heavyRecoil.SetIso(target.A + beam.A - recoil.A, target.Z + beam.Z - recoil.Z);

    double Q = target.Mass + beam.Mass - recoil.Mass - heavyRecoil.Mass;

    double haha1 = sqrt(beam.Mass + beamKE + recoil.Mass)/(recoil.Mass + heavyRecoil.Mass) / cos(angleDegree * SPS::deg2rad);
    double haha2 = ( beamKE * ( heavyRecoil.Mass + beam.Mass)  + heavyRecoil.Mass * Q) / (recoil.Mass + heavyRecoil.Mass);

    double recoilKE = pow(haha1 + sqrt(haha1*haha1 + haha2), 2);

    printf("Q value : %f \n", Q);
    printf("proton enegry : %f \n", recoilKE);

    double recoilP = sqrt( recoilKE* ( recoilKE + 2*recoil.Mass));
    double rho = recoilP/(target.Z * Bfield * SPS::c); // in m
    double haha = sqrt( recoil.Mass * beam.Mass * beamKE / recoilKE );
    double k = haha * sin(angleDegree * SPS::deg2rad) / ( recoil.Mass + heavyRecoil.Mass - haha * cos(angleDegree * SPS::deg2rad));

    const double SPS_DISPERSION = 1.96; // x-position/rho
    const double SPS_MAGNIFICATION = 0.39; // in x-position

    zOffset = -1000.0 * rho * k * SPS_DISPERSION * SPS_MAGNIFICATION;

    printf("rho: %f m; z-offset: %f cm\n", rho, zOffset);

  }

  void Clear(){
    eSR = 0;  tSR = 0;
    eSL = 0;  tSL = 0;
    eFR = 0;  tFR = 0;
    eFL = 0;  tFL = 0;
    eBR = 0;  tBR = 0;
    eBL = 0;  tBL = 0;
    eCath = 0;  tCath = 0;
    eAF = 0;  tAF = 0;
    eAB = 0;  tAB = 0;

    eSAvg = -1;
    x1 = NAN;
    x2 = NAN;
    theta = NAN;
    xAvg = NAN;
  }

  void CalData(){

    if( eSR  > 0 && eSL  > 0 ) eSAvg = (eSR + eSL)/2;
    if( eSR  > 0 && eSL == 0 ) eSAvg = eSR;
    if( eSR == 0 && eSL  > 0 ) eSAvg = eSL;

    if( tFR > 0 && tFL > 0 ) x1 = (tFL - tFR)/2./2.1;
    if( tBR > 0 && tBL > 0 ) x2 = (tBL - tBR)/2./1.98;

    if( !std::isnan(x1)  && !std::isnan(x2)) {

      if( x2 > x1 ) {
        theta = atan((x2-x1)/36.0);
      }else if(x2 < x1){
        theta = SPS::pi + atan((x2-x1)/36.0);
      }else{
        theta = SPS::pi * 0.5;
      }

      double w1 = 0.5 - zOffset/4.28625;
      xAvg = w1 * x1 + (1-w1)* x2;

    }

  }

private:

  Isotope target;
  Isotope beam;
  Isotope recoil;
  Isotope heavyRecoil;

  double Bfield;
  double angleDegree;
  double beamKE;

  double zOffset;

};

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

    hit.CalZoffset(leTarget->text(), leBeam->text(), leRecoil->text(), sbBfield->value(), sbAngle->value(), sbEnergy->value()); 


    hit.Clear();

  }

  /// ~SplitPole(); // comment out = defalt destructor

  void SetUpCanvas();

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

  QCheckBox * runAnalyzer;

};


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
      hit.CalZoffset(leTarget->text(), leBeam->text(), leRecoil->text(), sbBfield->value(), sbAngle->value(), sbEnergy->value());  
    });
    
    connect(leBeam, &QLineEdit::returnPressed, this, [=](){
      hit.CalZoffset(leTarget->text(), leBeam->text(), leRecoil->text(), sbBfield->value(), sbAngle->value(), sbEnergy->value());  
    });

    connect(leRecoil, &QLineEdit::returnPressed, this, [=](){
      hit.CalZoffset(leTarget->text(), leBeam->text(), leRecoil->text(), sbBfield->value(), sbAngle->value(), sbEnergy->value());  
    });

    connect(sbBfield, &RSpinBox::returnPressed, this, [=](){
      hit.CalZoffset(leTarget->text(), leBeam->text(), leRecoil->text(), sbBfield->value(), sbAngle->value(), sbEnergy->value());  
    });

    connect(sbAngle, &RSpinBox::returnPressed, this, [=](){
      hit.CalZoffset(leTarget->text(), leBeam->text(), leRecoil->text(), sbBfield->value(), sbAngle->value(), sbEnergy->value());  
    });

    connect(sbEnergy, &RSpinBox::returnPressed, this, [=](){
      hit.CalZoffset(leTarget->text(), leBeam->text(), leRecoil->text(), sbBfield->value(), sbAngle->value(), sbEnergy->value());  
    });

    runAnalyzer = new QCheckBox("Run Analyzer", this);
    boxLayout->addWidget(runAnalyzer, 4, 1);

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
  h1->AddDataList("Test", Qt::red); // add another histogram in h1, Max Data List is 10
  layout->addWidget(h1, 1, 1);
  
  h1g = new Histogram1D("Spectrum (gated)", "x", 300, 30, 70, this);
  layout->addWidget(h1g, 2, 1);

  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(1, 1);

  //===========fill fake data
  // int min = 0;
  // int max = 8;

  // double meanX[9] = { 500,  500, 1000, 1000, 1000, 1500, 3000, 3000, 3000};
  // double stdX[9] =  { 100,  100,  300,  300,  300,  100,  500,  500,  500};
  // double meanY[9] = {1000, 1000, 3000, 3000, 1500, 2000,  500,  500,  500};
  // double stdY[9] =  { 100,  100,  500,  500,  500,  200,  100,  100,  100};

  // int mu[9] = {1, 2, 3, 4, 5, 6, 6, 5, 6};

  // double ex[9] = {60, 60, 50, 45, 45, 45, 45, 42, 42};

  // for( int i = 0; i < 2456; i++){
  //   int index = QRandomGenerator::global()->bounded(min, max + 1);

  //   double radX = generateGaussian(meanX[index], stdX[index]);
  //   double radY = generateGaussian(meanY[index], stdY[index]);

  //   double radEx = generateGaussian(ex[index], 0.1);

  //   double rad = generateGaussian(55, 20);

  //   printf("%5d | %2d %6f %6f %6f %6f\n", i, index, radX, radY, radEx, rad);

  //   hPID->Fill(radX, radY);

  //   if( i % 3 != 0 ){
  //     h1-> Fill(radEx);
  //   }else{
  //     h1->Fill(rad);
  //   }

  //   hMulti->Fill(mu[index]);

  //   if ( i% 3 != 0) h1g->Fill(radEx);
  // }

  // hPID->UpdatePlot();
  // h1->UpdatePlot();
  // hMulti->UpdatePlot();
  // h1g->UpdatePlot();

}

inline void SplitPole::UpdateHistograms(){

  if( this->isVisible() == false ) return;
  if( runAnalyzer->isChecked() == false ) return;

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

    hit.Clear();

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

    hPID->Fill(hit.eSL, hit.eSR); // x, y

    h1->Fill(hit.eSL);
    h1->Fill(hit.eSR, 1);
    
    //check events inside any Graphical cut and extract the rate, using tSR only
    for(int p = 0; p < cutList.count(); p++ ){ 
      if( cutList[p].isEmpty() ) continue;
      if( cutList[p].containsPoint(QPointF(hit.eSL, hit.eSR), Qt::OddEvenFill) ){
        if( hit.tSR < tMin[p] ) tMin[p] = hit.tSR;
        if( hit.tSR > tMax[p] ) tMax[p] = hit.tSR;
        count[p] ++;
        //printf(".... %d \n", count[p]);
        if( p == 0 ) h1g->Fill(hit.eSR);
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