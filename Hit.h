#ifndef Hit_H
#define Hit_H

#include <vector>

class Hit{
public:
  unsigned short sn;
  unsigned short ch;
  unsigned short energy;
  unsigned short energy2;
  unsigned long long timestamp;
  unsigned short fineTime;
  bool pileUp;

  unsigned short traceLength;
  std::vector<short> trace;

  Hit(){
    Clear();
  }

  void Clear(){
    sn = 0;
    ch = 0;
    energy = 0;
    energy2 = 0;
    timestamp = 0;
    fineTime = 0;
    traceLength = 0;
    pileUp = false;
    trace.clear();
  }

  void Print(){
    printf("(%5d, %2d) %6d %16llu, %6d, %d, %5ld\n", sn, ch, energy, timestamp, fineTime, pileUp, trace.size());
  }

  void PrintTrace(){
    for( unsigned short i = 0; i < traceLength; i++){
      printf("%3u | %6d \n", i, trace[i]);
    }
  }

  // Define operator< for sorting
  bool operator<(const Hit& other) const {
    return timestamp < other.timestamp;
  }  


  void WriteHitsToCAENBinary(FILE * file, uint32_t header){
    if( file == nullptr ) return;

    uint32_t flag = 0;
    uint8_t  waveFormCode = 1; // input

    // uint16_t header = 0xCAE1; // default to have the energy only
    // if( energy2 > 0 ) header += 0x4;
    // if( traceLength > 0 && withTrace ) header += 0x8;

    size_t dummy;
    dummy = fwrite(&sn, 2, 1, file);
    dummy = fwrite(&ch, 2, 1, file);

    uint64_t timestampPS = timestamp * 1000 + fineTime;
    dummy = fwrite(&timestampPS, 8, 1, file);

    dummy = fwrite(&energy, 2, 1, file);

    if( (header & 0x4) ) dummy = fwrite(&energy2, 2, 1, file);

    dummy = fwrite(&flag, 4, 1, file);

    if( traceLength > 0 && (header & 0x8) ){
      dummy = fwrite(&waveFormCode, 1, 1, file);
      dummy = fwrite(&traceLength, 4, 1, file);
      for( int j = 0; j < traceLength; j++ ){
        dummy = fwrite(&(trace[j]), 2, 1, file);
      }
    }

    if( dummy != 1 ) printf("write file error.\n");

  }

};

#endif