#ifndef ENCOREANLAYZER_H
#define ENCOREANLAYZER_H

#include "Analyser.h"
#include "Isotope.h"

namespace ChMap{

  const int mapping[3][16] = {
    // 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
    {200, -1,  0, -1,  8, -1,300, -1,108, -1,102, -1,109, -1, 16, -1 },
    {201, -1,110,107,111,106,112,105,113,104,114,103,115, -1,116,101 },
    {202,  1, 15,  2, 14,  3, 13,  4, 12,  5, 11,  6, 10,  7,  9, -1 }
  };

  const double corr[16][3] = {
    { 1.00000,  1.00000,  1.00000},
    { 1.00000,  1.00000,  1.03158},
    { 1.00000,  1.00000,  0.99240},
    { 1.00000,  1.03704,  0.94004},
    { 1.01031,  1.02084,  1.10114},
    { 1.00000,  0.94685,  1.00513},
    { 1.00000,  1.03431,  1.00513},
    { 1.00000,  0.92670,  0.96078},
    { 1.03431,  0.94685,  0.96314},
    { 1.00000,  1.03158,  0.95145},
    { 0.95145,  1.00256,  0.97270},
    { 1.00000,  1.00256,  0.90950},
    { 1.03704,  0.99492,  0.98740},
    { 1.00000,  1.00000,  0.99746},
    { 0.96078,  1.03980,  1.00513},
    { 1.00000,  1.05095,  1.00000},
  };


};

class Encore : public Analyzer{
  Q_OBJECT
public:

  Encore(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr): Analyzer(digi, nDigi, parent){

    SetUpdateTimeInSec(1.0);

    SetBackwardBuild(false, 100); // using normal building (acceding in time) or backward building, int the case of backward building, default events to be build is 100. 
    evtbder = GetEventBuilder();
    evtbder->SetTimeWindow(500);

    SetUpCanvas();
  }

  // Encore(){};

  void SetUpCanvas();

public slots:
  void UpdateHistograms();

private:

  MultiBuilder *evtbder;

  Histogram2D * hLeft;
  Histogram2D * hRight;
  Histogram2D * hLR;

  Histogram1D * hMulti;

  QCheckBox * runAnalyzer;

};


inline void Encore::SetUpCanvas(){

  setGeometry(0, 0, 1600, 1600);  

  runAnalyzer = new QCheckBox("Run Analyzer", this);
  layout->addWidget(runAnalyzer, 0, 0);

  hLeft  = new Histogram2D("Left", "Ch", "Energy", 17, 0, 16, 200, 0, 20000, this);
  layout->addWidget(hLeft, 1, 0);
  hRight = new Histogram2D("Right", "Ch", "Energy", 17, 0, 16, 200, 0, 20000, this);
  layout->addWidget(hRight, 1, 1);
  hLR    = new Histogram2D("Left + Right", "Ch", "Energy", 17, 0, 16, 200, 0, 20000, this);
  layout->addWidget(hLR, 2, 0);
  hMulti = new Histogram1D("Multi", "multiplicity", 40, 0, 40);
  layout->addWidget(hMulti, 2, 1);

}

inline void Encore::UpdateHistograms(){

  if( this->isVisible() == false ) return;
  if( runAnalyzer->isChecked() == false ) return;

  BuildEvents(); // call the event builder to build events

  //============ Get events, and do analysis
  long eventBuilt = evtbder->eventBuilt;
  if( eventBuilt == 0 ) return;

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

    double sum[17] = {0};
    for( int k = 0; k < (int) event.size(); k++ ){

      int bd = event[k].bd;
      int ch = event[k].ch;

      int ID = ChMap::mapping[bd][ch];

      if( ID < 0 ) continue;

      double eC = event[k].energy;
      if(   0 <= ID && ID < 100 ) {
         eC *=  ChMap::corr[ch][bd];
         hLeft->Fill(ID, eC);
         sum[ID] += eC;
      }
      if( 100 <= ID && ID < 200 ) {
         eC *=  ChMap::corr[ch][bd];
         hRight->Fill(ID-100, eC );
         sum[ID-100] += eC ;
      }
    }

    for( int ch = 0; ch < 17; ch++){
     if( sum[ch] > 0 ) hLR->Fill(ch, sum[ch]);
     //printf("%d | sum %d\n", ch, sum[ch]);
     }

  }

  hLeft->UpdatePlot();
  hRight->UpdatePlot();
  hMulti->UpdatePlot();
  hLR->UpdatePlot();

}

#endif