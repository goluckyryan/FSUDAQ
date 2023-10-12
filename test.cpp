#include "macro.h"
#include "ClassData.h"
#include "ClassDigitizer.h"

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

  Digitizer * digi = new Digitizer(0, 2, false, true);

  Reg reg("test", 0x1020, RW::ReadWrite, false, 0xFFF, -1);

  digi->WriteRegister(reg, 0x13, -1);

  digi->ReadAllSettingsFromBoard();

  digi->GetSettingFromMemory(DPP::QDC::NumberEventsPerAggregate);


  // digi->Reset();
  // digi->ProgramBoard_PHA();


  for( int ch = 0; ch < 16; ch++){
    printf("%2d | 0x%X \n", ch, digi->GetSettingFromMemory(reg, ch));
  }

  //digi->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::RecordTrace, 1, -1);

  // Data * data = digi->GetData();

  //digi->StartACQ();

  // for( int i = 0; i < 4; i ++ ){
  //   usleep(1000*1000);
  //   digi->ReadData();
  //   data->DecodeBuffer(false, 100);
  //   data->PrintStat();

  //   //data->SaveData();

  //   // int index = data->NumEventsDecoded[0];
  //   // printf("-------------- %ld \n", data->Waveform1[0][index].size());

  //   //data->PrintAllData();

  // }

  // digi->StopACQ();



  digi->CloseDigitizer();
  delete digi;



  /********************** DPP-PHA
  Digitizer * digi = new Digitizer(0, 2, false, true);
  digi->Reset();
  digi->WriteRegister(DPP::SoftwareClear_W, 1);

  digi->WriteRegister(DPP::QDC::RecordLength, 6000/16, -1);
  digi->WriteRegister(DPP::QDC::PreTrigger, 1000/16, -1);
  digi->WriteRegister(DPP::QDC::GateWidth, 200/16, -1);
  digi->WriteRegister(DPP::QDC::GateOffset, 0, -1);
  digi->WriteRegister(DPP::QDC::FixedBaseline, 0, -1);
  //digi->WriteRegister(DPP::QDC::DPPAlgorithmControl, 0x300112); // with test pulse
  digi->WriteRegister(DPP::QDC::DPPAlgorithmControl, 0x300102); // No test pulse
  digi->WriteRegister(DPP::QDC::TriggerHoldOffWidth, 100/16, -1);
  digi->WriteRegister(DPP::QDC::TRGOUTWidth, 100/16, -1);
  //digi->WriteRegister(DPP::QDC::OverThresholdWidth, 100/16, -1);
  //digi->WriteRegister(DPP::QDC::DCOffset, 100/16, -1);
  digi->WriteRegister(DPP::QDC::SubChannelMask, 0xFF, -1);

  digi->WriteRegister(DPP::QDC::TriggerThreshold_sub0, 10, -1);
  digi->WriteRegister(DPP::QDC::TriggerThreshold_sub1, 10, -1);
  digi->WriteRegister(DPP::QDC::TriggerThreshold_sub2, 10, -1);
  digi->WriteRegister(DPP::QDC::TriggerThreshold_sub3, 10, -1);
  digi->WriteRegister(DPP::QDC::TriggerThreshold_sub4, 10, -1);
  digi->WriteRegister(DPP::QDC::TriggerThreshold_sub5, 10, -1);
  digi->WriteRegister(DPP::QDC::TriggerThreshold_sub6, 10, -1);
  digi->WriteRegister(DPP::QDC::TriggerThreshold_sub7, 10, -1);


  digi->WriteRegister(DPP::BoardConfiguration, 0xC0110);
  digi->WriteRegister(DPP::AggregateOrganization, 0x1);
  digi->WriteRegister(DPP::QDC::NumberEventsPerAggregate, 0x4);
  digi->WriteRegister(DPP::AcquisitionControl, 0x0);
  digi->WriteRegister(DPP::GlobalTriggerMask, 0x0);
  digi->WriteRegister(DPP::FrontPanelTRGOUTEnableMask, 0x0);
  digi->WriteRegister(DPP::FrontPanelIOControl, 0x0);
  digi->WriteRegister(DPP::QDC::GroupEnableMask, 0xFF);
  digi->WriteRegister(DPP::MaxAggregatePerBlockTransfer, 0x3FF);


  //digi->WriteRegister(DPP::QDC::DPPAlgorithmControl, 0x300112, 0); // with  pulse for grp 0

  digi->WriteRegister(DPP::BoardID, 0x7);

  // digi->PrintSettingFromMemory();

  Data * data =  digi->GetData();

  data->ClearData();

  data->PrintStat();


  digi->StartACQ();

  for( int i = 0; i < 10; i ++ ){
    usleep(1000*1000);
    digi->ReadData();
    data->DecodeBuffer(false, 0);
    data->PrintStat();

    //data->SaveData();

    // int index = data->NumEventsDecoded[0];
    // printf("-------------- %ld \n", data->Waveform1[0][index].size());

    //data->PrintAllData();

  }

  digi->StopACQ();



  digi->CloseDigitizer();
  delete digi;

 
  /*

  const int nBoard = 1;
  Digitizer **dig = new Digitizer *[nBoard];
  
  for( int i = 0 ; i < nBoard; i++){
    int board = i % 3;
    int port = i/3;
    dig[i] = new Digitizer(board, port, false, true);
  }

  //   dig[i]->StopACQ();
  // dig[0]->WriteRegister(DPP::SoftwareClear_W, 1);

  // dig[0]->ProgramBoard();
  // dig[0]->ProgramBoard_PSD();

  // const float tick2ns = dig[0]->GetTick2ns();

  //dig[2]->ReadRegister(DPP::QDC::RecordLength, 0, 0, "");

  Data * data =  dig[0]->GetData();
  data->ClearData();

  // data->OpenSaveFile("haha");

  // printf("################# DPP Type : %d , %s\n", data->DPPType, data->DPPTypeStr.c_str());

  dig[0]->StartACQ();

  for( int i = 0; i < 9; i ++ ){
    usleep(1000*1000);
    dig[0]->ReadData();
    data->DecodeBuffer(false, 0);
    //data->PrintStat();

    //data->SaveData();

    // int index = data->NumEventsDecoded[0];
    // printf("-------------- %ld \n", data->Waveform1[0][index].size());

    //data->PrintAllData();

  }

  dig[0]->StopACQ();

  */

  

  /*  
  TApplication * app = new TApplication("app", &argc, argv);
  TCanvas * canvas = new TCanvas("c", "haha", 1200, 400);
  canvas->Divide(3, 1);
  
  TH1F * h1 = new TH1F("h1", "count", dig[0]->GetNChannel(), 0, dig[0]->GetNChannel());
  TH1F * h2 = new TH1F("h2", "energy ch-0", 400, 0, 40000);
  
  TGraph * g1 = new TGraph();
  
  canvas->cd(1); h1->Draw("hist");
  canvas->cd(2); h2->Draw();
  canvas->cd(3); g1->Draw("AP");
  
  Data * data =  dig[0]->GetData();
  data->Allocate80MBMemory();
  
  remove("test.bin");
  
  dig[0]->StartACQ();
  
  std::vector<unsigned short> haha ;

  uint32_t PreviousTime = get_time();
  uint32_t CurrentTime = 0;
  uint32_t ElapsedTime = 0;

  int waveFormLength = dig[0]->ReadRegister(Register::DPP::RecordLength_G);

  while(true){
   
    if(keyboardhit()) {
      break;
    }
    
    usleep(1000);
    dig[0]->ReadData();
    
    if( data->nByte > 0 ){
      data->SaveBuffer("test");
      data->DecodeBuffer(0);
      
      unsigned short nData = data->DataIndex[0]; //channel-0
      haha = data->Waveform1[0][nData-1];
      for( int i = 0; i < waveFormLength; i++) g1->SetPoint(i, i*tick2ns, haha[i]);

      canvas->cd(3); g1->Draw("AP");
      
      canvas->Modified();
      canvas->Update(); 
      gSystem->ProcessEvents();
      
    }
    
    CurrentTime = get_time();
    ElapsedTime = CurrentTime - PreviousTime; /// milliseconds

    if( ElapsedTime > 1000 ){
      int temp = system("clear");
      data->PrintStat();
      
      for(int i = 0; i < dig[0]->GetNChannel(); i++){
        h1->Fill(i, data->DataIndex[i]);
      }
      
      for( int i = 0; i < data->DataIndex[0]; i++){
        h2->Fill( data->Energy[0][i]);
      }
      data->ClearData();
      
      canvas->cd(1); h1->Draw("hist");
      canvas->cd(2); h2->Draw();
      canvas->Modified();
      canvas->Update(); 
      gSystem->ProcessEvents();
      
      PreviousTime = CurrentTime;
      
      printf("Press any key to Stop\n");
    }
   
  }
  app->Run();
  */

  /*
  printf("Closing digitizers..............\n");
  for( int i = 0; i < nBoard; i++){
    if(dig[i]->IsConnected()) dig[i]->StopACQ();
    delete dig[i];
  }
  delete [] dig;
  */

  ////##################### Demo for loading and change setting without open a digitizer
  
  /**
  Digitizer * dig = new Digitizer();
  dig->OpenDigitizer(0, 1, false, true);
  dig->LoadSettingBinaryToMemory("expDir/settings/setting_323.bin");

  
  //dig->ProgramBoard_PHA();
  //dig->OpenSettingBinary("setting_323.bin");
  //dig->ReadAllSettingsFromBoard();
  
  //dig->PrintSettingFromMemory();
  //dig->StopACQ();
  
  //dig->WriteRegister(Register::DPP::SoftwareClear_W, 1);
  
  printf("========== %d \n", dig->ReadSettingFromFile(Register::DPP::MaxAggregatePerBlockTransfer));
  
  
  ///dig->SaveSettingAsText("haha.txt");
  ///std::remove("Test_323_139_000.fsu");
  //printf("========== %d \n", dig->ReadRegister(Register::DPP::MaxAggregatePerBlockTransfer));
  
  delete dig;
  
  {///============ Checking the buffer size calculation
    unsigned short B = 10; /// BLT
    unsigned short Eg = 511; /// Event / dual channel
    bool DT = 1; /// dual trace
    bool E2 = 1; /// extra 2;
    bool Wr = 1; /// wave record;
    unsigned short AP2 = 0; /// 00 = input, 01 = Threshold, 10 = Trapezoid - Baseline, 11 = baseline  
    unsigned short AP1 = 1; /// 00 = input, 01 = RC-CR, 10 = RC-CR2, 11 = Trapezoid
    unsigned short DP1 = 0x0000; /// peaking, 
    unsigned short RL = 100; /// record Length
    unsigned short AO = 0x0;
    
    for( int i = 0; i < dig->GetNChannel(); i++){
      dig->WriteRegister(Register::DPP::NumberEventsPerAggregate_G, Eg, i); 
      dig->WriteRegister(Register::DPP::RecordLength_G, RL, i); 
    }
    dig->WriteRegister(Register::DPP::MaxAggregatePerBlockTransfer, B);
    dig->WriteRegister(Register::DPP::AggregateOrganization, AO);
    
    uint32_t bit = 0x0C0115;
    bit += (DT << 11);   
    bit += (AP1 << 12);  
    bit += (AP2 << 14);  
    bit += (Wr << 16);  
    bit += (E2 << 17);  
    bit += (DP1 << 20);
    
    printf("---- Bd Config : 0x%08X \n", bit);
    
    dig->WriteRegister(Register::DPP::BoardConfiguration, bit); 
      
    unsigned int bSize =  dig->CalByteForBuffer();

    int bbbb = (((2 + E2 + Wr*RL*4) * Eg + 2)*8 + 2)*B *4 *2 + 4 * 4; 
    printf("=========== exp Buffer size : %8u byte \n", bbbb);

    usleep(1e6);

    ///using CAEN method 
    char * buffer = NULL;
    uint32_t size;
    CAEN_DGTZ_MallocReadoutBuffer(dig->GetHandle(), (char **)& buffer, &size);
    
    printf("CAEN calculated Buffer Size : %8u byte = %.2f MB \n", size, size/1024./1024.);   
    printf("                       diff : %8u byte \n", size > 2*bSize ? size - 2*bSize : 2*bSize - size);
      
    delete buffer;
  }
  */
  
  //dig->GetData()->SetSaveWaveToMemory(true);
  
  //dig->StartACQ();
  //
  //for( int i = 0; i < 60; i++){    
  //  usleep(500*1000);
  //  dig->ReadData();
  //  printf("------------------- %d\n", i);
  //  unsigned long time1 = get_time();
  //  dig->GetData()->DecodeBuffer(false,0);
  //  unsigned long time2 = get_time();
  //  printf("********************* decode time : %lu usec\n", time2-time1);
  //  dig->GetData()->PrintStat();
  //  //dig->GetData()->SaveBuffer("Test");
  //}
  //
  //dig->StopACQ();
  

  
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
