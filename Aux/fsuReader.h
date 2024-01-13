#include "../ClassData.h"
#include "../Hit.h"
#include <algorithm>

class FSUReader{

  public:
    FSUReader();
    FSUReader(std::string fileName,  bool verbose = true);
    ~FSUReader();

    void OpenFile(std::string fileName,  bool verbose = true);

    void ScanNumBlock(bool verbose = true);
    int  ReadNextBlock(bool fast = false, int verbose = 0, bool saveData = false);
    int  ReadBlock(unsigned int ID, int verbose = 0);

    unsigned long GetTotNumBlock() const{ return totNumBlock;}

    Data * GetData() const{return data;}

    std::string GetFileName() const{return fileName;}
    int GetDPPType()   const{return DPPType;}
    int GetSN()        const{return sn;}
    int GetTick2ns()   const{return tick2ns;}
    int GetNumCh()     const{return numCh;}
    int GetFileOrder() const{return order;}  
    unsigned long GetFileByteSize() const {return inFileSize;}

    std::vector<unsigned int> GetBlockTimestamp() const {return blockTimeStamp;}

    Hit GetHit(int id) const {return hit[id];}
    std::vector<Hit> GetHitVector() const {return hit;}

    unsigned long GetHitCount() const{ return hit.size();}

  private:

    FILE * inFile;
    Data * data;

    std::string fileName;
    unsigned long inFileSize;
    unsigned int  filePos;
    unsigned long totNumBlock;
    unsigned int  blockID;

    // for dual block 
    int sn;
    int DPPType;
    int tick2ns;
    int order;
    int chMask;
    int numCh;

    std::vector<unsigned int> blockPos;
    std::vector<unsigned int > blockTimeStamp;

    unsigned long numHit;

    std::vector<Hit> hit;

    unsigned int word[1]; /// 4 byte
    size_t dummy;
    char * buffer;

};

inline FSUReader::FSUReader(){
  inFile = nullptr;
  data = nullptr;

  blockPos.clear();
  blockTimeStamp.clear();
  hit.clear();

}

inline FSUReader::FSUReader(std::string fileName, bool verbose){
  OpenFile(fileName, verbose);
}

inline void FSUReader::OpenFile(std::string fileName, bool verbose){

  inFile = fopen(fileName.c_str(), "r");

  if( inFile == NULL ){
    printf("Cannot open file : %s \n", fileName.c_str());
    this->fileName = "";
    return;
  }

  this->fileName = fileName;

  fseek(inFile, 0L, SEEK_END);
  inFileSize = ftell(inFile);
  if(verbose) printf("%s | file size : %ld Byte = %.2f MB\n", fileName.c_str() , inFileSize, inFileSize/1024./1024.);
  fseek(inFile, 0L, SEEK_SET);
  filePos = 0;

  totNumBlock = 0;
  blockID = 0;
  blockPos.clear();
  blockTimeStamp.clear();

  numHit = 0;
  hit.clear();

  //check is the file is *.fsu or *.fsu.X
  size_t found = fileName.find_last_of('.');
  std::string ext = fileName.substr(found + 1);

  if( ext.find("fsu") != std::string::npos ) {
    if(verbose) printf("It is an raw data *.fsu format\n");
  }else{
    chMask = atoi(ext.c_str());
    if(verbose) printf("It is a splitted dual block data *.fsu.X format, dual channel mask : %d \n", chMask);
  }

  std::string fileNameNoExt;
  size_t found2 = fileName.find_last_of('/');
  if( found == std::string::npos ){
    fileNameNoExt = fileName.substr(0, found);
  }else{
    fileNameNoExt = fileName.substr(found2+1, found);
  }

  // Split the string by underscores
  std::istringstream iss(fileNameNoExt);
  std::vector<std::string> tokens;
  std::string token;

  while (std::getline(iss, token, '_')) { tokens.push_back(token); }
  sn = atoi(tokens[2].c_str());
  tick2ns = atoi(tokens[4].c_str());
  order = atoi(tokens[5].c_str());

  DPPType = -1;
  if( fileName.find("PHA") != std::string::npos ) DPPType = DPPType::DPP_PHA_CODE;
  if( fileName.find("PSD") != std::string::npos ) DPPType = DPPType::DPP_PSD_CODE;
  if( fileName.find("QDC") != std::string::npos ) DPPType = DPPType::DPP_QDC_CODE;

  numCh = DPPType == DPPType::DPP_QDC_CODE ? 64 : 16;

  data = new Data(numCh);
  data->tick2ns = tick2ns;
  data->boardSN = sn;
  data->DPPType = DPPType;
  //ScanNumBlock();

}

inline FSUReader::~FSUReader(){
  delete data;

}

inline int FSUReader::ReadNextBlock(bool fast, int verbose,bool saveData){
  if( inFile == NULL ) return -1;
  if( feof(inFile) ) return -1;
  if( filePos >= inFileSize) return -1;
  
  dummy = fread(word, 4, 1, inFile);
  fseek(inFile, -4, SEEK_CUR);

  if( dummy != 1) {
    printf("fread error, should read 4 bytes, but read %ld x 4 byte, file pos: %ld byte\n", dummy, ftell(inFile));
    return -10;
  }
  short header = ((word[0] >> 28 ) & 0xF);

  Hit temp;

  if( header == 0xA ) { ///normal header
 
    unsigned int aggSize = (word[0] & 0x0FFFFFFF) * 4; ///byte    
    buffer = new char[aggSize];
    dummy = fread(buffer, aggSize, 1, inFile);
    filePos = ftell(inFile);
    if( dummy != 1) {
      printf("fread error, should read %d bytes, but read %ld x %d byte, file pos: %ld byte \n", aggSize, dummy, aggSize, ftell(inFile));
      return -30;
    }


    data->DecodeBuffer(buffer, aggSize, fast, verbose); // data will own the buffer

    if( saveData ){

      for( int ch = 0; ch < data->GetNChannel(); ch++){
        if( data->NumEventsDecoded[ch] == 0 ) continue;

        int start = data->DataIndex[ch] - data->NumEventsDecoded[ch] + 1;
        int stop  = data->DataIndex[ch];

        for( int i = start; i <= stop; i++ ){
          i = i % MaxNData;
         
          temp.sn = sn;
          temp.ch = ch;
          temp.energy = data->Energy[ch][i];
          temp.timestamp = data->Timestamp[ch][i];
          temp.energy2 = data->Energy2[ch][i];
          temp.fineTime = data->fineTime[ch][i];

          hit.push_back(temp);

          numHit ++;
        }
      }
    }

    data->ClearTriggerRate();
    data->ClearBuffer(); // this will clear the buffer.

  }else if( (header & 0xF ) == 0x8 ) { /// dual channel header

    unsigned int dualSize = (word[0] & 0x7FFFFFFF) * 4; ///byte    
    buffer = new char[dualSize];
    dummy = fread(buffer, dualSize, 1, inFile);
    filePos = ftell(inFile);

    data->buffer = buffer;
    data->DecodeDualBlock(buffer, dualSize, DPPType, chMask, false, verbose);
    data->ClearTriggerRate();
    data->ClearBuffer();
    

  }else{
    printf("incorrect header.\n trminate.");
    return -20;
  }

  return 0;

}

inline int FSUReader::ReadBlock(unsigned int ID, int verbose){
  if( totNumBlock == 0 )return -1;
  if( ID >= totNumBlock )return -1;

  data->ClearData();

  fseek( inFile, 0L, SEEK_SET);

  if( verbose ) printf("Block index: %u, File Pos: %u byte\n", ID, blockPos[ID]);

  fseek(inFile, blockPos[ID], SEEK_CUR);
  filePos = blockPos[ID];
  blockID = ID;
  return ReadNextBlock(false, verbose, false);

}

inline void FSUReader::ScanNumBlock(bool verbose){
  if( feof(inFile) ) return;

  blockID = 0;
  blockPos.push_back(0);

  data->ClearData();
  fseek(inFile, 0L, SEEK_SET);
  filePos = 0;

  while( ReadNextBlock(true, false, true) == 0 ){
    blockPos.push_back(filePos);
    blockTimeStamp.push_back(data->aggTime);
    blockID ++;
    if(verbose) printf("%u, %.2f%% %u/%lu\n\033[A\r", blockID, filePos*100./inFileSize, filePos, inFileSize);
  }

  totNumBlock = blockID;
  if(verbose) {
    printf("\nScan complete: number of data Block : %lu\n", totNumBlock);
    printf(  "                      number of hit : %lu\n", numHit);

    size_t sizeT = sizeof(hit[0]) * hit.size();
    printf("size of hit array : %lu byte = %.2f kByte, = %.2f MByte\n", sizeT, sizeT/1024., sizeT/1024./1024.);

  } 
  rewind(inFile);
  blockID = 0;  
  filePos = 0;

  if(verbose)  printf("\nQuick Sort hit array according to time...");

  std::sort(hit.begin(), hit.end(), [](const Hit& a, const Hit& b) {
    return a.timestamp < b.timestamp;
  });

  if(verbose) printf(".......done.\n");
}