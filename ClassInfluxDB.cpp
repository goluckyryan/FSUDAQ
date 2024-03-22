#include "ClassInfluxDB.h"
#include "macro.h"

#include <regex>

InfluxDB::InfluxDB(){
  DebugPrint("%s", "InfluxDB");
  curl = curl_easy_init();  
  databaseIP = "";
  respondCode = 0;
  dataPoints = "";
  headers = nullptr;
  influxVersionStr = "";
  influxVersion = -1;
  token = "";
  connectionOK = false;
}

InfluxDB::InfluxDB(std::string url, bool verbose){
  DebugPrint("%s", "InfluxDB");
  curl = curl_easy_init();  
  if( verbose) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
  SetURL(url);
  respondCode = 0;
  dataPoints = "";
  headers = nullptr;
  influxVersionStr = "";
  influxVersion = -1;
  token = "";
  connectionOK = false;
}

InfluxDB::~InfluxDB(){
  DebugPrint("%s", "InfluxDB");
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
}

void InfluxDB::SetURL(std::string url){
  DebugPrint("%s", "InfluxDB");
  // check the last char of url is "/"
  if( url.back() != '/') {
    this->databaseIP = url + "/";
  }else{
    this->databaseIP = url;
  }
}

void InfluxDB::SetToken(std::string token){
  DebugPrint("%s", "InfluxDB");
  this->token = token;
  headers = curl_slist_append(headers, "Accept: application/csv");
  if( !token.empty() ) headers = curl_slist_append(headers, ("Authorization: Token " + token).c_str());
}

bool InfluxDB::TestingConnection(bool debug){
  DebugPrint("%s", "InfluxDB");
  CheckInfluxVersion(debug);
  if( respond != CURLE_OK ) return false;
  connectionOK = true;
  return true;
}

std::string InfluxDB::CheckInfluxVersion(bool debug){
  DebugPrint("%s", "InfluxDB");
  curl_easy_setopt(curl, CURLOPT_URL, (databaseIP   + "ping").c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL);
  curl_easy_setopt(curl, CURLOPT_HEADER, 1);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallBack);
  std::string respondStr;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respondStr);

  Execute();

  if( respond != CURLE_OK) return "CURL Error.";

  if( debug) printf("%s\n", respondStr.c_str());

  //Find Version from readBuffer
  std::regex pattern(R"(X-Influxdb-Version: (.*))");
  std::smatch match;

  if (regex_search(respondStr, match, pattern)) {
    influxVersionStr = match[1];

    size_t dotPosition = influxVersionStr.find('.');
    if( dotPosition != std::string::npos){
      influxVersion = atoi(influxVersionStr.substr(dotPosition-1, 1).c_str());
    }
  }

  // printf("Influx Version : %s | %u\n", influxVersionStr.c_str(), influxVersion);

  return respondStr;

}

std::string InfluxDB::CheckDatabases(){
  DebugPrint("%s", "InfluxDB");
  if( ! connectionOK ) return "no connection. try TestConnection() again.";
  if( influxVersion == 2 && token.empty() ) return "token no provided, abort.";

  curl_easy_setopt(curl, CURLOPT_POST, 1);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_HEADER, 0);
  
  curl_easy_setopt(curl, CURLOPT_URL, (databaseIP   + "query").c_str());
  
  std::string postFields="q=Show databases";
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(postFields.length()));
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
  
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallBack);
  std::string respondStr;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respondStr);
  
  Execute();
  
  // printf("|%s|\n", respondStr.c_str());

  if( respond != CURLE_OK) return "CURL Error.";

  databaseList.clear();

  // Split the input string into lines
  std::istringstream iss(respondStr);
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(iss, line)) {
      lines.push_back(line);
  }

  // Extract the third column from each line and store it in a vector
  std::vector<std::string> thirdColumn;
  for (const auto& l : lines) {
      std::istringstream lineIss(l);
      std::string token;
      for (int i = 0; std::getline(lineIss, token, ','); ++i) {
          if (i == 2) {  // Third column
              databaseList.push_back(token);
              break;
          }
      }
  }

  // {//============ when output is JSON  
  //   size_t pos = readBuffer.find("values");    
  //   if( pos > 0 ){
  //     std::string kaka = readBuffer.substr(pos+8);
  //     pos = kaka.find("}");
  //     kaka = kaka.substr(0, pos);
  //     int len = kaka.length();
  //     bool startFlag = false;
  //     std::string lala;
  //     char yaya = '"';
  //     for( int i = 0; i < len; i++){
  //       if( startFlag == false && kaka[i] == yaya ) {
  //         startFlag = true;
  //         lala = "";
  //         continue;
  //       }
  //       if( startFlag && kaka[i] == yaya ){
  //         startFlag = false;
  //         databaseList.push_back(lala);
  //         continue;
  //       }
  //       if( startFlag ) lala += kaka[i];
  //     }  
  //   }
  // }

  return respondStr;

}

void InfluxDB::PrintDataBaseList(){
  DebugPrint("%s", "InfluxDB");
  for( size_t i = 0; i < databaseList.size(); i++){
    printf("%2ld| %s\n", i, databaseList[i].c_str());
  }

}

std::string InfluxDB::Query(std::string databaseName, std::string influxQL_query){
  DebugPrint("%s", "InfluxDB");
  if( ! connectionOK ) return "no connection. try TestConnection() again.";
  if( influxVersion == 2 && token.empty() ) return "token no provided, abort.";

  curl_easy_setopt(curl, CURLOPT_POST, 1);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_HEADER, 0);

  curl_easy_setopt(curl, CURLOPT_URL, (databaseIP   + "query?db=" + databaseName).c_str());

  std::string postFields = "q=" + influxQL_query;
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(postFields.length()));
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallBack);
  std::string respondStr;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respondStr);
  
  Execute();
  
  //printf("|%s|\n", readBuffer.c_str());
  
  return respondStr;
}

void InfluxDB::CreateDatabase(std::string databaseName){
  DebugPrint("%s", "InfluxDB");
  if( ! connectionOK ) return ;
  if( influxVersion == 2 && token.empty() ) return;

  curl_easy_setopt(curl, CURLOPT_URL, (databaseIP + "query").c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_HEADER, 0);
  
  std::string postFields = "q=CREATE DATABASE " + databaseName;
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(postFields.length()));
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
  
  Execute();
}

void InfluxDB::AddDataPoint(std::string fullString){
  DebugPrint("%s", "InfluxDB");
  // printf(" InfluxDB::%s |%s| \n", __func__, fullString.c_str());
  dataPoints += fullString + "\n";
}

void InfluxDB::ClearDataPointsBuffer(){
  DebugPrint("%s", "InfluxDB");
  // printf(" InfluxDB::%s \n", __func__);
  dataPoints = "";
}

void InfluxDB::PrintDataPoints(){
  DebugPrint("%s", "InfluxDB");
  // printf(" InfluxDB::%s \n", __func__);
  printf("%s\n", dataPoints.c_str());
}

void InfluxDB::WriteData(std::string databaseName){
  DebugPrint("%s", "InfluxDB");
  if( ! connectionOK ) return ;
  if( influxVersion == 2 && token.empty() ) return;

  // printf(" InfluxDB::%s \n", __func__);
  if( dataPoints.length() == 0 ) return;
  //printf("|%s|\n", (databaseIP + "write?db=" + databaseName).c_str());
  curl_easy_setopt(curl, CURLOPT_URL, (databaseIP + "write?db=" + databaseName).c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_HEADER, 0);

  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(dataPoints.length()));
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, dataPoints.c_str());
  Execute();
}

void InfluxDB::Execute(){
  DebugPrint("%s", "InfluxDB");
  // printf(" InfluxDB::%s \n", __func__);
  try{
    respond = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &respondCode);
    //printf("==== respond %d (OK = %d)\n", respond, CURLE_OK);
    if( respond != CURLE_OK ) printf("############# InfluxDB::Execute fail | %ld\n", respondCode);
  } catch (std::exception& e){ // in case of unexpected error
    printf("%s\n", e.what());
    respond = CURLE_SEND_ERROR;
  }
}

size_t InfluxDB::WriteCallBack(char *contents, size_t size, size_t nmemb, void *userp){
  DebugPrint("%s", "InfluxDB");
  // printf(" InfluxDB::%s \n", __func__);
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}
