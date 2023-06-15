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

    RedefineEventBuilder({0}); // only build for the 0-th digitizer;
    evtbder = GetEventBuilder();

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

};


inline void SplitPole::SetUpCanvas(){

  setGeometry(0, 0, 1600, 800);  

  // the "this" make the histogram a child of the SplitPole class. When SplitPole destory, all childs destory as well.
  h2 = new Histogram2D("Split Pole PID", "x", "y", 400, 0, 10000, 400, 0, 10000, this);
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

  long eventIndex = evtbder->eventIndex;
  long eventStart = eventIndex - eventBuilt + 1;
  if(eventStart < 0 ) eventStart += MaxNEvent;

  //============ Processing data and fill histograms
  unsigned short e1 = 0, e2 = 0;

  for( long i = eventStart ; i <= eventIndex; i ++ ){
    std::vector<EventMember> event = evtbder->events[i];

    for( int k = 0; k < (int) event.size(); k++ ){
      if( event[k].ch ==  9 ) e1 = event[k].energy;
      if( event[k].ch == 10 ) e2 = event[k].energy;
    }

    h2->Fill(e1, e2);
    h1->Fill(e1);
  }

  h2->UpdatePlot();
  h1->UpdatePlot();

  h2->PrintCutEntry();

}


#endif