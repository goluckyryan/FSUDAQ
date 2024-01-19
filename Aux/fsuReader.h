#include "../ClassData.h"
#include "../Hit.h"
#include <algorithm>
#include <filesystem>

class FSUReader{

  public:
    FSUReader();
    FSUReader(std::string fileName, uShort dataSize = 100, int verbose = 1);
    ~FSUReader();

    void OpenFile(std::string fileName, uShort dataSize, int verbose = 1);
    bool isOpen() const{return inFile == nullptr ? false : true;}

    void ScanNumBlock(int verbose = 1, uShort saveData = 0);
    int  ReadNextBlock(bool skipTrace= false, int verbose = 0, uShort saveData = 0); // saveData = 0 (no save), 1 (no trace), 2 (with trace);
    int  ReadBlock(unsigned int ID, int verbose = 0);

    unsigned long GetTotNumBlock() const{ return totNumBlock;}
    std::vector<unsigned int> GetBlockTimestamp() const {return blockTimeStamp;}

    Data * GetData() const{return data;}

    std::string GetFileName() const{return fileName;}
    int GetDPPType()   const{return DPPType;}
    int GetSN()        const{return sn;}
    int GetTick2ns()   const{return tick2ns;}
    int GetNumCh()     const{return numCh;}
    int GetFileOrder() const{return order;}  
    int GetChMask()    const{return chMask;}
    unsigned long GetFileByteSize() const {return inFileSize;}

    void ClearHitList() { hit.clear();}
    Hit GetHit(int id) const {return hit[id];}
    ulong GetHitListLength() const {return hit.size();}
 
    void ClearHitCount() {numHit = 0;}
    ulong GetHitCount() const{ return numHit;}
    std::vector<Hit> GetHitVector() const {return hit;}
    void SortHit(int verbose = false);

    std::string SaveHit2NewFile(std::string saveFolder = "./");

  private:

    FILE * inFile;
    Data * data;

    std::string fileName;
    unsigned long inFileSize;
    unsigned int  filePos;
    unsigned long totNumBlock;
    unsigned int  blockID;

    bool isDualBlock;

    int sn;
    int DPPType;
    int tick2ns;
    int order;
    int chMask;
    uShort numCh;

    std::vector<unsigned int> blockPos;
    std::vector<unsigned int > blockTimeStamp;

    unsigned long numHit;

    std::vector<Hit> hit;

    unsigned int word[1]; /// 4 byte
    size_t dummy;
    char * buffer;

};

inline FSUReader::~FSUReader(){
  delete data;

  fclose(inFile);

}

inline FSUReader::FSUReader(){
  inFile = nullptr;
  data = nullptr;

  blockPos.clear();
  blockTimeStamp.clear();
  hit.clear();

}

inline FSUReader::FSUReader(std::string fileName, uShort dataSize, int verbose){
  OpenFile(fileName, dataSize, verbose);
}

inline void FSUReader::OpenFile(std::string fileName, uShort dataSize, int verbose){

  inFile = fopen(fileName.c_str(), "r");

  if( inFile == NULL ){
    printf("FSUReader::Cannot open file : %s \n", fileName.c_str());
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
    isDualBlock = false;
    chMask = -1;
  }else{
    chMask = atoi(ext.c_str());
    isDualBlock = true;
    if(verbose) printf("It is a splitted dual block data *.fsu.X format, dual channel mask : %d \n", chMask);
  }

  std::string fileNameNoExt;
  found = fileName.find_last_of(".fsu");
  size_t found2 = fileName.find_last_of('/');
  if( found2 == std::string::npos ){
    fileNameNoExt = fileName.substr(0, found-4);
  }else{
    fileNameNoExt = fileName.substr(found2+1, found-4);
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

  numCh = (DPPType == DPPType::DPP_QDC_CODE ? 64 : 16);

  data = new Data(numCh, dataSize);
  data->tick2ns = tick2ns;
  data->boardSN = sn;
  data->DPPType = DPPType;

}

inline int FSUReader::ReadNextBlock(bool skipTrace, int verbose, uShort saveData){
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

    data->DecodeBuffer(buffer, aggSize, skipTrace, verbose); // data will own the buffer

  }else if( (header & 0xF ) == 0x8 ) { /// dual channel header

    unsigned int dualSize = (word[0] & 0x7FFFFFFF) * 4; ///byte    
    buffer = new char[dualSize];
    dummy = fread(buffer, dualSize, 1, inFile);
    filePos = ftell(inFile);

    data->buffer = buffer;
    data->DecodeDualBlock(buffer, dualSize, DPPType, chMask, skipTrace, verbose);    

  }else{
    printf("incorrect header.\n trminate.");
    return -20;
  }

  for( int ch = 0; ch < data->GetNChannel(); ch++){
    if( data->NumEventsDecoded[ch] == 0 ) continue;

    numHit += data->NumEventsDecoded[ch];

    if( saveData ){
      int start = data->DataIndex[ch] - data->NumEventsDecoded[ch] + 1;
      int stop  = data->DataIndex[ch];

      for( int i = start; i <= stop; i++ ){
        i = i % data->GetDataSize();
        
        temp.sn = sn;
        temp.ch = ch;
        temp.energy = data->Energy[ch][i];
        temp.energy2 = data->Energy2[ch][i];
        temp.timestamp = data->Timestamp[ch][i];
        temp.fineTime = data->fineTime[ch][i];
        if( saveData > 1 ) {
          temp.traceLength = data->Waveform1[ch][i].size();
          temp.trace = data->Waveform1[ch][i];
        }else{
          temp.traceLength = 0;
          if( temp.trace.size() > 0 ) temp.trace.clear();
        }

        hit.push_back(temp);

      }
    }
  }

  data->ClearTriggerRate();
  data->ClearBuffer(); // this will clear the buffer.

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

inline void FSUReader::SortHit(int verbose){
  if( verbose) printf("\nQuick Sort hit array according to time...");
  std::sort(hit.begin(), hit.end(), [](const Hit& a, const Hit& b) {
    return a.timestamp < b.timestamp;
  });
  if( verbose) printf(".......done.\n");
}

inline void FSUReader::ScanNumBlock(int verbose, uShort saveData){
  if( feof(inFile) ) return;

  blockID = 0;
  blockPos.push_back(0);

  data->ClearData();
  fseek(inFile, 0L, SEEK_SET);
  filePos = 0;

  while( ReadNextBlock(saveData < 2 ? true : false, verbose - 1, saveData) == 0 ){
    blockPos.push_back(filePos);
    blockTimeStamp.push_back(data->aggTime);
    blockID ++;
    if(verbose) printf("%u, %.2f%% %u/%lu\n\033[A\r", blockID, filePos*100./inFileSize, filePos, inFileSize);
  }

  totNumBlock = blockID;
  if(verbose) {
    printf("\nScan complete: number of data Block : %lu\n", totNumBlock);
    printf(  "                      number of hit : %lu\n", numHit);

    if( saveData ){
      size_t sizeT = sizeof(hit[0]) * hit.size();
      printf("size of hit array : %lu byte = %.2f kByte, = %.2f MByte\n", sizeT, sizeT/1024., sizeT/1024./1024.);
    }
  } 
  rewind(inFile);
  blockID = 0;  
  filePos = 0;

  SortHit(verbose);

}

inline std::string FSUReader::SaveHit2NewFile(std::string saveFolder){

  printf("FSUReader::%s\n", __func__);

  std::string folder = "";
  size_t found = fileName.find_last_of('/');
  std::string outFileName = fileName;
  if( found != std::string::npos ){
    folder = fileName.substr(0, found + 1);
    outFileName = fileName.substr(found +1 );
  }

  if( saveFolder.empty() ) saveFolder = "./";
  if( saveFolder.back() != '/') saveFolder += '/';

  //checkif the saveFolder exist;
  if( saveFolder != "./"){
    if (!std::filesystem::exists(saveFolder)) {
      if (std::filesystem::create_directory(saveFolder)) {
        std::cout << "Directory created successfully." << std::endl;
      } else {
        std::cerr << "Failed to create directory." << std::endl;
      }
    } 
  }

  outFileName = saveFolder + outFileName + ".ts";

  FILE * outFile = fopen(outFileName.c_str(), "wb"); //overwrite binary

  uint32_t header = 0xAA000000;
  header += sn;
  fwrite( &header, 4, 1, outFile );

  for( ulong i = 0; i < numHit; i++){

    printf("Saving %lu/%lu (%.2f%%)\n\033[A\r", i, numHit, i*100./numHit);

    fwrite( &(hit[i].sn), 2, 1, outFile);
    fwrite( &(hit[i].ch), 1, 1, outFile);
    fwrite( &(hit[i].energy), 2, 1, outFile);
    fwrite( &(hit[i].energy2), 2, 1, outFile);
    fwrite( &(hit[i].timestamp), 8, 1, outFile);
    fwrite( &(hit[i].fineTime), 2, 1, outFile);
    fwrite( &(hit[i].traceLength), 2, 1, outFile);
    
    for( uShort j = 0; j < hit[i].traceLength; j++){
      fwrite( &(hit[i].trace[j]), 2, 1, outFile);
    }
    
  }

  off_t outFileSize = ftello(outFile);  // unsigned int =  Max ~4GB
  fclose(outFile);

  printf("Saved to %s, size: ", outFileName.c_str());
  if( outFileSize < 1024 ) {
    printf(" %ld Byte", outFileSize);
  }else if( outFileSize < 1024*1024 ) {
    printf(" %.2f kB", outFileSize/1024.);
  }else if( outFileSize < 1024*1024*1024){
    printf(" %.2f MB", outFileSize/1024./1024.);
  }else{
    printf(" %.2f GB", outFileSize/1024./1024./1024.);
  }
  printf("\n");

  return outFileName;

}
