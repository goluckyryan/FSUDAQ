#include "SplitPoleAnalyzer.h"

SplitPole::SplitPole(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent): Analyzer(digi, nDigi, parent){

  SetDigiID(0);
  SetUpdateTimeInSec(1.0);

  oeb = GetEventBuilder();

  SetUpCanvas();

}


SplitPole::~SplitPole(){


}

void SplitPole::SetUpCanvas(){

  setGeometry(0, 0, 1600, 800);  

  // the "this" make the histogram a child of the SplitPole class. When SplitPole destory, all childs destory as well.
  h2 = new Histogram2D("testing", "x", "y", 400, 0, 10000, 400, 0, 10000, this);
  //layout is inheriatge from Analyzer
  layout->addWidget(h2, 0, 0);

  h1 = new Histogram1D("testing", "x", 400, 0, 10000, this);
  layout->addWidget(h1, 0, 1);

}

void SplitPole::UpdateHistograms(){

  BuildEvents();

  //oeb->PrintStat();

  //============ Get events, and do analysis
  long eventBuilt = oeb->eventBuilt;
  if( eventBuilt == 0 ) return;

  long eventIndex = oeb->eventIndex;
  long eventStart = eventIndex - eventBuilt + 1;
  if(eventStart < 0 ) eventStart += MaxNEvent;

  //============ Processing data and fill histograms
  unsigned short e1 = 0, e2 = 0;

  for( long i = eventStart ; i <= eventIndex; i ++ ){
    std::vector<dataPoint> event = oeb->events[i];

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