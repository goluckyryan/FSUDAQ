#include "../Hit.h"
#include "../macro.h"
#include <stdio.h>
#include <string>
#include <sstream>
#include <cmath>
#include <cstring>  ///memset
#include <iostream> ///cout
#include <sstream>
#include <algorithm>
#include <bitset>
#include <vector>

class FSUTSReader{

  public:
    FSUTSReader();
    FSUTSReader(std::string fileName, int verbose = true);
    ~FSUTSReader();

    void OpenFile(std::string fileName, int verbose = true);
    bool isOpen() const{return inFile == nullptr ? false : true;}

    void ScanFile(int verbose = true);
    int  ReadNext(int verbose = 0);
    int  ReadBlock(unsigned int ID, int verbose = 0);

    unsigned long GetNumHit() const{ return numHit;}

    std::string   GetFileName() const{return fileName;}
    unsigned long GetFileByteSize() const {return inFileSize;}
    int           GetSN()        const{return sn;}

 
  private:

    FILE * inFile;
 
    std::string fileName;
    unsigned long inFileSize;
    unsigned int  filePos;
    unsigned long numHit;

    bool isDualBlock;
    ushort sn;

    unsigned int  blockID;
    std::vector<unsigned int> blockPos;

    Hit hit;

    uint32_t header;
    size_t dummy;

};

inline FSUTSReader::~FSUTSReader(){

  fclose(inFile);

}

inline FSUTSReader::FSUTSReader(){
  inFile = nullptr;

  blockPos.clear();
  hit.Clear();
}

inline FSUTSReader::FSUTSReader(std::string fileName, int verbose){
  OpenFile(fileName, verbose);
}

inline void FSUTSReader::OpenFile(std::string fileName, int verbose){

  inFile = fopen(fileName.c_str(), "r");

  if( inFile == NULL ){
    printf("FSUTSReader::Cannot open file : %s \n", fileName.c_str());
    this->fileName = "";
    return;
  }

  this->fileName = fileName;

  fseek(inFile, 0L, SEEK_END);
  inFileSize = ftell(inFile);
  if(verbose) printf("%s | file size : %ld Byte = %.2f MB\n", fileName.c_str() , inFileSize, inFileSize/1024./1024.);
  fseek(inFile, 0L, SEEK_SET);
  filePos = 0;

  numHit = 0;
  blockID = 0;
  blockPos.clear();
  hit.Clear();

  //check is the file is .ts file by checking the 1st 4 byte 
  
  dummy = fread(&header, 4, 1, inFile);
  printf(" header : 0x%8X \n", header);
  if( (header >> 24) != 0xAA ){
    printf(" This is not a time-sorted fsu (*.fsu.ts) file. Abort.");
    return;
  }
  sn = (header & 0xFFFFFF);
  hit.sn = sn;
  printf(" S/N : %u \n", sn);

}

inline int FSUTSReader::ReadNext(int verbose){
  if( inFile == NULL ) return -1;
  if( feof(inFile) ) return -1;
  if( filePos >= inFileSize) return -1;
  
  dummy = fread(&(hit.sn), 2, 1, inFile);
  dummy = fread(&(hit.ch), 1, 1, inFile);
  dummy = fread(&(hit.energy), 2, 1, inFile);
  dummy = fread(&(hit.energy2), 2, 1, inFile);
  dummy = fread(&(hit.timestamp), 8, 1, inFile);
  dummy = fread(&(hit.fineTime), 2, 1, inFile);
  dummy = fread(&(hit.traceLength), 2, 1, inFile);

  hit.trace.clear();
  if( hit.traceLength > 0 ){
    for(uShort j = 0; j < hit.traceLength; j++){
      short temp;
      fread( &temp, 2, 1, inFile);
      hit.trace.push_back(temp);
    }
  }

  filePos = ftell(inFile);

  if(verbose >= 1) hit.Print();
  if(verbose >= 2) hit.PrintTrace();
  
  return 0;
}

inline int FSUTSReader::ReadBlock(unsigned int ID, int verbose){
  if( numHit == 0 ) return -1;
  if( ID >= numHit ) return -1;

  fseek( inFile, 0L, SEEK_SET);

  if( verbose ) printf("Block index: %u, File Pos: %u byte\n", ID, blockPos[ID]);

  fseek(inFile, blockPos[ID], SEEK_CUR);
  filePos = blockPos[ID];
  blockID = ID;
  return ReadNext(verbose);

}

inline void FSUTSReader::ScanFile(int verbose){
  if( feof(inFile) ) return;

  blockPos.clear();
  fseek(inFile, 0L, SEEK_SET);
  dummy = fread(&header, 4, 1, inFile);
  filePos = ftell(inFile);
  blockPos.push_back(filePos);
  blockID = 0;

  while( ReadNext(verbose) == 0 ){
    blockPos.push_back(filePos);
    blockID ++;
    if(verbose) printf("%u, %.2f%% %u/%lu\n\033[A\r", blockID, filePos*100./inFileSize, filePos, inFileSize);
  }

  numHit = blockID;
  if(verbose) printf("\nScan complete: number of hit : %lu\n", numHit);

  rewind(inFile);
  blockID = 0;  
  filePos = 0;

}
