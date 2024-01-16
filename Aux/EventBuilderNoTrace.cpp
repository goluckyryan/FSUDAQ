#include "fsuReader.h"
#include <chrono>

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
  uInt sn;
  unsigned short currentID ; // the ID of the readerIDList;
  ulong hitCount ; // this is the hitCount for the currentID;
  ulong hitID ; // this is the ID for the reader->GetHit(hitID);
  bool finished;

};


unsigned long long getTime_ns(){

  std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
  std::chrono::nanoseconds nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime.time_since_epoch());
  return nanoseconds.count();

}


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
    // reader[i]->FillHitList();
  
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

  printf("----- total number of hit : %u.\n", totHitCount);
  
  //*======================================= Sort files into groups 
  std::vector<GroupInfo> group;

  for( int i = 0; i < nFile; i++){
    if( fileInfo[i].ID / ORDERSHIFT == 0 ){
      group.push_back(GroupInfo());
      group.back().readerIDList.push_back(fileInfo[i].readerID); // an empty struct
      group.back().currentID = 0;
      group.back().hitCount = fileInfo[i].hitCount;
      group.back().hitID = 0;
      group.back().sn = fileInfo[i].SN;
      group.back().finished = false;

    }else{
      group.back().readerIDList.push_back(fileInfo[i].readerID);
    }

  }

  int nGroup = group.size();
  printf("===================================== number of file Group by digitizer %d.\n", nGroup);  
  for( int i = 0; i < nGroup; i++){
    printf(" Digi-%d, DPPType: %d \n", reader[group[i].readerIDList[0]]->GetSN(), reader[group[i].readerIDList[0]]->GetDPPType());
    for( int j = 0; j< (int) group[i].readerIDList.size(); j++){
      uShort rID = group[i].readerIDList[j];
      printf("             %s \n", reader[rID]->GetFileName().c_str());

      uLong hitCount = reader[rID]->GetHitCount();
      for( uLong k = 0; k < (hitCount < 5 ? hitCount : 10); k++){
        reader[rID]->GetHit(k).Print();
      }
    }
  }

  // //*====================================== create tree
  TFile * outRootFile = new TFile(outFileName, "recreate");
  TTree * tree = new TTree("tree", outFileName);

  unsigned long long                evID = -1;
  unsigned short                   multi = 0;
  unsigned short           sn[MAX_MULTI] = {0}; /// board SN
  //unsigned short           bd[MAX_MULTI] = {0}; /// boardID
  unsigned short           ch[MAX_MULTI] = {0}; /// chID
  unsigned short            e[MAX_MULTI] = {0}; /// 15 bit
  unsigned short           e2[MAX_MULTI] = {0}; /// 15 bit
  unsigned long long      e_t[MAX_MULTI] = {0}; /// timestamp 47 bit
  unsigned short          e_f[MAX_MULTI] = {0}; /// fine time 10 bit 

  tree->Branch("evID",           &evID, "event_ID/l"); 
  tree->Branch("multi",         &multi, "multi/s"); 
  tree->Branch("sn",                sn, "sn[multi]/s");
  //tree->Branch("bd",                bd, "bd[multi]/s");
  tree->Branch("ch",                ch, "ch[multi]/s");
  tree->Branch("e",                  e, "e[multi]/s");
  tree->Branch("e2",                e2, "e2[multi]/s");
  tree->Branch("e_t",              e_t, "e_timestamp[multi]/l");
  tree->Branch("e_f",              e_f, "e_timestamp[multi]/s");
  
  //TClonesArray * arrayTrace = nullptr;
  //unsigned short  traceLength[MAX_MULTI] = {0};
  //TGraph * trace = nullptr;

  if( traceOn ) {
    // arrayTrace = new TClonesArray("TGraph");
    // tree->Branch("traceLength", traceLength, "traceLength[multi]/s");
    // tree->Branch("trace",       arrayTrace, 2560000);
    // arrayTrace->BypassStreamer();
  }


  //*====================================== build events
  printf("================= Building events....\n");

  evID = 0;
  std::vector<Hit> event;
  Hit temp;

  ullong t0 = -1;
  uShort group0 = -1;

  uInt hitProcessed = 0;

  do{

    event.clear();
    t0 = -1;

    //// Find earliest time
    // ullong torg = getTime_ns(); 
    for( int gpID = 0; gpID < nGroup; gpID++){

      if( group[gpID].finished ) continue;

      //when all hit are used, go to next file or make the group.finished = true
      if( group[gpID].hitID >=  group[gpID].hitCount) {

        printf(" group ID : %d, reader ID : %d is finished. \n", gpID, group[gpID].readerIDList[group[gpID].currentID]);

        group[gpID].currentID ++;

        if( group[gpID].currentID >= group[gpID].readerIDList.size() ) {
          group[gpID].finished = true;
          printf("-----> no more file for this group, S/N : %d.\n", group[gpID].sn);
          continue;
        }else{
          group[gpID].hitID = 0;
          uShort rID = group[gpID].readerIDList[group[gpID].currentID];
          group[gpID].hitCount = reader[rID]->GetHitCount();
          printf("-----> go to the next file, %s \n", fileInfo[rID].fileName.Data() );
        }
      }

      uShort rID = group[gpID].readerIDList[group[gpID].currentID];
      ulong hitID = group[gpID].hitID;
      ullong t = reader[rID]->GetHit(hitID).timestamp;
      if( t < t0 ) {
        t0 = t;
        group0 = gpID;
      }
    }
    if (debug ) printf("the eariliest time is %llu at Group : %u, hitID : %lu, %s\n", t0, group0, group[group0].hitID, fileInfo[group[group0].currentID].fileName.Data());

    // ullong t1 = getTime_ns();
    // printf("Find earliest Time used : %llu ns \n", t1 - torg);

    printf("hit Porcessed %u/%u....%.2f%%\n\033[A\r", hitProcessed, totHitCount,  hitProcessed*100./totHitCount);
    
    for(int i = 0; i < nGroup; i++){
      uShort gpID = (i + group0) % nGroup;

      if( group[gpID].finished ) continue;

      uShort rID = group[gpID].readerIDList[group[gpID].currentID];

      for( ulong iHit = group[gpID].hitID; iHit < group[gpID].hitCount; iHit ++ ){

        if( reader[rID]->GetHit(iHit).timestamp - t0 <= timeWindow ) {

          event.push_back(reader[rID]->GetHit(iHit));
          group[gpID].hitID ++;
          hitProcessed ++;

        }else{
          break;
        }

        if( timeWindow == 0 ) break;
      }

      if( timeWindow == 0  ) break;
    }

    // ullong t2 = getTime_ns();
    // printf(" getting an event used %llu ns\n", t2 - t1);

    if( event.size() > 1) {
      std::sort(event.begin(), event.end(), [](const Hit& a, const Hit& b) {
        return a.timestamp < b.timestamp;
      });
    }

    multi = event.size();

    if (debug )printf("########### evID : %llu, multi : %u \n", evID, multi); 

    if( multi == 0 ) break;

    // ullong t3 = getTime_ns();
    // printf(" sort event used %llu ns\n", t3 - t2);


    for( size_t j = 0; j < multi ; j++){      
      sn[j]  = event[j].sn;
      ch[j]  = event[j].ch;
      e[j]   = event[j].energy;
      e2[j]  = event[j].energy2;
      e_t[j] = event[j].timestamp;
      e_f[j] = event[j].fineTime;

      if (debug )event[j].Print();
    } 

    outRootFile->cd();
    tree->Fill();

    // ullong t4 = getTime_ns();
    // printf(" Fill tree used %llu ns\n", t4 - t3);

    //check if all groups are finished 
    int gpCount = 0;
    for( size_t i = 0; i < group.size(); i++){
      if( group[i].finished ) gpCount ++;
    }
    if( gpCount == (int) group.size() ) break;

    evID ++;
  }while(true);

  tree->Write();

  printf("========================= finished.\n");
  printf("total events built = %llu(%llu)\n", evID, tree->GetEntriesFast());
  printf("=======> saved to %s \n", outFileName.Data());

  outRootFile->Close();

  for( int i = 0 ; i < nFile; i++) delete reader[i];
  delete [] reader;

}

