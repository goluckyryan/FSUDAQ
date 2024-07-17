#define splitpole_cxx

#include "splitpole.h"
#include <TH2.h>
#include <TH1.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TMath.h>


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

TH2F * hXavgVQ;

TH2F * haha;

TH1F * hEx;

ULong64_t t1, t2;

#define XMIN -20
#define XMAX  100

void splitpole::Begin(TTree * /*tree*/){

  TString option = GetOption();

  PID = new TH2F("hPID", "PID; Scin_X ; AnodeB", 200, 0, 30000, 100, 0, 70000);
  coin = new TH2F("hCoin", "Coincident ", 16, 0, 16, 16, 0, 16);

  hMulti = new TH1F("hMulti", "Multiplicity", 16, 0, 16); 

  hF = new TH1F("hF", "Front delay line position", 600, XMIN, XMAX);
  hB = new TH1F("hB", "Back delay line position", 600, XMIN, XMAX);
  hXavg = new TH1F("hAvg", "Xavg", 600, XMIN, XMAX);

  hFocal = new TH2F("hFocal", "Front vs Back ", 200, XMIN, XMAX, 200, XMIN, XMAX);
  hXavgVQ = new TH2F("hXavgVQ", "Xavg vs Q ", 200, XMIN, XMAX, 200, 0, 40000);

  haha = new TH2F("haha", "", 400, XMIN, XMAX, 400, -50, 50);

  hEx = new TH1F("hEx", "Ex; Ex [MeV]; count/100 keV", 250, -5, 20);

  hit.SetMassTablePath("../analyzers/mass20.txt");
  hit.CalConstants("12C", "12C", "4He", 80, 5); // 80MeV, 5 deg
  hit.CalZoffset(1.41); // 1.41 T

  t1 = 0;
  t2 = 0;

}


Bool_t splitpole::Process(Long64_t entry){

  b_multi->GetEntry(entry);
  b_ch->GetEntry(entry);
  b_e->GetEntry(entry);
  b_e2->GetEntry(entry);
  b_e_t->GetEntry(entry);
  b_e_f->GetEntry(entry);

  //if( multi < 9) return kTRUE;
  hit.ClearData();

  hMulti->Fill(multi);

  for( int i = 0; i < multi; i++){

    t2 = e_t[i];

    if( t2 < t1 ) printf("entry %lld-%d, timestamp is not in order. %llu, %llu\n", entry, i, t2, t1);

    if( i == 0 ) t1 = e_t[i];

    if( ch[i] == ChMap::ScinR )    {hit.eSR   = e[i]; hit.tSR   = e_t[i] + e_f[i]/1000.;}
    if( ch[i] == ChMap::ScinL )    {hit.eSL   = e[i]; hit.tSL   = e_t[i] + e_f[i]/1000.;}
    if( ch[i] == ChMap::dFR )      {hit.eFR   = e[i]; hit.tFR   = e_t[i] + e_f[i]/1000.;}
    if( ch[i] == ChMap::dFL )      {hit.eFL   = e[i]; hit.tFL   = e_t[i] + e_f[i]/1000.;}
    if( ch[i] == ChMap::dBR )      {hit.eBR   = e[i]; hit.tBR   = e_t[i] + e_f[i]/1000.;}
    if( ch[i] == ChMap::dBL )      {hit.eBL   = e[i]; hit.tBL   = e_t[i] + e_f[i]/1000.;}
    if( ch[i] == ChMap::Cathode )  {hit.eCath = e[i]; hit.tCath = e_t[i] + e_f[i]/1000.;}
    if( ch[i] == ChMap::AnodeF )   {hit.eAF   = e[i]; hit.tAF   = e_t[i] + e_f[i]/1000.;}
    if( ch[i] == ChMap::AnodeB )   {hit.eAB   = e[i]; hit.tAB   = e_t[i] + e_f[i]/1000.;}

    for( int j = i+1; j < multi; j++){
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

  hit.CalData();

  if( !TMath::IsNaN(hit.x1) || !TMath::IsNaN(hit.x2) ) {
    hFocal->Fill(hit.x1, hit.x2);
    hF->Fill(hit.x1);
    hB->Fill(hit.x2);
    hXavg->Fill(hit.xAvg);

    hXavgVQ->Fill(hit.xAvg, dQ);

    for( int i = 0; i < 400; i++){
      double y = -50 + 100/400.*i;

      double x = (y/42.8625 + 0.5)* ( hit.x2-hit.x1) + hit.x1;

      haha->Fill(x,y);
    }

    double ex = hit.Rho2Ex( (hit.xAvg/100 + 0.363)   );
    //if( XMIN < hit.xAvg && hit.xAvg < XMAX) printf("x1 : %6.2f, x2 : %6.2f, xAvg %6.2f cm , ex : %f \n", hit.x1, hit.x2, hit.xAvg, ex);
    hEx->Fill(ex);
  }

  return kTRUE;
}


void splitpole::Terminate(){

  TCanvas * canvas = new TCanvas("cc", "Split-Pole", 1800, 1200);

  gStyle->SetOptStat("neiou");

  canvas->Divide(3, 3);

  canvas->cd(1); PID->Draw("colz");
  //canvas->cd(2); coin->Draw("colz");
  canvas->cd(2); haha->Draw("colz");

  canvas->cd(3); hF->Draw();
  canvas->cd(4); hB->Draw();

  canvas->cd(5); hXavgVQ->Draw("colz");

  canvas->cd(6); hXavg->Draw("colz");

  canvas->cd(7); hEx->Draw();

  canvas->cd(8); coin->Draw("colz");

  canvas->cd(9); canvas->cd(9)->SetLogy(); hMulti->Draw();

}
