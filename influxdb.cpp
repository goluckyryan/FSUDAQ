#include "influxdb.h"


InfluxDB::InfluxDB(std::string url, bool verbose){

  curl = curl_easy_init();  
  if( verbose) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
  SetURL(url);
  respondCode = 0;
  dataPoints = "";
}

InfluxDB::~InfluxDB(){
  curl_easy_cleanup(curl);
}

void InfluxDB::SetURL(std::string url){
  // check the last char of url is "/"
  if( url.back() != '/') {
    this->databaseIP = url + "/";
  }else{
    this->databaseIP = url;
  }
}

bool InfluxDB::TestingConnection(){
  CheckDatabases();
  if( respond != CURLE_OK ) return false;
  return true;
}

std::string InfluxDB::CheckDatabases(){
  curl_easy_setopt(curl, CURLOPT_POST, 1);
  
  curl_easy_setopt(curl, CURLOPT_URL, (databaseIP   + "query").c_str());
  
  std::string postFields="q=Show databases";
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(postFields.length()));
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
  
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallBack);
  std::string readBuffer;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
  
  Execute();
  
  //printf("|%s|\n", readBuffer.c_str());
  
  if( respond != CURLE_OK) return "";

  databaseList.clear();
  
  size_t pos = readBuffer.find("values");
  
  if( pos > 0 ){
    std::string kaka = readBuffer.substr(pos+8);
  
    pos = kaka.find("}");
    kaka = kaka.substr(0, pos);
  
    int len = kaka.length();
    bool startFlag = false;
    std::string lala;
    
    char yaya = '"';
    
    for( int i = 0; i < len; i++){
      
      if( startFlag == false && kaka[i] == yaya ) {
        startFlag = true;
        lala = "";
        continue;
      }

      if( startFlag && kaka[i] == yaya ){
        startFlag = false;
        databaseList.push_back(lala);
        continue;
      }
      if( startFlag ) lala += kaka[i];
    }  
  }
  
  return readBuffer;
}

std::string InfluxDB::Query(std::string databaseName, std::string query){
  
  curl_easy_setopt(curl, CURLOPT_POST, 1);

  curl_easy_setopt(curl, CURLOPT_URL, (databaseIP   + "query?db=" + databaseName).c_str());

  std::string postFields = "q=" + query;
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(postFields.length()));
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallBack);
  std::string readBuffer;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
  
  Execute();
  
  //printf("|%s|\n", readBuffer.c_str());
  
  return readBuffer;
}

void InfluxDB::CreateDatabase(std::string databaseName){
  curl_easy_setopt(curl, CURLOPT_URL, (databaseIP + "query").c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1);
  
  std::string postFields = "q=CREATE DATABASE " + databaseName;
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(postFields.length()));
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
  
  Execute();
}

void InfluxDB::AddDataPoint(std::string fullString){
  dataPoints += fullString + "\n";
}

void InfluxDB::ClearDataPointsBuffer(){
  dataPoints = "";
}

void InfluxDB::PrintDataPoints(){
  printf("%s\n", dataPoints.c_str());
}

void InfluxDB::WriteData(std::string databaseName){  
  if( dataPoints.length() == 0 ) return;
  //printf("|%s|\n", (databaseIP + "write?db=" + databaseName).c_str());
  curl_easy_setopt(curl, CURLOPT_URL, (databaseIP + "write?db=" + databaseName).c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(dataPoints.length()));
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, dataPoints.c_str());
  Execute();
}


void InfluxDB::Execute(){
  try{
    respond = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &respondCode);
    //printf("==== respond %d (OK = %d)\n", respond, CURLE_OK);
    if( respond != CURLE_OK) printf("############# InfluxDB::Execute fail\n");
  } catch (std::exception& e){ // in case of unexpected error
    printf("%s\n", e.what());
    respond = CURLE_SEND_ERROR;
  }
}

size_t InfluxDB::WriteCallBack(char *contents, size_t size, size_t nmemb, void *userp){
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}
