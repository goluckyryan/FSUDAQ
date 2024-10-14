#include "../macro.h"
#include "../ClassData.h"
#include "../ClassDigitizer.h"
#include "../MultiBuilder.h"
#include "../ClassInfluxDB.h"
#include "ClassDigitizerAPI.h"

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

#include <CAENVMElib.h>

#include <time.h>

static struct termios g_old_kbd_mode;

static void cooked(void);
static void uncooked(void);
static void raw(void);
int keyboardhit();
int getch(void);

#include <curl/curl.h>

size_t WriteCallBack(char *contents, size_t size, size_t nmemb, void *userp){
  // printf(" InfluxDB::%s \n", __func__);
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}

void testInflux(){
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

}


void CheckBufferSize(int MaxAggPreRead, int EvtPreAgg){


  //Buffer depends on 

  Digitizer * digi = new Digitizer(0, 26006, false, true);
  digi->Reset();
  digi->ProgramBoard();

  digi->WriteRegister(DPP::SoftwareClear_W, 1);

  digi->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::RecordTrace, 0, -1);
  digi->WriteRegister(DPP::RecordLength_G, 10, -1);

  digi->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::EnableExtra2, 1, -1);

  digi->WriteRegister(DPP::MaxAggregatePerBlockTransfer, MaxAggPreRead);
  digi->WriteRegister(DPP::NumberEventsPerAggregate_G, EvtPreAgg);

  unsigned int bufferSize = digi->CalByteForBuffer(true);
  unsigned int bufferSizeCAEN = digi->CalByteForBufferCAEN();

  printf("Manual Buffer Size : %u Byte = %u words\n", bufferSize, bufferSize/4);
  printf("  CAEN Buffer Size : %u Byte = %u words\n", bufferSizeCAEN, bufferSizeCAEN/4);

  unsigned int haha  = bufferSize*2  + 16 *( 1-  MaxAggPreRead );
  printf("----                 %u        %u \n", haha, haha/4);


  delete digi;

}

void GetOneAgg(){

  Digitizer * digi = new Digitizer(0, 26006, false, true);

  if( digi->IsConnected() ){
    digi->Reset();
    digi->ProgramBoard();

    digi->WriteRegister(DPP::SoftwareClear_W, 1);

    digi->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::RecordTrace, 0, -1);
    digi->WriteRegister(DPP::RecordLength_G, 10, -1);

    digi->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::EnableExtra2, 1, -1);

    digi->WriteRegister(DPP::MaxAggregatePerBlockTransfer, 1);
    digi->WriteRegister(DPP::NumberEventsPerAggregate_G, 2);

    unsigned int bufferSize = digi->CalByteForBuffer(true);
    unsigned int bufferSizeCAEN = digi->CalByteForBufferCAEN();

    printf("Manual Buffer Size : %u Byte = %u words\n", bufferSize, bufferSize/4);
    printf("  CAEN Buffer Size : %u Byte = %u words\n", bufferSizeCAEN, bufferSizeCAEN/4);


    digi->StartACQ();

    usleep(5000*1000); // wait 1sec 


    digi->ReadData();
    digi->GetData()->DecodeBuffer(false, 4);

    digi->StopACQ();
  }

  delete digi;

}

void TestVME(){

  int ret = -1;
  char SWRel[100];
  ret = CAENVME_SWRelease(SWRel);
  printf("ret = %d | Software release : %s\n", ret, SWRel);

  short ConnetNode = 0;
  uint32_t link = 0;
  int32_t handle;

  ret = CAENVME_Init2(cvPCIE_A5818_V3718, &link, ConnetNode, &handle);  
  printf("ret = %d \n", ret);

  // ret = CAENVME_DeviceReset(handle); // only for A2818, A2719, and V2718
  // printf("ret = %d \n", ret);

  char FWRel[100];
  ret = CAENVME_BoardFWRelease(handle, FWRel);
  printf("ret = %d | Firmware release : %s\n", ret, FWRel);

  char DrRel[100];
  ret = CAENVME_DriverRelease(handle, DrRel);
  printf("ret = %d | Driver release : %s\n", ret, DrRel);

  CVVMETimeouts timeoutValue = CVVMETimeouts::cvTimeout50us;
  ret = CAENVME_SetTimeout(handle, timeoutValue); 
  printf("ret = %d \n", ret);

  ret = CAENVME_GetTimeout(handle, &timeoutValue);
  printf("ret = %d | timeout : %d\n", ret, timeoutValue);


  // ret = CAENVME_ReadRegister(


  ret = CAENVME_End(handle);
  printf("ret = %d \n", ret);

}

int TestDigitizerRaw(){

  int handle;
  int ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_OpticalLink, 0, 0, 0, &handle);
  if( ret != 0 ) { 
    printf("==== open digitizer fail.\n");
    printf("=========== close Digitizer \n");
    CAEN_DGTZ_SWStopAcquisition(handle);
    CAEN_DGTZ_CloseDigitizer(handle);
    return 0;
  }
  
  CAEN_DGTZ_BoardInfo_t BoardInfo;
  ret = (int) CAEN_DGTZ_GetInfo(handle, &BoardInfo);

  printf("Connected to Model %s with handle %d\n", BoardInfo.ModelName, handle);
  printf("          Family Code : %d \n", BoardInfo.FamilyCode);
  printf("No. of Input Channels : %d \n", BoardInfo.Channels);
  printf("         SerialNumber : %d \n", BoardInfo.SerialNumber);
  printf("              ADC bit : %d \n", BoardInfo.ADC_NBits);
  printf("     ROC FPGA Release : %s \n", BoardInfo.ROC_FirmwareRel);
  printf("     AMC FPGA Release : %s \n", BoardInfo.AMC_FirmwareRel);

  timespec ta, tb; 
  long long duration;
  uint32_t value;
  
  clock_gettime(CLOCK_REALTIME, &ta);
  ret = CAEN_DGTZ_WriteRegister(handle, 0x8034, 3);
  clock_gettime(CLOCK_REALTIME, &tb);
  printf("ret = %d \n", ret);
  duration = tb.tv_nsec - ta.tv_nsec;
  printf("duration = %lld ns\n", duration);

  clock_gettime(CLOCK_REALTIME, &ta);
  ret = CAEN_DGTZ_ReadRegister(handle, 0x1034, &value);
  clock_gettime(CLOCK_REALTIME, &tb);
  printf("ret = %d \n", ret);
  duration = tb.tv_nsec - ta.tv_nsec;
  printf("duration = %lld ns | value = %u\n", duration, value);


  printf("=========== close Digitizer \n");
  CAEN_DGTZ_SWStopAcquisition(handle);
  CAEN_DGTZ_CloseDigitizer(handle);
  return 0;

}


void SimpleDAQ(){

  std::unique_ptr<Digitizer> digi = std::make_unique<Digitizer>(0, 49093, false, true);

  digi->ProgramBoard();
  digi->SetBits(DPP::QDC::DPPAlgorithmControl, DPP::QDC::Bit_DPPAlgorithmControl::Polarity, 0, -1);
  digi->WriteRegister(DPP::QDC::NumberEventsPerAggregate, 5);
  digi->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::RecordTrace, 1, -1); // enable trace recording
  digi->WriteRegister(DPP::MaxAggregatePerBlockTransfer, 10);

  Data * data = digi->GetData();
  data->OpenSaveFile("haha2");

  digi->StartACQ();

  for( int i = 0; i < 10 ; i++ ){

    usleep(500*1000);

    digi->ReadData();
    data->DecodeBuffer(true, 0);
    data->SaveData(2);

    data->PrintStat();

  }

  digi->StopACQ();

}


void Compare_CAEN_Decoder(){

  std::unique_ptr<Digitizer> digi = std::make_unique<Digitizer>(0, 49093, false, true);
  Data * data =  digi->GetData();

  int ret;
  int handle = digi->GetHandle();  
  CAEN_DGTZ_DPP_PSD_Event_t  *Events[16];  /// events buffer
  uint32_t NumEvents[16];

  uint32_t AllocatedSize = 0;

  ret |= CAEN_DGTZ_MallocDPPEvents(handle, reinterpret_cast<void**>(&Events), &AllocatedSize) ;
  printf("allowcated %d byte for Events\n", AllocatedSize);


  printf("======================== start ACQ \n");
  digi->StartACQ();

  int ch = 0;
  for( int i = 0; i < 5; i ++ ){
    usleep(1000*1000); // every 1 second

    digi->ReadData();
    // data->CopyBuffer(cpBuffer, bufferSize);
    data->DecodeBuffer(false, 4);

    if( data->nByte > 0 ){
      ret = (CAEN_DGTZ_ErrorCode) CAEN_DGTZ_GetDPPEvents(handle, data->buffer, data->nByte, reinterpret_cast<void**>(&Events), NumEvents);
      if (ret) {
        printf("Error when getting events from data %d\n", ret);
        continue;
      }

      printf("============ %u\n", NumEvents[0]);

      for( int ev = 0; ev < NumEvents[0]; ev++ ){

        printf("-------- ev %d\n", ev);
        printf( " Format : 0x%04x\n", Events[ch][ev].Format);
        printf( "TimeTag : 0x%08x\n", Events[ch][ev].TimeTag);
        printf(" E_short : 0x%04x\n", Events[ch][ev].ChargeShort);
        printf("  E_long : 0x%04x\n", (Events[ch][ev].ChargeLong & 0xffff));
        printf("Baseline : 0x%04x\n", (Events[ch][ev].Baseline & 0xffff));
        printf("     Pur : 0x%04x\n", Events[ch][ev].Pur);
        printf("   Extra : 0x%08x\n", Events[ch][ev].Extras);
          

      }
    }

  }
  
  digi->StopACQ();

  printf("======================== ACQ Stopped.\n");

}

//^======================================
int main(int argc, char* argv[]){

  // Compare_CAEN_Decoder();

  // Data * data =  digi->GetData();

  SimpleDAQ();

  // MultiBuilder * builder = new MultiBuilder(data, DPPType::DPP_PHA_CODE, digi->GetSerialNumber());
  // builder->SetTimeWindow(100);


  // std::unique_ptr<DigitizerAPI> digi = std::make_unique<DigitizerAPI>(0, 49093, false, true);




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
