#include <TROOT.h>
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include "TTreeReaderArray.h"
#include <ROOT/TTreeProcessorMP.hxx>
#include <ROOT/TTreeProcessorMT.hxx>
#include "ROOT/TProcessExecutor.hxx"
#include "ROOT/TThreadedObject.hxx"

#include "TH2F.h"
#include "TH1F.h"
#include "TCutG.h"
#include "TCanvas.h"

#include "SplitPolePlotter.C"
#include "../analyzers/SplitPoleHit.h"


void SplotPolePlotter_MT(TChain * chain, const int nThread,  TCutG * pidCut = nullptr, double rhoOffset = 0, double rhoScaling = 1, bool isFSUDAQ = true){

  //^====================== Thread Object, destoryed when merge
  ROOT::TThreadedObject<TH2F> pCoin("hCoin", "Coincident ", 16, 0, 16, 16, 0, 16);
  ROOT::TThreadedObject<TH1F> phMulti("hMulti", "Multiplicity", 16, 0, 16); 
  ROOT::TThreadedObject<TH2F> pPID("hPID", "PID; Scin_X ; AnodeB", 200, 0, 20000, 100, 0, isFSUDAQ ? 40000 : 5000);
  ROOT::TThreadedObject<TH2F> phXavg_Q("hXavg_Q", "Xavg vs Q ", 200, XMIN, XMAX, 200, 0, isFSUDAQ ? 40000 : 5000);
  ROOT::TThreadedObject<TH1F> phF("hF", "Front delay line position", 600, XMIN, XMAX);
  ROOT::TThreadedObject<TH1F> phB("hB", "Back delay line position", 600, XMIN, XMAX);
  ROOT::TThreadedObject<TH1F> phXavg("hAvg", "Xavg", 600, XMIN, XMAX);
  ROOT::TThreadedObject<TH2F> phFocal("hFocal", "Front vs Back ", 200, XMIN, XMAX, 200, XMIN, XMAX);
  ROOT::TThreadedObject<TH2F> phXavg_Theta("hXavg_Theta", "Xavg vs Theta ", 200, XMIN, XMAX, 200, 0.5, 1.4);
  ROOT::TThreadedObject<TH2F> phRay("hRay", "Ray plot", 400, XMIN, XMAX, 400, -50, 50);
  ROOT::TThreadedObject<TH1F> phEx("hEx", "Ex; Ex [MeV]; count/10 keV", 600, -1, 5);
  ROOT::TThreadedObject<TH2F> phEx_Multi("hEx_Multi", "Ex vs Multi; Ex; Multi", 600, -1, 5, 16, 0, 16);


  //^==================== TTreeProcessorMT
  ROOT::EnableImplicitMT(nThread);

  std::vector<std::string_view> fileList_view;
  std::vector<std::string> fileList;
  for( int k = 0; k < chain->GetNtrees(); k++){
    fileList_view.push_back(chain->GetListOfFiles()->At(k)->GetTitle());
    fileList.push_back(chain->GetListOfFiles()->At(k)->GetTitle());
  }
  ROOT::TTreeProcessorMT tp(fileList_view, "tree");
  // tp.SetTasksPerWorkerHint(1);

  std::mutex mutex;

  int count = 0;

  //^======================= Define process
  auto ProcessTask = [&](TTreeReader &reader){

    TTreeReaderValue<ULong64_t>  evID = {reader, "evID"};
    TTreeReaderValue<UInt_t>    multi = {reader, "multi"};
    TTreeReaderArray<UShort_t>     sn = {reader, "sn"};
    TTreeReaderArray<UShort_t>     ch = {reader, "ch"};
    TTreeReaderArray<UShort_t>      e = {reader, "e"};
    TTreeReaderArray<UShort_t>     e2 = {reader, "e2"};
    TTreeReaderArray<ULong64_t>   e_t = {reader, "e_t"};
    TTreeReaderArray<UShort_t>    e_f = {reader, "e_f"};
      
    mutex.lock();
    count++;
    printf("-------------- Thread_ID: %d \n", count);
    mutex.unlock();

    SplitPoleHit hit;
    hit.SetMassTablePath("../analyzers/mass20.txt");
    hit.CalConstants("12C", "d", "p", 16, 18); // 80MeV, 5 deg
    hit.CalZoffset(0.750); // 1.41 T


    while (reader.Next()) {

      hit.ClearData();

      phMulti->Fill(*multi);

      // if( *multi != 9 ) continue;

      for( int i = 0; i < *multi; i++){

        // t2 = e_t[i];
        // if( t2 < t1 ) printf("entry %lld-%d, timestamp is not in order. %llu, %llu\n", processedEntries, i, t2, t1);
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
          pCoin->Fill(ch[i], ch[j]);
        }
      }

      unsigned int dQ = hit.eAB; // delta Q
      unsigned int Qt = hit.eSL; // total Q

      if( Qt > 0 && dQ > 0 ) {
        pPID->Fill(Qt, dQ);
      }

      //=============== PID gate cut
      if( pidCut  ){
        if( !pidCut->IsInside(Qt, dQ) ) continue;
      }

      hit.CalData(2);

      if( hit.theta > 1.2 || 0.5 > hit.theta ) continue;

      if( (!TMath::IsNaN(hit.x1) || !TMath::IsNaN(hit.x2)) ) {
        phFocal->Fill(hit.x1, hit.x2);
        phF->Fill(hit.x1);
        phB->Fill(hit.x2);
        phXavg->Fill(hit.xAvg);
        phXavg_Q->Fill(hit.xAvg, dQ);
        phXavg_Theta->Fill( hit.xAvg, hit.theta);

        for( int i = 0; i < 400; i++){
          double z = -50 + 100/400.*i;

          double x = (z/42.8625 + 0.5)* ( hit.x2-hit.x1) + hit.x1;

          phRay->Fill(x,z);
        }

        double ex = hit.Rho2Ex( ((hit.xAvg - rhoOffset)/1000/rhoScaling + hit.GetRho0() )   );
        //if( XMIN < hit.xAvg && hit.xAvg < XMAX) printf("x1 : %6.2f, x2 : %6.2f, xAvg %6.2f cm , ex : %f \n", hit.x1, hit.x2, hit.xAvg, ex);

        phEx->Fill(ex);
        phEx_Multi->Fill(ex, *multi);
      }
    }

    return 0;
  };

  //^============================ Run TP
  tp.Process(ProcessTask);

  //^============================ Merge all ThreadedObject
  auto coin = pCoin.Merge();
  auto hMulti= phMulti.Merge();
  auto hEx = phEx.Merge();
  auto hEx_Multi = phEx_Multi.Merge();
  auto PID = pPID.Merge();
  auto hFocal = phFocal.Merge();
  auto hF = phF.Merge();
  auto hB = phB.Merge();
  auto hXavg = phXavg.Merge();
  auto hXavg_Q = phXavg_Q.Merge();
  auto hXavg_Theta = phXavg_Theta.Merge();
  auto hRay = phRay.Merge();

  //^============================== Plot
  gStyle->SetOptStat("neiou");

  TCanvas * canvas = new TCanvas("cc", "Split-Pole", 2500, 1000);
  canvas->Divide(5, 2);

  canvas->cd(1); {
    PID->DrawCopy("colz");
    if( pidCut ) pidCut->Draw("same");
  }

  canvas->cd(2); hRay->DrawCopy("colz");

  canvas->cd(3); hF->DrawCopy();
  canvas->cd(4); hB->DrawCopy();

  canvas->cd(5); hXavg_Q->DrawCopy("colz");

  canvas->cd(6); hXavg->DrawCopy("colz");

  canvas->cd(7); hEx->DrawCopy();

  //canvas->cd(8); coin->DrawCopy("colz");
  canvas->cd(8); hEx_Multi->DrawCopy("colz");

  canvas->cd(9); canvas->cd(9)->SetLogy(); hMulti->DrawCopy();

  canvas->cd(10); hXavg_Theta->DrawCopy("colz");

}