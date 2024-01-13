#ifndef Hit_H
#define Hit_H

class Hit{
public:
  int sn;
  unsigned short bd;
  unsigned short ch;
  unsigned short energy;
  unsigned short energy2;
  unsigned long long timestamp;
  unsigned short fineTime;

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
    trace.clear();
  }

  void Print(){
    printf("(%5d, %2d) %6d %10llu, %6d, %5ld\n", sn, ch, energy, timestamp, fineTime, trace.size());
  }

};

#endif