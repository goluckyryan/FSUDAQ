#include "ClassData.h"

#include "TROOT.h"
#include "TSystem.h"
#include "TClonesArray.h"
#include "TGraph.h"
#include "TFile.h"
#include "TTree.h"

#define MAX_MULTI  100
#define NTimeWinForBuffer 3

TFile * outRootFile = NULL;
TTree * tree = NULL;

unsigned long long                evID = 0;
unsigned short                   multi = 0;
unsigned short           bd[MAX_MULTI] = {0}; /// boardID
unsigned short           ch[MAX_MULTI] = {0}; /// chID
unsigned short            e[MAX_MULTI] = {0}; /// 15 bit
unsigned long long      e_t[MAX_MULTI] = {0}; /// timestamp 47 bit
unsigned short          e_f[MAX_MULTI] = {0}; /// fine time 10 bit 


class Trace{
  public:
    Trace() {trace.clear(); }
    ~Trace();
    void Clear() { trace.clear(); };
    Trace operator = (std::vector<unsigned short> v){
      Trace tt;
      for( int i = 0 ; i < (int) v.size() ; i++){
        trace.push_back(v[i]);
      }
      return tt;
    }
    std::vector<unsigned short> trace;
};

/// using TClonesArray to hold the trace in TGraph
TClonesArray * arrayTrace = NULL;
unsigned short  traceLength[MAX_MULTI] = {0};
TGraph * trace = NULL;

template<typename T> void swap(T * a, T *b );
int partition(int arr[], int kaka[], TString file[], int start, int end);
void quickSort(int arr[], int kaka[], TString file[], int start, int end);
unsigned long get_time();
void EventBuilder(Data * data, const unsigned int timeWin, bool traceOn = false, bool isLastData = false, unsigned int verbose = 0);

int main(int argc, char **argv) {
  
  printf("=====================================\n");
  printf("===    *.fsu Events Builder       ===\n");
  printf("=====================================\n");  
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
  printf("=====================================\n");  

  ///============= sorting file by the serial number & order
  int ID[nFile]; /// serial+ order*1000;
  int type[nFile];
  for( int i = 0; i < nFile; i++){
    int snPos = inFileName[i].Index("_");
    snPos = inFileName[i].Index("_", snPos+1);
    int sn = atoi(&inFileName[i][snPos+1]);
    type[i] = atoi(&inFileName[i][snPos+5]);
    int order = atoi(&inFileName[i][snPos+9]);
    ID[i] = sn + order * 1000;
  }
  quickSort(&(ID[0]), &(type[0]), &(inFileName[0]), 0, nFile-1);
  for( int i = 0 ; i < nFile; i++){
    printf("%d | %6d | %3d | %s \n", i, ID[i], type[i], inFileName[i].Data());
  }
  
  ///=============== Seperate files
  std::vector<int> idCat;
  std::vector<std::vector<int>> typeCat;
  std::vector<std::vector<TString>> fileCat;
  for( int i = 0; i < nFile; i++){
    if( ID[i] / 1000 == 0 ) {
      std::vector<TString> temp = {inFileName[i]};
      std::vector<int> temp2 = {type[i]};
      fileCat.push_back(temp);
      typeCat.push_back(temp2);
      idCat.push_back(ID[i]%1000);
    }else{
      for( int p = 0; p < (int) idCat.size(); p++){
        if( (ID[i] % 1000) == idCat[p] ) {
          fileCat[p].push_back(inFileName[i]);
          typeCat[p].push_back(type[i]);
        } 
      }
    }
  }
  
  printf("=====================================\n");  
  for( int i = 0; i < (int) idCat.size(); i++){
    printf("............ %d \n", idCat[i]);
    for( int j = 0; j< (int) fileCat[i].size(); j++){
      printf("%s | %d\n", fileCat[i][j].Data(), typeCat[i][j]);
    }
  }
  
  ///============= Set Root Tree
  outRootFile = new TFile(outFileName, "recreate");
  tree = new TTree("tree", outFileName);
  tree->Branch("evID",           &evID, "event_ID/l"); 
  tree->Branch("multi",         &multi, "multi/s"); 
  tree->Branch("bd",                bd, "bd[multi]/s");
  tree->Branch("ch",                ch, "ch[multi]/s");
  tree->Branch("e",                  e, "e[multi]/s");
  tree->Branch("e_t",              e_t, "e_timestamp[multi]/l");
  tree->Branch("e_f",              e_f, "e_timestamp[multi]/s");
  
  if( traceOn ) {
    arrayTrace = new TClonesArray("TGraph");
    tree->Branch("traceLength", traceLength, "traceLength[multi]/s");
    tree->Branch("trace",       arrayTrace, 2560000);
    arrayTrace->BypassStreamer();
  }
  
  ///============= Open input Files
  printf("##############################################\n");  
  FILE * haha = fopen(fileCat[0][0], "r");
  if( haha == NULL ){
    printf("#### Cannot open file : %s. Abort.\n", fileCat[0][0].Data());
    return -1;
  }
  fseek(haha, 0L, SEEK_END);
  const size_t inFileSize = ftell(haha);
  printf("%s | file size : %d Byte = %.2f MB\n", fileCat[0][0].Data(), (int) inFileSize, inFileSize/1024./1024.);
  fclose(haha);
  
  
  Data * data = new Data();  
  data->DPPType = typeCat[0][0];
  data->boardSN = idCat[0];
  data->SetSaveWaveToMemory(true);
  
  ///============= Main Loop
  haha = fopen(inFileName[0], "r"); 
  int countBdAgg = 0;
  
  unsigned long currentTime = 0;
  unsigned long oldTime  = 0; 
  
  char * buffer = NULL;
  do{
    
    ///========== Get 1 aggreration
    oldTime = get_time();
    if( debug) printf("*********************** file pos : %d, %lu\n", (int) ftell(haha), oldTime);
    unsigned int word[1]; /// 4 bytes
    size_t dump = fread(word, 4, 1, haha);
    fseek(haha, -4, SEEK_CUR);
    
    short header = ((word[0] >> 28 ) & 0xF);
    if( header != 0xA ) break;
    
    unsigned int aggSize = (word[0] & 0x0FFFFFFF) * 4; ///byte
    
    if( debug) printf("Board Agg. has %d word  = %d bytes\n", aggSize/4, aggSize);    

    buffer = new char[aggSize];
    dump = fread(buffer, aggSize, 1, haha);
    countBdAgg ++;
    if( debug) printf("==================== %d Agg\n", countBdAgg);    

    data->DecodeBuffer(buffer, aggSize, false, 0);
    data->ClearBuffer();
    if( !data->IsNotRollOverFakeAgg ) continue;
    
    currentTime = get_time();
    
    if( debug) {
      printf("~~~~~~~~~~~~~~~~ time used : %lu usec\n", currentTime - oldTime);
      data->PrintStat();
    }

    EventBuilder(data, timeWindow, traceOn, false, debug);    
    
    if( debug) printf("---------- event built : %llu \n", evID); 
        
    //if( countBdAgg > 74) break;
    
  }while(!feof(haha) && ftell(haha) < inFileSize);

  fclose(haha);  
  
  printf("=======@@@@@@@@###############============= end of loop \n");
  EventBuilder(data, timeWindow, traceOn, true, debug);

  
  tree->Write();
  outRootFile->Close();
  
  printf("========================= finsihed.\n");
  printf("total events built = %llu \n", evID);
  printf("=======> saved to %s \n", outFileName.Data());
  
}

void EventBuilder(Data * data, const unsigned int timeWin, bool traceOn, bool isLastData, unsigned int verbose){
  
  if( verbose)  {
    printf("======================== Event Builder \n");
    data->PrintData();
  }

  /// find the last event timestamp;
  unsigned long long firstTimeStamp = -1;
  unsigned long long lastTimeStamp = 0;
  unsigned long long smallestLastTimeStamp = -1;
  unsigned int maxNumEvent = 0;
  for( int chI = 0; chI < MaxNChannels ; chI ++){
    if( data->NumEvents[chI] == 0 ) continue;
    
    if( data->Timestamp[chI][0] < firstTimeStamp ) { 
      firstTimeStamp = data->Timestamp[chI][0];
    }
    unsigned short ev = data->NumEvents[chI]-1;
    if( data->Timestamp[chI][ev] > lastTimeStamp ) { 
      lastTimeStamp = data->Timestamp[chI][ev]; 
    }
    if( ev + 1 > maxNumEvent ) maxNumEvent = ev + 1;
    if( data->Timestamp[chI][ev] < smallestLastTimeStamp ){
      smallestLastTimeStamp = data->Timestamp[chI][ev]; 
    }
  }
  
  if( maxNumEvent == 0 ) return;
  
  if( verbose) printf("================  time range : %llu - %llu, smallest Last %llu\n", firstTimeStamp, lastTimeStamp, smallestLastTimeStamp );      
  unsigned short lastEv[MaxNChannels] = {0}; /// store the last event number for each ch
  unsigned short exhaustedCh = 0; /// when exhaustedCh == MaxNChannels ==> stop
  bool singleChannelExhaustedFlag = false; /// when a single ch has data but exhaused ==> stop
  
  do {
    
    /// find the 1st event
    int ch1st = -1;
    unsigned long long time1st = -1;
    for( int chI = 0; chI < MaxNChannels ; chI ++){
      if( data->NumEvents[chI] == 0 ) continue;
      if( data->NumEvents[chI] <= lastEv[chI] )  continue; 
      if( data->Timestamp[chI][lastEv[chI]] < time1st ) { 
        time1st = data->Timestamp[chI][lastEv[chI]]; 
        ch1st = chI;
      }
    }
    if( !isLastData  && ((smallestLastTimeStamp - time1st) < NTimeWinForBuffer * timeWin) && maxNumEvent < MaxNData * 0.6 ) break;
    if( ch1st > MaxNChannels ) break;
    
    multi ++;
    bd[multi-1] = data->boardSN;
    ch[multi-1] = ch1st;
    e[multi-1] = data->Energy[ch1st][lastEv[ch1st]];
    e_t[multi-1] = data->Timestamp[ch1st][lastEv[ch1st]];
    e_f[multi-1] = data->fineTime[ch1st][lastEv[ch1st]];
    if( traceOn ){
      arrayTrace->Clear("C");
      traceLength[multi-1] = (unsigned short) data->Waveform1[ch1st][lastEv[ch1st]].size();
      ///if( verbose )printf("------- trace Length : %u \n", traceLength[multi-1]);
      trace = (TGraph *) arrayTrace->ConstructedAt(multi-1, "C");
      trace->Clear();
      for( int hh = 0; hh < traceLength[multi-1]; hh++){
        trace->SetPoint(hh, hh, data->Waveform1[ch1st][lastEv[ch1st]][hh]);
        ///if( verbose )if( hh % 200 == 0 ) printf("%3d | %u \n", hh, data->Waveform1[ch1st][lastEv[ch1st]][hh]);
      }
    }
    lastEv[ch1st] ++;
        
    /// build the rest of the event
    exhaustedCh = 0;
    singleChannelExhaustedFlag = false;
    for( int chI = ch1st; chI < ch1st + MaxNChannels; chI ++){
      unsigned short chX = chI % MaxNChannels;
      if( data->NumEvents[chX] == 0 ) {
        exhaustedCh ++;
        continue;
      }
      if( data->NumEvents[chX] <= lastEv[chX] )  {
        exhaustedCh ++;
        singleChannelExhaustedFlag = true;
        continue; 
      }
      if( timeWin == 0 ) continue;
      for( int ev  = lastEv[chX]; ev < data->NumEvents[chX] ; ev++){
        if( data->Timestamp[chX][ev] > 0 &&  (data->Timestamp[chX][ev] - e_t[0] ) < timeWin ) {
          multi ++;
          bd[multi-1] = data->boardSN;
          ch[multi-1] = chX;
          e[multi-1] = data->Energy[chX][ev];
          e_t[multi-1] = data->Timestamp[chX][ev];
          e_f[multi-1] = data->fineTime[chX][ev];
          if( traceOn ){
            traceLength[multi-1] = (unsigned short) data->Waveform1[chX][ev].size();
            trace = (TGraph *) arrayTrace->ConstructedAt(multi-1, "C");
            trace->Clear();
            for( int hh = 0; hh < traceLength[multi-1]; hh++){
              trace->SetPoint(hh, hh, data->Waveform1[chX][ev][hh]);
            }
          }
          lastEv[chX] = ev + 1;
          if( lastEv[chX] == data->NumEvents[chX] ) exhaustedCh ++;
        }
      }
    }
    
    if( verbose) {
      printf("=============== multi : %d , ev : %llu\n", multi, evID);
      for( int ev = 0; ev < multi; ev++){
        printf("%3d, ch : %2d, %u, %llu \n", ev, ch[ev], e[ev], e_t[ev]);
      }
      
      printf("=============== Last Ev , exhaustedCh %d \n", exhaustedCh);
      for( int chI = 0; chI < MaxNChannels ; chI++){
        if( lastEv[chI] == 0 ) continue;
        printf("%2d, %d %d\n", chI, lastEv[chI], data->NumEvents[chI]);
      }
    }
    
    /// fill Tree
    outRootFile->cd();
    tree->Fill();
    evID++;
    multi = 0;
    
  }while( !singleChannelExhaustedFlag || (exhaustedCh < MaxNChannels)  );
  
  ///========== clear built data
  /// move the last data to the top, 
  for( int chI = 0; chI < MaxNChannels; chI++){
    if( data->NumEvents[chI] == 0 ) continue;
    int count = 0;
    for( int ev = lastEv[chI] ; ev < data->NumEvents[chI] ; ev++){
      data->Energy[chI][count] = data->Energy[chI][ev];
      data->Timestamp[chI][count] = data->Timestamp[chI][ev];
      data->fineTime[chI][count] = data->fineTime[chI][ev];
      count++;
    }
    int lala = data->NumEvents[chI] - lastEv[chI];
    data->NumEvents[chI] = (lala >= 0 ? lala: 0);
  }

  if( verbose > 0 ) {
    printf("&&&&&&&&&&&&&&&&&&&&&&&&&& end of one event build loop\n");
    data->PrintData(); 
  }

}

unsigned long get_time(){
  unsigned long time_us;
  struct timeval t1;
  struct timezone tz;
  gettimeofday(&t1, &tz);
  time_us = (t1.tv_sec) * 1000000 + t1.tv_usec;
  return time_us;
}


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
