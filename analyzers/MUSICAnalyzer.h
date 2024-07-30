#ifndef MUSICANLAYZER_H
#define MUSICANLAYZER_H

#include "Analyser.h"
#include "Isotope.h"

#include <map>
#include <QApplication>
#include <QScreen>

namespace MUSICChMap{

  const std::map<unsigned short, int> SN2Bd = {
    {16828, 0},
    {16829, 1},
    {16827, 2},
    {23986, 3}
  };

  //Left 0->15
  //Right 16->31
  //Individual{32=Grid, 33=S0, 34=cathode, 35=S17, 36=Si_dE, 100>pulser},
  //-1=empty
  const int mapping[4][16] = {
    // 0,  1,  2,   3,  4,   5,  6,   7,  8,   9, 10,  11, 12, 13,  14, 15
    {34,  -1,  1,  -1, 33, 101,  5,  -1,  0,  -1,  9,  -1, 17, 13,  -1, 32},
    { 2,  -1, 16,  -1, 21, 102, 20,  -1,  8,  -1, 24,  -1, 27, 28,  -1, 14},
    {19,  -1,  3,  -1,  6, 103,  7,  -1, 25,  -1, 11,  -1, 12, 15,  -1, 10},
    { 4,  -1, 18,  36, 23, 104, 22,  -1, 29,  -1, 26,  -1, 31, 30,  -1, 35}
  };

  // Gain matching [ch][bd]
  const double corr[16][4] = {
    { 1.00000,  1.00000,  1.00000, 1.0000},
    { 1.00000,  1.00000,  1.03158, 1.0000},
    { 1.00000,  1.00000,  0.99240, 1.0000},
    { 1.00000,  1.03704,  0.94004, 1.0000},
    { 1.01031,  1.02084,  1.10114, 1.0000},
    { 1.00000,  0.94685,  1.00513, 1.0000},
    { 1.00000,  1.03431,  1.00513, 1.0000},
    { 1.00000,  0.92670,  0.96078, 1.0000},
    { 1.03431,  0.94685,  0.96314, 1.0000},
    { 1.00000,  1.03158,  0.95145, 1.0000},
    { 0.95145,  1.00256,  0.97270, 1.0000},
    { 1.00000,  1.00256,  0.90950, 1.0000},
    { 1.03704,  0.99492,  0.98740, 1.0000},
    { 1.00000,  1.00000,  0.99746, 1.0000},
    { 0.96078,  1.03980,  1.00513, 1.0000},
    { 1.00000,  1.05095,  1.00000, 1.0000},
  };


};

class MUSIC : public Analyzer{
  Q_OBJECT
public:

  MUSIC(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr): Analyzer(digi, nDigi, parent){

    SetUpdateTimeInSec(1.0);

    SetBackwardBuild(false, 100); // using normal building (acceding in time) or backward building, int the case of backward building, default events to be build is 100. 
    evtbder = GetEventBuilder();
    evtbder->SetTimeWindow(10000);

    SetUpCanvas();
  }

  // MUSIC(){};

  void SetUpCanvas();

public slots:
  void UpdateHistograms();

private:

  MultiBuilder *evtbder;

  Histogram2D * hLeft;
  Histogram2D * hRight;
  Histogram2D * hLR;

  Histogram1D * hMulti;

  QCheckBox * chkRunAnalyzer;

};


inline void MUSIC::SetUpCanvas(){

  //====== resize window if screen too small
  QScreen * screen = QGuiApplication::primaryScreen();
  QRect screenGeo = screen->geometry();
  if( screenGeo.width() < 1600 || screenGeo.height() < 1600) {
    setGeometry(0, 0, screenGeo.width() - 100, screenGeo.height() -100);
  }else{
    setGeometry(0, 0, 1600, 1600);
  }
  chkRunAnalyzer = new QCheckBox("Run Analyzer", this);
  layout->addWidget(chkRunAnalyzer, 0, 0);

  hLeft  = new Histogram2D("Left", "Ch", "Energy", 17, 0, 16, 200, 0, 20000, this);
  layout->addWidget(hLeft, 1, 0);
  hRight = new Histogram2D("Right", "Ch", "Energy", 17, 0, 16, 200, 0, 20000, this);
  layout->addWidget(hRight, 1, 1);
  hLR    = new Histogram2D("Left + Right", "Ch", "Energy", 17, 0, 16, 200, 0, 20000, this);
  layout->addWidget(hLR, 2, 0);
  hMulti = new Histogram1D("Multi", "multiplicity", 40, 0, 40);
  layout->addWidget(hMulti, 2, 1);

}

inline void MUSIC::UpdateHistograms(){

  if( this->isVisible() == false ) return;
  if( chkRunAnalyzer->isChecked() == false ) return;

  BuildEvents(); // call the event builder to build events

  printf("MUSIC::%s----------- 1\n", __func__);

  //============ Get events, and do analysis
  long eventBuilt = evtbder->eventBuilt;
  if( eventBuilt == 0 ) return;

  //============ Processing data and fill histograms
  long eventIndex = evtbder->eventIndex;
  long eventStart = eventIndex - eventBuilt + 1;
  if(eventStart < 0 ) eventStart += MaxNEvent;
  
  printf("MUSIC::%s----------- 2\n", __func__);
  for( long i = eventStart ; i <= eventIndex; i ++ ){
    std::vector<Hit> event = evtbder->events[i];
    //printf("-------------- %ld\n", i);

    hMulti->Fill((int) event.size());
    //if( event.size() < 9 ) return;
    if( event.size() == 0 ) return;

    double sum[17] = {0};
    for( int k = 0; k < (int) event.size(); k++ ){

      printf("MUSIC::%s----------- i, k %d, %d\n", __func__, i, k);
      int bd = MUSICChMap::SN2Bd.at(event[k].sn);

      int ch = event[k].ch;

      int ID = MUSICChMap::mapping[bd][ch];

      if( ID < 0 ) continue;

      double eC = event[k].energy;
      if(   0 <= ID && ID < 100 ) {
         eC *=  MUSICChMap::corr[ch][bd];
         hLeft->Fill(ID, eC);
         sum[ID] += eC;
      }
      if( 100 <= ID && ID < 200 ) {
         eC *=  MUSICChMap::corr[ch][bd];
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