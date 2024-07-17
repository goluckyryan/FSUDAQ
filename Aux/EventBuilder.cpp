#include "fsuReader.h"

#include "TROOT.h"
#include "TSystem.h"
#include "TClonesArray.h"
#include "TGraph.h"
#include "TFile.h"
#include "TTree.h"
#include "TMacro.h"
#include "TMath.h"

#define MAX_TRACE_LENGTH 2000
#define MAX_MULTI  100

struct FileInfo{

  std::string fileName;
  int fileID;
  unsigned long hitCount;

};

#define NMINARG 5 
#define debug 0

//^#############################################################
//^#############################################################
int main(int argc, char **argv) {
  
  printf("=========================================\n");
  printf("===      *.fsu Events Builder         ===\n");
  printf("=========================================\n");  
  if (argc < NMINARG)    {
    printf("Incorrect number of arguments:\n");
    printf("%s [timeWindow] [withTrace] [format] [inFile1]  [inFile2] .... \n", argv[0]);
    printf("    timeWindow : in ns, -1 = no event building \n");   
    printf("     withTrace : 0 for no trace, 1 for trace \n");   
    printf("        format : 0 for root, 1 for CoMPASS binary  \n");   
    printf("    Output file name is contructed from inFile1 \n");   
    printf("\n");
    printf(" Example: %s -1 0 0 '\\ls -1 *001*.fsu' (no event build, no trace, no verbose)\n", argv[0]);
    printf("          %s 100 0 0 '\\ls -1 *001*.fsu' (event build with 100 ns, no trace, no verbose)\n", argv[0]);
    printf("\n\n");

    return 1;
  }

  uInt runStartTime = getTime_us();

  ///============= read input
  long timeWindow = atoi(argv[1]);
  bool traceOn = atoi(argv[2]);
  // unsigned int debug = atoi(argv[3]);
  unsigned short format = atoi(argv[3]);
  unsigned int batchSize = 2* DEFAULT_HALFBUFFERSIZE;
  int nFile = argc - NMINARG + 1;
  TString inFileName[nFile];
  for( int i = 0 ; i < nFile ; i++){ inFileName[i] = argv[i + NMINARG -1];}
  
  /// Form outFileName;
  TString outFileName = inFileName[0];
  int pos = outFileName.Last('/');
  pos = outFileName.Index("_", pos+1); // find next "_"
  pos = outFileName.Index("_", pos+1); // find next "_"
  if( nFile == 1 ) pos = outFileName.Index("_", pos+1); // find next "_", S/N
  outFileName.Remove(pos); // remove the rest
  outFileName += "_" + ( timeWindow >= 0 ? std::to_string(timeWindow) : "single");

  TString outFileFullName;
  if( format == 0 ){
    outFileFullName = outFileName + ".root";
  }else{
    outFileFullName = outFileName + ".bin";
  }

  printf("-------> Out file name : %s \n", outFileFullName.Data());
  printf("========================================= Number of Files : %d \n", nFile);
  for( int i = 0; i < nFile; i++) printf("%2d | %s \n", i, inFileName[i].Data());
  printf("=========================================\n"); 
  printf("    Time Window = %ld ns = %.1f us\n", timeWindow, timeWindow/1000.);
  printf("  Include Trace = %s\n", traceOn ? "Yes" : "No");
  printf("    Debug level = %d\n", debug);
  printf(" Max multiplity = %d hits/event (hard coded)\n", MAX_MULTI);
  printf("========================================= Grouping files\n");  

  std::vector<std::vector<FileInfo>> fileGroupList; // fileName and ID = SN * 1000 + index
  std::vector<FileInfo> fileList;

  unsigned long long int totalHitCount = 0;

  FSUReader * readerA = new FSUReader(inFileName[0].Data(), 1, 1);
  readerA->ScanNumBlock(0,0);
  if( readerA->GetOptimumBatchSize() > batchSize ) batchSize = readerA->GetOptimumBatchSize();
  FileInfo fileInfo = {inFileName[0].Data(), readerA->GetSN() * 1000 +  readerA->GetFileOrder(), readerA->GetTotalHitCount()};
  fileList.push_back(fileInfo);
  totalHitCount += readerA->GetTotalHitCount();

  for( int i = 1; i < nFile; i++){
    FSUReader * readerB = new FSUReader(inFileName[i].Data(), 1, 1);
    readerB->ScanNumBlock(0,0);
    if( readerB->GetOptimumBatchSize() > batchSize ) batchSize = readerB->GetOptimumBatchSize();

    totalHitCount += readerB->GetTotalHitCount();
    fileInfo = {inFileName[i].Data(), readerB->GetSN() * 1000 +  readerB->GetFileOrder(), readerB->GetTotalHitCount()};
    
    if( readerA->GetSN() == readerB->GetSN() ){
      fileList.push_back(fileInfo);
    }else{
      fileGroupList.push_back(fileList);
      fileList.clear();
      fileList.push_back(fileInfo);
    }
  
    delete readerA;
    readerA = readerB;
  }
  fileGroupList.push_back(fileList);
  delete readerA;

  printf("======================= total Hit Count : %llu\n", totalHitCount);
  printf(">>>>>>>>>>>>>>>>>>>>>>>>>>   Batch size : %d events/file\n", batchSize);

  for( size_t i = 0; i < fileGroupList.size(); i++){
    printf("group ----- %ld \n", i);
    //sort by ID
    std::sort(fileGroupList[i].begin(), fileGroupList[i].end(), [](const FileInfo & a, const FileInfo & b) {
      return a.fileID < b.fileID;
    });
    for( size_t j = 0; j < fileGroupList[i].size(); j++){
      printf("%3ld | %8d | %9lu| %s \n", j, fileGroupList[i][j].fileID, fileGroupList[i][j].hitCount, fileGroupList[i][j].fileName.c_str() );
    }
  }

  TFile * outRootFile = nullptr;
  TTree * tree = nullptr;
  unsigned long long                evID = 0;
  unsigned int                     multi = 0;
  unsigned short           sn[MAX_MULTI] = {0}; /// board SN
  unsigned short           ch[MAX_MULTI] = {0}; /// chID
  unsigned short            e[MAX_MULTI] = {0}; /// 15 bit
  unsigned short           e2[MAX_MULTI] = {0}; /// 15 bit
  unsigned long long      e_t[MAX_MULTI] = {0}; /// timestamp 47 bit
  unsigned short          e_f[MAX_MULTI] = {0}; /// fine time 10 bit 
  unsigned short  traceLength[MAX_MULTI];
  short trace[MAX_MULTI][MAX_TRACE_LENGTH];

  FILE * caen = nullptr;

  if( format == 0 ){
    // //*====================================== create tree
    outRootFile = new TFile(outFileFullName, "recreate");
    tree = new TTree("tree", outFileFullName);
  
    tree->Branch("evID",           &evID, "event_ID/l"); 
    tree->Branch("multi",         &multi, "multi/i"); 
    tree->Branch("sn",                sn, "sn[multi]/s");
    tree->Branch("ch",                ch, "ch[multi]/s");
    tree->Branch("e",                  e, "e[multi]/s");
    tree->Branch("e2",                e2, "e2[multi]/s");
    tree->Branch("e_t",              e_t, "e_timestamp[multi]/l");
    tree->Branch("e_f",              e_f, "e_fineTime[multi]/s");
    tree->Branch("traceLength", traceLength, "traceLength[multi]/s");
  
    if( traceOn ) {
      tree->Branch("trace", trace,"trace[multi][MAX_TRACE_LENGTH]/S");
      tree->GetBranch("trace")->SetCompressionSettings(205);
    }
  }else{

    caen = fopen(outFileFullName.Data(), "wb");
    if( caen == nullptr ){
      perror("Failed to open file");
      return -1;
    }

  }

  //*======================================= Open files
  printf("========================================= Open files & Build Events.\n"); 

  const short nGroup = fileGroupList.size();
  std::vector<Hit> hitList[nGroup];
  
  FSUReader ** reader = new FSUReader * [nGroup];
  ulong ID[nGroup];
  for( short i = 0; i < nGroup; i++){
    std::vector<std::string> fList;
    for( size_t j = 0; j < fileGroupList[i].size(); j++){
      fList.push_back( fileGroupList[i][j].fileName );
    }
    reader[i] = new FSUReader(fList, 1024, debug); // 1024 is the maximum event / agg.
    hitList[i] = reader[i]->ReadBatch(batchSize, debug );
    reader[i]->PrintHitListInfo(&hitList[i], "hitList-" + std::to_string(reader[i]->GetSN()));
    ID[i] = 0;
    if( debug ) {

      for( size_t p = 0; p < 10; p ++ ){
        if( hitList[i].size() <= p ) break;
        hitList[i][p].Print();
      }

    }
  }

  unsigned long long tStart = 0;
  unsigned long long tEnd = 0;

  //find earliest time group;
  unsigned long long t0 = -1;
  short g0 = 0 ;
  for( short i = 0; i < nGroup; i++){
    if( hitList[i].size() == 0 ) continue;
    if( hitList[i][0].timestamp < t0 ) {
      t0 = hitList[i][0].timestamp;
      g0 = i;
    }
  }
  tStart = t0;
  if( debug ) printf("First timestamp is %llu, group : %u\n", t0, g0);

  int nFileFinished = 0;
  multi = 0;
  evID = 0;
  std::vector<Hit> events;

  unsigned long long hitProcessed = 0;

  do{

    //*============= Build events from hitList[i]
    if( debug ) printf("################################ ev build %llu \n", evID);
    events.clear();

    for( short i = 0; i < nGroup; i++){
      short ig = (i + g0 ) % nGroup;

      if( hitList[ig].size() == 0 ) continue;

      //chekc if reached the end of hitList
      if( ID[ig] >= hitList[ig].size() ) {
        hitList[ig] = reader[ig]->ReadBatch(batchSize, debug + 1);
        if( debug ) reader[ig]->PrintHitListInfo( &hitList[ig], "hitList-" + std::to_string(ig));
        ID[ig] = 0;
        if( hitList[ig].size() == 0 ) continue;
      }

      if( timeWindow >= 0 ){
      
        do{

          if( (long int)(hitList[ig].at(ID[ig]).timestamp  - t0) <= timeWindow ){
            events.push_back(hitList[ig].at(ID[ig]));
            ID[ig] ++;
          }else{
            break;
          }

          //check if reached the end of hitList
          if( ID[ig] >= hitList[ig].size() ) {
            hitList[ig] = reader[ig]->ReadBatch(batchSize, debug);
            if( debug ) reader[ig]->PrintHitListInfo( &hitList[ig], "hitList-" + std::to_string(ig));
            ID[ig] = 0;
            if( hitList[ig].size() == 0 ) break;
          }
      
        }while(ID[ig] < hitList[ig].size());

      }else{
        events.push_back(hitList[ig].at(ID[ig]));
        ID[ig] ++;
      }

      if( timeWindow < 0) break;

    }

    if( events.size() > 1 ){
      std::sort(events.begin(), events.end(), [](const Hit& a, const Hit& b) {
        return a.timestamp < b.timestamp;
      });
    }
    
    tEnd = events.back().timestamp;

    hitProcessed += events.size();    
    if( hitProcessed % (traceOn ? 10000 : 10000) == 0 ) printf("hit Porcessed %llu/%llu hit....%.2f%%\n\033[A\r", hitProcessed, totalHitCount, hitProcessed*100./totalHitCount);

    multi = events.size() ;
    if( events.size() >= MAX_MULTI ) {
      printf("\033[31m event %lld has size = %d > MAX_MULTI = %d \033[0m\n", evID, multi, MAX_MULTI);
      multi = MAX_MULTI;
    }
    if( debug ) printf("=================================== filling data | %u \n", multi);

    for( size_t p = 0; p < multi ; p ++ ) {
      if( debug ) {printf("%4zu | ", p); events[p].Print();}

      sn[p] = events[p].sn;
      ch[p] = events[p].ch;
      e[p] = events[p].energy;
      e2[p] = events[p].energy2;
      e_t[p] = events[p].timestamp;
      e_f[p] = events[p].fineTime;
      
      traceLength[p] = events[p].traceLength;
      if( traceOn ){
        if( traceLength[p] > MAX_TRACE_LENGTH ) {
          printf("\033[31m event %lld has trace length = %d > MAX_TRACE_LENGTH = %d \033[0m\n", evID, traceLength[p], MAX_TRACE_LENGTH);
          traceLength[p] = MAX_TRACE_LENGTH;
        }

        for( int hh = 0; hh < traceLength[p]; hh++){
          trace[p][hh] = events[p].trace[hh];
        }
      }
    }

    if( format == 0 ){
      outRootFile->cd();
      tree->Fill();
      // tree->Write();
    }else{
      if( caen ) {
        for( size_t gg = 0; gg < events.size(); gg++ ){
          events[gg].WriteHitsToCAENBinary(caen, traceOn);
        }
      }
    }

    multi = 0;
    evID ++;

    ///===================== find the next first timestamp
    t0 = -1;
    g0 = -1;

    for( int i = 0; i < nGroup; i++) {
      if( hitList[i].size() == 0 ) continue;
      if( hitList[i][ID[i]].timestamp < t0 ) {
        t0 = hitList[i][ID[i]].timestamp;
        g0 = i;
      }
    } 
    if( debug ) printf("Next First timestamp is %llu, group : %u\n", t0, g0);

    //*=============
    nFileFinished = 0;
    for( int i = 0 ; i < nGroup; i++) {
      if( hitList[i].size() == 0 ) {
        nFileFinished ++;
        continue;
      }else{
        if( ID[i] >= hitList[i].size( )) {
          hitList[i] = reader[i]->ReadBatch(batchSize, debug);
          ID[i] = 0;
          if( hitList[i].size() == 0 ) nFileFinished ++;
        }
      }
    }
    if( debug > 1 ) printf("========== nFileFinished : %d\n", nFileFinished);

  }while( nFileFinished < nGroup);

  if( format == 0 ) tree->Write();

  uInt runEndTime = getTime_us();
  double runTime = (runEndTime - runStartTime) * 1e-6;
  printf("========================================= finished.\n");
  printf(" event building time = %.2f sec = %.2f min\n", runTime, runTime/60.);
  // printf("  total events built = %llu by event builder (%llu in tree)\n", evID, tree->GetEntriesFast());
  printf("  total events built = %llu by event builder\n", evID);
  double tDuration_sec = (tEnd - tStart) * 1e-9;
  printf("     first timestamp = %20llu ns\n", tStart);
  printf("      last timestamp = %20llu ns\n", tEnd);
  printf(" total data duration = %.2f sec = %.2f min\n", tDuration_sec, tDuration_sec/60.);
  printf("========================================> saved to %s \n", outFileFullName.Data());

  if( format == 0 ){
    TMacro info;
    info.AddLine(Form("tStart= %20llu ns",tStart));
    info.AddLine(Form("  tEnd= %20llu ns",tEnd));
    info.Write("info");
    outRootFile->Close();
  }else{
    fclose(caen);
  }
  
  for( int i = 0; i < nGroup; i++) delete reader[i];
  delete [] reader;

  printf("####################################### end of %s\n", argv[0]);

  return 0;

}

