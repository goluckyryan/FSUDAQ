#ifndef RASIOR_h
#define RASIOR_h

/*********************************************
 * This is online analyzer for RASIOR, ANL
 * 
 * Created by Ryan @ 2023-10-16
 * 
 * ******************************************/
#include "Analyser.h"


class RAISOR : public Analyzer{

public:
  RAISOR(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr): Analyzer(digi, nDigi, parent){


    SetUpdateTimeInSec(1.0);

    RedefineEventBuilder({0}); // only builder for the 0-th digitizer.
    tick2ns = digi[0]->GetTick2ns();

    SetBackwardBuild(false, 100); // using normal building (acceding in time) or backward building, int the case of backward building, default events to be build is 100. 
    evtbder = GetEventBuilder();
    evtbder->SetTimeWindow(500);
 
    //========== use the influx from the Analyzer
    influx = new InfluxDB("https://fsunuc.physics.fsu.edu/influx/");
    dataBaseName = "testing"; 

    SetUpCanvas(); // see below

  };

  void SetUpCanvas();

public slots:
  void UpdateHistograms();


private:

  MultiBuilder *evtbder;

  Histogram2D * hPID;

  int tick2ns;

  float dE, E;
  unsigned long long dE_t, E_t;

};


inline void RAISOR::SetUpCanvas(){

  setGeometry(0, 0, 500, 500);  

  //============ histograms
  hPID = new Histogram2D("RAISOR", "E", "dE", 100, 0, 5000, 100, 0, 20000, this);
  layout->addWidget(hPID, 0, 0);  

}

inline void RAISOR::UpdateHistograms(){

  if( this->isVisible() == false ) return;
  
  BuildEvents(false); // call the event builder to build events

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

    if( event.size() == 0 ) return;

    for( int k = 0; k < (int) event.size(); k++ ){
      //event[k].Print();
      if( event[k].ch == 0 ) {dE = event[k].energy; dE_t = event[k].timestamp;}
      if( event[k].ch == 1 ) {E = event[k].energy;   E_t = event[k].timestamp;}
      
    }

    // printf("(E, dE) = (%f, %f)\n", E, dE);
    hPID->Fill(E + RandomGauss(0, 100), dE+ RandomGauss(0, 100)); // x, y
    
    //check events inside any Graphical cut and extract the rate
    for(int p = 0; p < cutList.count(); p++ ){ 
      if( cutList[p].isEmpty() ) continue;
      if( cutList[p].containsPoint(QPointF(E, dE), Qt::OddEvenFill) ){
        if( dE_t < tMin[p] ) tMin[p] = dE_t;
        if( dE_t > tMax[p] ) tMax[p] = dE_t;
        count[p] ++;
        //printf(".... %d \n", count[p]);
      }
    }
  }

  hPID->UpdatePlot();

  //========== output to Influx
  QList<QString> cutNameList = hPID->GetCutNameList();
  for( int p = 0; p < cutList.count(); p ++){
    if( cutList[p].isEmpty() ) continue;
    double dT = (tMax[p]-tMin[p]) * tick2ns / 1e9; // tick to sec
    double rate = count[p]*1.0/(dT);
    //printf("%llu %llu, %f %d\n", tMin[p], tMax[p], dT, count[p]);
    //printf("%10s | %d | %f Hz \n", cutNameList[p].toStdString().c_str(), count[p], rate);  
    
    influx->AddDataPoint("Cut,name=" + cutNameList[p].toStdString()+ " value=" + std::to_string(rate));
    influx->WriteData(dataBaseName.toStdString());
    influx->ClearDataPointsBuffer();
  }
}


#endif