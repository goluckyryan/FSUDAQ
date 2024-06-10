#include "../ClassData.h"
#include "../Hit.h"
#include <algorithm>
#include <filesystem>

// #include "AggSeparator.h"

class FSUReader{

  public:
    FSUReader();
    FSUReader(std::string fileName, uInt dataSize = 100, int verbose = 1);
    FSUReader(std::vector<std::string> fileList, uInt dataSize = 100, int verbose = 1);
    ~FSUReader();

    void OpenFile(std::string fileName, uInt dataSize, int verbose = 1);
    bool IsOpen() const{return inFile == nullptr ? false : true;}
    bool IsEndOfFile() const { 
      // printf("%s : %d | %ld |%ld\n", __func__, feof(inFile), ftell(inFile), inFileSize);
      if(fileList.empty() ) {
        if( (uLong )ftell(inFile) >= inFileSize){
          return true;
        }else{
          return false;
        }
      }else{
        if( fileID + 1 == (int) fileList.size() && ((uLong)ftell(inFile) >= inFileSize) ) {
          return true;
        }else{
          return false;
        }
      }
    }

    void ScanNumBlock(int verbose = 1, uShort saveData = 0); // saveData = 0 (no save), 1 (no trace), 2 (with trace);
    int  ReadNextBlock(bool traceON = false, int verbose = 0, uShort saveData = 0); // saveData = 0 (no save), 1 (no trace), 2 (with trace);
    int  ReadBlock(unsigned int ID, int verbose = 0);

    unsigned int GetFilePos() const {return filePos;}
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
    ulong GetHitListLength() const {return hit.size();}
    std::vector<Hit> GetHitVector() const {return hit;}
    void SortHit(int verbose = false);
    Hit GetHit(int id) const {
      if( id < 0 ) id = hit.size() + id;
      return hit[id];
    }

    void ClearHitCount() {hitCount = 0;}
    ulong GetHitCount() const{return hitCount;}

    std::vector<Hit> ReadBatch(unsigned int batchSize = 1000000, bool verbose = false); // output the sorted Hit

    // std::string SaveHit(std::vector<Hit> hitList, bool isAppend = false);
    // std::string SaveHit2NewFile(std::string saveFolder = "./", std::string indexStr = "");
    // void SortAndSaveTS(unsigned int batchSize = 1000000, bool verbose = false);
    // off_t GetTSFileSize() const {return tsFileSize;}

    //TODO 
    //void SplitFile(unsigned long hitSizePreFile);

    void PrintHit(ulong numHit = -1, ulong startIndex = 0) {
      for( ulong i = startIndex; i < std::min(numHit, hitCount); i++){
        printf("%10zu ", i); hit[i].Print();
      }
    }

    static void PrintHitListInfo(std::vector<Hit> * hitList, std::string name){
      size_t n = hitList->size();
      size_t s = sizeof(Hit);
      printf("============== %s, size : %zu | %.2f MByte\n", name.c_str(), n, n*s/1024./1024.);
      if( n > 0 ){
        printf("t0 : %15llu ns\n", hitList->front().timestamp);
        printf("t1 : %15llu ns\n", hitList->back().timestamp);
        printf("dt : %15.3f ms\n", (hitList->back().timestamp - hitList->front().timestamp)/1e6);
      }
    }

    void PrintHitListInfo(){
      size_t n = hit.size();
      size_t s = sizeof(Hit);
      printf("============== reader.hit, size : %zu | %.2f MByte\n", n, n*s/1024./1024.);
      if( n > 0 ){
        printf("t0 : %15llu ns\n", hit.at(0).timestamp);
        printf("t1 : %15llu ns\n", hit.back().timestamp);
        printf("dt : %15.3f ms\n", (hit.back().timestamp - hit.front().timestamp)/1e6);
      }
    }


    //void SaveAsCAENCoMPASSFormat();

  private:

    FILE * inFile;
    Data * data;

    std::string fileName;
    std::vector<std::string> fileList;
    short fileID;
    unsigned long inFileSize;
    unsigned int  filePos;
    unsigned long totNumBlock;
    unsigned int  blockID;

    bool isDualBlock;

    uShort sn;
    uShort DPPType;
    uShort tick2ns;
    uShort order;
    uShort chMask;
    uShort numCh;

    std::vector<unsigned int> blockPos;
    std::vector<unsigned int > blockTimeStamp;

    unsigned long hitCount;

    std::vector<Hit> hit;

    unsigned int word[1]; /// 4 byte
    size_t dummy;
    char * buffer;

    off_t  tsFileSize;

};

inline FSUReader::~FSUReader(){
  delete data;

  if( inFile  ) fclose(inFile);

}

inline FSUReader::FSUReader(){
  inFile = nullptr;
  data = nullptr;

  blockPos.clear();
  blockTimeStamp.clear();
  hit.clear();

  fileList.clear();
  fileID = -1;

}

inline FSUReader::FSUReader(std::string fileName, uInt dataSize, int verbose){
  inFile = nullptr;
  data = nullptr;

  blockPos.clear();
  blockTimeStamp.clear();
  hit.clear();

  fileList.clear();
  fileID = -1;
  OpenFile(fileName, dataSize, verbose);
}

inline FSUReader::FSUReader(std::vector<std::string> fileList, uInt dataSize, int verbose){
  inFile = nullptr;
  data = nullptr;

  blockPos.clear();
  blockTimeStamp.clear();
  hit.clear();
  //The files are the same DPPType and sn
  this->fileList = fileList;
  fileID = 0;
  OpenFile(fileList[fileID], dataSize, verbose);

}

inline void FSUReader::OpenFile(std::string fileName, uInt dataSize, int verbose){

  /// File format must be YYY...Y_runXXX_AAA_BBB_TT_CCC.fsu
  /// YYY...Y  = prefix
  /// XXX = runID, 3 digits
  /// AAA = board Serial Number, 3 digits
  /// BBB = DPPtype, 3 digits
  ///  TT = tick2ns, any digits
  /// CCC = over size index, 3 digits

  if( inFile != nullptr ) fclose(inFile);

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

  if( fileID > 0 ) return;

  totNumBlock = 0;
  blockID = 0;
  blockPos.clear();
  blockTimeStamp.clear();

  hitCount = 0;
  hit.clear();

  //check is the file is *.fsu or *.fsu.X
  size_t found = fileName.find_last_of('.');
  std::string ext = fileName.substr(found + 1);

  if( ext.find("fsu") != std::string::npos ) {
    if(verbose > 1) printf("It is an raw data *.fsu format\n");
    isDualBlock = false;
    chMask = -1;
  }else{
    chMask = atoi(ext.c_str());
    isDualBlock = true;
    if(verbose > 1) printf("It is a splitted dual block data *.fsu.X format, dual channel mask : %d \n", chMask);
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

  DPPType = 0;
  if( fileName.find("PHA") != std::string::npos ) DPPType = DPPTypeCode::DPP_PHA_CODE;
  if( fileName.find("PSD") != std::string::npos ) DPPType = DPPTypeCode::DPP_PSD_CODE;
  if( fileName.find("QDC") != std::string::npos ) DPPType = DPPTypeCode::DPP_QDC_CODE;
  if( DPPType == 0 ){
    fclose(inFile);
    inFile = nullptr;
    printf("Cannot find DPPType in the filename. Abort.");
    return ;
  }

  numCh = (DPPType == DPPTypeCode::DPP_QDC_CODE ? 64 : 16);

  data = new Data(numCh, dataSize);
  data->tick2ns = tick2ns;
  data->boardSN = sn;
  data->DPPType = DPPType;

}

inline int FSUReader::ReadNextBlock(bool traceON, int verbose, uShort saveData){
  if( inFile == NULL ) return -1;
  if( feof(inFile) || filePos >= inFileSize) {
    if( fileID >= 0 && fileID + 1  < (short) fileList.size() ){
      printf("-------------- next file\n");
      fileID ++;
      OpenFile(fileList[fileID], data->GetDataSize(), 1 );
    }else{
      return -1;
    }
  }
  
  dummy = fread(word, 4, 1, inFile);
  fseek(inFile, -4, SEEK_CUR);

  if( dummy != 1) {
    printf("fread error, should read 4 bytes, but read %ld x 4 byte, file pos: %ld / %ld byte\n", 
            dummy, ftell(inFile), inFileSize);
    return -10;
  }
  short header = ((word[0] >> 28 ) & 0xF);

  Hit temp;

  if( header == 0xA ) { ///normal header
 
    unsigned int aggSize = (word[0] & 0x0FFFFFFF) * 4; ///byte
    if( aggSize > inFileSize - ftell(inFile)) aggSize = inFileSize - ftell(inFile);
    buffer = new char[aggSize];
    dummy = fread(buffer, aggSize, 1, inFile);
    filePos = ftell(inFile);
    if( dummy != 1) {
      printf("fread error, should read %d bytes, but read %ld x %d byte, file pos: %ld / %ld byte \n", 
             aggSize, dummy, aggSize, ftell(inFile), inFileSize);
      return -30;
    }

    data->DecodeBuffer(buffer, aggSize, !traceON, verbose); // data will own the buffer
    //printf(" word Index = %u | filePos : %u | ", data->GetWordIndex(), filePos);

  }else if( (header & 0xF ) == 0x8 ) { /// dual channel header

    unsigned int dualSize = (word[0] & 0x7FFFFFFF) * 4; ///byte    
    buffer = new char[dualSize];
    dummy = fread(buffer, dualSize, 1, inFile);
    filePos = ftell(inFile);

    data->buffer = buffer;
    data->DecodeDualBlock(buffer, dualSize, DPPType, chMask, !traceON, verbose);

  }else{
    printf("incorrect header.\n trminate.");
    return -20;
  }

  unsigned int eventCout  = 0;

  for( int ch = 0; ch < data->GetNChannel(); ch++){
    if( data->NumEventsDecoded[ch] == 0 ) continue;

    hitCount += data->NumEventsDecoded[ch];
    eventCout += data->NumEventsDecoded[ch];

    if( saveData ){
      int start = data->GetDataIndex(ch) - data->NumEventsDecoded[ch] + 1;
      if( start < 0 ) start = start + data->GetDataSize();

      for( int i = start; i < start + data->NumEventsDecoded[ch]; i++ ){
        int k  = i % data->GetDataSize();
        
        temp.sn = sn;
        temp.ch = ch;
        temp.energy    = data->GetEnergy(ch, k);
        temp.energy2   = data->GetEnergy2(ch, k);
        temp.timestamp = data->GetTimestamp(ch, k);
        temp.fineTime  = data->GetFineTime(ch, k);
        temp.pileUp    = data->GetPileUp(ch, k);
        if( saveData > 1 ) {
          temp.traceLength = data->Waveform1[ch][k].size();
          temp.trace = data->Waveform1[ch][k];
        }else{
          temp.traceLength = 0;
          if( temp.trace.size() > 0 ) temp.trace.clear();
        }

        hit.push_back(temp);
      }
    }
  }

  data->ClearTriggerRate();
  data->ClearNumEventsDecoded();
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
  if( inFile == nullptr ) return;
  if( feof(inFile) ) return;

  blockID = 0;
  blockPos.push_back(0);

  data->ClearData();
  rewind(inFile);
  filePos = 0;

  bool isTraceOn = saveData < 2 ? false : true;

  while( ReadNextBlock(isTraceOn, verbose - 1, saveData) == 0 ){
    blockPos.push_back(filePos);
    blockTimeStamp.push_back(data->aggTime);
    blockID ++;
    if(verbose && blockID % 10000 == 0) printf("%u, %.2f%% %u/%lu\n\033[A\r", blockID, filePos*100./inFileSize, filePos, inFileSize);
  }

  totNumBlock = blockID;
  if(verbose) {
    printf("\nScan complete: number of data Block : %lu\n", totNumBlock);
    printf(  "                      number of hit : %lu", hitCount);
    if( hitCount > 1e6 ) printf(" = %.3f million", hitCount/1e6); 
    printf("\n");
    if( saveData )printf(  "              size of the hit array : %lu\n", hit.size());

    if( saveData ){
      size_t sizeT = sizeof(hit[0]) * hit.size();
      printf("size of hit array : %lu byte = %.2f kByte, = %.2f MByte\n", sizeT, sizeT/1024., sizeT/1024./1024.);
    }
  } 

  if( fileList.size() > 0  ){
    fileID = 0;
    OpenFile(fileList[fileID], data->GetDataSize(), 0);
  }

  rewind(inFile);
  blockID = 0;  
  filePos = 0;

  //check is the hitCount == hit.size();
  if( saveData ){
    if( hitCount != hit.size()){
      printf("!!!!!! the Data::dataSize is not big enough. !!!!!!!!!!!!!!!\n");
    }else{
      SortHit(verbose+1);
    }
  }
}

inline std::vector<Hit> FSUReader::ReadBatch(unsigned int batchSize, bool verbose){

  // printf("%s sn:%d. filePos : %lu\n", __func__, sn, ftell(inFile));

  std::vector<Hit> hitList_A;
  if( IsEndOfFile() ) {
    hitList_A = hit;
    hit.clear();  
    return hitList_A;
  }

  if( hit.size() == 0 ){
    int res = 0;
    do{
      res = ReadNextBlock(true, 0, 3);
    }while ( hit.size() < batchSize && res == 0);
    SortHit();
    uLong t0_B = hit.at(0).timestamp;
    uLong t1_B = hit.back().timestamp;
    if( verbose ) {
      printf(" hit in memeory : %7zu | %u | %lu \n", hit.size(), filePos, inFileSize);
      printf("t0 : %15lu ns\n", t0_B);
      printf("t1 : %15lu ns\n", t1_B);
      printf("dt : %15.3f ms\n", (t1_B - t0_B)/1e6);
    }

    hitList_A = hit;
    hit.clear();

  }else{

    hitList_A = hit;
    hit.clear();

  }

  if( IsEndOfFile() ) return hitList_A; // when file finished for 1st batch read

  int res = 0;
  do{
    res = ReadNextBlock(true, 0, 3);
  }while ( hit.size() < batchSize && res == 0);
  SortHit();
  uLong t0_B = hit.at(0).timestamp;
  uLong t1_B = hit.back().timestamp;

  if( verbose ) {
    printf(" hit in memeory : %7zu | %u | %lu \n", hit.size(), filePos, inFileSize);
    printf("t0 : %15lu\n", t0_B);
    printf("t1 : %15lu\n", t1_B);
    printf("dt : %15.3f ms\n", (t1_B - t0_B)/1e6);
  }

  uLong t0_A = hitList_A.at(0).timestamp;
  uLong t1_A = hitList_A.back().timestamp;
  ulong ID_A = 0;
  ulong ID_B = 0;

  if( t0_A >= t0_B) {
    printf("\033[0;31m!!!!!!!!!!!!!!!!! %s | Need to increase the batch size. \033[0m\n", __func__);
    return std::vector<Hit> ();
  }

  if( t1_A > t0_B) { // need to sort between two hitList

    if( verbose ) {
      printf("############# need to sort \n");
      printf("=========== sume of A + B : %zu \n", hitList_A.size() + hit.size());
    }

    std::vector<Hit> hitTemp;

    // find the hit that is >= t0_B, save them to hitTemp
    for( size_t j = 0; j < hitList_A.size() ; j++){
      if( hitList_A[j].timestamp < t0_B ) continue;;
      if( ID_A == 0 ) ID_A = j;
      hitTemp.push_back(hitList_A[j]);
    }

    // remove hitList_A element that is >= t0_B
    hitList_A.erase(hitList_A.begin() + ID_A, hitList_A.end() );
    
    // find the hit that is <= t1_A, save them to hitTemp
    for( size_t j = 0; j < hit.size(); j++){
      if( hit[j].timestamp > t1_A ) {
        break;
      }
      hitTemp.push_back(hit[j]);
      ID_B = j + 1;
    }

    // remove hit elements that is <= t1_A
    hit.erase(hit.begin(), hit.begin() + ID_B  );

    // sort hitTemp
    std::sort(hitTemp.begin(), hitTemp.end(), [](const Hit& a, const Hit& b) {
      return a.timestamp < b.timestamp;
    });


    if( verbose ) {
      printf("----------------- ID_A : %lu, Drop\n", ID_A);
      printf("----------------- ID_B : %lu, Drop\n", ID_B);
      PrintHitListInfo(&hitList_A, "hitList_A");
      PrintHitListInfo(&hitTemp, "hitTemp");
      PrintHitListInfo();
      printf("=========== sume of A + B + Temp : %zu \n", hitList_A.size() + hit.size()  + hitTemp.size());
      printf("----------------- refill hitList_A \n");
    }

    for( size_t j = 0; j < hitTemp.size(); j++){
      hitList_A.push_back(hitTemp[j]);
    }
    hitTemp.clear();
    
    if( verbose ) {
      PrintHitListInfo(&hitList_A, "hitList_A");
      PrintHitListInfo();
      printf("=========== sume of A + B : %zu \n", hitList_A.size() + hit.size());
    }
   
  }

  return hitList_A;

}

/*
inline void FSUReader::SortAndSaveTS(unsigned int batchSize, bool verbose){

  int count = 0;
  std::vector<Hit> hitList_A ;

  do{

    if( verbose ) printf("***************************************************\n");

    int res = 0;
    do{
      res = ReadNextBlock(true, 0, 3);
    }while ( hit.size() < batchSize && res == 0);

    SortHit();
    uLong t0_B = hit.at(0).timestamp;
    uLong t1_B = hit.back().timestamp;

    if( verbose ) {
      printf(" hit in memeory : %7zu | %u | %lu \n", hit.size(), filePos, inFileSize);
      printf("t0 : %15lu\n", t0_B);
      printf("t1 : %15lu\n", t1_B);
    }

    if( count == 0 ) {
      hitList_A = hit; // copy hit
    }else{

      uLong t0_A = hitList_A.at(0).timestamp;
      uLong t1_A = hitList_A.back().timestamp;
      ulong ID_A = 0;
      ulong ID_B = 0;

      if( t0_A > t0_B) {
        printf("Need to increase the batch size. \n");
        return;
      }

      if( t1_A > t0_B) { // need to sort between two hitList

        if( verbose ) {
          printf("############# need to sort \n");
          printf("=========== sume of A + B : %zu \n", hitList_A.size() + hit.size());
        }

        std::vector<Hit> hitTemp;

        for( size_t j = 0; j < hitList_A.size() ; j++){
          if( hitList_A[j].timestamp < t0_B ) continue;
          if( ID_A == 0 ) ID_A = j;
          hitTemp.push_back(hitList_A[j]);
        }

        hitList_A.erase(hitList_A.begin() + ID_A, hitList_A.end() );
        if( verbose ) {
          printf("----------------- ID_A : %lu, Drop\n", ID_A);
          PrintHitListInfo(hitList_A, "hitList_A");
        }
  
      
        for( size_t j = 0; j < hit.size(); j++){
          if( hit[j].timestamp > t1_A ) {
            ID_B = j;
            break;
          }
          hitTemp.push_back(hit[j]);
        }

        std::sort(hitTemp.begin(), hitTemp.end(), [](const Hit& a, const Hit& b) {
          return a.timestamp < b.timestamp;
        });

        hit.erase(hit.begin(), hit.begin() + ID_B  );

        if( verbose ) {
          PrintHitListInfo(hitTemp, "hitTemp");
          printf("----------------- ID_B : %lu, Drop\n", ID_B);
          PrintHitListInfo(hit, "hit");
          printf("=========== sume of A + B + Temp : %zu \n", hitList_A.size() + hit.size()  + hitTemp.size());
          printf("----------------- refill hitList_A \n");
        }
        ulong ID_Temp = 0;
        for( size_t j = 0; j < hitTemp.size(); j++){
          hitList_A.push_back(hitTemp[j]);
          if( hitList_A.size() >= batchSize ) {
            ID_Temp = j+1;
            break;
          }
        }

        hitTemp.erase(hitTemp.begin(), hitTemp.begin() + ID_Temp );
        for( size_t j = 0 ; j < hit.size(); j ++){
          hitTemp.push_back(hit[j]);
        }
        SaveHit(hitList_A, count <= 1 ? false : true);

        if( verbose ) {
          PrintHitListInfo(hitList_A, "hitList_A");
          PrintHitListInfo(hitTemp, "hitTemp");
          printf("----------------- replace hitList_A by hitTemp \n");
        }

        hitList_A.clear();
        hitList_A = hitTemp;
        hit.clear();

        if( verbose ) {
          PrintHitListInfo(hitList_A, "hitList_A");
          printf("===========================================\n");
        }

      }else{ // save hitList_A, replace hitList_A 
        
        SaveHit(hitList_A, count <= 1? false : true);
        hitList_A.clear();
        hitList_A = hit;
        if( verbose ) PrintHitListInfo(hitList_A, "hitList_A");

      }
    }

    ClearHitList();
    count ++;
  }while(filePos < inFileSize);  

  SaveHit(hitList_A, count <= 1 ? false : true);

  printf("================= finished.\n");
}
*/

/*
inline std::string FSUReader::SaveHit(std::vector<Hit> hitList, bool isAppend){

  std::string outFileName;
  if( fileList.empty() ) {
    outFileName =  fileName + ".ts" ;
  }else{
    outFileName =  fileList[0] + ".ts" ;
  }
  uint64_t hitSize = hitList.size();

  FILE * outFile ;
  if( isAppend ) {
    outFile = fopen(outFileName.c_str(), "rb+"); //read/write bineary

    rewind(outFile);
    fseek( outFile, 4, SEEK_CUR);
    uint64_t org_hitSize;
    fread(&org_hitSize, 8, 1, outFile);

    rewind(outFile);
    fseek( outFile, 4, SEEK_CUR);

    org_hitSize += hitSize;

    fwrite(&org_hitSize, 8, 1, outFile);
    fseek(outFile, 0, SEEK_END);

  }else{
    outFile = fopen(outFileName.c_str(), "wb"); //overwrite binary
    uint32_t header = 0xAA000000;
    header += sn;
    fwrite( &header, 4, 1, outFile );
    fwrite( &hitSize, 8, 1, outFile);
  }


  for( ulong i = 0; i < hitSize; i++){

    if( i% 10000 == 0 ) printf("Saving %lu/%lu Hit (%.2f%%)\n\033[A\r", i, hitSize, i*100./hitSize);

    uint16_t flag = hitList[i].ch + (hitList[i].pileUp << 8) ;

    if( DPPType == DPPTypeCode::DPP_PSD_CODE ) flag += ( 1 << 15);
    if( hitList[i].traceLength > 0 ) flag += (1 << 14);

    // fwrite( &(hit[i].ch), 1, 1, outFile);
    fwrite( &flag, 2, 1, outFile);
    fwrite( &(hitList[i].energy), 2, 1, outFile);
    if( DPPType == DPPTypeCode::DPP_PSD_CODE ) fwrite( &(hitList[i].energy2), 2, 1, outFile);
    fwrite( &(hitList[i].timestamp), 6, 1, outFile);
    fwrite( &(hitList[i].fineTime), 2, 1, outFile);
    if( hitList[i].traceLength > 0 ) fwrite( &(hitList[i].traceLength), 2, 1, outFile);
    
    for( uShort j = 0; j < hitList[i].traceLength; j++){
      fwrite( &(hitList[i].trace[j]), 2, 1, outFile);
    }
    
  }

  off_t tsFileSize = ftello(outFile);  // unsigned int =  Max ~4GB
  fclose(outFile);

  printf("Saved to %s, size: ", outFileName.c_str());
  if( tsFileSize < 1024 ) {
    printf(" %ld Byte", tsFileSize);
  }else if( tsFileSize < 1024*1024 ) {
    printf(" %.2f kB", tsFileSize/1024.);
  }else if( tsFileSize < 1024*1024*1024){
    printf(" %.2f MB", tsFileSize/1024./1024.);
  }else{
    printf(" %.2f GB", tsFileSize/1024./1024./1024.);
  }
  printf("\n");

  return outFileName;

}
*/


