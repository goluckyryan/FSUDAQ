#ifndef DATA_H
#define DATA_H

#include <stdio.h>
#include <string>
#include <sstream>
#include <cmath>
#include <cstring>  ///memset
#include <iostream> ///cout
#include <sstream>
#include <iomanip> // for setw
#include <algorithm>
#include <bitset>
#include <vector>
#include <sys/stat.h>

#include "macro.h"

enum DPPTypeCode{ 
  DPP_PHA_CODE = 0x8B,
  DPP_PSD_CODE = 0x88,
  DPP_QDC_CODE = 0x87
};

enum ModelTypeCode{
  VME = 0,
  DT = 1
};

class Data{

  public:
    char * buffer;                     /// readout buffer
    int DPPType;
    std::string DPPTypeStr; /// only for saving fiel name
    unsigned short boardSN;
    int tick2ns;  /// use in convert the timestamp to ns, and use in TriggerRate calculation
    
    unsigned int nByte;               /// number of byte from read buffer
    uint32_t AllocatedSize;

    float          TriggerRate          [MaxNChannels]; /// Hz
    float          NonPileUpRate        [MaxNChannels]; /// Hz
    unsigned long  TotNumNonPileUpEvents[MaxNChannels]; /// also exclude overthrow
    unsigned short NumEventsDecoded     [MaxNChannels]; /// reset after trig-rate calculation
    unsigned short NumNonPileUpDecoded  [MaxNChannels]; /// reset after trig-rate calculation
    uShort countNumEventDecodeZero[MaxNChannels]; /// when > 3, set trigger rate to be zero;

    unsigned int   TotalAggCount ; 
    unsigned short AggCount ; /// reset after trig-rate calculation
    unsigned int   aggTime; /// update every decode

    int GetLoopIndex(unsigned short ch) const {return LoopIndex[ch];}
    int GetDataIndex(unsigned short ch) const {return DataIndex[ch];}
    long GetAbsDataIndex(unsigned short ch) const {return LoopIndex[ch] * dataSize + DataIndex[ch];}

    uShort GetDataSize() const {return dataSize;}

    ullong GetTimestamp(unsigned short ch, unsigned int index) const {return Timestamp[ch][index % dataSize];}
    uShort GetFineTime(unsigned short ch, unsigned int index)  const {return fineTime[ch][index % dataSize];}
    uShort GetEnergy(unsigned short ch, unsigned int index)    const {return Energy[ch][index % dataSize];}
    uShort GetEnergy2(unsigned short ch, unsigned int index)   const {return Energy2[ch][index % dataSize];}
    bool   GetPileUp(unsigned short ch, unsigned int index)    const {return PileUp[ch][index % dataSize];}

    uInt GetWordIndex() const {return nw;}

    std::vector<short> ** Waveform1    ; // used at least 14 MB
    std::vector<short> ** Waveform2    ;
    std::vector<bool>  ** DigiWaveform1;
    std::vector<bool>  ** DigiWaveform2;
    std::vector<bool>  ** DigiWaveform3;
    std::vector<bool>  ** DigiWaveform4;


  public:
    Data(unsigned short numCh, uInt dataSize = DefaultDataSize);
    ~Data();
  
    void Allocate80MBMemory();
    void AllocateMemory(uint32_t size);
    
    void AllocateDataSize(int dataSize);
    void ClearDataPointer();
    void ClearData();
    void ClearTriggerRate();
    void ClearNumEventsDecoded();
    void ClearBuffer();

    unsigned short GetNChannel() const {return numInputCh;}

    void PrintBuffer();
    void CopyBuffer( const char * buffer, const unsigned int size); 
        
    void DecodeBuffer(bool fastDecode, int verbose = 0); /// fastDecode will not save waveform
    void DecodeBuffer(char * &buffer, unsigned int size, bool fastDecode, int verbose = 0); // for outside data
  
    void DecodeDualBlock(char * &buffer, unsigned int size, int DPPType, int chMask, bool fastDecode, int verbose = 0); // for outside data

    void PrintStat(bool skipEmpty = true);
    void PrintAllData(bool tableMode = true, unsigned int maxRowDisplay = 0) const;
    void PrintChData(unsigned short ch, unsigned int maxRowDisplay = 0) const;

    //^================= Saving data
    bool OpenSaveFile(std::string fileNamePrefix); // return false when fail
    std::string GetOutFileName() const {return outFileName;}
    void SetDecimationFactor(unsigned short factor) { decimation = factor; printf("Set Decimation Factor to be %d\n", factor);}
    void SaveData(); 
    void CloseSaveFile();
    unsigned int GetFileSize() const {return outFileSize;}
    uint64_t GetTotalFileSize() const {return FinishedOutFilesSize + outFileSize;}
    void ZeroTotalFileSize() { FinishedOutFilesSize = 0; }

    void CalTriggerRate(); // this method is called by FSUDAQ::UpdateScalar()
    void ClearReferenceTime();

  protected:
  
    const unsigned short numInputCh;
    unsigned int nw;

    int dataSize;

    int       LoopIndex[MaxNChannels];     /// number of loop in the circular memory
    int       DataIndex[MaxNChannels];

    ullong ** Timestamp; /// 47 bit
    uShort ** fineTime;  /// 10 bits, in unit of tick2ns / 1000 = ps
    uShort ** Energy ;   /// 15 bit
    uShort ** Energy2 ;  /// 15 bit, in PSD, Energy = Qshort, Energy2 = Qlong
    bool   ** PileUp ;   /// pile up flag

    ///for temperary
    std::vector<short> tempWaveform1; 
    std::vector<short> tempWaveform2; 
    std::vector<bool> tempDigiWaveform1;
    std::vector<bool> tempDigiWaveform2;
    std::vector<bool> tempDigiWaveform3;
    std::vector<bool> tempDigiWaveform4;

    unsigned short decimation;

    FILE * outFile;
    uint64_t FinishedOutFilesSize; // sum of files size.
    unsigned int outFileIndex;
    std::string outFilePrefix;
    std::string outFileName;
    unsigned int outFileSize; // should be max at 2 GB

    ullong t0[MaxNChannels]; // for trigger rate calculation

    short calIndexes[MaxNChannels][2]; /// the index for trigger rate calculation

    unsigned int ReadBuffer(unsigned int nWord, int verbose = 0);

    int DecodePHADualChannelBlock(unsigned int ChannelMask, bool fastDecode, int verbose);
    int DecodePSDDualChannelBlock(unsigned int ChannelMask, bool fastDecode, int verbose);
    int DecodeQDCGroupedChannelBlock(unsigned int ChannelMask, bool fastDecode, int verbose);
};

//==========================================

inline Data::Data(unsigned short numCh, uInt dataSize): numInputCh(numCh){
  tick2ns = 2.0;
  boardSN = 0;
  DPPType = DPPTypeCode::DPP_PHA_CODE;
  DPPTypeStr = "";
  buffer = NULL;

  AllocateDataSize(dataSize);  

  for ( int i = 0; i < MaxNChannels; i++) {
    TotNumNonPileUpEvents[i] = 0;
    t0[i] = 0;
    countNumEventDecodeZero[i] = 0;
  }
  ClearData();
  ClearTriggerRate();
  ClearNumEventsDecoded();
  nw = 0;

  decimation = 0;

  outFileIndex = 0;
  outFilePrefix = "";
  outFileName = "";
  outFile = nullptr;
  outFileSize = 0; // should be max at 2 GB
  FinishedOutFilesSize = 0; // sum of files size.

}

inline Data::~Data(){
  if( buffer != NULL ) delete buffer;

  ClearDataPointer();
}

inline void Data::AllocateDataSize(int dataSize){

  if( dataSize < 1) {
    printf("dataSize cannot < 1, set dataSize = 1.\n");
    dataSize = 1;
  }
  //printf("Data::%s, size: %u, No. Ch: %u\n", __func__, dataSize, numInputCh);

  this->dataSize = dataSize;

  Timestamp = new ullong * [numInputCh];
  fineTime = new uShort * [numInputCh]; 
  Energy = new uShort * [numInputCh];
  Energy2 = new uShort * [numInputCh];
  PileUp = new bool * [numInputCh];

  Waveform1 = new std::vector<short> * [numInputCh];    
  Waveform2 = new std::vector<short> * [numInputCh];    
  DigiWaveform1 = new std::vector<bool> * [numInputCh];
  DigiWaveform2 = new std::vector<bool> * [numInputCh];
  DigiWaveform3 = new std::vector<bool> * [numInputCh];
  DigiWaveform4 = new std::vector<bool> * [numInputCh];

  for(int ch = 0; ch < numInputCh;  ch++){
    Timestamp[ch] = new ullong[dataSize];
    fineTime[ch] = new uShort[dataSize];
    Energy[ch] = new uShort[dataSize];
    Energy2[ch] = new uShort[dataSize];
    PileUp[ch] = new bool[dataSize];

    Waveform1[ch] = new std::vector<short> [dataSize];    
    Waveform2[ch] = new std::vector<short> [dataSize];    
    DigiWaveform1[ch] = new std::vector<bool> [dataSize];
    DigiWaveform2[ch] = new std::vector<bool> [dataSize];
    DigiWaveform3[ch] = new std::vector<bool> [dataSize];
    DigiWaveform4[ch] = new std::vector<bool> [dataSize];
  }

}

inline void Data::ClearDataPointer(){

  if( dataSize == 0) return;

  for(int ch = 0; ch < numInputCh;  ch++){
    delete [] Timestamp[ch] ;
    delete [] fineTime[ch];
    delete [] Energy[ch];
    delete [] Energy2[ch];
    delete [] PileUp[ch];

    delete [] Waveform1[ch];    
    delete [] Waveform2[ch];    
    delete [] DigiWaveform1[ch];
    delete [] DigiWaveform2[ch];
    delete [] DigiWaveform3[ch];
    delete [] DigiWaveform4[ch];
  }

  delete [] Timestamp;
  delete [] fineTime;
  delete [] Energy;
  delete [] Energy2;
  delete [] PileUp;

  delete [] Waveform1;    
  delete [] Waveform2;    
  delete [] DigiWaveform1;
  delete [] DigiWaveform2;
  delete [] DigiWaveform3;
  delete [] DigiWaveform4;

  dataSize = 0;

}

inline void Data::AllocateMemory(uint32_t size){
  ClearBuffer();
  AllocatedSize = size;
  buffer = (char *) malloc( AllocatedSize); 
  printf("Allocated %u byte ( %.2f MB) for buffer = %u words\n", AllocatedSize, AllocatedSize/1024./1024.,  AllocatedSize / 4);
}

inline void Data::Allocate80MBMemory(){
  AllocateMemory( 80 * 1024 * 1024 ); /// 80 M Byte
}

inline void Data::ClearTriggerRate(){ 
  for( int i = 0 ; i < MaxNChannels; i++) {
    TriggerRate[i] = 0.0; 
    NonPileUpRate[i] = 0.0;
  }
}

inline void Data::ClearNumEventsDecoded(){
  for( int i = 0 ; i < MaxNChannels; i++) {
    NumEventsDecoded[i] = 0;
    NumNonPileUpDecoded[i] = 0;
    countNumEventDecodeZero[i] = 0;
  }
  AggCount = 0;
}

inline void Data::ClearData(){
  nByte = 0;
  AllocatedSize = 0;
  TotalAggCount = 0;
  for( int ch = 0 ; ch < MaxNChannels; ch++){
    LoopIndex[ch] = 0;
    DataIndex[ch] = -1;
    
    TotNumNonPileUpEvents[ch] = 0 ;

    calIndexes[ch][0] = -1;
    calIndexes[ch][1] = -1;

    if( ch >= numInputCh) break;
    for( int j = 0; j < dataSize; j++){
      Timestamp[ch][j] = 0;
      fineTime[ch][j] = -1;
      Energy[ch][j] = 0;
      Energy2[ch][j] = 0;
      Waveform1[ch][j].clear();
      Waveform2[ch][j].clear();
      DigiWaveform1[ch][j].clear();
      DigiWaveform2[ch][j].clear();
      DigiWaveform3[ch][j].clear();
      DigiWaveform4[ch][j].clear();
    }

  }
  
  tempWaveform1.clear();
  tempWaveform2.clear();
  tempDigiWaveform1.clear();
  tempDigiWaveform2.clear();
  tempDigiWaveform3.clear();
  tempDigiWaveform4.clear();

  outFileIndex = 0;

  ClearNumEventsDecoded();
  ClearTriggerRate();

}

inline void Data::ClearBuffer(){
  //printf("==== Data::%s \n", __func__);
  delete buffer;
  buffer = nullptr;
  AllocatedSize = 0;
  nByte = 0;
}

inline void Data::CopyBuffer(const char * buffer, const unsigned int size){
  if( this->buffer ) delete this->buffer;
  this->buffer = (char*) malloc(size);
  std::memcpy(this->buffer, buffer, size);
  this->nByte = size;
}

inline void Data::ClearReferenceTime(){
  for( int ch = 0; ch < numInputCh; ch ++ ) t0[ch] = 0;
}

inline void Data::CalTriggerRate(){ // this method is called by FSUDAQ::UpdateScalar()

  unsigned long long dTime = 0;
  double sec = -999;

  for( int ch = 0; ch < numInputCh; ch ++ ){
    if( t0[ch] == 0 || countNumEventDecodeZero[ch] > 3) {
      TriggerRate[ch] = 0;
      NonPileUpRate[ch] = 0;
      countNumEventDecodeZero[ch] = 0;
      continue;
    }

    if( NumEventsDecoded[ch] == 0 ) {
      countNumEventDecodeZero[ch] ++;
      continue;
    }

    if( NumEventsDecoded[ch] < dataSize ){

      dTime = Timestamp[ch][DataIndex[ch]] - t0[ch]; 
      double sec =  dTime / 1e9;

      TriggerRate[ch] = (NumEventsDecoded[ch])/sec;
      NonPileUpRate[ch] = (NumNonPileUpDecoded[ch])/sec;

      // printf("%2d | %d | %f %f \n", ch, NumEventsDecoded[ch], sec, TriggerRate[ch]);

    }else{

      uShort nEvent = 100;
      dTime = Timestamp[ch][DataIndex[ch]] - Timestamp[ch][DataIndex[ch] - 100]; 
      sec =  dTime / 1e9;

      TriggerRate[ch] = (nEvent)/sec;
      NonPileUpRate[ch] = (NumNonPileUpDecoded[ch])/sec * (nEvent)/(NumEventsDecoded[ch]);

    }

    if( std::isinf(TriggerRate[ch]) ) {
      printf("%2d | %d(%d)| %llu  %llu | %d %d | %llu, %.3e | %.2f, %.2f\n", ch, DataIndex[ch], LoopIndex[ch], 
                                                                          t0[ch], Timestamp[ch][DataIndex[ch]], 
                                                                          NumEventsDecoded[ch], NumNonPileUpDecoded[ch], 
                                                                          dTime, sec , 
                                                                          TriggerRate[ch], NonPileUpRate[ch]);
    }

    t0[ch] = Timestamp[ch][DataIndex[ch]];
    NumEventsDecoded[ch] = 0;
    NumNonPileUpDecoded[ch] = 0;
  }

  AggCount = 0;
}

//^###############################################
//^############################################### Save fsu file
inline bool Data::OpenSaveFile(std::string fileNamePrefix){

  outFilePrefix = fileNamePrefix;

  std::ostringstream oss;
  oss << outFilePrefix << "_"
      << std::setfill('0') << std::setw(3) << boardSN << "_"
      << DPPTypeStr << "_"
      << std::fixed << std::setprecision(0) << tick2ns << "_"
      << std::setfill('0') << std::setw(3) << outFileIndex << ".fsu";
  std::string saveFileName = oss.str();

  //char saveFileName[100];
  //sprintf(saveFileName, "%s_%03d_%3s_%03u.fsu", outFilePrefix.c_str() , boardSN, DPPTypeStr.c_str(), outFileIndex);

  outFileName = saveFileName;
  outFile = fopen(saveFileName.c_str(), "wb"); // overwrite binary
  
  if (outFile == NULL) {
    printf("Failed to open the file. Probably Read-ONLY.\n");
    return false;
  } 

  fseek(outFile, 0L, SEEK_END);
  outFileSize = ftell(outFile);
    
  return true;
}

inline void Data::SaveData(){

  if( buffer == nullptr) {
    printf("buffer is null.\n");
    return;
  }

  if( outFile == nullptr ) return;

  if( outFileSize > (unsigned int) MaxSaveFileSize){
    FinishedOutFilesSize += ftell(outFile);
    CloseSaveFile();
    outFileIndex ++;

    // char saveFileName[100];
    // sprintf(saveFileName, "%s_%03d_%3s_%03u.fsu", outFilePrefix.c_str() , boardSN, DPPTypeStr.c_str(), outFileIndex);

    std::ostringstream oss;
    oss << outFilePrefix << "_"
        << std::setfill('0') << std::setw(3) << boardSN << "_"
        << DPPTypeStr << "_"
        << std::fixed << std::setprecision(0) << tick2ns << "_"
        << std::setfill('0') << std::setw(3) << outFileIndex << ".fsu";
    std::string saveFileName = oss.str();

    outFileName = saveFileName;
    outFile = fopen(outFileName.c_str(), "wb"); //overwrite binary
  }

  if( decimation == 0){
    fwrite(buffer, nByte, 1, outFile);
  }else{
    
    int Deci = pow(2, decimation);

    // printf("Decimation Factor : %d | Deci : %d | nByte %d | nWord %d\n", decimation, Deci, nByte, nByte / 4);

    const size_t chunkSize = 4;
    size_t numChunk = nByte / chunkSize;

    uint32_t word = 0;

    int bdAggWordCount = 0;
    int groupWordCount = 0;
    int chWordCount = 0;
    int sampleWordCount = 0;

    int bdAggSize = 0;
    int groupAggSize = 0;
    int sampleSize = 0;
    int chAggSize = 0;

    uint32_t oldHeader1 = 0;
    uint32_t oldHeader2 = 0;
    uint32_t oldHeader3 = 0;

    uint16_t average = 0; // to calculate Decimation average

    for( size_t i = 0; i < numChunk; i++ ){

      bdAggWordCount ++;
      memcpy(&word, buffer + i * chunkSize, chunkSize);

      if( bdAggWordCount <= 4) {
        
        if( bdAggWordCount == 1 ) {
          bdAggSize = word & 0x0FFFFFFF;
          // printf("###################### Bd Agg Size : %d\n", bdAggSize);
        }

        // fwrite(buffer + i * chunkSize, sizeof(char), chunkSize, outFile);
        // fwrite(&word, sizeof(word), 1, outFile);

        if( bdAggWordCount == 2 ) oldHeader1 = word;
        if( bdAggWordCount == 3 ) oldHeader2 = word;
        if( bdAggWordCount == 4 ) oldHeader3 = word;
      
      }else{

        groupWordCount ++;

        if( groupWordCount == 1 ) {
          groupAggSize = word & 0x3FFFFFFF;
          // printf("============= Coupled Channel Agg Size : %d \n", groupAggSize);
        }
        if( groupWordCount == 2 ) {
          sampleSize = (word & 0xFFF) * 8;
          bool isExtra = ( (word >> 28 ) & 0x1 );
          chAggSize = 2 + sampleSize / 2 + isExtra;
          uint32_t newSampleSize = sampleSize / Deci;
          // uint32_t oldWord = word;
          // word = (word & 0xFFFFF000) + (newSampleSize / 8 ); // change the number of sample
          // printf("============= Sample Size : %d | Ch Size : %d | old %08X new %08X\n", sampleSize, chAggSize, oldWord, word);

          int nEvent = (groupAggSize - 2 ) / chAggSize;
          int newGroupAggSize = 2 + nEvent * ( 2 + newSampleSize / 2 + isExtra );
          int newBdAggSize = 4 + newGroupAggSize;

          //Write board header and Agg header
          uint32_t newHeader0 = (0xA << 28) + newBdAggSize;
          fwrite(&newHeader0, sizeof(uint32_t), 1, outFile);
          fwrite(&oldHeader1, sizeof(uint32_t), 1, outFile);
          fwrite(&oldHeader2, sizeof(uint32_t), 1, outFile);
          fwrite(&oldHeader3, sizeof(uint32_t), 1, outFile);

          uint32_t newAggHeader0 = (0x8 << 28) + newGroupAggSize ; // add decimation factor in the word
          uint32_t newAggHeader1 = (word & 0xFFFFF000) + (newSampleSize / 8 ) + (decimation << 12); // add decimation factor in the word
          fwrite(&newAggHeader0, sizeof(uint32_t), 1, outFile);
          fwrite(&newAggHeader1, sizeof(uint32_t), 1, outFile);

          // printf(" New Board Agg Size : %d \n", newBdAggSize);
          // printf(" New Group Agg Size : %d \n", newGroupAggSize);
          // printf("             nEvent : %d \n", nEvent);
          // printf(" New Event Agg Size : %d \n", 2 + sampleSize / Deci / 2 + isExtra);

          // printf("%3d | %08X \n", 1,  newHeader0); 
          // printf("%3d | %08X \n", 2,  oldHeader1); 
          // printf("%3d | %08X \n", 3,  oldHeader2); 
          // printf("%3d | %08X \n", 4,  oldHeader3); 
          // printf("%3d | %3d | %08X \n", 5, 1, newAggHeader0); 
          // printf("%3d | %3d | %08X \n", 6, 2, newAggHeader1); 
        }

        if( groupWordCount > 2 ) {
          chWordCount ++;

          if( 1 < chWordCount && chWordCount <= chAggSize - 2 ){ // trace
            sampleWordCount ++;
            uint16_t S0 = word & 0xFFFF;
            uint16_t S1 = (word >> 16) & 0xFFFF;

            if( decimation == 1 ){
              average = S0/2 + S1/2;
              // printf("%3d | %3d | %3d | %3d | %08X | %4X \n", bdAggWordCount, groupWordCount, chWordCount, sampleWordCount, word, average); 
              fwrite(&average, sizeof(average), 1, outFile);
            }else{
              average += S0/Deci + S1/Deci;
              // printf("%3d | %3d | %3d | %3d | %08X | %4X \n", bdAggWordCount, groupWordCount, chWordCount, sampleWordCount, word, average); 
              if( sampleWordCount % (Deci/2) == 0)  {
                // fwrite(&S0, sizeof(S0), 1, outFile);
                // printf("                       --> %4X \n", average);
                fwrite(&average, sizeof(average), 1, outFile);
                average = 0;
              }
            }

          }else{
            // printf("%3d | %3d | %3d | %08X \n", bdAggWordCount, groupWordCount, chWordCount, word); 
            fwrite(&word, sizeof(word), 1, outFile);
          }
        }

        if( sampleWordCount == sampleSize / 2 ) sampleWordCount = 0;

        if( chAggSize == chWordCount) chWordCount = 0;

        if( groupWordCount == groupAggSize ) groupWordCount = 0;

      }

      if( bdAggWordCount == bdAggSize ) bdAggWordCount = 0; 

    }
    
  }

  outFileSize = ftell(outFile);
}

inline void Data::CloseSaveFile(){
  if( outFile != nullptr ){
    fclose(outFile);
    outFile = nullptr;
    int result = chmod(outFileName.c_str(), S_IRUSR | S_IRGRP | S_IROTH);
    if( result != 0 ) printf("somewrong when set file (%s) to read only.", outFileName.c_str());
  }
}

//^#######################################################
//^####################################################### Print
inline void Data::PrintStat(bool skipEmpty) {

  printf("============================= Print Stat. Digi-%d, TotalAggCount = %d\n", boardSN, TotalAggCount);
  printf("%2s | %6s | %9s | %9s | %6s | %6s(%4s)\n", "ch", "# Evt.", "Rate [Hz]", "Accept", "Tot. Evt.", "index", "loop");
  printf("---+--------+-----------+-----------+----------\n");
  for(int ch = 0; ch < numInputCh; ch++){
    //if( skipEmpty && TriggerRate[ch] == 0 ) continue;
    if( skipEmpty && DataIndex[ch] < 0 ) continue;
    printf("%2d | %6d | %9.2f | %9.2f | %6lu | %6d(%2d)\n", ch, NumEventsDecoded[ch], TriggerRate[ch], NonPileUpRate[ch], TotNumNonPileUpEvents[ch], DataIndex[ch], LoopIndex[ch]);
  }
  printf("---+--------+-----------+-----------+----------\n");

  ClearTriggerRate();
  ClearNumEventsDecoded();

}

inline void Data::PrintAllData(bool tableMode, unsigned int maxRowDisplay) const{
  printf("============================= Print Data Digi-%d\n", boardSN);

  if( tableMode ){
    int entry = 0;

    int MaxEntry = 0;
    printf("%4s|", "");
    for( int ch = 0; ch < numInputCh; ch++){
      if( LoopIndex[ch] > 0 ) {
        MaxEntry = dataSize-1;
      }else{
        if( DataIndex[ch] > MaxEntry ) MaxEntry = DataIndex[ch];
      }
      if( DataIndex[ch] < 0 ) continue;
      printf("    %5s-%02d,%2d,%-6d   |", "ch", ch, LoopIndex[ch], DataIndex[ch]);
    }
    printf("\n");
    

    do{
      printf("%4d|", entry ); 
      for( int ch = 0; ch < numInputCh; ch++){
        if( DataIndex[ch] < 0 ) continue;
        printf(" %5d,%17lld |", Energy[ch][entry], Timestamp[ch][entry]);
      }
      printf("\n");
      entry ++;

      if( maxRowDisplay > 0 && (unsigned int) entry >= maxRowDisplay ) break;
    }while(entry <= MaxEntry);

  }else{
    for( int ch = 0; ch < numInputCh ; ch++){
      if( DataIndex[ch] < 0  ) continue;
      printf("------------ ch : %d, DataIndex : %d, loop : %d\n", ch, DataIndex[ch], LoopIndex[ch]);
      for( int ev = 0; ev <= (LoopIndex[ch] > 0 ? dataSize : DataIndex[ch]) ; ev++){
        if( DPPType == DPPTypeCode::DPP_PHA_CODE || DPPType == DPPTypeCode::DPP_QDC_CODE ) printf("%4d, %5u, %18llu, %5u \n", ev, Energy[ch][ev], Timestamp[ch][ev], fineTime[ch][ev]);
        if( DPPType == DPPTypeCode::DPP_PSD_CODE ) printf("%4d, %5u, %5u, %18llu, %5u \n", ev, Energy[ch][ev], Energy2[ch][ev], Timestamp[ch][ev], fineTime[ch][ev]);
        if( maxRowDisplay > 0 && (unsigned int) ev > maxRowDisplay ) break;
      }
    }
  }
}

inline void Data::PrintChData(unsigned short ch, unsigned int maxRowDisplay) const{

  if( DataIndex[ch] < 0  ) printf("no data in ch-%d\n", ch);
  printf("------------ ch : %d, DataIndex : %d, loop : %d\n", ch, DataIndex[ch], LoopIndex[ch]);
  for( int ev = 0; ev < (LoopIndex[ch] > 0 ? dataSize : DataIndex[ch]) ; ev++){
    if( DPPType == DPPTypeCode::DPP_PHA_CODE || DPPType == DPPTypeCode::DPP_QDC_CODE ) printf("%4d, %5u, %15llu, %5u \n", ev, Energy[ch][ev], Timestamp[ch][ev], fineTime[ch][ev]);
    if( DPPType == DPPTypeCode::DPP_PSD_CODE ) printf("%4d, %5u, %5u, %15llu, %5u \n", ev, Energy[ch][ev], Energy2[ch][ev], Timestamp[ch][ev], fineTime[ch][ev]);
    if( maxRowDisplay > 0 && (unsigned int) ev > maxRowDisplay ) break;
  }

}

//^#######################################################
//^####################################################### Decode
inline void Data::PrintBuffer(){
  if( buffer == NULL || nByte == 0 ) return;
  printf("============== Received nByte : %u\n", nByte);
  for( unsigned int i = 0; i < nByte/4; i++ ) {
    ReadBuffer(i, 2);
    printf("\n");
  }
}

inline unsigned int Data::ReadBuffer(unsigned int nWord, int verbose){
  if( buffer == NULL ) return 0;
  
  unsigned int word = 0;
  // for( int i = 0 ; i < 4 ; i++) word += ((buffer[i + 4 * nWord] & 0xFF) << 8*i);
  memcpy(&word, buffer + 4 * nWord, 4);  // Copy 4 bytes directly into word
  if( verbose >= 2) printf("%6d | 0x%08X |", nWord, word);
  return word;
}

inline void Data::DecodeBuffer(char * &buffer, unsigned int size, bool fastDecode, int verbose){
  this->buffer = buffer;
  this->nByte = size;
  DecodeBuffer(fastDecode, verbose);
}

inline void Data::DecodeBuffer(bool fastDecode, int verbose){
  /// verbose : 0 = off, 1 = only energy + timestamp, 2 = show header, 3 = wave 
  
  if( buffer == NULL ) {
    if( verbose >= 1 ) printf(" buffer is empty \n");
    return;
  } 

  //for( int ch = 0; ch < MaxNChannels; ch ++) {
  //  NumEventsDecoded[ch] = 0;
  //  NumNonPileUpDecoded[ch] = 0;
  //}

  // if( DPPType == DPPType::DPP_QDC_CODE ) verbose = 10;

  if( nByte == 0 ) return;
  nw = 0;

  //printf("############################# agg\n");
  
  do{
    if( verbose >= 1 ) printf("Data::DecodeBuffer ######################################### Board Agg.\n");
    unsigned int word = ReadBuffer(nw, verbose);
    if( ( (word >> 28) & 0xF ) == 0xA ) { /// start of Board Agg
      unsigned int nWord = word & 0x0FFFFFFF ;
      if( verbose >= 1 ) printf("Number of words in this Agg : %u = %u Byte\n", nWord, nWord * 4);
      AggCount ++;
      TotalAggCount ++;

      nw = nw + 1; word = ReadBuffer(nw, verbose);
      unsigned int BoardID = ((word >> 27) & 0x1F);
      unsigned short pattern = ((word >> 8 ) & 0x7FFF );
      bool BoardFailFlag =  ((word >> 26) & 0x1 );
      unsigned int ChannelMask = ( word & 0xFF ) ;
      if( verbose >= 1 ) printf("Board ID(type) : %d, FailFlag = %d, Patten = %u, ChannelMask = 0x%X\n", 
                                    BoardID, BoardFailFlag, pattern, ChannelMask);
      
      if( BoardID > 0 ) {
        switch(BoardID){
          case 0x8 : DPPType = DPPTypeCode::DPP_PSD_CODE; break;
          case 0xB : DPPType = DPPTypeCode::DPP_PHA_CODE; break;
          case 0x7 : DPPType = DPPTypeCode::DPP_QDC_CODE; break;
        }
      }
      
      nw = nw + 1; 
      unsigned int bdAggCounter = ReadBuffer(nw, verbose);
      if( verbose >= 1 ) printf("Board Agg Counter : %u \n", bdAggCounter & 0x7FFFFF);
      
      nw = nw + 1; 
      aggTime = ReadBuffer(nw, verbose);
      if( verbose >= 1 ) printf("Agg Time Tag : %u \n", aggTime);
      
      for( int chMask = 0; chMask < 8 ; chMask ++ ){ // the max numnber of Coupled/RegChannel is 8 for PHA, PSD, QDC
        if( ((ChannelMask >> chMask) & 0x1 ) == 0 ) continue;
        if( verbose >= 2 ) printf("==================== Dual/Group Channel Block, ch Mask : 0x%X, nw : %d\n", chMask *2, nw);

        nw = nw + 1; 
        
        if( DPPType == DPPTypeCode::DPP_PHA_CODE ) {
          if ( DecodePHADualChannelBlock(chMask, fastDecode, verbose) < 0 ) break;
        }
        if( DPPType == DPPTypeCode::DPP_PSD_CODE ) {
          if ( DecodePSDDualChannelBlock(chMask, fastDecode, verbose) < 0 ) break;
        }
        if( DPPType == DPPTypeCode::DPP_QDC_CODE ) {
          if ( DecodeQDCGroupedChannelBlock(chMask, fastDecode, verbose) < 0 ) break;
        }
      }
    }else{
      if( verbose >= 1 ) printf("nw : %d, incorrect buffer header. \n", nw);
      break;
    }
    nw++;
    ///printf("nw : %d ,x 4 = %d, nByte : %d \n", nw, 4*nw, nByte);
  }while(4*nw < nByte);
  
  //Set the t0 for when frist hit comes
  for( int ch = 0; ch < numInputCh; ch++){
    if( t0[ch] == 0  && DataIndex[ch] > 0 ) t0[ch] = Timestamp[ch][0];
  }

}

//*=================================================
inline void Data::DecodeDualBlock(char * &buffer, unsigned int size, int DPPType, int chMask, bool fastDecode, int verbose){
  this->buffer = buffer;
  this->nByte = size;

  nw = 0;

  if( DPPType == DPPTypeCode::DPP_PHA_CODE ) {
    DecodePHADualChannelBlock(chMask, fastDecode, verbose) ; 
  }
  if( DPPType == DPPTypeCode::DPP_PSD_CODE ) {
    DecodePSDDualChannelBlock(chMask, fastDecode, verbose) ;
  }
  if( DPPType == DPPTypeCode::DPP_QDC_CODE ) {
    DecodeQDCGroupedChannelBlock(chMask, fastDecode, verbose) ;
  }

}

inline int Data::DecodePHADualChannelBlock(unsigned int ChannelMask, bool fastDecode, int verbose){
  
  //printf("======= %s\n", __func__);

  //nw = nw + 1; 
  unsigned int word = ReadBuffer(nw, verbose);

  bool hasFormatInfo = ((word >> 31) & 0x1);
  unsigned int aggSize = ( word & 0x7FFFFFFF ) ;
  if( verbose >= 2 ) printf("Dual Channel size : %d \n",  aggSize);
  unsigned short decimation = (word >> 12) & 0xF ;
  unsigned int nSample = 0; /// wave form;
  unsigned int nEvents = 0;
  unsigned int extra2Option = 0;
  bool hasWaveForm = false;
  bool hasExtra2 = false;
  bool hasDualTrace = 0 ;
  if( hasFormatInfo ){
    nw = nw + 1; word = ReadBuffer(nw, verbose);
    
    nSample = ( word & 0xFFFF )  * 8;
    extra2Option = ( (word >> 24 ) & 0x7 );
    hasExtra2    = ( (word >> 28 ) & 0x1 );
    if( !fastDecode || verbose >= 2){
      unsigned int digitalProbe = ( (word >> 16 ) & 0xF );
      unsigned int analogProbe2 = ( (word >> 20 ) & 0x3 );
      unsigned int analogProbe1 = ( (word >> 22 ) & 0x3 );
      hasWaveForm  = ( (word >> 27 ) & 0x1 );
      bool hasTimeStamp = ( (word >> 29 ) & 0x1 );
      bool hasEnergy    = ( (word >> 30 ) & 0x1 );
      hasDualTrace = ( (word >> 31 ) & 0x1 );
      
      if( verbose >= 2 ) {
        printf("DualTrace : %d, Energy : %d, Time: %d, Wave : %d, Extra2: %d \n", 
                                hasDualTrace, hasEnergy, hasTimeStamp, hasWaveForm, hasExtra2);
      }
      if( verbose >= 3){
        if( hasExtra2 ){
          printf("...... extra 2 : ");
          switch (extra2Option){
            case 0: printf("[0:15] trapwzoid baseline * 4 [16:31] Extended timestamp (16-bit)\n"); break;
            case 1: printf("Reserved\n"); break;
            case 2: printf("[0:9] Fine time stamp [10:15] Reserved [16:31] Extended timestamp (16-bit)\n"); break;
            case 3: printf("Reserved\n"); break;
            case 4: printf("[0:15] Total trigger counter [16:31] Lost trigger counter\n"); break;
            case 5: printf("[0:15] Event after Zero crossing [16:31] Event before Zero crossing\n"); break;
            case 6: printf("Reserved\n"); break;
            case 7: printf("Reserved\n"); break;
          }
        }
        if( hasWaveForm ){
          printf("Sample Size : %d | Decimation: %d \n", nSample, decimation);
          printf("...... Analog Probe 1 : ");
          switch (analogProbe1 ){
            case 0 : printf("Input \n"); break;
            case 1 : printf("RC-CR (1st derivative) \n"); break;
            case 2 : printf("RC-CR2 (2st derivative) \n"); break;
            case 3 : printf("trapazoid \n"); break;
          }
          printf("...... Analog Probe 2 : ");
          switch (analogProbe2 ){
            case 0 : printf("Input \n"); break;
            case 1 : printf("Theshold \n"); break;
            case 2 : printf("trapezoid - baseline \n"); break;
            case 3 : printf("baseline \n"); break;
          }
          printf("...... Digital Probe : ");
          switch (digitalProbe ){
            case  0 : printf("Peaking \n"); break;
            case  1 : printf("Armed (trigger) \n"); break;
            case  2 : printf("Peak Run \n"); break;
            case  3 : printf("Pile up \n"); break;
            case  4 : printf("Peaking \n"); break;
            case  5 : printf("Trigger Validation Window \n"); break;
            case  6 : printf("Baseline for energy calculation \n"); break;
            case  7 : printf("Trigger holdoff \n"); break;
            case  8 : printf("Trigger Validation \n"); break;
            case  9 : printf("ACQ Busy \n"); break;
            case 10 : printf("Trigger window \n"); break;
            case 11 : printf("Ext. Trigger \n"); break;
            case 12 : printf("Busy = memory is full \n"); break;
          }
        }
      }
    }
    nEvents = (aggSize - 2) / (nSample/2 + 2 + hasExtra2 );
    if( verbose >= 2 ) printf("----------------- nEvents : %d, fast decode : %d\n", nEvents, fastDecode);
  }else{
    if( verbose >= 2 ) printf("does not has format info. unable to read buffer.\n");
    return 0;
  }
  
  ///========== decode an event
  for( unsigned int ev = 0; ev < nEvents ; ev++){
    if( verbose >= 2 ) printf("------ event : %d\n", ev);
    nw = nw +1 ; word = ReadBuffer(nw, verbose);
    bool channelTag = ((word >> 31) & 0x1);
    unsigned int timeStamp0 = (word & 0x7FFFFFFF);
    int channel = ChannelMask*2 + channelTag;
    if( verbose >= 2 ) printf("ch : %d, timeStamp0 %u \n", channel, timeStamp0);
    
    ///===== read waveform
    if( !fastDecode ) {
      tempWaveform1.clear();
      tempWaveform2.clear();
      tempDigiWaveform1.clear();
    }
    
    unsigned int triggerAtSample = 0 ;
    if( fastDecode ){
      nw += nSample/2;
    }else{
      if( hasWaveForm ){
        for( unsigned int wi = 0; wi < nSample/2; wi++){
          nw = nw +1 ; word = ReadBuffer(nw, verbose-2);
          ///The CAEN manual is wrong, the bit [31:16] is anaprobe 1
          bool isTrigger1 = (( word >> 31 ) & 0x1 );
          bool dp1 = (( word >> 30 ) & 0x1 );
          unsigned short wave1 = (( word >> 16) & 0x3FFF);
          short trace1 = 0;
          if( wave1 & 0x2000){
            trace1 = static_cast<short>(~wave1 + 1 + 0x3FFF);
            trace1 = - trace1;
          }else{
            trace1 = static_cast<short>(wave1);
          }
          
          ///The CAEN manual is wrong, the bit [31:16] is anaprobe 2
          bool isTrigger0 = (( word >> 15 ) & 0x1 );
          bool dp0 = (( word >> 14 ) & 0x1 );
          unsigned short wave0 = ( word & 0x3FFF);
          short trace0 = 0;
          if( wave0 & 0x2000){
            trace0 = static_cast<short>(~wave0 + 1 + 0x3FFF);
            trace0 = - trace0;
          }else{
            trace0 = static_cast<short>(wave0);
          }
          
          if( hasDualTrace ){
            tempWaveform1.push_back(trace1);
            tempWaveform2.push_back(trace0);
            tempDigiWaveform1.push_back(dp1);
            tempDigiWaveform2.push_back(dp0);
          }else{
            tempWaveform1.push_back(trace1);          
            tempWaveform1.push_back(trace0);
            tempDigiWaveform1.push_back(dp1);
            tempDigiWaveform1.push_back(dp0);
          }

          if( isTrigger0 == 1 ) triggerAtSample = 2*wi ;      
          if( isTrigger1 == 1 ) triggerAtSample = 2*wi + 1;
          
          if( verbose >= 4 ){
            if( !hasDualTrace ){ 
              printf("%4d| %5d, %d, %d \n",   2*wi, trace0, dp0, isTrigger0);
              printf("%4d| %5d, %d, %d \n", 2*wi+1, trace1, dp1, isTrigger1);
            }else{
              printf("%4d| %5d, %5d | %d, %d | %d  %d\n",   wi, trace0, trace1, dp0, dp1, isTrigger0, isTrigger1);
            }
          }
        } 
      }   
    }
    unsigned long long extTimeStamp = 0;
    unsigned int extra2 = 0;
    if( hasExtra2 ){
      nw = nw +1 ; word = ReadBuffer(nw, verbose);
      extra2 = word;
      if( extra2Option == 0 || extra2Option == 2 ) extTimeStamp = (extra2 >> 16);
    }
  
    unsigned long long timeStamp = (extTimeStamp << 31) ;
    timeStamp = timeStamp + timeStamp0;
    
    if( verbose >= 2 && hasExtra2 ) printf("extra2 : 0x%0X, TimeStamp : %llu\n", extra2, timeStamp);
    nw = nw +1 ; word = ReadBuffer(nw, verbose);
    unsigned int extra = (( word >> 16) & 0x3FF);
    unsigned int energy = (word & 0x7FFF);
    bool rollOver = (extra & 0x002);
    bool pileUp   = (extra & 0x200);
    bool pileUpOrRollOver = ((word >> 15) & 0x1);
    
    if( verbose >= 3 ) {
      printf("PileUp or RollOver : %d\n", pileUpOrRollOver);
      printf("PileUp : %d , extra : 0x%03x, energy : %d \n", pileUp, extra, energy);      
      printf("     lost event : %d \n", ((extra >>  0) & 0x1) );
      printf("      roll-over : %d (is fake event ?)\n", ((extra >>  1) & 0x1) );
      printf("     fake-event : %d \n", ((extra >>  3) & 0x1) );
      printf("     input sat. : %d \n", ((extra >>  4) & 0x1) );
      printf("       lost trg : %d \n", ((extra >>  5) & 0x1) );
      printf("        tot trg : %d \n", ((extra >>  6) & 0x1) );
      printf("     coincident : %d \n", ((extra >>  7) & 0x1) );
      printf("      not coin. : %d \n", ((extra >>  8) & 0x1) );
      printf("        pile-up : %d \n", ((extra >>  9) & 0x1) );
      printf(" trapezoid sat. : %d \n", ((extra >> 10) & 0x1) );
    }


    if( rollOver == 0 ) { // non-time roll over fake event
      DataIndex[channel] ++; 
      if( DataIndex[channel] >= dataSize ) {
        LoopIndex[channel] ++;
        DataIndex[channel] = 0;
      }

      Energy[channel][DataIndex[channel]] = energy;
      Timestamp[channel][DataIndex[channel]] = timeStamp * tick2ns;
      if(extra2Option == 2 ) {
        fineTime[channel][DataIndex[channel]] = (extra2 & 0x03FF ) * tick2ns; // in ps, the tick2ns is a conversion factor
      }else{
        fineTime[channel][DataIndex[channel]] = -1;
      }
      PileUp[channel][DataIndex[channel]] = pileUp;
      NumEventsDecoded[channel] ++; 

      if( !pileUp ) {
        NumNonPileUpDecoded[channel] ++;
        TotNumNonPileUpEvents[channel] ++;
      }

      if( !fastDecode && hasWaveForm) {
        if( hasDualTrace ){
          Waveform1[channel][DataIndex[channel]] = tempWaveform1;
          Waveform2[channel][DataIndex[channel]] = tempWaveform2;
        }else{
          Waveform1[channel][DataIndex[channel]] = tempWaveform1;
        }
        DigiWaveform1[channel][DataIndex[channel]] = tempDigiWaveform1;
      }  
    }

    //if( DataIndex[channel] > dataSize ) ClearData(); // if any channel has more data then dataSize, clear all stored data

    if( verbose >= 1 ) printf("evt %4d(%2d) | ch : %2d, PileUp : %d , energy : %5d, rollOver: %d,  timestamp : %16llu (%10llu), triggerAt : %d, nSample : %d, %f sec\n", 
                                DataIndex[channel], LoopIndex[channel], channel, pileUp, energy, rollOver, timeStamp * tick2ns, timeStamp, triggerAtSample, nSample , timeStamp * 4. / 1e9);
    
  }
  
  ///=========== Key information
  /// ch, energy, timestamp
  /// trace

  return nw;
}

//*=================================================
inline int Data::DecodePSDDualChannelBlock(unsigned int ChannelMask, bool fastDecode, int verbose){
  
  //printf("======= %s\n", __func__);

  //nw = nw + 1; 
  unsigned int word = ReadBuffer(nw, verbose);
  
  if( (word >> 31) != 1 ) return 0;
  
  unsigned int aggSize = ( word & 0x3FFFFF ) ;
  if( verbose >= 2 ) printf(" size : %d \n",  aggSize);

  unsigned int nEvents = 0;
  nw = nw + 1; word = ReadBuffer(nw, verbose);
  unsigned short decimation = (word >> 12) & 0xF ;
  unsigned int nSample = ( word & 0xFFFF )  * 8;
  unsigned int digitalProbe1 = ( (word >> 16 ) & 0x7 );
  unsigned int digitalProbe2 = ( (word >> 19 ) & 0x7 );
  unsigned int analogProbe = ( (word >> 22 ) & 0x3 );
  unsigned int extraOption = ( (word >> 24 ) & 0x7 );
  bool hasWaveForm  = ( (word >> 27 ) & 0x1 );
  bool hasExtra     = ( (word >> 28 ) & 0x1 );
  bool hasTimeStamp = ( (word >> 29 ) & 0x1 );
  bool hasCharge    = ( (word >> 30 ) & 0x1 );
  bool hasDualTrace = ( (word >> 31 ) & 0x1 );
  
  if( verbose >= 2 ) {
    printf("dualTrace : %d, Charge : %d, Time: %d, Wave : %d, Extra: %d\n", 
                            hasDualTrace, hasCharge, hasTimeStamp, hasWaveForm, hasExtra);
    if( hasExtra ){
      printf(".... extra : ");
      switch(extraOption){
        case 0: printf("[0:15] baseline * 4 [16:31] Extended timestamp (16-bit)\n"); break;
        case 1: printf("[0:11] reserved [12] lost trigger counted [13] 1024 trigger counted [14] Over-range\n");
                printf("[15] trigger lost [16:31] Extended timestamp (16-bit)\n"); break;
        case 2: printf("[0:9] Fine time stamp [10:15] flag [10:15] Reserved [16:31] Extended timestamp (16-bit)\n"); break;
        case 3: printf("Reserved\n"); break;
        case 4: printf("[0:15] Total trigger counter [16:31] Lost trigger counter\n"); break;
        case 5: printf("[0:15] Event after Zero crossing [16:31] Event before Zero crossing\n"); break;
        case 6: printf("Reserved\n"); break;
        case 7: printf("debug, must be 0x12345678\n"); break;
      }
    }
    if( hasWaveForm ){
      printf("Sample Size : %d | Decimation: %d \n", nSample, decimation);
      printf(".... digital Probe 1 : ");
      switch(digitalProbe1){
        case 0 : printf("Long gate \n"); break;
        case 1 : printf("Over threshold \n"); break;
        case 2 : printf("Shaped TRG \n"); break;
        case 3 : printf("TRG Val. Acceptance \n"); break;
        case 4 : printf("Pile-Up \n"); break;
        case 5 : printf("Coincidence \n"); break;
        case 6 : printf("Reserved \n"); break;
        case 7 : printf("Trigger \n"); break;
      }
      printf(".... digital Probe 2 : ");
      switch(digitalProbe2){
        case 0 : printf("Short gate \n"); break;
        case 1 : printf("Over threshold \n"); break;
        case 2 : printf("TRG Validation \n"); break;
        case 3 : printf("TRG HoldOff \n"); break;
        case 4 : printf("Pile-Up \n"); break;
        case 5 : printf("Coincidence \n"); break;
        case 6 : printf("Reserved \n"); break;
        case 7 : printf("Trigger \n"); break;
      }
      printf(".... analog Probe (dual trace : %d): ", hasDualTrace);
      if( hasDualTrace ) {
        switch(analogProbe){
          case 0 : printf("Input and baseline \n"); break;
          case 1 : printf("CFD and baseline \n"); break;
          case 2 : printf("Input and CFD \n"); break;
        }
      }else{
        switch(analogProbe){
          case 0 : printf("Input \n"); break;
          case 1 : printf("CFD \n"); break;
        }
      }
    }

    if( !hasExtra && !hasWaveForm) printf("\n");
    
  }
  
  nEvents = (aggSize -2) / (nSample/2 + 2 + hasExtra );
  if( verbose >= 2 ) printf("----------------- nEvents : %d, fast decode : %d\n", nEvents, fastDecode);

  ///========= Decode an event
  for( unsigned int ev = 0; ev < nEvents ; ev++){
    if( verbose >= 2 ) printf("--------------------------- event : %d\n", ev);
    nw = nw +1 ; word = ReadBuffer(nw, verbose);
    bool channelTag = ((word >> 31) & 0x1);
    unsigned int timeStamp0 = (word & 0x7FFFFFFF);
    int channel = ChannelMask*2 + channelTag;
    if( verbose >= 2 ) printf("ch : %d, timeStamp %u \n", channel, timeStamp0);
    
    ///===== read waveform
    if( !fastDecode ) {
      tempWaveform1.clear();
      tempWaveform2.clear();
      tempDigiWaveform1.clear();
      tempDigiWaveform2.clear();
    }
    
    if( fastDecode ){
      nw += nSample/2;
    }else{
      if( hasWaveForm ){
        for( unsigned int wi = 0; wi < nSample/2; wi++){
          nw = nw +1 ; word = ReadBuffer(nw, verbose-4);
          bool dp2b = (( word >> 31 ) & 0x1 );
          bool dp1b = (( word >> 30 ) & 0x1 );
          unsigned short waveb = (( word >> 16) & 0x3FFF);
          
          bool dp2a = (( word >> 15 ) & 0x1 );
          bool dp1a = (( word >> 14 ) & 0x1 );
          unsigned short wavea = ( word & 0x3FFF);
          
          if( hasDualTrace ){
            tempWaveform1.push_back(wavea);
            tempWaveform2.push_back(waveb);
          }else{
            tempWaveform1.push_back(wavea);          
            tempWaveform1.push_back(waveb);          
          }
          tempDigiWaveform1.push_back(dp1a);
          tempDigiWaveform1.push_back(dp1b);
          tempDigiWaveform2.push_back(dp2a);
          tempDigiWaveform2.push_back(dp2b);
          
          if( verbose >= 3 ){
            printf("%4d| %5d, %d, %d \n",   2*wi, wavea, dp1a, dp2a);
            printf("%4d| %5d, %d, %d \n", 2*wi+1, waveb, dp1b, dp2b);
          }
        }
      }
    }

    unsigned int extra = 0;
    unsigned long long extTimeStamp = 0;

    if( hasExtra ){
      nw = nw +1 ; word = ReadBuffer(nw, verbose);
      if( verbose > 2 ) printf("extra \n");
      extra = word;
      extTimeStamp = 0;
      if( extraOption == 0 || extraOption == 2 ) extTimeStamp = (extra >> 16);
    }
    
    unsigned long long timeStamp = (extTimeStamp << 31) ;
    timeStamp = timeStamp + timeStamp0;
    
    nw = nw +1 ; word = ReadBuffer(nw, verbose);
    unsigned int Qlong  = (( word >> 16) & 0xFFFF);
    bool pileup = ((word >> 15) & 0x1);
    unsigned int Qshort = (word & 0x7FFF);
    bool isEnergyCorrect = ((word >> 15) & 0x1); // the PUR, either pileup or saturated
    
    if( isEnergyCorrect == 0 ) {
      DataIndex[channel] ++; 
      if( DataIndex[channel] >= dataSize ) {
        LoopIndex[channel] ++;
        DataIndex[channel] = 0;
      }

      Energy2[channel][DataIndex[channel]] = Qshort;
      Energy[channel][DataIndex[channel]] = Qlong;
      Timestamp[channel][DataIndex[channel]] = timeStamp * tick2ns;
      if( extraOption == 2 ) {
        fineTime[channel][DataIndex[channel]] = (extra & 0x3FF) * tick2ns; //in ps, tick2ns is justa conversion factor
      }else{
        fineTime[channel][DataIndex[channel]] = -1; //in ps, tick2ns is justa conversion factor
      }

      NumEventsDecoded[channel] ++; 
      if( !pileup){
        NumNonPileUpDecoded[channel] ++; 
        TotNumNonPileUpEvents[channel] ++;
      }

      if( !fastDecode && hasWaveForm) {
        if( hasDualTrace ){
          Waveform1[channel][DataIndex[channel]] = tempWaveform1;
          Waveform2[channel][DataIndex[channel]] = tempWaveform2;
        }else{
          Waveform1[channel][DataIndex[channel]] = tempWaveform1;
        }
        DigiWaveform1[channel][DataIndex[channel]] = tempDigiWaveform1;
        DigiWaveform2[channel][DataIndex[channel]] = tempDigiWaveform2;

      }

    }

    //if( DataIndex[channel] >= dataSize ) ClearData();
     
    //if( verbose >= 2 ) printf("extra : 0x%08x, Qshort : %d, Qlong : %d \n", extra, Qshort, Qlong);
    if( verbose >= 1 ) {
      if( extraOption == 0){
        printf("Qshort : %6d, Qlong : %6d, timestamp : %llu, baseline : %u\n", 
                               Qshort, Qlong, timeStamp * tick2ns, (extra & 0xFFFF) * 4);
      }
      if( extraOption == 2){
        printf("Qshort : %6d, Qlong : %6d, timestamp : %llu, fineTime : %u\n", 
                               Qshort, Qlong, timeStamp * tick2ns, (extra & 0x3FF) * tick2ns);
      }
    }

    
    
    
  }
  
  ///=========== Key information
  /// ch, Qshort, Qlong , timestamp
  /// trace

  return nw;
}

//*=================================================
inline int Data::DecodeQDCGroupedChannelBlock(unsigned int ChannelMask, bool fastDecode, int verbose){
  if( verbose ) printf("########## %s \n", __func__);

  //nw = nw + 1; 
  unsigned int word = ReadBuffer(nw, verbose);
  
  if( (word >> 31) != 1 ) return 0;

  unsigned int aggSize = ( word & 0x3FFFFF ) ;
  if( verbose >= 2 ) printf(" Group agg. size : %d words\n",  aggSize);

  unsigned int nEvents = 0;
  nw = nw + 1; word = ReadBuffer(nw, verbose);
  unsigned short decimation = (word >> 12) & 0xF ;
  unsigned int nSample = ( word & 0xFFF )  * 8;
  unsigned int analogProbe = ( (word >> 22 ) & 0x3 );
  bool hasWaveForm  = ( (word >> 27 ) & 0x1 );
  bool hasExtra     = ( (word >> 28 ) & 0x1 );
  bool hasTimeStamp = ( (word >> 29 ) & 0x1 );
  bool hasEnergy    = ( (word >> 30 ) & 0x1 );
  if( (word >> 31 ) != 0 ) return 0;

  if( verbose >= 2 ) {
    printf("Charge : %d, Time: %d, Wave : %d, Extra: %d\n", hasEnergy, hasTimeStamp, hasWaveForm, hasExtra);
    if( hasWaveForm ){
      printf("Sample Size : %d | Decimation %d .... analog Probe (%d): ", nSample, decimation, analogProbe);
      switch(analogProbe){
        case 0 : printf("Input\n"); break;
        case 1 : printf("Smoothed Input\n"); break;
        case 2 : printf("Baseline\n"); break;
      }
    }
  }

  nEvents = (aggSize -2) / (nSample/2 + 2 + hasExtra );
  if( verbose >= 2 ) printf("----------------- nEvents : %d, fast decode : %d\n", nEvents, fastDecode);

  ///========= Decode an event
  for( unsigned int ev = 0; ev < nEvents ; ev++){
    if( verbose >= 2 ) printf("--------------------------- event : %d\n", ev);
    nw = nw +1 ; word = ReadBuffer(nw, verbose);
    unsigned int timeStamp0 = (word & 0xFFFFFFFF);
    if( verbose >= 2 ) printf("timeStamp %u \n", timeStamp0);
    
    ///===== read waveform
    if( !fastDecode && hasWaveForm ) {
      tempWaveform1.clear();
      tempDigiWaveform1.clear();
      tempDigiWaveform2.clear();
      tempDigiWaveform3.clear();
      tempDigiWaveform4.clear();
    }
    
    if( fastDecode ){
      nw += nSample/2;
    }else{
      if( hasWaveForm ){
        for( unsigned int wi = 0; wi < nSample/2; wi++){
          nw = nw +1 ; word = ReadBuffer(nw, verbose-4);

          tempWaveform1.push_back(( word & 0xFFF));          
          tempWaveform1.push_back((( word >> 16) & 0xFFF));          

          tempDigiWaveform1.push_back((( word >> 12 ) & 0x1 )); //Gate
          tempDigiWaveform1.push_back((( word >> 28 ) & 0x1 ));

          tempDigiWaveform2.push_back((( word >> 13 ) & 0x1 )); //Trigger
          tempDigiWaveform2.push_back((( word >> 29 ) & 0x1 ));

          tempDigiWaveform3.push_back((( word >> 14 ) & 0x1 )); //Triger Hold Off
          tempDigiWaveform3.push_back((( word >> 30 ) & 0x1 ));
          
          tempDigiWaveform4.push_back((( word >> 15 ) & 0x1 )); //Over-Threshold
          tempDigiWaveform4.push_back((( word >> 31 ) & 0x1 ));

          if( verbose >= 3 ){
            printf("%4d| %5d, %d, %d, %d, %d \n",   2*wi, (word & 0xFFF)         , (( word >> 12 ) & 0x1 ), (( word >> 13 ) & 0x1 ), (( word >> 14 ) & 0x1 ), (( word >> 15 ) & 0x1 ));
            printf("%-21s", "");
            printf("%4d| %5d, %d, %d, %d, %d \n", 2*wi+1, (( word >> 16) & 0xFFF), (( word >> 28 ) & 0x1 ), (( word >> 29 ) & 0x1 ), (( word >> 30 ) & 0x1 ), (( word >> 31 ) & 0x1 ));
          }
        }
      }
    }

    unsigned long long extTimeStamp = 0;
    unsigned int baseline = 0;
    unsigned long extra = 0;
    if( hasExtra ){
      nw = nw +1 ; word = ReadBuffer(nw, verbose);
      extra = word;
      extTimeStamp = (word & 0xFFF);
      baseline = (word >> 16) / 16;
      if( verbose >= 2 ) printf("extra : 0x%lx, baseline : %d\n", extra, baseline);
    }
    
    unsigned long long timeStamp = (extTimeStamp << 32) ;
    timeStamp = timeStamp + timeStamp0;
    
    nw = nw +1 ; word = ReadBuffer(nw, verbose);
    unsigned int energy  = ( word  & 0xFFFF);
    bool pileup = ((word >> 27) & 0x1);
    bool OverRange = ((word >> 26)& 0x1);
    unsigned short subCh = ((word >> 28)& 0xF);

    unsigned short channel = ChannelMask*8 + subCh;
    
    DataIndex[channel] ++; 
    if( DataIndex[channel] >= dataSize ) {
      LoopIndex[channel] ++;
      DataIndex[channel] = 0;
    }

    Energy[channel][DataIndex[channel]] = energy;
    Timestamp[channel][DataIndex[channel]] = timeStamp * tick2ns;

    NumEventsDecoded[channel] ++; 
    if( !pileup && !OverRange){
      NumNonPileUpDecoded[channel] ++; 
      TotNumNonPileUpEvents[channel] ++;
    }

    if( !fastDecode && hasWaveForm) {
      Waveform1[channel][DataIndex[channel]] = tempWaveform1;
      DigiWaveform1[channel][DataIndex[channel]] = tempDigiWaveform1;
      DigiWaveform2[channel][DataIndex[channel]] = tempDigiWaveform2;
      DigiWaveform3[channel][DataIndex[channel]] = tempDigiWaveform3;
      DigiWaveform4[channel][DataIndex[channel]] = tempDigiWaveform4;
    }

    if( verbose == 1 ) printf("ch : %2d, energy : %d, timestamp : %llu\n",  channel, energy, timeStamp * tick2ns);
    if( verbose > 1 ) printf("ch : %2d, energy : %d, timestamp : %llu, pileUp : %d, OverRange : %d\n",  channel, energy, timeStamp * tick2ns, pileup, OverRange);

    if( verbose == 1) printf("Decoded : %d, total : %ld \n", NumEventsDecoded[channel], TotNumNonPileUpEvents[channel]);
  }


  return nw;

}


#endif
