#include "../macro.h"
#include "../ClassData.h"
#include "../ClassDigitizer.h"
#include "../MultiBuilder.h"

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

static struct termios g_old_kbd_mode;

static void cooked(void);
static void uncooked(void);
static void raw(void);
int keyboardhit();
int getch(void);

//^======================================
int main(int argc, char* argv[]){

  Digitizer * digi = new Digitizer(0, 26006, false, true);
  digi->Reset();

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

  digi->CloseDigitizer();
  delete digi;
  
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
