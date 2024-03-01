#include "../macro.h"
#include "../ClassData.h"
#include "../ClassDigitizer.h"
#include "../MultiBuilder.h"
#include "../ClassInfluxDB.h"

#include <TROOT.h>
#include <TSystem.h>
#include <TApplication.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>
#include <TFile.h>
#include <TTree.h>

#include <sys/time.h> /** struct timeval, select() */
#include <termios.h>  /** tcgetattr(), tcsetattr() */
#include <vector>
#include <regex>

static struct termios g_old_kbd_mode;

static void cooked(void);
static void uncooked(void);
static void raw(void);
int keyboardhit();
int getch(void);

// #include <curl/curl.h>

size_t WriteCallBack(char *contents, size_t size, size_t nmemb, void *userp){
  // printf(" InfluxDB::%s \n", __func__);
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}

//^======================================
int main(int argc, char* argv[]){

  InfluxDB * influx = new InfluxDB();
  
  influx->SetURL("https://fsunuc.physics.fsu.edu/influx/");
  //influx->SetURL("http://128.186.111.5:8086/");

  influx->SetToken("wS-Oy17bU99qH0cTPJ-Q5tbiOWfaKyoASUx7WwmdM7KG8EJ1BpRowYkqpnPw8oeatnDaZfZtwIFT0kv_aIOAxQ==");

  influx->TestingConnection();

  influx->CheckDatabases();
  influx->PrintDataBaseList();

  // printf("=-------------------------\n");
  // influx->TestingConnection(true);

  printf("%s \n", influx->Query("testing", "show measurements").c_str());
  
  // printf("%s \n", influx->Query("testing", "SELECT * from haha ORDER by time DESC LIMIT 5").c_str());
  


  delete influx;


  // CURL *curl = curl_easy_init();  
  // CURLcode res;

  // struct curl_slist * headers = nullptr;

  // headers = curl_slist_append(headers, "Authorization: Token wS-Oy17bU99qH0cTPJ-Q5tbiOWfaKyoASUx7WwmdM7KG8EJ1BpRowYkqpnPw8oeatnDaZfZtwIFT0kv_aIOAxQ==");
  // // // headers = curl_slist_append(headers, "Content-Type: text/plain; charset=utf-8");
  // headers = curl_slist_append(headers, "Accept: application/csv");

  
  // printf("%s\n",headers->data);
  // printf("%s\n",  headers->next->data);


  // curl_slist_free_all(headers);

  // // printf("%p \n",headers);

  // headers = curl_slist_append(headers, "Accept: application/csv");

  // printf("%s\n",headers->data);
  

  // curl_easy_setopt(curl, CURLOPT_POST, 1);
  // curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  // std::string databaseIP = "https://fsunuc.physics.fsu.edu/influx/";
  // std::string databaseIP = "http://128.186.111.5:8086/";


  //*===================== Check version

  // curl_easy_setopt(curl, CURLOPT_URL, (databaseIP   + "ping").c_str());
  // curl_easy_setopt(curl, CURLOPT_HEADER, 1);

  //*===================== Query data 
  
  //=============== query databases
  // curl_easy_setopt(curl, CURLOPT_URL, (databaseIP   + "query").c_str());
  // std::string postFields="q=Show databases";

  //=============== query measurement  
  // curl_easy_setopt(curl, CURLOPT_URL, (databaseIP   + "query?db=testing").c_str());
  // std::string postFields="q=SELECT * FROM \"haha\"";

  //=============== write measurement
  // curl_easy_setopt(curl, CURLOPT_URL, (databaseIP   + "write?db=testing").c_str());
  // std::string postFields = "haha,BD=1 state=2.345";
  // postFields += "\n";
  // postFields += "haha,BD=2 state=9.876";
  // postFields += "\n";
  
  // curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(postFields.length()));
  // curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());


  // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallBack);
  // std::string readBuffer;
  // curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
  
  // // //curl_easy_setopt(curl, CURLOPT_URL, "https://fsunuc.physics.fsu.edu/influx/api/v2/write?org=FSUFoxLab&bucket=testing");

  // res = curl_easy_perform(curl);
  // long respondCode;
  // curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &respondCode);

  // printf("respond code : %ld \n", respondCode);
  // if( res == CURLE_OK ) {
  //   printf("================respond \n%s\n", readBuffer.c_str());
  // }else{
  //   printf("=========== curl_easy_perform fail.\n");
  // }

  // curl_slist_free_all(headers);
  // curl_easy_cleanup(curl);


  // std::regex pattern(R"(X-Influxdb-Version: (.*))");
  // std::smatch match;

  // if (regex_search(readBuffer, match, pattern)) {
  //   // Extract and print the version
  //   std::string version = match[1];

  //   unsigned short vno = -1;
  //   size_t dotPosition = version.find('.');
  //   if( dotPosition != std::string::npos){
  //     vno = atoi(version.substr(dotPosition-1, 1).c_str());
  //   }
  //   printf("%s | %d\n", version.c_str(), vno);  

  // }

  //============================================= end of influxDB example

  // Digitizer * digi = new Digitizer(0, 26006, false, true);
  // digi->Reset();

  //digi->ProgramBoard_PHA();

  //digi->WriteRegister(DPP::SoftwareClear_W, 1);

  // digi->WriteRegister(DPP::QDC::RecordLength, 31, -1); // T = N * 8 * 16
  // digi->WriteRegister(DPP::QDC::PreTrigger, 60, -1);

  // digi->WriteRegister(DPP::QDC::TriggerThreshold_sub2, 17, -1);
  // digi->SetBits(DPP::QDC::DPPAlgorithmControl, DPP::QDC::Bit_DPPAlgorithmControl::ChargeSensitivity, 0, -1);
  // digi->SetBits(DPP::QDC::DPPAlgorithmControl, DPP::QDC::Bit_DPPAlgorithmControl::InputSmoothingFactor, 4, -1);
  // digi->SetBits(DPP::QDC::DPPAlgorithmControl, DPP::QDC::Bit_DPPAlgorithmControl::BaselineAvg, 2, -1);

  // digi->WriteRegister(DPP::QDC::GateWidth, 608/16, -1);

  // digi->WriteRegister(DPP::QDC::GroupEnableMask, 0x01);

  // digi->WriteRegister(DPP::QDC::NumberEventsPerAggregate, 10, -1);
  // digi->WriteRegister(DPP::AggregateOrganization, 0, -1);
  // digi->WriteRegister(DPP::MaxAggregatePerBlockTransfer, 100, -1);

  // digi->SetBits(DPP::QDC::DPPAlgorithmControl, DPP::QDC::Bit_DPPAlgorithmControl::Polarity, 0, -1);

  /*
  digi->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::EnableExtra2, 1, -1);
  digi->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::RecordTrace, 0, -1);  

  Data * data =  digi->GetData();

  MultiBuilder * builder = new MultiBuilder(data, DPPType::DPP_PHA_CODE, digi->GetSerialNumber());
  builder->SetTimeWindow(100);

  //remove("haha_*.fsu");
  //data->OpenSaveFile("haha");

  digi->StartACQ();

  for( int i = 0; i < 5; i ++ ){
    usleep(1000*1000);
    digi->ReadData();
    data->DecodeBuffer(true, 0);
    //data->DecodeBuffer(false, 2);
    //data->SaveData();
    //data->PrintStat();
    
    data->PrintAllData(true);

    //builder->BuildEvents(false, true, true);
    builder->BuildEventsBackWard(20, true);

    builder->PrintStat();
    // int index = data->NumEventsDecoded[0];
    // printf("-------------- %ld \n", data->Waveform1[0][index].size());

  }
  digi->StopACQ();

  //data->CloseSaveFile();
  builder->BuildEvents(true, true, true);

  data->PrintAllData();

  builder->PrintAllEvent(); // TODO
  */

  // digi->CloseDigitizer();
  // delete digi;
  
  return 0;
}

//*********************************
//*********************************

static void cooked(void){
  tcsetattr(0, TCSANOW, &g_old_kbd_mode);
}

static void uncooked(void){
  struct termios new_kbd_mode;
  /** put keyboard (stdin, actually) in raw, unbuffered mode */
  tcgetattr(0, &g_old_kbd_mode);
  memcpy(&new_kbd_mode, &g_old_kbd_mode, sizeof(struct termios));
  new_kbd_mode.c_lflag &= ~(ICANON | ECHO);
  new_kbd_mode.c_cc[VTIME] = 0;
  new_kbd_mode.c_cc[VMIN] = 1;
  tcsetattr(0, TCSANOW, &new_kbd_mode);
}

static void raw(void){

  static char init;
  if(init) return;
  /** put keyboard (stdin, actually) in raw, unbuffered mode */
  uncooked();
  /** when we exit, go back to normal, "cooked" mode */
  atexit(cooked);

  init = 1;
}

int keyboardhit(){

  struct timeval timeout;
  fd_set read_handles;
  int status;
  
  raw();
  /** check stdin (fd 0) for activity */
  FD_ZERO(&read_handles);
  FD_SET(0, &read_handles);
  timeout.tv_sec = timeout.tv_usec = 0;
  status = select(0 + 1, &read_handles, NULL, NULL, &timeout);
  if(status < 0){
    printf("select() failed in keyboardhit()\n");
    exit(1);
  }
  return (status);
}

int getch(void){
  unsigned char temp;
  raw();
  /** stdin = fd 0 */
  if(read(0, &temp, 1) != 1) return 0;
  return temp;
}
