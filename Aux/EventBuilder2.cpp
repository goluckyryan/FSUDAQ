#include "fsuReader.h"
#include "AggSeparator.h"
#include "../MultiBuilder.h"
#include <chrono>

#include "TROOT.h"
#include "TSystem.h"
#include "TClonesArray.h"
#include "TGraph.h"
#include "TFile.h"
#include "TTree.h"

#define MAX_MULTI  100

#define ORDERSHIFT 100000

struct FileInfo {
  TString fileName;
  unsigned int fileSize;
  unsigned int SN;
  unsigned long hitCount;
  unsigned short DPPType;
  unsigned short tick2ns;
  unsigned short order;
  unsigned short readerID;
  int chMask;

  long ID; 

  void CalOrder(){ ID = ORDERSHIFT * SN + 10 * order + (chMask == -1 ? 0 : chMask); }

  void Print(){
    printf("%6ld | %3d | %30s | %2d | %6lu | %u Bytes = %.2f MB\n", 
            ID, DPPType, fileName.Data(), tick2ns, hitCount, fileSize, fileSize/1024./1024.);  
  }
};

struct GroupInfo{

  std::vector<unsigned short> readerIDList;
  uInt sn;
  int chMask;
  unsigned short currentID ; // the ID of the readerIDList;
  ulong hitCount ; // this is the hitCount for the currentID;
  ulong hitID ; // this is the ID for the reader->GetHit(hitID);
  uShort DPPType; 
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
    printf("%s [timeWindow] [withTrace] [verbose] [tempFolder] [inFile1]  [inFile2] .... \n", argv[0]);
    printf("    timeWindow : in ns \n");   
    printf("     withTrace : 0 for no trace, 1 for trace \n");   
    printf("       verbose : > 0 for debug  \n");   
    printf("    tempFolder : temperary folder for file breakdown  \n");   
    printf("    Output file name is contructed from inFile1 \n");   
    printf("\n");
    printf("=========================== Working flow\n");
    printf("  1) Break down the fsu files into dual channel, save in tempFolder as *.fsu.X\n");
    printf("  2) Load the *.fsu.X files and do the event building\n");
    printf("\n\n");
    return 1;
  }

  /// File format must be YYY...Y_runXXX_AAA_BBB_TT_CCC.fsu
  /// YYY...Y  = prefix
  /// XXX = runID, 3 digits
  /// AAA = board Serial Number, 3 digits
  /// BBB = DPPtype, 3 digits
  ///  TT = tick2ns, any digits
  /// CCC = over size index, 3 digits
  
  ///============= read input
  unsigned int  timeWindow = atoi(argv[1]);
  bool traceOn = atoi(argv[2]);
  unsigned int debug = atoi(argv[3]);
  std::string tempFolder = argv[4];
  int nFile = argc - 5;
  TString inFileName[nFile];
  for( int i = 0 ; i < nFile ; i++){ inFileName[i] = argv[i+5];}
  
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
  printf("===================================== Breaking down files\n");  
  
  std::vector<std::string> tempFileList;
  
  for( int i = 0; i < nFile; i++){
    std::vector<std::string> haha = AggSeperator(inFileName[i].Data(), tempFolder);
    tempFileList.insert(tempFileList.end(), haha.begin(), haha.end()) ;
  }

  ///========================================
  printf("===================================== Load the files\n");  
  nFile = tempFileList.size();

  ///============= sorting file by the serial number & order
  std::vector<FileInfo> fileInfo; 
  FSUReader ** reader = new FSUReader*[nFile];

  // file name format is expName_runID_SN_DPP_tick2ns_order.fsu
  for( int i = 0; i < nFile; i++){
  
    printf("Processing %s (%d/%d) ..... \n\033[A\r", tempFileList[i].c_str(), i+1, nFile);

    reader[i] = new FSUReader(tempFileList[i], false);
    if( !reader[i]->isOpen() ) continue;

    reader[i]->ScanNumBlock(false, 0); //not saving data
    // reader[i]->FillHitList();
  
    FileInfo tempInfo;
    tempInfo.fileName = tempFileList[i];
    tempInfo.readerID = i;
    tempInfo.SN = reader[i]->GetSN();
    tempInfo.hitCount = reader[i]->GetHitCount();
    tempInfo.fileSize = reader[i]->GetFileByteSize();
    tempInfo.tick2ns = reader[i]->GetTick2ns();
    tempInfo.DPPType = reader[i]->GetDPPType();
    tempInfo.order = reader[i]->GetFileOrder();
    tempInfo.chMask = reader[i]->GetChMask();

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
    //printf(" %30s | ID %10ld \n", fileInfo[i].fileName.Data(), fileInfo[i].ID);
  }

  printf("----- total number of hit : %u.\n", totHitCount);
  
  //*======================================= Sort files into groups 
  std::vector<GroupInfo> group; // group by SN and chMask

  for( int i = 0; i < nFile; i++){
    if( i == 0 || group.back().sn != fileInfo[i].SN || group.back().chMask != fileInfo[i].chMask ){
      group.push_back(GroupInfo());
      group.back().readerIDList.push_back(fileInfo[i].readerID); // an empty struct
      group.back().currentID = 0;
      group.back().hitCount = fileInfo[i].hitCount;
      group.back().hitID = 0;
      group.back().sn = fileInfo[i].SN;
      group.back().chMask = fileInfo[i].chMask;
      group.back().DPPType = fileInfo[i].DPPType;
      group.back().finished = false;

    }else{
      group.back().readerIDList.push_back(fileInfo[i].readerID);
    }

  }

  int nGroup = group.size();
  printf("===================================== number of file Group by digitizer %d.\n", nGroup);  
  for( int i = 0; i < nGroup; i++){

    printf(" Digi-%d, chMask-%d, DPPType: %d \n", group[i].sn, group[i].chMask, reader[group[i].readerIDList[0]]->GetDPPType());
    for( int j = 0; j< (int) group[i].readerIDList.size(); j++){
      uShort rID = group[i].readerIDList[j];
      printf("             %s \n", reader[rID]->GetFileName().c_str());

      //uLong hitCount = reader[rID]->GetHitCount();
      //for( uLong k = 0; k < (hitCount < 5 ? hitCount : 5); k++){ reader[rID]->GetHit(k).Print();}
    }
  }

  // //*====================================== create tree
  TFile * outRootFile = new TFile(outFileName, "recreate");
  TTree * tree = new TTree("tree", outFileName);

  unsigned long long                evID = -1;
  unsigned short                   multi = 0;
  unsigned short           sn[MAX_MULTI] = {0}; /// board SN
  unsigned short           ch[MAX_MULTI] = {0}; /// chID
  unsigned short            e[MAX_MULTI] = {0}; /// 15 bit
  unsigned short           e2[MAX_MULTI] = {0}; /// 15 bit
  unsigned long long      e_t[MAX_MULTI] = {0}; /// timestamp 47 bit
  unsigned short          e_f[MAX_MULTI] = {0}; /// fine time 10 bit 

  tree->Branch("evID",           &evID, "event_ID/l"); 
  tree->Branch("multi",         &multi, "multi/s"); 
  tree->Branch("sn",                sn, "sn[multi]/s");
  tree->Branch("ch",                ch, "ch[multi]/s");
  tree->Branch("e",                  e, "e[multi]/s");
  tree->Branch("e2",                e2, "e2[multi]/s");
  tree->Branch("e_t",              e_t, "e_timestamp[multi]/l");
  tree->Branch("e_f",              e_f, "e_timestamp[multi]/s");
  
  TClonesArray * arrayTrace = nullptr;
  unsigned short  traceLength[MAX_MULTI];
  TGraph * trace = nullptr;

  if( traceOn ) {
    arrayTrace = new TClonesArray("TGraph");
    tree->Branch("traceLength", traceLength, "traceLength[multi]/s");
    tree->Branch("trace",       arrayTrace, 2560000);
    arrayTrace->BypassStreamer();
  }

  //*====================================== build events
  printf("================= Building events....\n");
  
  uInt hitProcessed = 0;

  std::vector<int> snList;
  std::vector<int> typeList; 
  Data ** data = nullptr;
  data = new Data * [nGroup];

  for( int i = 0; i < nGroup; i++){
    uShort rID = group[i].readerIDList[group[i].currentID];
    data[i] = reader[rID]->GetData();
    data[i]->ClearData();
    reader[i]->ClearHitListandCount();
    snList.push_back(group[i].sn);
    typeList.push_back(group[i].DPPType);
  }

  MultiBuilder * mb = new MultiBuilder(data, typeList, snList);
  mb->SetTimeWindow(timeWindow);
  mb->SetTimeJump(-1);

  do{

    // use reader to fill Data, 
    for(int gpID = 0; gpID < nGroup; gpID++){
      if( group[gpID].finished ) continue;

      uShort rID = group[gpID].readerIDList[group[gpID].currentID];

      int preLoad = 100; // #. pre-read block
      for( int i = 0; i < preLoad; i++ ) reader[rID]->ReadNextBlock(false, 0, 0);
      group[gpID].hitID = reader[rID]->GetHitCount();
      if( debug){
        printf(" %lu / %lu \n", group[gpID].hitID ,  group[gpID].hitCount);
        data[gpID]->PrintAllData();
      }

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
          group[gpID].hitCount = reader[rID]->GetHitCount();
          data[gpID] = reader[rID]->GetData();
          printf("-----> go to the next file, %s \n", fileInfo[rID].fileName.Data() );
        }
      }

    }

    mb->BuildEvents(false, !traceOn, debug);
    if( debug ) mb->PrintStat();

    ///----------- save to tree;
    long startIndex = mb->eventIndex - mb->eventBuilt + 1;
    //printf("startIndex : %6ld -> %6ld, %6ld, %6ld, %ld | %llu\n", startIndex, startIndex < 0 ? startIndex + MaxNEvent : startIndex, mb->eventIndex, mb->eventBuilt, mb->totalEventBuilt, tree->GetEntries());
    if (startIndex < 0 ) startIndex += MaxNEvent;
    for( long p = startIndex; p < startIndex + mb->eventBuilt; p++){
      int k =  p % MaxNEvent;
      multi = mb->events[k].size();
      if( multi > MAX_MULTI) {
        printf("!!!!! MAX_MULTI %d reached.\n", MAX_MULTI);
        break;
      }
      evID ++;
      for( int j = 0; j < multi; j ++){
        sn[j]  = mb->events[k][j].sn;
        ch[j]  = mb->events[k][j].ch;
        e[j]   = mb->events[k][j].energy;
        e2[j]  = mb->events[k][j].energy2;
        e_t[j] = mb->events[k][j].timestamp;
        e_f[j] = mb->events[k][j].fineTime;

        if( traceOn ){
          traceLength[j] = mb->events[k][j].trace.size();
          trace = (TGraph *) arrayTrace->ConstructedAt(j, "C");
          trace->Clear();
          for( int hh = 0; hh < traceLength[j]; hh++){
            trace->SetPoint(hh, hh, mb->events[k][j].trace[hh]);
          }
        }
        
        hitProcessed ++;

      }
      outRootFile->cd();
      tree->Fill();
    }

    int gpCount = 0;
    for( size_t i = 0; i < group.size(); i++){
      if( group[i].finished ) gpCount ++;
    }
    if( gpCount == (int) group.size() ) break;

  }while(true);


  if( timeWindow > 0 ){
    printf("------------------- build the last data\n");

    mb->BuildEvents(1, 0, debug);
    //mb->PrintStat();

    ///----------- save to tree;
    long startIndex = mb->eventIndex - mb->eventBuilt + 1;
    //printf("startIndex : %ld -> %ld, %ld, %ld, %ld\n", startIndex, startIndex < 0 ? startIndex + MaxNEvent : startIndex, mb->eventIndex, mb->eventBuilt, mb->totalEventBuilt);
    if( startIndex < 0 ) startIndex += MaxNEvent;
    for( long p = startIndex; p < startIndex + mb->eventBuilt; p++){
      int k =  p % MaxNEvent;
      multi = mb->events[k].size();
      if( multi > MAX_MULTI) {
        printf("!!!!! MAX_MULTI %d reached.\n", MAX_MULTI);
        break;
      }
      evID ++;
      for( int j = 0; j < multi; j ++){
        sn[j]  = mb->events[k][j].sn;
        ch[j]  = mb->events[k][j].ch;
        e[j]   = mb->events[k][j].energy;
        e2[j]  = mb->events[k][j].energy2;
        e_t[j] = mb->events[k][j].timestamp;
        e_f[j] = mb->events[k][j].fineTime;
        if( traceOn ){
          traceLength[j] = mb->events[k][j].trace.size();
          trace = (TGraph *) arrayTrace->ConstructedAt(j, "C");
          trace->Clear();
          for( int hh = 0; hh < traceLength[j]; hh++){
            trace->SetPoint(hh, hh, mb->events[k][j].trace[hh]);
          }
        }

        hitProcessed ++;
      }
      outRootFile->cd();
      tree->Fill();
    } 
  }

  tree->Write();

  printf("========================= finished.\n");
  printf("total events built = %llu(%llu)\n", evID + 1, tree->GetEntriesFast());
  printf("=======> saved to %s \n", outFileName.Data());

  outRootFile->Close();

  for( int i = 0 ; i < nFile; i++) delete reader[i];
  delete [] reader;

  delete [] data;

}

