#ifndef Hit_H
#define Hit_H

class Hit{
public:
  unsigned short sn;
  uint8_t bd;
  uint8_t ch;
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
    bd = 0;
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

};

#endif