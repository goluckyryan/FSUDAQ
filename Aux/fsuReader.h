#include "../ClassData.h"

class FSUReader{

  public:
    FSUReader(std::string fileName, unsigned short numCh);
    ~FSUReader();

    void ScanNumBlock();
    int  ReadNextBlock(bool fast = false, int verbose = 0);
    int  ReadBlock(unsigned int ID, int verbose = 0);

    unsigned long GetTotNumBlock() const{ return totNumBlock;}

    Data * GetData() const{return data;}

  private:

    FILE * inFile;
    Data * data;

    unsigned long inFileSize;
    unsigned int  filePos;
    unsigned long totNumBlock;
    unsigned int  blockID;

    // for dual block 
    int DPPType;
    int chMask;

    std::vector<unsigned int> blockPos;

    unsigned int word[1]; /// 4 byte
    size_t dummy;
    char * buffer;

};

inline FSUReader::FSUReader(std::string fileName, unsigned short numCh){

  inFile = fopen(fileName.c_str(), "r");

  if( inFile == NULL ){
    printf("Cannot open file : %s \n", fileName.c_str());
    return;
  }

  fseek(inFile, 0L, SEEK_END);
  inFileSize = ftell(inFile);
  printf("%s | file size : %ld Byte = %.2f MB\n", fileName.c_str() , inFileSize, inFileSize/1024./1024.);
  fseek(inFile, 0L, SEEK_SET);
  filePos = 0;

  data = new Data(numCh);

  totNumBlock = 0;
  blockID = 0;
  blockPos.clear();

  //Get DPPType from file name;
  DPPType = -1;
  if( fileName.find("PHA") != std::string::npos ) DPPType = DPPType::DPP_PHA_CODE;
  if( fileName.find("PSD") != std::string::npos ) DPPType = DPPType::DPP_PSD_CODE;
  if( fileName.find("QDC") != std::string::npos ) DPPType = DPPType::DPP_QDC_CODE;

  //check is the file is *.fsu or *.fsu.X
  size_t found = fileName.find_last_of('.');
  std::string ext = fileName.substr(found + 1);

  if( ext.find("fsu") != std::string::npos ) {
    printf("It is an raw data *.fsu format\n");
  }else{
    chMask = atoi(ext.c_str());
    printf("It is a splitted dual block data *.fsu.X format, dual channel mask : %d \n", chMask);
  }

  //ScanNumBlock();

}

inline FSUReader::~FSUReader(){
  delete data;

}

inline int FSUReader::ReadNextBlock(bool fast, int verbose){
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
    data->ClearBuffer(); // this will clear the buffer.

  }else if( (header & 0xF ) == 0x8 ) { /// dual channel header

    unsigned int dualSize = (word[0] & 0x7FFFFFFF) * 4; ///byte    
    buffer = new char[dualSize];
    dummy = fread(buffer, dualSize, 1, inFile);
    filePos = ftell(inFile);

    data->buffer = buffer;
    data->DecodeDualBlock(buffer, dualSize, DPPType, chMask, false, verbose);
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
  return ReadNextBlock(false, verbose);

}

inline void FSUReader::ScanNumBlock(){
  if( feof(inFile) ) return;

  blockID = 0;
  blockPos.push_back(0);

  data->ClearData();
  fseek(inFile, 0L, SEEK_SET);
  filePos = 0;

  while( ReadNextBlock(true) == 0 ){
    blockPos.push_back(filePos);
    blockID ++;
    printf("%u, %.2f%% %u/%lu\n\033[A\r", blockID, filePos*100./inFileSize, filePos, inFileSize);
  }

  totNumBlock = blockID;
  printf("\nScan complete: number of data Block : %lu\n", totNumBlock);
  
  rewind(inFile);
  blockID = 0;  
  filePos = 0;

}