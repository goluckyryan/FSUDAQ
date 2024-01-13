#include "fsuReader.h"

#include "TROOT.h"
#include "TSystem.h"
#include "TClonesArray.h"
#include "TGraph.h"
#include "TFile.h"
#include "TTree.h"

#define MAX_MULTI  100
#define TIMEJUMP 1e8 // 0.1 sec or 10 Hz, any signal less than 10 Hz should increase the value.

#define ORDERSHIFT 100000

typedef unsigned short uShort;
typedef unsigned int uInt;
typedef unsigned long  uLong;
typedef unsigned long long ullong;

struct FileInfo {
  TString fileName;
  unsigned int fileSize;
  unsigned int SN;
  unsigned long hitCount;
  unsigned short DPPType;
  unsigned short tick2ns;
  unsigned short order;
  unsigned short readerID;

  unsigned ID; // sn + 100000 * order

  void CalOrder(){ ID = ORDERSHIFT * order + SN; }

  void Print(){
    printf("%6d | %3d | %30s | %2d | %6lu | %u Bytes = %.2f MB\n", 
            ID, DPPType, fileName.Data(), tick2ns, hitCount, fileSize, fileSize/1024./1024.);
    
  }
};

struct GroupInfo{

  std::vector<unsigned short> readerIDList;

  unsigned short currentID;

};

//^#############################################################
//^#############################################################
int main(int argc, char **argv) {
  
  printf("=========================================\n");
  printf("===      *.fsu Events Builder         ===\n");
  printf("=========================================\n");  
  if (argc <= 3)    {
    printf("Incorrect number of arguments:\n");
    printf("%s [timeWindow] [verbose] [inFile1]  [inFile2] .... \n", argv[0]);
    printf("    timeWindow : in ns \n");   
    printf("       verbose : > 0 for debug  \n");   
    printf("    Output file name is contructed from inFile1 \n");   
    printf("\n");
    printf("  * there is a TIMEJUMP = 1e8 ns in EventBuilder.cpp.\n");
    printf("       This control the time diff for a time jumping.\n");
    printf("       Any signal with trigger rate < 1/TIMEJUMP should increase the value.\n");
    return 1;
  }

/// File format must be YYY...Y_runXXX_AAA_BBB_CCC.fsu
  /// YYY...Y  = prefix
  /// XXX = runID, 3 digits
  /// AAA = board Serial Number, 3 digits
  /// BBB = DPPtype, 3 digits
  /// CCC = over size index, 3 digits
  
  ///============= read input
  unsigned int  timeWindow = atoi(argv[1]);
  //float bufferSize = atof(argv[2]);
  //bool traceOn = atoi(argv[3]);
  bool traceOn = false;
  unsigned int debug = atoi(argv[2]);
  int nFile = argc - 3;
  TString inFileName[nFile];
  for( int i = 0 ; i < nFile ; i++){
    inFileName[i] = argv[i+3];
  }
  
  /// Form outFileName;
  TString outFileName = inFileName[0];
  int pos = outFileName.Index("_");
  pos = outFileName.Index("_", pos+1);
  outFileName.Remove(pos);  
  outFileName += ".root";
  printf("-------> Out file name : %s \n", outFileName.Data());
  
  
  printf(" Number of Files : %d \n", nFile);
  for( int i = 0; i < nFile; i++) printf("%2d | %s \n", i, inFileName[i].Data());
  printf("=====================================\n");  
  printf(" Time Window = %u ns = %.1f us\n", timeWindow, timeWindow/1000.);
  //printf(" Buffer size = %.0f event/channel\n", MaxNData * bufferSize);
  printf("===================================== input files:\n");  

  ///============= sorting file by the serial number & order
  std::vector<FileInfo> fileInfo;
  FSUReader ** reader = new FSUReader*[nFile];

  // file name format is expName_runID_SN_DPP_tick2ns_order.fsu
  for( int i = 0; i < nFile; i++){
  
    printf("Processing %s (%d/%d) ..... \n\033[A\r", inFileName[i].Data(), i+1, nFile);

    reader[i] = new FSUReader(inFileName[i].Data(), false);
    reader[i]->ScanNumBlock(false);
  
    FileInfo tempInfo;
    tempInfo.fileName = inFileName[i];
    tempInfo.readerID = i;
    tempInfo.SN = reader[i]->GetSN();
    tempInfo.hitCount = reader[i]->GetHitCount();
    tempInfo.fileSize = reader[i]->GetFileByteSize();
    tempInfo.tick2ns = reader[i]->GetTick2ns();
    tempInfo.DPPType = reader[i]->GetDPPType();
    tempInfo.order = reader[i]->GetFileOrder();

    tempInfo.CalOrder();

    fileInfo.push_back(tempInfo);

  }

  std::sort(fileInfo.begin(), fileInfo.end(), [](const FileInfo& a, const FileInfo& b) {
    return a.ID < b.ID;
  });
  
  unsigned int totHitCount = 0;
  
  for( int i = 0 ; i < nFile; i++){
    printf("%d |", i);
    fileInfo[i].Print();
    totHitCount += fileInfo[i].hitCount;
  }

  printf("----- total number of block : %u.\n", totHitCount);
  
  //*======================================= Sort files into groups 
  std::vector<GroupInfo> group;

  for( int i = 0; i < nFile; i++){
    if( fileInfo[i].ID / ORDERSHIFT == 0 ){
      group.push_back(GroupInfo());
      group.back().readerIDList.push_back(fileInfo[i].readerID); // an empty struct
      group.back().currentID = fileInfo[i].readerID;
    }else{
      group.back().readerIDList.push_back(fileInfo[i].readerID);
    }

  }

  int nGroup = group.size();
  printf("===================================== number of file Group by digitizer %d.\n", nGroup);  
  for( int i = 0; i < nGroup; i++){
    printf(" Digi-%d, DPPType: %d \n", reader[group[i].currentID]->GetSN(), reader[group[i].currentID]->GetDPPType());
    for( int j = 0; j< (int) group[i].readerIDList.size(); j++){
      uShort rID = group[i].readerIDList[j];
      printf("             %s \n", reader[rID]->GetFileName().c_str());

      uLong hitCount = reader[rID]->GetHitCount();
      for( int k = 0; k < (hitCount < 10 ? hitCount : 10); k++){
        (reader[group[i].readerIDList[j]]->GetHit(k)).Print();
      }

    }
  }

  // //*====================================== create tree
  // TFile * outRootFile = new TFile(outFileName, "recreate");
  // TTree * tree = new TTree("tree", outFileName);

  // unsigned long long                evID = -1;
  // unsigned short                   multi = 0;
  // unsigned short           sn[MAX_MULTI] = {0}; /// board SN
  // unsigned short           bd[MAX_MULTI] = {0}; /// boardID
  // unsigned short           ch[MAX_MULTI] = {0}; /// chID
  // unsigned short            e[MAX_MULTI] = {0}; /// 15 bit
  // unsigned short           e2[MAX_MULTI] = {0}; /// 15 bit
  // unsigned long long      e_t[MAX_MULTI] = {0}; /// timestamp 47 bit
  // unsigned short          e_f[MAX_MULTI] = {0}; /// fine time 10 bit 

  // tree->Branch("evID",           &evID, "event_ID/l"); 
  // tree->Branch("multi",         &multi, "multi/s"); 
  // tree->Branch("sn",                sn, "sn[multi]/s");
  // tree->Branch("bd",                bd, "bd[multi]/s");
  // tree->Branch("ch",                ch, "ch[multi]/s");
  // tree->Branch("e",                  e, "e[multi]/s");
  // tree->Branch("e2",                e2, "e2[multi]/s");
  // tree->Branch("e_t",              e_t, "e_timestamp[multi]/l");
  // tree->Branch("e_f",              e_f, "e_timestamp[multi]/s");
  
  // TClonesArray * arrayTrace = nullptr;
  // unsigned short  traceLength[MAX_MULTI] = {0};
  // TGraph * trace = nullptr;

  // if( traceOn ) {
  //   arrayTrace = new TClonesArray("TGraph");
  //   tree->Branch("traceLength", traceLength, "traceLength[multi]/s");
  //   tree->Branch("trace",       arrayTrace, 2560000);
  //   arrayTrace->BypassStreamer();
  // }


  //*====================================== build events
  printf("================= Building events....\n");

  // //Find earlist time among the first file 
  unsigned long long t0 = -1;
  unsigned short group0 = -1;
  for( int gpID = 0; gpID < nGroup; gpID++){
    uShort rID = group[gpID].currentID;
    unsigned long long t = reader[rID]->GetHit(0).timestamp;
    if( t < t0 ) {
      t0 = t;
      group0 = gpID;
    }
  }

  printf("the eariliest time is %llu at %s\n", t0, reader[group[group0].currentID]->GetFileName().c_str());

  do{

    std::vector<Hit> event;

    for(int i = 0; i < nGroup; i++){

      


    }

  }while(true);



  // printf("========================= finished.\n");
  // printf("total events built = %llu(%llu)\n", evID + 1, tree->GetEntriesFast());
  // printf("=======> saved to %s \n", outFileName.Data());

  // outRootFile->Close();

  for( int i = 0 ; i < nFile; i++) delete reader[i];
  delete [] reader;

}

