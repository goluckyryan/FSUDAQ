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
  }

  /// ~SplitPole(); // comment out = defalt destructor

  void SetUpCanvas();

public slots:
  void UpdateHistograms();

private:

  MultiBuilder *evtbder;

  // declaie histograms
  Histogram2D * h2;
  Histogram1D * h1;

  int tick2ns;

};


inline void SplitPole::SetUpCanvas(){

  setGeometry(0, 0, 1600, 800);  

  // the "this" make the histogram a child of the SplitPole class. When SplitPole destory, all childs destory as well.
  h2 = new Histogram2D("Split Pole PID", "x", "y", 100, 0, 10000, 100, 0, 10000, this);
  //layout is inheriatge from Analyzer
  layout->addWidget(h2, 0, 0);

  h1 = new Histogram1D("Spectrum", "x", 400, 0, 10000, this);
  layout->addWidget(h1, 0, 1);
  

}

inline void SplitPole::UpdateHistograms(){

  BuildEvents(); // call the event builder to build events

  //============ Get events, and do analysis
  long eventBuilt = evtbder->eventBuilt;
  if( eventBuilt == 0 ) return;

  //============ Get the cut list, if any
  QList<QPolygonF> cutList = h2->GetCutList();
  const int nCut = cutList.count();
  unsigned long long tMin[nCut] = {0xFFFFFFFFFFFFFFFF}, tMax[nCut] = {0};
  unsigned int count[nCut]={0};

  //============ Processing data and fill histograms
  long eventIndex = evtbder->eventIndex;
  long eventStart = eventIndex - eventBuilt + 1;
  if(eventStart < 0 ) eventStart += MaxNEvent;
  
  for( long i = eventStart ; i <= eventIndex; i ++ ){
    unsigned short e1 = 0, e2 = 0;
    unsigned long long t1 = 0, t2 = 0;
    std::vector<EventMember> event = evtbder->events[i];
    //printf("-------------- %ld\n", i);
    for( int k = 0; k < (int) event.size(); k++ ){
      //event[k].Print();
      if( event[k].ch ==  9 ) {e1 = event[k].energy; t1 = event[k].timestamp;}
      if( event[k].ch == 10 ) {e2 = event[k].energy; t2 = event[k].timestamp;}
    }

    if( e1 == 0 ) continue;
    if( e2 == 0 ) continue;

    h2->Fill(e1, e2);
    h1->Fill(e1);

    //check events inside any Graphical cut and extract the rate, using t1 only
    for(int p = 0; p < cutList.count(); p++ ){ 
      if( cutList[p].isEmpty() ) continue;
      if( cutList[p].containsPoint(QPointF(e1, e2), Qt::OddEvenFill) ){
        if( t1 < tMin[p] ) tMin[p] = t1;
        if( t1 > tMax[p] ) tMax[p] = t1;
        count[p] ++;
        //printf(".... %d \n", count[p]);
      }
    }
  }

  h2->UpdatePlot();
  h1->UpdatePlot();

  QList<QString> cutNameList = h2->GetCutNameList();
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