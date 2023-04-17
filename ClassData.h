#ifndef DATA_H
#define DATA_H

#include <stdio.h>
#include <string>
#include <sstream>
#include <cmath>
#include <cstring>  ///memset
#include <iostream> ///cout
#include <bitset>
#include <vector>
#include <sys/stat.h>

#include "CAENDigitizerType.h"
#include "macro.h"

#define MaxNData 10000 /// 10k events per channels

class Data{

  public:
    int DPPType;
    std::string DPPTypeStr;
    unsigned short boardSN;
    float ch2ns;
    
    unsigned int nByte;               /// number of byte from read buffer
    char *buffer;                     /// readout buffer
    uint32_t AllocatedSize;      
    
    double TriggerRate[MaxNChannels]; /// Hz
    unsigned long TotNumEvents[MaxNChannels];
    unsigned short NumEventsDecoded[MaxNChannels];

    /// store a single Raw event
    bool IsNotRollOverFakeAgg;
    unsigned short     NumEvents[MaxNChannels];
    unsigned long long Timestamp[MaxNChannels][MaxNData]; /// 47 bit
    unsigned short     fineTime[MaxNChannels][MaxNData];  /// 10 bits, in unit of ch2ns / 1000 = ps
    unsigned short     Energy[MaxNChannels][MaxNData];   /// 15 bit
    unsigned short     Energy2[MaxNChannels][MaxNData];  /// 15 bit, in PSD, Energy = Qshort, Energy2 = Qlong
    
    std::vector<unsigned short> Waveform1[MaxNChannels][MaxNData];
    std::vector<unsigned short> Waveform2[MaxNChannels][MaxNData];
    std::vector<bool>           DigiWaveform1[MaxNChannels][MaxNData];
    std::vector<bool>           DigiWaveform2[MaxNChannels][MaxNData];

  public:
    Data();
    ~Data();
  
    void Allocate80MBMemory();
    void AllocateMemory(uint32_t size);
    
    void SetSaveWaveToMemory(bool OnOff) { this->SaveWaveToMemory = OnOff; }
    
    void ClearData();
    void ClearTriggerRate();
    void ClearBuffer();
    
    void CopyBuffer( const char * buffer, const unsigned int size); 
        
    void PrintBuffer() const; //Incorrect
    void DecodeBuffer(bool fastDecode, int verbose = 0); /// fastDecode will not save waveform
    void DecodeBuffer(char * buffer, unsigned int size, bool fastDecode, int verbose = 0); // for outside data
  
    void PrintStat() const;

    void PrintData() const;

    //^================= Saving data
    void OpenSaveFile(std::string fileNamePrefix);
    void SaveData();
    void CloseSaveFile();
    unsigned int GetFileSize() const {return outFileSize;}
    uint64_t GetTotalFileSize() const {return FinishedOutFilesSize + outFileSize;}

  
  protected:
    
    unsigned int nw;
    bool SaveWaveToMemory;

    unsigned int outFileIndex;
    std::string outFilePrefix;
    std::string outFileName;
    FILE * outFile;
    unsigned int outFileSize; // should be max at 2 GB
    uint64_t FinishedOutFilesSize; // sum of files size.
    
    ///for temperary
    std::vector<unsigned short> tempWaveform1; 
    std::vector<unsigned short> tempWaveform2; 
    std::vector<bool> tempDigiWaveform1;
    std::vector<bool> tempDigiWaveform2;
  
    unsigned int ReadBuffer(unsigned int nWord, int verbose = 0);

    int DecodePHADualChannelBlock(unsigned int ChannelMask, bool fastDecode, int verbose);
    int DecodePSDDualChannelBlock(unsigned int ChannelMask, bool fastDecode, int verbose);

    
};

//==========================================

inline Data::Data(){
  ch2ns = 2.0;
  boardSN = 0;
  DPPType = V1730_DPP_PHA_CODE;
  DPPTypeStr = "";
  IsNotRollOverFakeAgg = false;
  buffer = NULL;
  for ( int i = 0; i < MaxNChannels; i++) TotNumEvents[i] = 0;
  ClearData();
  ClearTriggerRate();
  SaveWaveToMemory = true;
  nw = 0;

  outFileIndex = 0;
  outFilePrefix = "";
  outFileName = "";
  outFile = nullptr;
  outFileSize = 0; // should be max at 2 GB
  FinishedOutFilesSize = 0; // sum of files size.
}

inline Data::~Data(){
  if( buffer != NULL ) delete buffer;
}

inline void Data::AllocateMemory(uint32_t size){
  ClearBuffer();
  AllocatedSize = size;
  buffer = (char *) malloc( AllocatedSize); 
  printf("Allocated %u byte for buffer = %u words\n", AllocatedSize, AllocatedSize / 4);
}

inline void Data::Allocate80MBMemory(){
  AllocateMemory( 80 * 1024 * 1024 ); /// 80 M Byte
}

inline void Data::ClearTriggerRate(){ 
  for( int i = 0 ; i < MaxNChannels; i++) {
    TriggerRate[i] = 0.0; 
    NumEventsDecoded[i] = 0;
  }
}

inline void Data::ClearData(){
  nByte = 0;
  AllocatedSize = 0;
  IsNotRollOverFakeAgg = false;
  for( int i = 0 ; i < MaxNChannels; i++){
    NumEvents[i] = 0;
    for( int j = 0; j < MaxNData; j++){
      Timestamp[i][j] = 0;
      fineTime[i][j] = 0;
      Energy[i][j] = 0;
      Energy2[i][j] = 0;
      Waveform1[i][j].clear();
      Waveform2[i][j].clear();
      DigiWaveform1[i][j].clear();
      DigiWaveform2[i][j].clear();
    }
  }
  
  tempWaveform1.clear();
  tempWaveform2.clear();
  tempDigiWaveform1.clear();
  tempDigiWaveform2.clear();
}

inline void Data::ClearBuffer(){
  delete buffer;
  AllocatedSize = 0;
  nByte = 0;
}

inline void Data::CopyBuffer(const char * buffer, const unsigned int size){
  std::memcpy(this->buffer, buffer, size);
}

inline void Data::OpenSaveFile(std::string fileNamePrefix){

  outFilePrefix = fileNamePrefix;
  char saveFileName[100];
  sprintf(saveFileName, "%s_%03d_%3s_%03u.fsu", outFilePrefix.c_str() , boardSN, DPPTypeStr.c_str(), outFileIndex);

  outFileName = saveFileName;
  outFile = fopen(saveFileName, "wb"); // overwrite binary
  fseek(outFile, 0L, SEEK_END);
  outFileSize = ftell(outFile);

}

inline void Data::SaveData(){

  if( outFileSize > (unsigned int) MaxSaveFileSize){
    FinishedOutFilesSize += ftell(outFile);
    CloseSaveFile();
    outFileIndex ++;
    char saveFileName[100];
    sprintf(saveFileName, "%s_%03d_%3s_%03u.fsu", outFilePrefix.c_str() , boardSN, DPPTypeStr.c_str(), outFileIndex);
    outFileName = saveFileName;
    outFile = fopen(outFileName.c_str(), "wb"); //overwrite binary
  }

  fwrite(buffer, nByte, 1, outFile);
  outFileSize = ftell(outFile);

}

inline void Data::CloseSaveFile(){
  if( outFile != NULL ){
    fclose(outFile);
    int result = chmod(outFileName.c_str(), S_IRUSR | S_IRGRP | S_IROTH);
    if( result != 0 ) printf("somewrong when set file (%s) to read only.", outFileName.c_str());
  }
}

inline void Data::PrintStat() const{

  if( !IsNotRollOverFakeAgg ) {
    printf(" this is roll-over fake event or no events.\n");
    return;
  }
  printf("%2s | %6s | %9s | %6s\n", "ch", "# Evt.", "Rate [Hz]", "Tot. Evt.");
  printf("---+--------+-----------+----------\n");
  for(int ch = 0; ch < MaxNChannels; ch++){
    printf("%2d | %6d | %9.2f | %6lu\n", ch, NumEventsDecoded[ch], TriggerRate[ch], TotNumEvents[ch]);
  }
  printf("---+--------+-----------+----------\n");
}

inline void Data::PrintBuffer() const{
  unsigned int length = sizeof(buffer);
  for( unsigned int i = 0; i < length; i++){
    printf("%3d | 0x%08x \n", i, buffer[i]);
  }
}

inline void Data::PrintData() const{
  printf("============================= Print Data\n");
  for( int ch = 0; ch < MaxNChannels ; ch++){
    if( NumEvents[ch] == 0 ) continue;
    printf("------------ ch : %d, %d \n", ch, NumEvents[ch]);
    for( int ev = 0; ev < NumEvents[ch] ; ev++){
      printf("%4d, %5u, %15llu, %5u \n", ev, Energy[ch][ev], Timestamp[ch][ev], fineTime[ch][ev]);
    }
  }
}

//####################################################### Decode
inline unsigned int Data::ReadBuffer(unsigned int nWord, int verbose){
  if( buffer == NULL ) return 0;
  
  unsigned int word = 0;
  for( int i = 0 ; i < 4 ; i++) word += ((buffer[i + 4 * nWord] & 0xFF) << 8*i);
  if( verbose >= 2) printf("%6d | 0x%08X | ", nWord, word);
  return word;
}

inline void Data::DecodeBuffer(char * buffer, unsigned int size, bool fastDecode, int verbose){
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

  if( nByte == 0 ) {
    return;
  } 

  nw = 0;
  
  ClearTriggerRate();
  
  do{
    if( verbose >= 1 ) printf("Data::DecodeBuffer ######################################### Board Agg.\n");
    unsigned int word = ReadBuffer(nw, verbose);
    if( ( (word >> 28) & 0xF ) == 0xA ) { /// start of Board Agg
      unsigned int nWord = word & 0x0FFFFFFF ;
      if( verbose >= 1 ) printf("Number of words in this Agg : %u = %u Byte\n", nWord, nWord * 4);
      
      nw = nw + 1; word = ReadBuffer(nw, verbose);
      unsigned int BoardID = ((word >> 27) & 0x1F);
      unsigned short pattern = ((word >> 8 ) & 0x7FFF );
      bool BoardFailFlag =  ((word >> 26) & 0x1 );
      unsigned int ChannelMask = ( word & 0xFF ) ;
      if( verbose >= 1 ) printf("Board ID(type) : %d, FailFlag = %d, Patten = %u, ChannelMask = 0x%X\n", 
                                    BoardID, BoardFailFlag, pattern, ChannelMask);
      
      if( BoardID > 0 ) {
        switch(BoardID){
          case 0x8 : DPPType = V1730_DPP_PSD_CODE; break;
          case 0xB : DPPType = V1730_DPP_PHA_CODE; break;
        }
      }
      
      nw = nw + 1; 
      unsigned int bdAggCounter = ReadBuffer(nw, verbose);
      if( verbose >= 1 ) printf("Board Agg Counter : %u \n", bdAggCounter & 0x7FFFFF);
      
      nw = nw + 1; 
      unsigned int bdAggTimeTag = ReadBuffer(nw, verbose);
      if( verbose >= 2 ) printf("Agg Counter : %u \n", bdAggTimeTag);
      
      for( int chMask = 0; chMask < MaxNChannels/2 ; chMask ++ ){
        if( ((ChannelMask >> chMask) & 0x1 ) == 0 ) continue;
        if( verbose >= 2 ) printf("==================== Dual Channel Block, ch Mask : %d, nw : %d\n", chMask *2, nw);

        if( DPPType == V1730_DPP_PHA_CODE ) {
          if ( DecodePHADualChannelBlock(chMask, fastDecode, verbose) < 0 ) break;
        }
        if( DPPType == V1730_DPP_PSD_CODE ) {
          if ( DecodePHADualChannelBlock(chMask, fastDecode, verbose) < 0 ) break;
        }
      }
    }else{
      if( verbose >= 1 ) printf("nw : %d, incorrect buffer header. \n", nw);
      break;
    }
    nw++;
    ///printf("nw : %d ,x 4 = %d, nByte : %d \n", nw, 4*nw, nByte);
  }while(4*nw < nByte);
  
  ///Calculate trigger rate and first and last Timestamp
  for(int ch = 0; ch < MaxNChannels; ch++){
    if( NumEventsDecoded[ch] > 0 ) IsNotRollOverFakeAgg = true;
    unsigned long long dTime = Timestamp[ch][NumEvents[ch]-1] - Timestamp[ch][NumEvents[ch] - NumEventsDecoded[ch]]; 
    double sec =  dTime * ch2ns / 1e9;
    if( sec != 0 && NumEventsDecoded[ch] > 1 ){
      TriggerRate[ch] = NumEventsDecoded[ch]/sec;
    }
  }
  
}

inline int Data::DecodePHADualChannelBlock(unsigned int ChannelMask, bool fastDecode, int verbose){
  
  nw = nw + 1; 
  unsigned int word = ReadBuffer(nw, verbose);

  bool hasFormatInfo = ((word >> 31) & 0x1);
  unsigned int aggSize = ( word & 0x7FFFFFFF ) ;
  if( verbose >= 2 ) printf("Dual Channel size : %d \n",  aggSize);
  unsigned int nSample = 0; /// wave form;
  unsigned int nEvents = 0;
  unsigned int extra2Option = 0;
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
      bool hasWaveForm  = ( (word >> 27 ) & 0x1 );
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
    nEvents = (aggSize - 2) / (nSample/2 + 2 + hasExtra2 );
    if( verbose >= 2 ) printf("-------------------nEvents : %d \n", nEvents);
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
    
    //TODO Skip
    ///===== read waveform
    if( !fastDecode && SaveWaveToMemory ) {
      tempWaveform1.clear();
      tempWaveform2.clear();
      tempDigiWaveform1.clear();
    }
    
    unsigned int triggerAtSample = 0 ;
    if( fastDecode ){
      nw += nSample/2;
    }else{
      for( unsigned int wi = 0; wi < nSample/2; wi++){
        nw = nw +1 ; word = ReadBuffer(nw, verbose-2);
        ///The CAEN manual is wrong, the bit [31:16] is anaprobe 1
        bool isTrigger1 = (( word >> 31 ) & 0x1 );
        bool dp1 = (( word >> 30 ) & 0x1 );
        unsigned short wave1 = (( word >> 16) & 0x3FFF);
        ///The CAEN manual is wrong, the bit [31:16] is anaprobe 2
        bool isTrigger0 = (( word >> 15 ) & 0x1 );
        bool dp0 = (( word >> 14 ) & 0x1 );
        unsigned short wave0 = ( word & 0x3FFF);
        
        if( SaveWaveToMemory){
          if( hasDualTrace ){
            tempWaveform1.push_back(wave1);
            tempWaveform2.push_back(wave0);
            tempDigiWaveform1.push_back(dp0);
          }else{
            tempWaveform1.push_back(wave1);          
            tempWaveform1.push_back(wave0);
            tempDigiWaveform1.push_back(dp0);
            tempDigiWaveform1.push_back(dp1);
          }
        }

        if( isTrigger0 == 1 ) triggerAtSample = 2*wi ;      
        if( isTrigger1 == 1 ) triggerAtSample = 2*wi + 1;
        
        if( verbose >= 4 && ev == 0 ){
          if( !hasDualTrace ){ 
            printf("%4d| %5d, %d, %d \n",   2*wi, wave0, dp0, isTrigger0);
            printf("%4d| %5d, %d, %d \n", 2*wi+1, wave1, dp1, isTrigger1);
          }else{
            printf("%4d| %5d, %5d | %d, %d | %d  %d\n",   wi, wave0, wave1, dp0, dp1, isTrigger0, isTrigger1);
          }
        }
      }
      
      if( SaveWaveToMemory ) {
        if( hasDualTrace ){
          Waveform1[channel][NumEvents[channel]] = tempWaveform1;
          Waveform2[channel][NumEvents[channel]] = tempWaveform2;
        }else{
          Waveform1[channel][NumEvents[channel]] = tempWaveform1;
        }
        DigiWaveform1[channel][NumEvents[channel]] = tempDigiWaveform1;
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
      printf("      roll-over : %d (fake event)\n", ((extra >>  1) & 0x1) );
      printf("     fake-event : %d \n", ((extra >>  3) & 0x1) );
      printf("     input sat. : %d \n", ((extra >>  4) & 0x1) );
      printf("       lost trg : %d \n", ((extra >>  5) & 0x1) );
      printf("        tot trg : %d \n", ((extra >>  6) & 0x1) );
      printf("     coincident : %d \n", ((extra >>  7) & 0x1) );
      printf("      not coin. : %d \n", ((extra >>  8) & 0x1) );
      printf("        pile-up : %d \n", ((extra >>  9) & 0x1) );
      printf(" trapezoid sat. : %d \n", ((extra >> 10) & 0x1) );
    }

    if( rollOver == 0 ) {
      Energy[channel][NumEvents[channel]] = energy;
      Timestamp[channel][NumEvents[channel]] = timeStamp;
      if(extra2Option == 0 || extra2Option == 2 ) fineTime[channel][NumEvents[channel]] = (extra2 & 0x07FF );
      NumEvents[channel] ++; 
      NumEventsDecoded[channel] ++; 
      TotNumEvents[channel] ++;
    }
    
    if( verbose >= 1 ) printf("%4d | ch : %2d, PileUp : %d , energy : %5d, rollOver: %d,  timestamp : %10llu, triggerAt : %d, nSample : %d, %f sec\n", 
                                NumEvents[channel], channel, pileUp, energy, rollOver, timeStamp, triggerAtSample, nSample , timeStamp * 4. / 1e9);
    
  }
  
  ///=========== Key information
  /// ch, energy, timestamp
  /// trace

  return nw;
}

inline int Data::DecodePSDDualChannelBlock(unsigned int ChannelMask, bool fastDecode, int verbose){
  
  nw = nw + 1; 
  unsigned int word = ReadBuffer(nw, verbose);
  
  if( (word >> 31) != 1 ) return 0;
  
  unsigned int aggSize = ( word & 0x3FFFFF ) ;
  if( verbose >= 2 ) printf(" size : %d \n",  aggSize);

  unsigned int nEvents = 0;
  nw = nw + 1; word = ReadBuffer(nw, verbose);
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
        case 0: printf("[0:15] trapwzoid baseline * 4 [16:31] Extended timestamp (16-bit)\n"); break;
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
  
  nEvents = (aggSize -2) / (nSample/2 + 2 + hasExtra );
  if( verbose >= 2 ) printf("----------------- nEvents : %d \n", nEvents);

  ///========= Decode an event
  for( unsigned int ev = 0; ev < nEvents ; ev++){
    if( verbose >= 2 ) printf("--------------------------- event : %d\n", ev);
    nw = nw +1 ; word = ReadBuffer(nw, verbose);
    bool channelTag = ((word >> 31) & 0x1);
    unsigned int timeStamp0 = (word & 0x7FFFFFFF);
    int channel = ChannelMask*2 + channelTag;
    if( verbose >= 2 ) printf("ch : %d, timeStamp %u \n", channel, timeStamp0);
    
    ///===== read waveform
    if( !fastDecode && SaveWaveToMemory ) {
      tempWaveform1.clear();
      tempWaveform2.clear();
      tempDigiWaveform1.clear();
      tempDigiWaveform2.clear();
    }
    
    if( fastDecode ){
      nw += nSample/2;
    }else{
      for( unsigned int wi = 0; wi < nSample/2; wi++){
        nw = nw +1 ; word = ReadBuffer(nw, verbose - 2);
        bool dp2b = (( word >> 31 ) & 0x1 );
        bool dp1b = (( word >> 30 ) & 0x1 );
        unsigned short waveb = (( word >> 16) & 0x3FFF);
        
        bool dp2a = (( word >> 15 ) & 0x1 );
        bool dp1a = (( word >> 14 ) & 0x1 );
        unsigned short wavea = ( word & 0x3FFF);
        
        if( SaveWaveToMemory){
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
        }
        
        if( verbose >= 3 && ev == 0 ){
          printf("%4d| %5d, %d, %d \n",   2*wi, wavea, dp1a, dp2a);
          printf("%4d| %5d, %d, %d \n", 2*wi+1, waveb, dp1b, dp2b);
        }
      }
      if( SaveWaveToMemory ) {
        if( hasDualTrace ){
          Waveform1[channel][NumEvents[channel]] = tempWaveform1;
          Waveform2[channel][NumEvents[channel]] = tempWaveform2;
        }else{
          Waveform1[channel][NumEvents[channel]] = tempWaveform1;
        }
        DigiWaveform1[channel][NumEvents[channel]] = tempDigiWaveform1;
        DigiWaveform2[channel][NumEvents[channel]] = tempDigiWaveform2;
      }
    }
    nw = nw +1 ; word = ReadBuffer(nw, verbose);
    unsigned int extra = word;
    unsigned long long extTimeStamp = 0;
    if( extraOption == 0 || extraOption == 2 ) extTimeStamp = (extra >> 16);
    
    unsigned long long timeStamp = (extTimeStamp << 31) ;
    timeStamp = timeStamp + timeStamp0;
    
    nw = nw +1 ; word = ReadBuffer(nw, verbose);
    unsigned int Qlong  = (( word >> 16) & 0xFFFF);
    unsigned int Qshort = (word & 0x7FFF);
    bool isEnergyCorrect = ((word >> 15) & 0x1);
    
    if( isEnergyCorrect == 1 ) {
      NumEvents[channel] ++; 
      NumEventsDecoded[channel] ++; 
      TotNumEvents[channel] ++;
    }
    
    if( verbose >= 2 ) printf("extra : 0x%08x, Qshort : %d, Qlong : %d \n", extra, Qshort, Qlong);
    
    if( verbose >= 1 ) printf("ch : %2d, Qshort : %d, Qlong : %d, timestamp : %llu\n", 
                                channel, Qshort, Qlong, timeStamp);
    
    Timestamp[channel][NumEvents[channel]] = timeStamp;
    
  }
  
  ///=========== Key information
  /// ch, Qshort, Qlong , timestamp
  /// trace

  return nw;
}

#endif
