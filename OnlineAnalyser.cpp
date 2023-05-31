#include "OnlineAnalyser.h"

#include <QRandomGenerator>
#include <random>

OnlineAnalyzer::OnlineAnalyzer(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent ): QMainWindow(parent){

  this->digi = digi;
  this->nDigi = nDigi;

  oeb = new OnlineEventBuilder * [nDigi];
  for( unsigned int i = 0; i < nDigi; i++ ) oeb[i] = new OnlineEventBuilder(digi[i]);

  buildTimerThread = new TimingThread(this);
  buildTimerThread->SetWaitTimeinSec(1.0); //^Set event build interval
  connect( buildTimerThread, &TimingThread::timeUp, this, &OnlineAnalyzer::UpdateHistograms);

  setWindowTitle("Online Analyzer");
  setGeometry(0, 0, 1000, 800);  

  QWidget * layoutWidget = new QWidget(this);
  setCentralWidget(layoutWidget);
  layout = new QGridLayout(layoutWidget);
  layoutWidget->setLayout(layout);

  // QPushButton * bnSetting = new QPushButton("Settings", this);
  // layout->addWidget(bnSetting);

  SetUpCanvas();

}

OnlineAnalyzer::~OnlineAnalyzer(){

  for( unsigned int i = 0; i < nDigi; i++ ) delete oeb[i];
  delete [] oeb;

}

void OnlineAnalyzer::StartThread(){
  buildTimerThread->start();
}

void OnlineAnalyzer::StopThread(){

  buildTimerThread->Stop();
  buildTimerThread->quit();
  buildTimerThread->wait();

}

void OnlineAnalyzer::SetUpCanvas(){

  h2 = new Histogram2D("testing", "x", "y", 400, 0, 4000, 400, 0, 4000, this);
  layout->addWidget(h2);

  h1 = new Histogram1D("testing", "x", 400, 0, 4000, this);
  layout->addWidget(h1);


  //Histogram * h1 = new Histogram("h1", 0, 5000, 200);


  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<double> distribution(2000.0, 1000);
  for( int i = 0; i < 1000 ; i++ ){
    //h2->Fill(distribution(gen), distribution(gen));
    h1->Fill(distribution(gen));
  }

  h1->UpdatePlot();

}

void OnlineAnalyzer::UpdateHistograms(){

  //Set with digitizer to be event build
  digiMTX[0].lock();
  oeb[0]->BuildEvents(100, false);
  digiMTX[0].unlock();

  //============ Get events, and do analysis
  long eventBuilt = oeb[0]->eventbuilt;
  if( eventBuilt == 0 ) return;

  long eventIndex = oeb[0]->eventIndex;

  long eventStart = eventIndex - eventBuilt + 1;
  if(eventStart < 0 ) eventStart += MaxNEvent;

  unsigned short e1 = 0, e2 = 0;

  for( long i = eventStart ; i <= eventIndex; i ++ ){
    std::vector<dataPoint> event = oeb[0]->events[i];

    for( int k = 0; k < (int) event.size(); k++ ){
      if( event[k].ch == 3 ) e1 = event[k].energy;
      if( event[k].ch == 4 ) e2 = event[k].energy;
    }

    h2->Fill(e1, e2);

    //h1->Fill(e1);
  }

  h2->UpdatePlot();

}