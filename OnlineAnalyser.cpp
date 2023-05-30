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

  //TODO a simple way to set it at once
  h2 = new Histogram2D("testing", 0, 4000, 0, 4000);
  RChartView * h2View = new RChartView(h2->GetChart());
  h2->SetMarkerSize(2);
  h2View->SetVRange(0, 4000); // this only set the reset key 'r' 
  h2View->SetHRange(0, 4000);

  layout->addWidget(h2View);


  //Histogram * h1 = new Histogram("h1", 0, 5000, 200);


  // std::random_device rd;
  // std::mt19937 gen(rd());
  // std::normal_distribution<double> distribution(0.0, 2.0);
  // for( int i = 0; i < 1000 ; i++ ){
  //   h2->Fill(distribution(gen), distribution(gen));
  // }

}

void OnlineAnalyzer::UpdateHistograms(){

  //Set with digitizer to be event build
  digiMTX[0].lock();
  //digi[0]->GetData()->PrintAllData();
  oeb[0]->BuildEvents(100, true);
  digiMTX[0].unlock();

  //============ Get events, and do analysis
  long eventBuilt = oeb[0]->eventbuilt;
  printf("------------- eventBuilt %ld \n", eventBuilt);
  if( eventBuilt == 0 ) return;

  long eventIndex = oeb[0]->eventIndex;

  long eventStart = eventIndex - eventBuilt + 1;
  if(eventStart < 0 ) eventStart += MaxNEvent;

  unsigned short e1 = 0, e2 = 0;

  int count = 0;
  for( long i = eventStart ; i <= eventIndex; i ++ ){
    std::vector<dataPoint> event = oeb[0]->events[i];

    for( int k = 0; k < (int) event.size(); k++ ){
      if( event[k].ch == 3 ) e1 = event[k].energy;
      if( event[k].ch == 4 ) e2 = event[k].energy;
    }

    h2->Fill(e1, e2);
    count ++;
    if( count > 500 ) break;
  }

}