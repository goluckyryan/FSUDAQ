#include "fsuReader.h"
#include "fsutsReader.h"

#include "TROOT.h"
#include "TSystem.h"
#include "TClonesArray.h"
#include "TGraph.h"
#include "TFile.h"
#include "TTree.h"

#define MAX_MULTI  1000

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
    printf("     1a) if the *.fsu file has trace, *.fsu.ts will also has trace.\n");
    printf("  2) Load the *.fsu.X files and do the event building\n");
    printf("\n\n");
    return 1;
  }

  uInt runStartTime = get_time_us();

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
  int pos = outFileName.Last('/');
  pos = outFileName.Index("_", pos+1); // find next "_"
  pos = outFileName.Index("_", pos+1); // find next "_"
  outFileName.Remove(pos); // remove the rest
  outFileName += "_" + std::to_string(timeWindow);
  outFileName += ".root";
  printf("-------> Out file name : %s \n", outFileName.Data());
  
  
  printf(" Number of Files : %d \n", nFile);
  for( int i = 0; i < nFile; i++) printf("%2d | %s \n", i, inFileName[i].Data());
  printf("=====================================\n");  
  printf(" Time Window = %u ns = %.1f us\n", timeWindow, timeWindow/1000.);
  //printf(" Buffer size = %.0f event/channel\n", MaxNData * bufferSize);
  printf("===================================== Breaking down files\n");  
  
  ///========================================
  printf("===================================== Load the files\n");  

  //check if all input files is ts file;
  bool isTSFiles = false;
  int count = 0;
  for( int i = 0; i < nFile; i++){
    FILE * temp = fopen(inFileName[i].Data(), "r");
    uint32_t header;
    fread(&header, 4, 1, temp);
    if( (header >> 24) == 0xAA ) count++; 
  }
  if( count == nFile ) isTSFiles = true;

  std::vector<FileInfo> fileInfo; 

  if( !isTSFiles ){
    printf("######### All files are not time-sorted files\n");

    ///============= sorting file by the serial number & order
    FSUReader ** reader = new FSUReader*[nFile];
    // file name format is expName_runID_SN_DPP_tick2ns_order.fsu
    for( int i = 0; i < nFile; i++){
      printf("Processing %s (%d/%d) ..... \n", inFileName[i].Data(), i+1, nFile);
      reader[i] = new FSUReader(inFileName[i].Data(), 600, false);

      if( !reader[i]->isOpen() ){
        printf("------- cannot open file.\n");
        continue;
      }
      if( reader[i]->GetFileByteSize() == 0 ){
        printf("------- file size is ZERO.\n");
        continue;
      }

      reader[i]->ScanNumBlock(false, 2); 
    
      std::string outFileName = reader[i]->SaveHit2NewFile(tempFolder);

      FileInfo tempInfo;
      tempInfo.fileName = outFileName;
      tempInfo.readerID = i;
      tempInfo.SN = reader[i]->GetSN();
      tempInfo.hitCount = reader[i]->GetHitCount();
      tempInfo.fileSize = reader[i]->GetTSFileSize();
      tempInfo.tick2ns = reader[i]->GetTick2ns();
      tempInfo.DPPType = reader[i]->GetDPPType();
      tempInfo.order = reader[i]->GetFileOrder();
      tempInfo.CalOrder();

      tempInfo.t0 = reader[i]->GetHit(0).timestamp;

      fileInfo.push_back(tempInfo);
    
      delete reader[i];
    }
    delete [] reader;

  }else{

    printf("######### All files are time sorted files\n");

    FSUTSReader ** reader = new FSUTSReader*[nFile];
    // file name format is expName_runID_SN_DPP_tick2ns_order.fsu
    for( int i = 0; i < nFile; i++){
      printf("Processing %s (%d/%d) ..... \n", inFileName[i].Data(), i+1, nFile);
      reader[i] = new FSUTSReader(inFileName[i].Data(), false);

      if( !reader[i]->isOpen() ){
        printf("------- cannot open file.\n");
        continue;
      }
      reader[i]->ScanFile(0); 

      if( reader[i]->GetNumHit() == 0 ){
        printf("------- file has no data.\n");
        continue;
      }

      FileInfo tempInfo;
      tempInfo.fileName = inFileName[i].Data();
      tempInfo.readerID = i;
      tempInfo.SN = reader[i]->GetSN();
      tempInfo.hitCount = reader[i]->GetNumHit();
      tempInfo.fileSize = reader[i]->GetFileByteSize();
      tempInfo.order = reader[i]->GetFileOrder();
      tempInfo.CalOrder();

      tempInfo.t0 = reader[i]->GetT0();;

      fileInfo.push_back(tempInfo);
    
      delete reader[i];
    }
    delete [] reader;

  }

  nFile = (int) fileInfo.size();

  std::sort(fileInfo.begin(), fileInfo.end(), [](const FileInfo& a, const FileInfo& b) {
    return a.ID < b.ID;
  });
  
  unsigned int totHitCount = 0;
  
  printf("===================================== number of file %d.\n", nFile);  
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
    if( i == 0 || group.back().sn != fileInfo[i].SN  ){
      group.push_back(GroupInfo());
      group.back().fileIDList.push_back(i); // an empty struct
      group.back().currentID = 0;
      group.back().hitCount = fileInfo[i].hitCount;
      group.back().sn = fileInfo[i].SN;
      group.back().finished = false;

    }else{
      group.back().fileIDList.push_back(i);
    }
  }

  int nGroup = group.size();
  printf("===================================== number of file Group by digitizer %d.\n", nGroup);  
  for( int i = 0; i < nGroup; i++){
    printf(" Digi-%d, DPPType: %d \n", group[i].sn, fileInfo[group[i].currentID].DPPType);
    for( int j = 0; j< (int) group[i].fileIDList.size(); j++){
      uShort fID = group[i].fileIDList[j];
      printf("             %s \n", fileInfo[fID].fileName.c_str());
    }
  }

  // //*====================================== create tree
  TFile * outRootFile = new TFile(outFileName, "recreate");
  TTree * tree = new TTree("tree", outFileName);

  unsigned long long                evID = 0;
  unsigned int                     multi = 0;
  unsigned short           sn[MAX_MULTI] = {0}; /// board SN
  unsigned short           ch[MAX_MULTI] = {0}; /// chID
  unsigned short            e[MAX_MULTI] = {0}; /// 15 bit
  unsigned short           e2[MAX_MULTI] = {0}; /// 15 bit
  unsigned long long      e_t[MAX_MULTI] = {0}; /// timestamp 47 bit
  unsigned short          e_f[MAX_MULTI] = {0}; /// fine time 10 bit 
  unsigned short  traceLength[MAX_MULTI];

  tree->Branch("evID",           &evID, "event_ID/l"); 
  tree->Branch("multi",         &multi, "multi/i"); 
  tree->Branch("sn",                sn, "sn[multi]/s");
  tree->Branch("ch",                ch, "ch[multi]/s");
  tree->Branch("e",                  e, "e[multi]/s");
  tree->Branch("e2",                e2, "e2[multi]/s");
  tree->Branch("e_t",              e_t, "e_timestamp[multi]/l");
  tree->Branch("e_f",              e_f, "e_timestamp[multi]/s");
  tree->Branch("traceLength", traceLength, "traceLength[multi]/s");
  
  TClonesArray * arrayTrace = nullptr;
  TGraph * trace = nullptr;

  if( traceOn ) {
    arrayTrace = new TClonesArray("TGraph");
    tree->Branch("trace",       arrayTrace, 2560000);
    arrayTrace->BypassStreamer();
  }

  //*======================================= Open time-sorted files
  printf("===================================== Open time-sorted files.\n"); 

  FSUTSReader ** tsReader = new FSUTSReader * [nGroup];
  for( int i = 0; i < nGroup; i++){
    std::string fileName = fileInfo[group[i].fileIDList[0]].fileName;
    tsReader[i] = new FSUTSReader(fileName);
    tsReader[i]->ScanFile(0);
    group[i].usedHitCount = 0;
  }

  //*====================================== build events
  printf("================= Building events....\n");
  
  uInt hitProcessed = 0;

  //find the earliest time
  ullong t0 = -1;
  uShort gp0 = -1;

  ullong tStart = 0;
  ullong tEnd = 0;

  bool hasEvent = false;

  for( int i = 0; i < nGroup; i++){
    if( fileInfo[group[i].fileIDList[0]].t0 < t0 ) {
      t0 = fileInfo[group[i].fileIDList[0]].t0;
      gp0 = i;
    }
  } 

  if( debug ) printf("First timestamp is %llu, group : %u\n", t0, gp0);
  do{

    if( debug ) printf("################################ ev build %llu \n", evID);

    ///===================== check if the file is finished.
    for( int i = 0; i < nGroup; i++){
      uShort gpID = (i + gp0) % nGroup;
      if( group[gpID].finished ) continue;
      short endCount = 0;
      do{

        if( group[gpID].usedHitCount > tsReader[gpID]->GetHitID() || tsReader[gpID]->GetFilePos() <= 4){
          if( tsReader[gpID]->ReadNextHit(traceOn, 0) == 0 ){ 
            hitProcessed ++;
            if( debug ){ printf("............ Get Data | "); tsReader[gpID]->GetHit()->Print();}
          }
        }

        if( tsReader[gpID]->GetHit()->timestamp - t0 <= timeWindow ) {

          if( evID == 0) tStart = tsReader[gpID]->GetHit()->timestamp;
          if( hitProcessed >= totHitCount ) tEnd = tsReader[gpID]->GetHit()->timestamp;

          sn[multi] = tsReader[gpID]->GetHit()->sn;
          ch[multi] = tsReader[gpID]->GetHit()->ch;
          e[multi] = tsReader[gpID]->GetHit()->energy;
          e2[multi] = tsReader[gpID]->GetHit()->energy2;
          e_t[multi] = tsReader[gpID]->GetHit()->timestamp;
          e_f[multi] = tsReader[gpID]->GetHit()->fineTime;
          
          traceLength[multi] = tsReader[gpID]->GetHit()->traceLength;
          if( traceOn ){
            trace = (TGraph *) arrayTrace->ConstructedAt(multi, "C");
            trace->Clear();
            for( int hh = 0; hh < traceLength[multi]; hh++){
              trace->SetPoint(hh, hh, tsReader[gpID]->GetHit()->trace[hh]);
            }
          }

          if( debug ) printf("(%5d, %2d) %6d %16llu, %u\n", sn[multi], ch[multi], e[multi], e_t[multi], traceLength[multi]);

          hasEvent = true;
          multi ++;

          group[gpID].usedHitCount ++;
          if( tsReader[gpID]->ReadNextHit(traceOn, 0) == 0 ){ 
            hitProcessed ++;
            if( debug ){ printf("..Get Data after fill | "); tsReader[gpID]->GetHit()->Print();}
          }

          if( multi > MAX_MULTI) {
            printf("  !!!!!! multi > %d\n", MAX_MULTI);
          }

        }else{
          break;
        }

        if( timeWindow == 0) break;

        if( tsReader[gpID]->GetHitID() + 1 >= tsReader[gpID]->GetNumHit() ) endCount ++; 
        if( endCount == 2 ) break;

      }while(true);

    }

    if( hasEvent ){
      outRootFile->cd();
      tree->Fill();
      multi = 0;
      evID ++;
      hasEvent = false;
    }

    if( hitProcessed % 10000 == 0 ) printf("hit Porcessed %u/%u hit....%.2f%%\n\033[A\r", hitProcessed, totHitCount,  hitProcessed*100./totHitCount);
    
    ///===================== find the next first timestamp
    t0 = -1;
    gp0 = -1;

    for( int i = 0; i < nGroup; i++) {
      if( group[i].finished ) continue;
      if( tsReader[i]->GetHit()->timestamp < t0) {
        t0 = tsReader[i]->GetHit()->timestamp;
        gp0 = i;
      }
    } 
    if( debug ) printf("Next First timestamp is %llu, group : %u\n", t0, gp0);


    ///===================== check if the file is finished.
    int gpCount = 0;
    for( int gpID = 0; gpID < nGroup; gpID ++) {      
      if( group[gpID].finished ) {
        gpCount ++;
        continue;
      }

      if( group[gpID].usedHitCount >= tsReader[gpID]->GetNumHit() ) {

        group[gpID].currentID ++;

        if( group[gpID].currentID >= group[gpID].fileIDList.size() ) {
          group[gpID].finished = true;
          printf("-----> no more file for this group, S/N : %d.\n", group[gpID].sn);
          
        }else{
          uShort fID = group[gpID].fileIDList[group[gpID].currentID];
          std::string fileName = fileInfo[fID].fileName;

          delete tsReader[gpID];
          tsReader[gpID] = new FSUTSReader(fileName);
          tsReader[gpID]->ScanFile(1);
          printf("-----> go to the next file, %s \n", fileName.c_str() );

          group[gpID].usedHitCount = 0;

        }
      }

      if( group[gpID].finished ) gpCount ++;
    }
    if( gpCount == (int) group.size() ) break;

  }while(true);

  tree->Write();

  uInt runEndTime = get_time_us();
  double runTime = (runEndTime - runStartTime) * 1e-6;

  printf("========================= finished.\n");
  printf(" event building time = %.2f sec = %.2f min\n", runTime, runTime/60.);
  printf("  total events built = %llu by event builder (%llu in tree)\n", evID, tree->GetEntriesFast());
  double tDuration_sec = (tEnd - tStart) * 1e-9;
  printf("     first timestamp = %20llu ns\n", tStart);
  printf("      last timestamp = %20llu ns\n", tEnd);
  printf(" total data duration = %.2f sec = %.2f min\n", tDuration_sec, tDuration_sec/60.);
  printf("=======> saved to %s \n", outFileName.Data());

  outRootFile->Close();

  for( int i = 0; i < nGroup; i++){
    delete tsReader[i];
  }
  delete tsReader;

  return 0;
}

