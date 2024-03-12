#include "../ClassData.h"
#include "../MultiBuilder.h"
#include "fsuReader.h"

#include "TROOT.h"
#include "TSystem.h"
#include "TClonesArray.h"
#include "TGraph.h"
#include "TFile.h"
#include "TTree.h"

#define MAX_MULTI  100
#define TIMEJUMP 1e8 // 0.1 sec or 10 Hz, any signal less than 10 Hz should increase the value.

template<typename T> void swap(T * a, T *b );
int partition(int arr[], int kaka[], TString file[], unsigned int fileSize[], unsigned int numBlock[], int t2ns[], int start, int end);
void quickSort(int arr[], int kaka[], TString file[], unsigned int fileSize[], unsigned int numBlock[], int t2ns[], int start, int end);

//^#############################################################
//^#############################################################
int main(int argc, char **argv) {
  
  printf("=========================================\n");
  printf("===      *.fsu Events Builder         ===\n");
  printf("=========================================\n");  
  if (argc <= 3)    {
    printf("Incorrect number of arguments:\n");
    printf("%s [timeWindow] [Buffer] [traceOn/Off] [verbose] [inFile1]  [inFile2] .... \n", argv[0]);
    printf("    timeWindow : in ns \n");   
    printf("        Buffer : Fraction of %d, recommand 0.4 \n", DefaultDataSize);   
    printf("   traceOn/Off : is traces stored \n");   
    printf("       verbose : > 0 for debug  \n");   
    printf("    Output file name is contructed from inFile1 \n");   
    printf("\n");
    printf("  * there is a TIMEJUMP = 1e8 ns in EventBuilder.cpp.\n");
    printf("       This control the time diff for a time jumping.\n");
    printf("       Any signal with trigger rate < 1/TIMEJUMP should increase the value.\n");
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
  float bufferSize = atof(argv[2]);
  bool traceOn = atoi(argv[3]);
  unsigned int debug = atoi(argv[4]);
  int nFile = argc - 5;
  TString inFileName[nFile];
  for( int i = 0 ; i < nFile ; i++){
    inFileName[i] = argv[i+5];
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
  printf(" Buffer size = %.0f event/channel\n", DefaultDataSize * bufferSize);
  printf("===================================== input files:\n");  
  printf("Scanning files.....\n");

  ///============= sorting file by the serial number & order
  int ID[nFile]; /// serial+ order*1000;
  int type[nFile];
  unsigned int fileSize[nFile];
  unsigned int numBlock[nFile];
  int tick2ns[nFile];
  // file name format is expName_runID_SN_DPP_tick2ns_order.fsu
  for( int i = 0; i < nFile; i++){
  
    FSUReader * reader = new FSUReader(inFileName[i].Data(), false);
    reader->ScanNumBlock(false);
    numBlock[i] = reader->GetTotNumBlock();
    fileSize[i] = reader->GetFileByteSize();
    tick2ns[i] = reader->GetTick2ns();
    type[i] = reader->GetDPPType();

    int sn = reader->GetSN();
    int order = reader->GetFileOrder();

    ID[i] = sn + order * 100000;

    delete reader;
  
  }
  quickSort(&(ID[0]), &(type[0]), &(inFileName[0]), &(fileSize[0]), &(numBlock[0]), &(tick2ns[0]), 0, nFile-1);
  unsigned int totBlock = 0;
  for( int i = 0 ; i < nFile; i++){
    printf("%d | %6d | %3d | %30s | %2d | %6u | %u Bytes = %.2f MB\n", i, 
                ID[i], type[i], inFileName[i].Data(), tick2ns[i], numBlock[i], fileSize[i], fileSize[i]/1024./1024.);
    totBlock += numBlock[i];
  }

  printf("----- total number of block : %u.\n", totBlock);
  
  //*======================================= Sort files into groups 
  std::vector<int> snList; // store the serial number of the group
  std::vector<int> typeList; // store the DPP type of the group
  std::vector<std::vector<TString>> fileList; // store the file list of the group
  std::vector<int> t2nsList;
  for( int i = 0; i < nFile; i++){
    if( ID[i] / 100000 == 0 ) {
      std::vector<TString> temp = {inFileName[i]};
      fileList.push_back(temp);
      typeList.push_back(type[i]);
      snList.push_back(ID[i]%100000);
      t2nsList.push_back(tick2ns[i]);
    }else{
      for( int p = 0; p < (int) snList.size(); p++){
        if( (ID[i] % 1000) == snList[p] ) {
          fileList[p].push_back(inFileName[i]);
        } 
      }
    }
  }
  
  int nGroup = snList.size();
  printf("===================================== number of file Group by digitizer %d.\n", nGroup);  
  for( int i = 0; i < nGroup; i++){
    printf("............ Digi-%d \n", snList[i]);
    for( int j = 0; j< (int) fileList[i].size(); j++){
      printf(" %s | %d\n", fileList[i][j].Data(), typeList[i]);
    }
  }


  //*======================================= open raw files 
  printf("##############################################\n"); 
  /// for each detector, open it
  std::vector<int> inFileIndex(nGroup); // the index of the the opened file for each group
  FILE ** inFile = new FILE *[nGroup];
  Data ** data = new Data *[nGroup];
  for( int i = 0; i < nGroup; i++){
    inFile[i] = fopen(fileList[i][0], "r");
    if( inFile[i] ){
      inFileIndex[i] = 0;
      if( typeList[i] == DPPTypeCode::DPP_PHA_CODE || typeList[i] == DPPTypeCode::DPP_PSD_CODE ) data[i] = new Data(16);
      if( typeList[i] == DPPTypeCode::DPP_QDC_CODE ) data[i] = new Data(64);
      data[i]->DPPType = typeList[i];
      data[i]->boardSN = snList[i];
      data[i]->tick2ns = t2nsList[i];
    }else{
      inFileIndex[i] = -1;
      data[i] = nullptr;
    }
  }

  //*====================================== create tree
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
  unsigned short  traceLength[MAX_MULTI] = {0};
  TGraph * trace = nullptr;

  if( traceOn ) {
    arrayTrace = new TClonesArray("TGraph");
    tree->Branch("traceLength", traceLength, "traceLength[multi]/s");
    tree->Branch("trace",       arrayTrace, 2560000);
    arrayTrace->BypassStreamer();
  }


  //*====================================== build events
  printf("================= Building events....\n");
  MultiBuilder * mb = new MultiBuilder(data, typeList, snList);
  mb->SetTimeWindow(timeWindow);
  mb->SetTimeJump(TIMEJUMP);

  ///------------------ read data
  char * buffer = nullptr;
  unsigned int word[1]; // 4 byte = 32 bit

  int lastDataIndex[nGroup][MAX_MULTI]; // keep track of the DataIndex
  int lastLoopIndex[nGroup][MAX_MULTI]; // keep track of the LoopIndex
  int aggCount[nGroup];

  for( int i = 0; i < nGroup; i++){
    aggCount[i] = 0;
    for( int j = 0; j < MAX_MULTI; j++){
      lastDataIndex[i][j] = 0;
      lastLoopIndex[i][j] = 0;
    }
  }

  do{

    /// fill the data class with some agg;
    bool fillFlag = true;
    do{

      // Get many agg. from each file.
      for ( int i = 0; i < nGroup; i++){
        if( inFile[i] == nullptr ) continue;

        size_t dummy = fread(word, 4, 1, inFile[i]);
        if( dummy != 1) {
          //printf("fread error, should read 4 bytes, but read %ld x 4 byte, file pos: %ld byte (%s)\n", dummy, ftell(inFile[i]), fileList[i][inFileIndex[i]].Data());
          // go to next file in same digitizer
          if( feof(inFile[i])){
            fclose(inFile[i]);
            if( inFileIndex[i] + 1 < (int) fileList[i].size() ){
              inFile[i] = fopen(fileList[i][inFileIndex[i]+1], "r");
              inFileIndex[i]++;
              printf("---- go to next file for digi-%d\n", snList[i]);
            }else{
              inFile[i] = nullptr;
              inFileIndex[i] = -1;
              printf("---- no more file for digi-%d.\n", snList[i]);
              continue;
            }
            // if( inFile[i] ){
            //   inFileIndex[i]++;
            //   printf("---- go to next file for digi-%d\n", snList[i]);
            // }else{
            //   inFileIndex[i] = -1;
            //   printf("---- no more file for digi-%d.\n", snList[i]);
            //   continue;
            // }
          }

        }else{
          fseek(inFile[i], -4, SEEK_CUR); // roll back

          short header = ((word[0] >> 28 ) & 0xF);
          if( header != 0xA ) break;
          unsigned int aggSize = (word[0] & 0x0FFFFFFF) * 4; ///byte

          buffer = new char[aggSize];
          size_t dummy2 = fread(buffer, aggSize, 1, inFile[i]);
          if( dummy2 != 1) {
            printf("fread error, should read %d bytes, but read %ld x %d byte, file pos: %ld byte (%s)\n", aggSize, dummy, aggSize, ftell(inFile[i]), fileList[i][inFileIndex[i]].Data());
          }else{
            data[i]->DecodeBuffer(buffer, aggSize, false, 0);
            data[i]->ClearBuffer();

            aggCount[i] ++;
          }
        }

      }

      //if all file exhausted, break
      int okFileNum = 0;
      for( int i = 0; i < nGroup; i++){
        if( inFileIndex[i] != -1 ) okFileNum ++;
      }
      if( okFileNum == 0 ) break;

      //check if Data Index near MaxNData. if near by 50%, break
      //printf("-----------------------------------\n");
      for( int i = 0; i < nGroup; i++){

        if( debug ){
          printf("-------------------------> %3d | agg : %d | %u \n", snList[i], aggCount[i], data[i]->aggTime);
          //data[i]->PrintStat();
        }

        uShort dataSize = data[i]->GetDataSize();

        for( int ch = 0; ch < data[i]->GetNChannel(); ch ++){

          int iData = data[i]->DataIndex[ch];
          int iLoop = data[i]->LoopIndex[ch];

          if( iData < 0 ) continue;

          if( (iLoop*dataSize + iData) - (lastLoopIndex[i][ch]*dataSize + lastDataIndex[i][ch]) > dataSize * bufferSize ) {
            if( debug ) printf("############# BREAK!!!! Group: %d, ch : %d | last : %d(%d), Present : %d(%d) | BufferSize : %.0f \n", i, ch, lastDataIndex[i][ch], lastLoopIndex[i][ch], iData, iLoop, dataSize * bufferSize);
            fillFlag = false;
          }

          if( debug ){
            unsigned long long t1 = data[i]->Timestamp[ch][iData];
            printf("digi:%5d | ch: %2d DataIndex: %5d (%d) [%5d(%d)] | %16llu\n", data[i]->boardSN, ch, iData, iLoop, lastDataIndex[i][ch], lastLoopIndex[i][ch], t1);
          }

        }
      }

    }while(fillFlag);

    for( int i = 0; i < nGroup; i++){
      for( int ch = 0; ch < data[i]->GetNChannel(); ch ++){
        lastDataIndex[i][ch] = data[i]->DataIndex[ch];
        lastLoopIndex[i][ch] = data[i]->LoopIndex[ch];
      }
      //data[i]->PrintAllData();
    }

    mb->BuildEvents(0, !traceOn, debug);
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
      }
      outRootFile->cd();
      tree->Fill();
    } 

    int okFileNum = 0;
    for( int i = 0; i < nGroup; i++){
      if( inFileIndex[i] != -1 ) okFileNum ++;
    }
    if( okFileNum == 0 ) break;

  }while(true);

  if( timeWindow >= 0 ){
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

  delete mb;
  for( int i = 0 ; i < nGroup; i++) delete data[i];
  delete [] data;

}

//^#############################################################
//^#############################################################
template<typename T> void swap(T * a, T *b ){
  T temp = * b;
  *b = *a;
  *a = temp;
}

int partition(int arr[], int kaka[], TString file[], unsigned int fileSize[], unsigned int numBlock[], int tick2ns[], int start, int end){
    int pivot = arr[start];
    int count = 0;
    for (int i = start + 1; i <= end; i++) {
        if (arr[i] <= pivot) count++;
    }
    /// Giving pivot element its correct position
    int pivotIndex = start + count;
    swap(&arr[pivotIndex], &arr[start]);
    swap(&file[pivotIndex], &file[start]);
    swap(&kaka[pivotIndex], &kaka[start]);
    swap(&fileSize[pivotIndex], &fileSize[start]);
    swap(&numBlock[pivotIndex], &numBlock[start]);
    swap(&tick2ns[pivotIndex], &tick2ns[start]);
    
    /// Sorting left and right parts of the pivot element
    int i = start, j = end;
    while (i < pivotIndex && j > pivotIndex) { 
        while (arr[i] <= pivot) {i++;}
        while (arr[j] > pivot) {j--;}
        if (i < pivotIndex && j > pivotIndex) {
          int ip = i++;
          int jm = j--;
          swap( &arr[ip],  &arr[jm]);
          swap(&file[ip], &file[jm]);
          swap(&kaka[ip], &kaka[jm]);
          swap(&fileSize[ip], &fileSize[jm]);
          swap(&numBlock[ip], &numBlock[jm]);
          swap(&tick2ns[ip], &tick2ns[jm]);
        }
    }
    return pivotIndex;
}
 
void quickSort(int arr[], int kaka[], TString file[], unsigned int fileSize[], unsigned int numBlock[], int tick2ns[], int start, int end){
    /// base case
    if (start >= end) return;
    /// partitioning the array
    int p = partition(arr, kaka, file, fileSize, numBlock, tick2ns, start, end); 
    /// Sorting the left part
    quickSort(arr, kaka, file, fileSize, numBlock, tick2ns, start, p - 1);
    /// Sorting the right part
    quickSort(arr, kaka, file, fileSize, numBlock, tick2ns, p + 1, end);
}
