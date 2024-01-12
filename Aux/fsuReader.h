#include "../ClassData.h"

class FSUReader{

  public:
    FSUReader(std::string fileName,  bool verbose = true);
    ~FSUReader();

    void ScanNumBlock(bool verbose = true);
    int  ReadNextBlock(bool fast = false, int verbose = 0, bool saveData = false);
    int  ReadBlock(unsigned int ID, int verbose = 0);

    unsigned long GetTotNumBlock() const{ return totNumBlock;}

    Data * GetData() const{return data;}

    int GetDPPType()   const{return DPPType;}
    int GetSN()        const{return sn;}
    int GetTick2ns()   const{return tick2ns;}
    int GetNumCh()     const{return numCh;}
    int GetFileOrder() const{return order;}  
    unsigned long GetFileByteSize() const {return inFileSize;}

    std::vector<unsigned int> GetBlockTimestamp() const {return blockTimeStamp;}

    std::vector<unsigned long long> GetTimestamp(int id) const {return timestamp[id];}
    std::vector<unsigned short>     GetEnergy(int id)    const {return energy[id];}
    std::vector<unsigned short>     GetEnergy2(int id)   const {return energy2[id];}
    std::vector<unsigned short>     GetChannel(int id)   const {return channel[id];}
    std::vector<unsigned short>     GeFineTime(int id)   const {return fineTime[id];}

    unsigned long GetHitCount() const{ return numHit;}

  private:

    FILE * inFile;
    Data * data;

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

    std::vector<std::vector<unsigned short>> channel;
    std::vector<std::vector<unsigned short>> energy;
    std::vector<std::vector<unsigned short>> energy2;
    std::vector<std::vector<unsigned long long>> timestamp;
    std::vector<std::vector<unsigned short>> fineTime;

    std::vector<unsigned short> tempChannel;
    std::vector<unsigned short> tempEnergy;
    std::vector<unsigned short> tempEnergy2;
    std::vector<unsigned long long> tempTimestamp;
    std::vector<unsigned short> tempFineTime;

    unsigned int word[1]; /// 4 byte
    size_t dummy;
    char * buffer;

};

inline FSUReader::FSUReader(std::string fileName, bool verbose){

  inFile = fopen(fileName.c_str(), "r");

  if( inFile == NULL ){
    printf("Cannot open file : %s \n", fileName.c_str());
    return;
  }

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
  channel.clear();
  timestamp.clear();
  energy.clear();


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
      tempTimestamp.clear();
      tempEnergy.clear();
      tempEnergy2.clear();
      tempChannel.clear();
      tempFineTime.clear();

      for( int ch = 0; ch < data->GetNChannel(); ch++){
        if( data->NumEventsDecoded[ch] == 0 ) continue;

        int start = data->DataIndex[ch] - data->NumEventsDecoded[ch] + 1;
        int stop  = data->DataIndex[ch];

        for( int i = start; i <= stop; i++ ){
          i = i % MaxNData;
          tempChannel.push_back(ch);
          tempEnergy.push_back(data->Energy[ch][i]);
          tempTimestamp.push_back(data->Timestamp[ch][i]);
          tempEnergy2.push_back(data->Energy2[ch][i]);
          tempFineTime.push_back(data->fineTime[ch][i]);
          
          numHit ++;
        }
      }

      timestamp.push_back(tempTimestamp);
      energy.push_back(tempEnergy);
      energy2.push_back(tempEnergy2);
      channel.push_back(tempChannel);
      fineTime.push_back(tempFineTime);
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

    size_t sizeT = sizeof(std::vector<unsigned long long>) * timestamp.size();
    for (size_t i = 0; i < timestamp.size(); ++i) {
        sizeT += sizeof(std::vector<unsigned long long>) + sizeof(unsigned long long) * timestamp[i].size();
    }

    printf("size of timestamp : %lu byte = %.2f kByte, = %.2f MByte\n", sizeT, sizeT/1024., sizeT/1024./1024.);

  } 
  rewind(inFile);
  blockID = 0;  
  filePos = 0;

}