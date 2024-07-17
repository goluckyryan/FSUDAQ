//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Wed Jun 14 15:51:03 2023 by ROOT version 6.26/04
// from TTree tree/temp_036.root
// found on file: temp_036.root
//////////////////////////////////////////////////////////

#ifndef splitpole_h
#define splitpole_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>
#include <TSelector.h>

// Header file for the classes stored in the TTree if any.

#define MaxMulti  100

class splitpole : public TSelector {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain

// Fixed size dimensions of array or collections stored in the TTree if any.

   // Declaration of leaf types
   ULong64_t       evID;
   UShort_t        multi;
   UShort_t        bd[MaxMulti];   //[multi]
   UShort_t        ch[MaxMulti];   //[multi]
   UShort_t        e[MaxMulti];   //[multi]
   UShort_t        e2[MaxMulti];   //[multi]
   ULong64_t       e_t[MaxMulti];   //[multi]
   UShort_t        e_f[MaxMulti];   //[multi]

   // List of branches
   TBranch        *b_event_ID;   //!
   TBranch        *b_multi;   //!
   TBranch        *b_bd;   //!
   TBranch        *b_ch;   //!
   TBranch        *b_e;   //!
   TBranch        *b_e2;   //!
   TBranch        *b_e_t;   //!
   TBranch        *b_e_f;   //!

   splitpole(TTree * /*tree*/ =0) : fChain(0) { }
   virtual ~splitpole() { }
   virtual Int_t   Version() const { return 2; }
   virtual void    Begin(TTree *tree);
   virtual void    SlaveBegin(TTree *tree);
   virtual void    Init(TTree *tree);
   virtual Bool_t  Notify();
   virtual Bool_t  Process(Long64_t entry);
   virtual Int_t   GetEntry(Long64_t entry, Int_t getall = 0) { return fChain ? fChain->GetTree()->GetEntry(entry, getall) : 0; }
   virtual void    SetOption(const char *option) { fOption = option; }
   virtual void    SetObject(TObject *obj) { fObject = obj; }
   virtual void    SetInputList(TList *input) { fInput = input; }
   virtual TList  *GetOutputList() const { return fOutput; }
   virtual void    SlaveTerminate();
   virtual void    Terminate();

   ClassDef(splitpole,0);
};

#endif

#ifdef splitpole_cxx
void splitpole::Init(TTree *tree){
   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("evID", &evID, &b_event_ID);
   fChain->SetBranchAddress("multi", &multi, &b_multi);
   fChain->SetBranchAddress("bd", bd, &b_bd);
   fChain->SetBranchAddress("ch", ch, &b_ch);
   fChain->SetBranchAddress("e", e, &b_e);
   fChain->SetBranchAddress("e2", e2, &b_e2);
   fChain->SetBranchAddress("e_t", e_t, &b_e_t);
   fChain->SetBranchAddress("e_f", e_f, &b_e_f);
}

Bool_t splitpole::Notify(){

   return kTRUE;
}


void splitpole::SlaveBegin(TTree * /*tree*/){

   TString option = GetOption();

}

void splitpole::SlaveTerminate(){

}


#endif // #ifdef splitpole_cxx
