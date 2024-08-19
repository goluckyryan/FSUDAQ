#ifndef SPLITPOLEPLOTTER
#define SPLITPOLEPLOTTER

#include "TFile.h"
#include "TChain.h"
#include "TH1F.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include "TTreeReaderArray.h"
#include "TClonesArray.h"
#include "TGraph.h"
#include "TCutG.h"
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
  const short dFR = 9;
  const short dFL = 8;
  const short dBR = 11;
  const short dBL = 10;
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

TH2F * hRay;
TH1F * hEx;

TH2F * hEx_Multi;

ULong64_t t1, t2;

#define XMIN -200
#define XMAX  200

//^###########################################

void SplitPolePlotter(TChain *tree, TCutG * pidCut = nullptr, double rhoOffset = 0, double rhoScaling = 1, bool isFSUDAQ = true){

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

  coin = new TH2F("hCoin", "Coincident ", 16, 0, 16, 16, 0, 16);
  hMulti = new TH1F("hMulti", "Multiplicity", 16, 0, 16); 

  if( isFSUDAQ ){
    PID = new TH2F("hPID", "PID; Scin_X ; AnodeB", 200, 0, 20000, 100, 0, 40000);
    hXavg_Q = new TH2F("hXavg_Q", "Xavg vs Q ", 200, XMIN, XMAX, 200, 0, 40000);
  }else{
    PID = new TH2F("hPID", "PID; Scin_X ; AnodeB", 200, 0, 4000, 100, 0, 5000);
    hXavg_Q = new TH2F("hXavg_Q", "Xavg vs Q ", 200, XMIN, XMAX, 200, 0, 5000);
  }


  hF = new TH1F("hF", "Front delay line position", 600, XMIN, XMAX);
  hB = new TH1F("hB", "Back delay line position", 600, XMIN, XMAX);
  hXavg = new TH1F("hAvg", "Xavg", 600, XMIN, XMAX);

  hFocal = new TH2F("hFocal", "Front vs Back ", 200, XMIN, XMAX, 200, XMIN, XMAX);
  hXavg_Theta = new TH2F("hXavg_Theta", "Xavg vs Theta ", 200, XMIN, XMAX, 200, 0.5, 1.4);

  hRay = new TH2F("hRay", "Ray plot", 400, XMIN, XMAX, 400, -50, 50);

  hEx = new TH1F("hEx", "Ex; Ex [MeV]; count/10 keV", 600, -1, 5);

  hEx_Multi = new TH2F("hEx_Multi", "Ex vs Multi; Ex; Multi", 600, -1, 5, 16, 0, 16);

  hit.SetMassTablePath("../analyzers/mass20.txt");
  hit.CalConstants("12C", "d", "p", 16, 18); // 80MeV, 5 deg
  hit.CalZoffset(0.750); // 1.41 T

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
    hMulti->Fill(*multi);

    // if( *multi != 9 ) continue;

    for( int i = 0; i < *multi; i++){

      t2 = e_t[i];
      if( t2 < t1 ) printf("entry %lld-%d, timestamp is not in order. %llu, %llu\n", processedEntries, i, t2, t1);
      if( i == 0 ) t1 = e_t[i];

      // if( e[i] == 65535 ) continue;

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
    unsigned int Qt = hit.eSL; // total Q

    if( Qt > 0 && dQ > 0 ) {
      PID->Fill(Qt, dQ);
    }

    //=============== PID gate cut
    if( pidCut  ){
      if( !pidCut->IsInside(Qt, dQ) ) continue;
    }

    hit.CalData(2);

    if( hit.theta > 1.2 || 0.5 > hit.theta ) continue;

    if( (!TMath::IsNaN(hit.x1) || !TMath::IsNaN(hit.x2)) ) {
      hFocal->Fill(hit.x1, hit.x2);
      hF->Fill(hit.x1);
      hB->Fill(hit.x2);
      hXavg->Fill(hit.xAvg);

      hXavg_Q->Fill(hit.xAvg, dQ);
      hXavg_Theta->Fill( hit.xAvg, hit.theta);

      for( int i = 0; i < 400; i++){
        double z = -50 + 100/400.*i;

        double x = (z/42.8625 + 0.5)* ( hit.x2-hit.x1) + hit.x1;

        hRay->Fill(x,z);
      }

      double ex = hit.Rho2Ex( ((hit.xAvg - rhoOffset)/1000/rhoScaling + hit.GetRho0() )   );
      //if( XMIN < hit.xAvg && hit.xAvg < XMAX) printf("x1 : %6.2f, x2 : %6.2f, xAvg %6.2f cm , ex : %f \n", hit.x1, hit.x2, hit.xAvg, ex);
      hEx->Fill(ex);

      hEx_Multi->Fill(ex, *multi);
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
  TCanvas * canvas = new TCanvas("cc", "Split-Pole", 2500, 1000);

  gStyle->SetOptStat("neiou");

  canvas->Divide(5, 2);

  canvas->cd(1); {
    PID->Draw("colz");
    if( pidCut ) pidCut->Draw("same");
  }

  canvas->cd(2); hRay->Draw("colz");

  canvas->cd(3); hF->Draw();
  canvas->cd(4); hB->Draw();

  canvas->cd(5); hXavg_Q->Draw("colz");

  canvas->cd(6); hXavg->Draw("colz");

  canvas->cd(7); hEx->Draw();

  //canvas->cd(8); coin->Draw("colz");
  canvas->cd(8); hEx_Multi->Draw("colz");

  canvas->cd(9); canvas->cd(9)->SetLogy(); hMulti->Draw();

  canvas->cd(10); hXavg_Theta->Draw("colz");

}

#endif