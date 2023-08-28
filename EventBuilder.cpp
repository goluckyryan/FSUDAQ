#include "ClassData.h"
#include "MultiBuilder.h"

#include "TROOT.h"
#include "TSystem.h"
#include "TClonesArray.h"
#include "TGraph.h"
#include "TFile.h"
#include "TTree.h"

#define MAX_MULTI  100

template<typename T> void swap(T * a, T *b );
int partition(int arr[], int kaka[], TString file[], int start, int end);
void quickSort(int arr[], int kaka[], TString file[], int start, int end);

//^#############################################################
//^#############################################################
int main(int argc, char **argv) {
  
  printf("=========================================\n");
  printf("===      *.fsu Events Builder         ===\n");
  printf("=========================================\n");  
  if (argc <= 3)    {
    printf("Incorrect number of arguments:\n");
    printf("%s [timeWindow] [traceOn/Off] [verbose] [inFile1]  [inFile2] .... \n", argv[0]);
    printf("    timeWindow : number of tick, 1 tick. default = 100 \n");   
    printf("   traceOn/Off : is traces stored \n");   
    printf("       verbose : > 0 for debug  \n");   
    printf("    Output file name is contructed from inFile1 \n");   
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
  bool traceOn = atoi(argv[2]);
  unsigned int debug = atoi(argv[3]);
  int nFile = argc - 4;
  TString inFileName[nFile];
  for( int i = 0 ; i < nFile ; i++){
    inFileName[i] = argv[i+4];
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
  printf(" Time Window = %u \n", timeWindow);

  printf("===================================== input files:\n");  

  ///============= sorting file by the serial number & order
  int ID[nFile]; /// serial+ order*1000;
  int type[nFile];
  unsigned int fileSize[nFile];
  for( int i = 0; i < nFile; i++){
    int snPos = inFileName[i].Index("_"); // first "_"
    //snPos = inFileName[i].Index("_", snPos + 1);
    int sn = atoi(&inFileName[i][snPos+5]);
    TString typeStr = &inFileName[i][snPos+9];
    typeStr.Resize(3);

    if( typeStr == "PHA" ) type[i] = V1730_DPP_PHA_CODE;
    if( typeStr == "PSD" ) type[i] = V1730_DPP_PSD_CODE;

    int order = atoi(&inFileName[i][snPos+13]);
    ID[i] = sn + order * 1000;

    FILE * temp = fopen(inFileName[i].Data(), "rb");
    if( temp == NULL ) {
      fileSize[i] = 0;
    }else{
      fseek(temp, 0, SEEK_END);
      fileSize[i] = ftell(temp);
    }
    fclose(temp);

    
    //printf("sn:%d, type:%d (%s), order:%d \n", sn, type[i], typeStr.Data(), order);

  }
  quickSort(&(ID[0]), &(type[0]), &(inFileName[0]), 0, nFile-1);
  for( int i = 0 ; i < nFile; i++){
    printf("%d | %6d | %3d | %s | %u\n", i, ID[i], type[i], inFileName[i].Data(), fileSize[i]);
  }
  
  //*======================================= Sort files in to group 
  std::vector<int> snList; // store the serial number of the group
  std::vector<int> typeList; // store the DPP type of the group
  std::vector<std::vector<TString>> fileList; // store the file list of the group
  for( int i = 0; i < nFile; i++){
    if( ID[i] / 1000 == 0 ) {
      std::vector<TString> temp = {inFileName[i]};
      fileList.push_back(temp);
      typeList.push_back(type[i]);
      snList.push_back(ID[i]%1000);
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
    printf("............ %d \n", snList[i]);
    for( int j = 0; j< (int) fileList[i].size(); j++){
      printf("%s | %d\n", fileList[i][j].Data(), typeList[i]);
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
      data[i] = new Data();
      data[i]->DPPType = typeList[0];
      data[i]->boardSN = snList[0];
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
  unsigned short           bd[MAX_MULTI] = {0}; /// boardID
  unsigned short           ch[MAX_MULTI] = {0}; /// chID
  unsigned short            e[MAX_MULTI] = {0}; /// 15 bit
  unsigned short           e2[MAX_MULTI] = {0}; /// 15 bit
  unsigned long long      e_t[MAX_MULTI] = {0}; /// timestamp 47 bit
  unsigned short          e_f[MAX_MULTI] = {0}; /// fine time 10 bit 

  tree->Branch("evID",           &evID, "event_ID/l"); 
  tree->Branch("multi",         &multi, "multi/s"); 
  tree->Branch("bd",                bd, "bd[multi]/s");
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

  ///------------------ re data
  char * buffer = nullptr;
  unsigned int word[1]; // 4 byte = 32 bit


  do{

    /// fill the data class with some agg;
    bool earlyTermination = false;
    int aggCount = 0;
    do{

      for ( int i = 0; i < nGroup; i++){
        if( inFile[i] == nullptr ) continue;
        size_t dummy = fread(word, 4, 1, inFile[i]);
        if( dummy != 1) {
          printf("fread error, should read 4 bytes, but read %ld x 4 byte, file pos: %ld byte (%s)\n", dummy, ftell(inFile[i]), fileList[i][inFileIndex[i]].Data());
          earlyTermination = true;
        }

        if( earlyTermination == false){  
          fseek(inFile[i], -4, SEEK_CUR); // rool back

          short header = ((word[0] >> 28 ) & 0xF);
          if( header != 0xA ) break;
          unsigned int aggSize = (word[0] & 0x0FFFFFFF) * 4; ///byte

          buffer = new char[aggSize];
          dummy = fread(buffer, aggSize, 1, inFile[i]);
          if( dummy != 1) {
            printf("fread error, should read %d bytes, but read %ld x %d byte, file pos: %ld byte (%s)\n", aggSize, dummy, aggSize, ftell(inFile[i]), fileList[i][inFileIndex[i]].Data());
            earlyTermination = true;
          }

          if( earlyTermination == false){
            data[i]->DecodeBuffer(buffer, aggSize, false, 0);
            data[i]->ClearBuffer();
          }
        }

        if( feof(inFile[i]) || earlyTermination ){
          fclose(inFile[i]);
          inFile[i] = fopen(fileList[i][inFileIndex[i]+1], "r");
          if( inFile[i] ){
            inFileIndex[i]++;
          }else{
            inFileIndex[i] = -1;
          }
        }
      }
      aggCount++;

    }while(aggCount < 10); // get 10 agg should be enough for events building

    mb->BuildEvents(0, 0, debug);

    ///----------- save to tree;
    long startIndex = mb->eventIndex - mb->eventBuilt + 1;
    while( startIndex < 0 ) startIndex += MaxNEvent;
    //printf("startIndex : %ld, %ld\n", startIndex, mb->eventIndex);
    for( long i = startIndex; i <= mb->eventIndex; i++){

      multi = mb->events[i].size();
      if( multi > MAX_MULTI) break;
      evID ++;
      for( int j = 0; j < multi; j ++){
        bd[j] = mb->events[i][j].bd;
        ch[j] = mb->events[i][j].ch;
        e[j] = mb->events[i][j].energy;
        e2[j] = mb->events[i][j].energy2;
        e_t[j] = mb->events[i][j].timestamp;
        e_f[j] = mb->events[i][j].fineTime;
        if( traceOn ){
          traceLength[j] = mb->events[i][j].trace.size();
          trace = (TGraph *) arrayTrace->ConstructedAt(j, "C");
          trace->Clear();
          for( int hh = 0; hh < traceLength[j]; hh++){
            trace->SetPoint(hh, hh, mb->events[i][j].trace[hh]);
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

  mb->BuildEvents(1, 0, debug);

  ///----------- save to tree;
  long startIndex = mb->eventIndex - mb->eventBuilt + 1;
  if( startIndex < 0 ) startIndex += MaxNEvent;
  //printf("startIndex : %ld, %ld\n", startIndex, mb->eventIndex);
  for( long i = startIndex; i <= mb->eventIndex; i++){
    evID ++;
    multi = mb->events[i].size();
    if( multi > MAX_MULTI) break;
    for( int j = 0; j < multi; j ++){
      bd[j] = mb->events[i][j].bd;
      ch[j] = mb->events[i][j].ch;
      e[j] = mb->events[i][j].energy;
      e2[j] = mb->events[i][j].energy2;
      e_t[j] = mb->events[i][j].timestamp;
      e_f[j] = mb->events[i][j].fineTime;
      if( traceOn ){
        traceLength[j] = mb->events[i][j].trace.size();
        trace = (TGraph *) arrayTrace->ConstructedAt(j, "C");
        trace->Clear();
        for( int hh = 0; hh < traceLength[j]; hh++){
          trace->SetPoint(hh, hh, mb->events[i][j].trace[hh]);
        }
      }
    } 
    outRootFile->cd();
    tree->Fill();
  }
    
  tree->Write();
  outRootFile->Close();

  printf("========================= finsihed.\n");
  printf("total events built = %llu \n", evID + 1);
  printf("=======> saved to %s \n", outFileName.Data());

  delete mb;

  for( int i = 0 ; i < nGroup; i++){
    delete data[i];
  }
  delete [] data;

}

//^#############################################################
//^#############################################################
template<typename T> void swap(T * a, T *b ){
  T temp = * b;
  *b = *a;
  *a = temp;
}

int partition(int arr[], int kaka[], TString file[], int start, int end){
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
        }
    }
    return pivotIndex;
}
 
void quickSort(int arr[], int kaka[], TString file[], int start, int end){
    /// base case
    if (start >= end) return;
    /// partitioning the array
    int p = partition(arr, kaka, file, start, end); 
    /// Sorting the left part
    quickSort(arr, kaka, file, start, p - 1);
    /// Sorting the right part
    quickSort(arr, kaka, file, p + 1, end);
}