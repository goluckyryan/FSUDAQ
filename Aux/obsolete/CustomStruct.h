#ifndef CustomStruct_H
#define CustomStruct_H

#include <string>
#include <vector>
#include <cstdio>

#define ORDERSHIFT 100000

struct FileInfo {
  std::string fileName;
  unsigned int fileSize;
  unsigned int SN;
  unsigned long hitCount;
  unsigned short DPPType;
  unsigned short tick2ns;
  unsigned short order;
  unsigned short readerID;

  unsigned long long t0;

  unsigned long ID; // sn + 100000 * order

  void CalOrder(){ ID = ORDERSHIFT * SN + order; }

  void Print(){
    printf(" %10lu | %3d | %60s | %2d | %6lu | %10u Bytes = %.2f MB\n", 
            ID, DPPType, fileName.c_str(), tick2ns, hitCount, fileSize, fileSize/1024./1024.);  
  }
};

struct GroupInfo{

  std::vector<unsigned short> fileIDList;
  unsigned int usedHitCount ;

  std::vector<unsigned short> readerIDList;
  unsigned long hitID ; // this is the ID for the reader->GetHit(hitID);

  unsigned short currentID ; // the ID of the readerIDList;
  unsigned long hitCount ; // this is the hitCount for the currentID;
  unsigned int sn;
  bool finished;

  unsigned long long timeShift; 

};

#endif