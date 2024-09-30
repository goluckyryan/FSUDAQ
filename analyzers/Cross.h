#ifndef Cross_h
#define Cross_h

/*********************************************
 * This is online analyzer for PID, ANL
 * 
 * Created by Khushi @ 2024-09-03
 * 
 * ******************************************/
#include "Analyser.h"

class Cross : public Analyzer{

public:
  Cross(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr): Analyzer(digi, nDigi, parent){

    SetUpdateTimeInSec(1.0);

    RedefineEventBuilder({0}); // only builder for the 0-th digitizer.
    tick2ns = digi[0]->GetTick2ns();
	
    SetBackwardBuild(false, 100); // using normal building (acceding in time) or backward building, int the case of backward building, default events to be build is 100. 
    evtbder = GetEventBuilder();
    evtbder->SetTimeWindow(500);
 
    SetDatabase("http://localhost:8086/", "testing", "zKhzKk4Yhf1l9QU-yE2GsIZ1RazqUgoW3NlF8LJqq_xDMwatOJwg1sKrjgq36uLEsQf8Fmn4sJALP7Kkilk14A==");

    SetUpCanvas(); // see below

  };

  void SetUpCanvas();

public slots:
  void UpdateHistograms();
  void ReplotHistograms();

private:

  MultiBuilder *evtbder;

  //Histogram2D * hPID;
  
  Histogram1D * hdE;    // raw dE (ch=1): ch1
  Histogram1D * hE; 	// raw E (ch=4) : ch4
  Histogram1D * hdT; 	// raw dT (ch=7): ch7
  
  Histogram1D * hTotE; 	// total energy (dE+E): ch1+ch4
  Histogram1D * hTWin; 	// coincidence time window TWin: (t4-t1)*1e9
  
  Histogram2D * hdEE; // dE versus E : ch1 versus ch4
  Histogram2D * hdEtotE; // dE versus totE : ch1 versus (ch1+ch4)
 
  Histogram2D * hdEdT; // dE versus TOF: ch1 versus (t7-t1)*1e9
 
  Histogram1D * hMulti; //Multiplicity of an event
  
  int tick2ns;
  
  int chDE, chE;
  float energyDE, energyE, ch7;
  unsigned long long t1, t4, t7;

  QPushButton * bnClearHist;  
  QLabel * lbInfluxIP;
  RComboBox * cbLocation;
  QCheckBox * chkDEFourTime;

};

inline void Cross::ReplotHistograms(){

  hdE->UpdatePlot();    
  hE->UpdatePlot();
  hdT->UpdatePlot();
  hTotE->UpdatePlot();
  hdEE->UpdatePlot();
  hdEtotE->UpdatePlot();
  hdEdT->UpdatePlot();
  hTWin->UpdatePlot();
  hMulti->UpdatePlot();

}

inline void Cross::SetUpCanvas(){

  setGeometry(0, 0, 2000, 1000);  

  //============ histograms
  
  //hPID = new Histogram2D("RAISOR", "E", "dE", 100, 0, 5000, 100, 0, 5000, this);
  //layout->addWidget(hPID, 2, 0); 

  int row = 0;
  cbLocation = new RComboBox(this);
  cbLocation->addItem("Cross", 0);
  cbLocation->addItem("Target", 1);
  layout->addWidget(cbLocation, row, 0);  

  connect(cbLocation, &RComboBox::currentIndexChanged, this, [=](){
    switch (cbLocation->currentData().toInt() ) {
      case 0 : {
        hdE->SetLineTitle("raw dE (ch = 0)");
        hE->SetLineTitle("raw E (ch = 2)");
        hdE->replot();
        hE->replot();
        chDE = 0;
        chE = 2;
        //Can also set histograms range

      }
      break;
      case 1 : {
        hdE->SetLineTitle("raw dE (ch = 1)");
        hE->SetLineTitle("raw E (ch = 4)");
        hdE->replot();
        hE->replot();
        chDE = 1;
        chE = 4;
        //Can also set histograms range
      }
    }
  });

  chkDEFourTime = new QCheckBox("dE channel / 4", this);
  layout->addWidget(chkDEFourTime, row, 1);

  bnClearHist = new QPushButton("Clear All Hist.", this);
  layout->addWidget(bnClearHist, row, 2);

  connect( bnClearHist, &QPushButton::clicked, this, [=](){
    hdE->Clear();    
    hE->Clear();
    hdT->Clear();
    hTotE->Clear();
    hdEE->Clear();
    hdEtotE->Clear();
    hdEdT->Clear();
    hTWin->Clear();
    hMulti->Clear();
  });


  QString haha;
  if( influx ) {
    haha = dataBaseIP + ", DB : " + dataBaseName;
  }else{
    haha = "No influxDB connection.";
  }
  lbInfluxIP = new QLabel( haha , this);
  if( influx == nullptr ) lbInfluxIP->setStyleSheet("color : red;");
  layout->addWidget(lbInfluxIP, row, 3, 1, 3);

  row ++;
  hdEE = new Histogram2D("dE vs E", "E[ch]", "dE[ch]", 500, -100, 5000, 500, -100, 5000, this);
  layout->addWidget(hdEE, row, 0, 1, 2); 
  
  hdE = new Histogram1D("raw dE (ch=0)", "dE [ch]", 300, 0, 5000, this);
  layout->addWidget(hdE, row, 2);
  
  hE = new Histogram1D("raw E (ch=2)", "E [ch]", 300, 0, 10000, this);
  layout->addWidget(hE, row, 3);
  
  hTotE = new Histogram1D("total energy (dE+E)", "TotE [ch]", 300, 0, 16000, this);
  layout->addWidget(hTotE, row, 4);

  hMulti = new Histogram1D("Multiplicity", "", 10, 0, 10, this);
  layout->addWidget(hMulti, row, 5);
  
  row ++;
  hdEtotE = new Histogram2D("dE vs TotE", "TotE[ch]", "dE[ch]", 500, 0, 10000, 500, 0, 5000, this);
  layout->addWidget(hdEtotE, row, 0, 1, 2);
  
  hdT = new Histogram1D("raw dT (ch=7)", "dT [ch]", 300, 0, 1000, this);
  layout->addWidget(hdT, row, 2);
  
  hdEdT = new Histogram2D("dE vs TOF", "TOF [ns]", "dE", 100, 0, 500, 100, 0, 4000, this);
  layout->addWidget(hdEdT, row, 3);
  
  hTWin = new Histogram1D("coincidence time window", "TWin [ns]", 100, 0, 100, this);
  layout->addWidget(hTWin, row, 4);

  
}

inline void Cross::UpdateHistograms(){

  if( this->isVisible() == false ) return;
  
  BuildEvents(false); // call the event builder to build events

  //============ Get events, and do analysis
  long eventBuilt = evtbder->eventBuilt;
  if( eventBuilt == 0 ) return;

  //============ Get the cut list, if any
  QList<QPolygonF> cutList1 = hdEE->GetCutList();
  const int nCut1 = cutList1.count();
  unsigned long long tMin1[nCut1], tMax1[nCut1];
  unsigned int count1[nCut1];
  
  QList<QPolygonF> cutList2 = hdEtotE->GetCutList();
  const int nCut2 = cutList2.count();
  unsigned long long tMin2[nCut2], tMax2[nCut2];
  unsigned int count2[nCut2];

  //============ Processing data and fill histograms
  long eventIndex = evtbder->eventIndex;
  long eventStart = eventIndex - eventBuilt + 1;
  if(eventStart < 0 ) eventStart += MaxNEvent;

  for( int i = 0; i < nCut1; i++) {
    tMin1[i] = -1;
    tMax1[i] = 0;
    count1[i] = 0;
  }

  for( int i = 0; i < nCut2; i++) {
    tMin2[i] = -1;
    tMax2[i] = 0;
    count2[i] = 0;
  }
  
  for( long i = eventStart ; i <= eventIndex; i ++ ){

    std::vector<Hit> event = evtbder->events[i];
    //printf("-------------- %ld\n", i);

    if( event.size() == 0 ) return;

    hMulti->Fill(event.size());

    energyDE = -100; t1 = 0;
    energyE = -100; t4 = 0;
    ch7 = -100; t7 = 0;
   
    for( int k = 0; k < (int) event.size(); k++ ){
      //event[k].Print();
      if( event[k].ch == chDE ) {energyDE = event[k].energy; t1 = event[k].timestamp;} // Reads channel 0 of the digitizer corresponding to dE
      if( event[k].ch == chE  ) {energyE = event[k].energy; t4 = event[k].timestamp;} // Reads channel 2 of the digitizer corresponding to E
      if( event[k].ch == 7 ) {ch7 = event[k].energy; t7 = event[k].timestamp;} //RF Timing if setup
     
    }

    // printf("(E, dE) = (%f, %f)\n", E, dE);
    //hPID->Fill(ch4 , ch1); // x, y
    //etotal = ch1*0.25*0.25 + ch4
    if( energyDE > 0 ) hdE->Fill(energyDE);    
    if( energyE > 0 ) hE->Fill(energyE);
    if( ch7 > 0 ) hdT->Fill(ch7);
    if( energyDE > 0 && energyE > 0 ){
      hTotE->Fill(0.25 * energyDE + energyE);
      hdEE->Fill(energyE,energyDE);
      if( t4 > t1 ) {
        hTWin->Fill((t4-t1));
      }else{
        hTWin->Fill((t1-t4));
      }
      hdEtotE->Fill( (chkDEFourTime->isChecked() ? 0.25 : 1) * energyDE + energyE,energyDE);
    }

    if( energyDE > 0 && ch7 > 0) hdEdT->Fill((t7-t1)*1e9,energyDE);
    
    //check events inside any Graphical cut and extract the rate
    // if( ch1 == 0 && ch4 == 0 ) continue;
    for(int p = 0; p < cutList1.count(); p++ ){ 
      if( cutList1[p].isEmpty() ) continue;
      if( cutList1[p].containsPoint(QPointF(energyE, energyDE), Qt::OddEvenFill) ){
        if( t1 < tMin1[p] ) tMin1[p] = t1;
        if( t1 > tMax1[p] ) tMax1[p] = t1;
        count1[p] ++;
        //printf("hdEE.... %d \n", count1[p]);
      }
    }
    
    for(int p = 0; p < cutList2.count(); p++ ){ 
      if( cutList2[p].isEmpty() ) continue;
      if( cutList2[p].containsPoint(QPointF(energyDE+energyE,energyDE), Qt::OddEvenFill) ){
        if( t1 < tMin2[p] ) tMin2[p] = t1;
        if( t1 > tMax2[p] ) tMax2[p] = t1;
        count2[p] ++;
        //printf("hdEtotE.... %d \n", count2[p]);
      }
    }
  }
  
 
  for(int p = 0; p < cutList2.count(); p++ ){ 
	  printf("hdEE.... %d %d \n", p, count1[p]);
  }	
  
  //========== output to Influx
  QList<QString> cutNameList1 = hdEE->GetCutNameList();
  for( int p = 0; p < cutList1.count(); p ++){
    if( cutList1[p].isEmpty() ) continue;
    double dT = (tMax1[p]-tMin1[p]) / 1e9; // tick to sec
    double rate = count1[p]*1.0/(dT);
    //printf("%llu %llu, %f %d\n", tMin1[p], tMax1[p], dT, count1[p]);
    printf("%10s | %d | %f Hz \n", cutNameList1[p].toStdString().c_str(), count1[p], rate);  
    
    if( influx ){
      influx->AddDataPoint("Cut,name=" + cutNameList1[p].toStdString()+ " value=" + std::to_string(rate));
      influx->WriteData("testing");
      influx->ClearDataPointsBuffer();
    }
  }
  
}

 
#endif
