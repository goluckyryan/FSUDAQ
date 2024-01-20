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
    FSUTSReader(std::string fileName, int verbose = 1);
    ~FSUTSReader();

    void OpenFile(std::string fileName, int verbose = 1);
    bool isOpen() const{return inFile == nullptr ? false : true;}

    void ScanFile(int verbose = 1);
    int  ReadNextHit(bool withTrace = true, int verbose = 0);
    int  ReadHitAt(unsigned int ID, bool withTrace = true, int verbose = 0);

    unsigned int  GetHitID()  const {return hitIndex;}
    unsigned long GetNumHit() const {return numHit;}

    std::string   GetFileName()     const {return fileName;}
    unsigned long GetFileByteSize() const {return inFileSize;}
    int           GetFileOrder()    const {return order;}  
    uShort        GetSN()           const {return sn;}
    ullong        GetT0()           const {return t0;}
    
    Hit* GetHit() const{return hit;}

  private:

    FILE * inFile;
 
    std::string fileName;
    unsigned long inFileSize;
    unsigned int  filePos;
    unsigned long numHit;

    uShort sn;
    int order;

    Hit* hit;
    unsigned int  hitIndex;
    std::vector<unsigned int> hitStartPos;

    unsigned long long t0;

    uint32_t header;
    size_t dummy;

};

inline FSUTSReader::~FSUTSReader(){

  fclose(inFile);

  delete hit;

}

inline FSUTSReader::FSUTSReader(){
  inFile = nullptr;

  hitStartPos.clear();
  hit = nullptr;
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

  std::string fileNameNoExt;
  size_t found = fileName.find_last_of(".fsu.ts");
  size_t found2 = fileName.find_last_of('/');
  if( found2 == std::string::npos ){
    fileNameNoExt = fileName.substr(0, found-7);
  }else{
    fileNameNoExt = fileName.substr(found2+1, found-7);
  }

  // Split the string by underscores
  std::istringstream iss(fileNameNoExt);
  std::vector<std::string> tokens;
  std::string token;

  while (std::getline(iss, token, '_')) { tokens.push_back(token); }
  sn = atoi(tokens[2].c_str());
  order = atoi(tokens[5].c_str());

  fseek(inFile, 0L, SEEK_END);
  inFileSize = ftell(inFile);
  if(verbose) printf("###### %s | file size : %ld Byte = %.2f MB\n", fileName.c_str() , inFileSize, inFileSize/1024./1024.);
  fseek(inFile, 0L, SEEK_SET);
  filePos = 0;

  numHit = 0;
  hitIndex = -1;
  hitStartPos.clear();
  hit = new Hit();
  hit->Clear();

  //check is the file is .ts file by checking the 1st 4 byte 
  
  dummy = fread(&header, 4, 1, inFile);
  printf(" header : 0x%8X.", header);
  if( (header >> 24) != 0xAA ){
    printf(" This is not a time-sorted fsu (*.fsu.ts) file. Abort.");
    return;
  }
  sn = (header & 0xFFFFFF);
  hit->sn = sn;
  printf(" S/N : %u \n", sn);

}

inline int FSUTSReader::ReadNextHit(bool withTrace, int verbose){
  if( inFile == NULL ) return -1;
  if( feof(inFile) ) return -1;
  if( filePos >= inFileSize) return -1;
  
  hitIndex ++;

  dummy = fread(&(hit->sn), 2, 1, inFile);
  dummy = fread(&(hit->ch), 1, 1, inFile);
  dummy = fread(&(hit->energy), 2, 1, inFile);
  dummy = fread(&(hit->energy2), 2, 1, inFile);
  dummy = fread(&(hit->timestamp), 8, 1, inFile);
  dummy = fread(&(hit->fineTime), 2, 1, inFile);
  dummy = fread(&(hit->traceLength), 2, 1, inFile);

  if( hit->trace.size() > 0 ) hit->trace.clear();

  if( withTrace && hit->traceLength > 0 ){
    for(uShort j = 0; j < hit->traceLength; j++){
      short temp;
      fread( &temp, 2, 1, inFile);
      hit->trace.push_back(temp);
    }
  }else{ 
    unsigned int jumpByte = hit->traceLength * 2;
    fseek(inFile, jumpByte, SEEK_CUR);
    hit->traceLength = 0;
  }

  filePos = ftell(inFile);

  // if(verbose) printf("Block index: %u, current file Pos: %u byte \n", hitIndex, filePos);

  if(verbose >= 2) hit->Print();
  if(verbose >= 3) hit->PrintTrace();
  
  return 0;
}

inline int FSUTSReader::ReadHitAt(unsigned int ID, bool withTrace, int verbose){
  if( numHit == 0 ) return -1;
  if( ID >= numHit ) return -1;

  fseek(inFile, 0L, SEEK_SET);

  if( verbose ) printf("Block index: %u, File Pos: %u byte\n", ID, hitStartPos[ID]);

  fseek(inFile, hitStartPos[ID], SEEK_CUR);
  filePos = hitStartPos[ID];
  hitIndex = ID - 1;
  return ReadNextHit(withTrace, verbose);

}

inline void FSUTSReader::ScanFile(int verbose){
  if( feof(inFile) ) return;

  hitStartPos.clear();
  fseek(inFile, 0L, SEEK_SET);
  dummy = fread(&header, 4, 1, inFile);
  filePos = ftell(inFile);
  hitStartPos.push_back(filePos);
  hitIndex = -1;

  while( ReadNextHit(false, verbose-1) == 0 ){ // no trace
    hitStartPos.push_back(filePos);

    if( hitIndex == 0 ) t0 = hit->timestamp;

    if(verbose > 1 ) printf("hitIndex : %u, Pos : %u - %u\n" , hitIndex, hitStartPos[hitIndex], hitStartPos[hitIndex+1]);

    if(verbose) printf(" %u, %.2f%% %u/%lu\n\033[A\r", hitIndex, filePos*100./inFileSize, filePos, inFileSize);
  }

  numHit = hitIndex + 1;
  if(verbose) printf("-----> Scan complete: number of hit : %lu\n", numHit);

  rewind(inFile);
  dummy = fread(&header, 4, 1, inFile);
  hitIndex = -1;  
  filePos = ftell(inFile);

}
