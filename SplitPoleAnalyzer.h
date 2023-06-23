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

class SplitPoleHit{

public:
  SplitPoleHit(){

    target.SetIso(12, 6);
    beam.SetIso(2,1);
    recoil.SetIso(1,1);

    Bfield = 0.76; // Tesla
    angleDegree = 20; // degree
    beamKE = 16; // MeV

    heavyRecoil.SetIso(target.A + beam.A - recoil.A, target.Z + beam.Z - recoil.Z);

    double Q = target.Mass + beam.Mass - recoil.Mass - heavyRecoil.Mass;

    double haha1 = sqrt(beam.Mass + beamKE + recoil.Mass)/(recoil.Mass + heavyRecoil.Mass) / cos(angleDegree * deg2rad);
    double haha2 = ( beamKE * ( heavyRecoil.Mass + beam.Mass)  + heavyRecoil.Mass * Q) / (recoil.Mass + heavyRecoil.Mass);

    double recoilKE = pow(haha1 + sqrt(haha1*haha1 + haha2), 2);

    printf("Q value : %f \n", Q);
    printf("proton enegry : %f \n", recoilKE);

    double recoilP = sqrt( recoilKE* ( recoilKE + 2*recoil.Mass));
    double rho = recoilP/(target.Z * Bfield * c); // in m
    double haha = sqrt( recoil.Mass * beam.Mass * beamKE / recoilKE );
    double k = haha * sin(angleDegree * deg2rad) / ( recoil.Mass + heavyRecoil.Mass - haha * cos(angleDegree * deg2rad));

    const double SPS_DISPERSION = 1.96; // x-position/rho
    const double SPS_MAGNIFICATION = 0.39; // in x-position

    zOffset = -1000.0 * rho * k * SPS_DISPERSION * SPS_MAGNIFICATION;

    printf("rho: %f m; z-offset: %f mm\n", rho, zOffset);

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
        theta = pi + atan((x2-x1)/36.0);
      }else{
        theta = pi * 0.5;
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

};


inline void SplitPole::SetUpCanvas(){

  setGeometry(0, 0, 1600, 1000);  

  // the "this" make the histogram a child of the SplitPole class. When SplitPole destory, all childs destory as well.
  hPID = new Histogram2D("Split Pole PID", "Scin-L", "Anode-Font", 100, 0, 2000, 100, 0, 2000, this);
  //layout is inheriatge from Analyzer
  layout->addWidget(hPID, 0, 0, 2, 1);

  h1 = new Histogram1D("Spectrum", "x", 100, 0, 2000, this);
  h1->SetColor(Qt::darkGreen);
  h1->AddDataList("Test", Qt::red); // add another histogram in h1, Max Data List is 10
  layout->addWidget(h1, 0, 1);
  
  hMulti = new Histogram1D("Multiplicity", "", 10, 0, 10, this);
  layout->addWidget(hMulti, 1, 1);

  h1g = new Histogram1D("Spectrum (gated)", "x", 100, 0, 2000, this);
  layout->addWidget(h1g, 2, 0, 1, 2);
  

}

inline void SplitPole::UpdateHistograms(){

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
      if( event[k].ch == ChMap::ScinR ) {hit.eSR = event[k].energy; hit.tSR = event[k].timestamp;}
      if( event[k].ch == ChMap::ScinL ) {hit.eSL = event[k].energy; hit.tSL = event[k].timestamp;}
      if( event[k].ch == ChMap::dFR )   {hit.eFR = event[k].energy; hit.tFR = event[k].timestamp;}
      if( event[k].ch == ChMap::dFL )   {hit.eFL = event[k].energy; hit.tFL = event[k].timestamp;}
      if( event[k].ch == ChMap::dBR )   {hit.eBL = event[k].energy; hit.tBL = event[k].timestamp;}
      if( event[k].ch == ChMap::dBL )   {hit.eBL = event[k].energy; hit.tBL = event[k].timestamp;}
      if( event[k].ch == ChMap::Cathode )   {hit.eCath = event[k].energy; hit.tCath = event[k].timestamp;}
      if( event[k].ch == ChMap::AnodeF )   {hit.eAF = event[k].energy; hit.tAF = event[k].timestamp;}
      if( event[k].ch == ChMap::AnodeB )   {hit.eAB = event[k].energy; hit.tAB = event[k].timestamp;}
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