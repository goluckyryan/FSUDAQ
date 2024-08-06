#include "TFile.h"
#include "TChain.h"
#include "TH1F.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include "TTreeReaderArray.h"
#include "TClonesArray.h"
#include "TGraph.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TStopwatch.h"
#include "TMath.h"

#include "vector"
#include "../analyzers/SplitPoleHit.h"


namespace ChMap{

  const short ScinR = 0;
  const short ScinL = 1;
  const short dFR = 8;
  const short dFL = 9;
  const short dBR = 10;
  const short dBL = 11;
  const short Cathode = 7;
  const short AnodeF = 13;
  const short AnodeB = 15;

};

const double c = 299.792458; // mm/ns
const double pi = M_PI;
const double deg2rad = pi/180.;

SplitPoleHit hit;

TH2F * PID;
TH2F * coin;

TH1F * hMulti;

TH1F * hF;
TH1F * hB;
TH1F * hXavg;

TH2F * hFocal;

TH2F * hXavg_Q;
TH2F * hXavg_Theta;

TH2F * haha;

TH1F * hEx;

ULong64_t t1, t2;

#define XMIN -20
#define XMAX  150

//^###########################################

void SplitPolePlotter(TChain *tree){

  printf("#####################################################################\n");
  printf("#################        SplitPolePlotter.C      ####################\n");
  printf("#####################################################################\n");

  TObjArray * fileList = tree->GetListOfFiles();
  printf("\033[0;31m========================================== Number of Files : %2d\n",fileList->GetEntries());
  fileList->Print();
  printf("========================================== Number of Files : %2d\033[0m\n",fileList->GetEntries());

  printf("///////////////////////////////////////////////////////////////////\n");
  printf("            Total Number of entries : %llu \n", tree->GetEntries());
  printf("///////////////////////////////////////////////////////////////////\n");
  
  if( tree->GetEntries() == 0 ) {
    printf("========= no events. Abort.\n");
    return;
  }

  //*====================================================== histograms

  PID = new TH2F("hPID", "PID; Scin_X ; AnodeB", 200, 0, 2000, 100, 0, 4000);
  coin = new TH2F("hCoin", "Coincident ", 16, 0, 16, 16, 0, 16);

  hMulti = new TH1F("hMulti", "Multiplicity", 16, 0, 16); 

  hF = new TH1F("hF", "Front delay line position", 600, XMIN, XMAX);
  hB = new TH1F("hB", "Back delay line position", 600, XMIN, XMAX);
  hXavg = new TH1F("hAvg", "Xavg", 600, XMIN, XMAX);

  hFocal = new TH2F("hFocal", "Front vs Back ", 200, XMIN, XMAX, 200, XMIN, XMAX);
  hXavg_Q = new TH2F("hXavg_Q", "Xavg vs Q ", 200, XMIN, XMAX, 200, 0, 5000);
  hXavg_Theta = new TH2F("hXavg_Theta", "Xavg vs Theta ", 200, XMIN, XMAX, 200, 2.5, 3);

  haha = new TH2F("haha", "", 400, XMIN, XMAX, 400, -50, 50);

  hEx = new TH1F("hEx", "Ex; Ex [MeV]; count/100 keV", 250, -5, 20);

  hit.SetMassTablePath("../analyzers/mass20.txt");
  hit.CalConstants("36S", "d", "p", 80, 12); // 80MeV, 5 deg
  hit.CalZoffset(1.41); // 1.41 T

  t1 = 0;
  t2 = 0;


  //*====================================================== Tree Reader
  TTreeReader reader(tree);

  TTreeReaderValue<ULong64_t>  evID = {reader, "evID"};
  TTreeReaderValue<UInt_t>    multi = {reader, "multi"};
  TTreeReaderArray<UShort_t>     sn = {reader, "sn"};
  TTreeReaderArray<UShort_t>     ch = {reader, "ch"};
  TTreeReaderArray<UShort_t>      e = {reader, "e"};
  TTreeReaderArray<UShort_t>     e2 = {reader, "e2"};
  TTreeReaderArray<ULong64_t>   e_t = {reader, "e_t"};
  TTreeReaderArray<UShort_t>    e_f = {reader, "e_f"};

  ULong64_t NumEntries = tree->GetEntries();

  //^###########################################################
  //^ * Process
  //^###########################################################
  printf("############################################### Processing...\n");
  fflush(stdout); // flush out any printf

  ULong64_t processedEntries = 0;
  float Frac = 0.1;
  TStopwatch StpWatch;
  StpWatch.Start();

  while (reader.Next()) {

    // if( processedEntries > 10 ) break;
    // printf("============== %5llu | multi : %d (%zu) \n", processedEntries, multi.Get()[0], sn.GetSize());
    // for( int i = 0; i < multi.Get()[0]; i++ ){
    //   printf("   %d | %5d %2d %7d %10llu\n", i, sn[i], ch[i], e[i], e_t[i]);
    // }

    hit.ClearData();
    hMulti->Fill(sn.GetSize());

    // if( multi.Get()[0] != 9 ) continue;

    for( int i = 0; i < sn.GetSize(); i++){

      t2 = e_t[i];
      if( t2 < t1 ) printf("entry %lld-%d, timestamp is not in order. %llu, %llu\n", processedEntries, i, t2, t1);
      if( i == 0 ) t1 = e_t[i];

      if( ch[i] == ChMap::ScinR )    {hit.eSR   = e[i]; hit.tSR   = e_t[i] + e_f[i]/1000;}
      if( ch[i] == ChMap::ScinL )    {hit.eSL   = e[i]; hit.tSL   = e_t[i] + e_f[i]/1000;}
      if( ch[i] == ChMap::dFR )      {hit.eFR   = e[i]; hit.tFR   = e_t[i] + e_f[i]/1000;}
      if( ch[i] == ChMap::dFL )      {hit.eFL   = e[i]; hit.tFL   = e_t[i] + e_f[i]/1000;}
      if( ch[i] == ChMap::dBR )      {hit.eBR   = e[i]; hit.tBR   = e_t[i] + e_f[i]/1000;}
      if( ch[i] == ChMap::dBL )      {hit.eBL   = e[i]; hit.tBL   = e_t[i] + e_f[i]/1000;}
      if( ch[i] == ChMap::Cathode )  {hit.eCath = e[i]; hit.tCath = e_t[i] + e_f[i]/1000;}
      if( ch[i] == ChMap::AnodeF )   {hit.eAF   = e[i]; hit.tAF   = e_t[i] + e_f[i]/1000;}
      if( ch[i] == ChMap::AnodeB )   {hit.eAB   = e[i]; hit.tAB   = e_t[i] + e_f[i]/1000;}

      for( int j = i+1; j < sn.GetSize(); j++){
          coin->Fill(ch[i], ch[j]);
      }
    }

    unsigned int dQ = hit.eAB; // delta Q
    unsigned int Qt = hit.eSR; // total Q

    if( Qt > 0 && dQ > 0 ) {
      PID->Fill(Qt, dQ);
    }

    //  if( hit.eAF < 50000 ) return kTRUE;
    // if( hit.eCath == 0 ) return kTRUE;
    // if( hit.eCath > 13000 ) return kTRUE;

    hit.CalData(4);

    if( !TMath::IsNaN(hit.x1) || !TMath::IsNaN(hit.x2) ) {
      hFocal->Fill(hit.x1, hit.x2);
      hF->Fill(hit.x1);
      hB->Fill(hit.x2);
      hXavg->Fill(hit.xAvg);

      hXavg_Q->Fill(hit.xAvg, dQ);
      hXavg_Theta->Fill( hit.xAvg, hit.theta);

      for( int i = 0; i < 400; i++){
        double y = -50 + 100/400.*i;

        double x = (y/42.8625 + 0.5)* ( hit.x2-hit.x1) + hit.x1;

        haha->Fill(x,y);
      }

      double ex = hit.Rho2Ex( (hit.xAvg/100 + 0.363)   );
      //if( XMIN < hit.xAvg && hit.xAvg < XMAX) printf("x1 : %6.2f, x2 : %6.2f, xAvg %6.2f cm , ex : %f \n", hit.x1, hit.x2, hit.xAvg, ex);
      hEx->Fill(ex);
    }

    //*============================================ Progress Bar
    processedEntries ++;
    if (processedEntries >= NumEntries*Frac - 1 ) {
      TString msg; msg.Form("%llu", NumEntries/1000);
      int len = msg.Sizeof();
      printf(" %3.0f%% (%*llu/%llu k) processed in %6.1f sec | expect %6.1f sec\n",
                Frac*100, len, processedEntries/1000,NumEntries/1000,StpWatch.RealTime(), StpWatch.RealTime()/Frac);
      fflush(stdout);
      StpWatch.Start(kFALSE);
      Frac += 0.1;
    }


  }

  //^###########################################################
  //^ * Plot
  //^###########################################################
  TCanvas * canvas = new TCanvas("cc", "Split-Pole", 1600, 1200);

  gStyle->SetOptStat("neiou");

  canvas->Divide(4, 3);

  canvas->cd(1); PID->Draw("colz");
  //canvas->cd(2); coin->Draw("colz");
  canvas->cd(2); haha->Draw("colz");

  canvas->cd(3); hF->Draw();
  canvas->cd(4); hB->Draw();

  canvas->cd(5); hXavg_Q->Draw("colz");

  canvas->cd(6); hXavg->Draw("colz");

  canvas->cd(7); hEx->Draw();

  canvas->cd(8); coin->Draw("colz");

  canvas->cd(9); canvas->cd(9)->SetLogy(); hMulti->Draw();

  canvas->cd(10); hXavg_Theta->Draw("colz");

}