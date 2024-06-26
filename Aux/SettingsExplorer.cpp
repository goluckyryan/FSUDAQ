#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <thread>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <vector>
#include <bitset>
#include <unistd.h>
#include <limits.h>
#include <ctime>
#include <sys/time.h> /* struct timeval, select() */
#include <termios.h> /* tcgetattr(), tcsetattr() */

#include "../RegisterAddress.h"
#include "../ClassDigitizer.h"

static struct termios g_old_kbd_mode;
static void cooked(void);  ///set keyboard behaviour as wait-for-enter
static void uncooked(void);  ///set keyboard behaviour as immediate repsond
static void raw(void);
int getch(void);
bool isNumeric(const std::string& str) ;
int keyboardhit();

Digitizer * digi = nullptr;

std::vector<Reg> RegList;

bool  QuitFlag = false;

void PrintCommands(){
  if (QuitFlag) return;
  printf("\n");
  printf("\e[96m=============  Command List  ===================\e[0m\n");
  printf("q ) Quit \n");         
  printf("b ) Print Board Settings\n");
  printf("c ) Print Channel Settings\n");
  printf("x ) Export binary to text file\n");
  printf("* invalid input = back to upper level \n");
}


void keyPressCommand(){
  
  char c = getch();
  if (c == 'q') { //========== quit
    QuitFlag = true;
  }
  if (c == 'b') { //========== 

    if( digi->GetDPPType() == DPPTypeCode::DPP_PHA_CODE || digi->GetDPPType() == DPPTypeCode::DPP_PSD_CODE ){
      RegList = RegisterBoardList_PHAPSD;
    }else if(digi->GetDPPType() == DPPTypeCode::DPP_QDC_CODE) {
      RegList = RegisterBoardList_QDC;
    }

    for( int i = 0; i < (int) RegList.size(); i++){
      std::string typeStr ;
      if( RegList[i].GetRWType() == RW::ReadWrite ) typeStr = "R/W";
      if( RegList[i].GetRWType() == RW::ReadONLY  ) typeStr = "R  ";
      if( RegList[i].GetRWType() == RW::WriteONLY ) typeStr = "  W";
      
      unsigned int value = digi->GetSettingFromMemory(RegList[i], 0);

      printf("%2d | 0x%04X %30s  %s 0x%08X = %10u\n", i, 
                                               RegList[i].GetAddress(), 
                                               RegList[i].GetNameChar(), 
                                               typeStr.c_str(),
                                               value, 
                                               value);
    }

    std::string input = "-1";
    cooked();
    do{
      std::cout << "Enter Setting ID to change the setting : ";
      std::getline(std::cin, input);

      if( !isNumeric(input) ) break;

      int ID = atoi(input.c_str());
      if( 0 <= ID && ID < (int) RegList.size() ){

        printf("\e[34mSelected %s = 0x%08X = %u \e[0m\n", RegList[ID].GetNameChar(), 
                                               digi->GetSettingFromMemory(RegList[ID], 0), 
                                               digi->GetSettingFromMemory(RegList[ID], 0));

        if( RegList[ID].GetRWType() == RW::ReadONLY ) {
          printf("This register is READ-ONLY.\n");
        }else if (RegList[ID].GetRWType() == RW::WriteONLY){
          printf("This register is WRITE-ONLY, which is a command. no need to set value.\n");
        }else{
          std::cout << "What value ? ";
          std::getline(std::cin, input);
          if( isNumeric(input) ){
            digi->SetSettingToMemory(RegList[ID], atoi(input.c_str()), 0); 
            digi->SaveSettingToFile(RegList[ID], atoi(input.c_str()), 0);
            printf("\e[31mNow  %s = 0x%08X = %u \e[0m\n", RegList[ID].GetNameChar(), 
                                              digi->GetSettingFromMemory(RegList[ID], 0), 
                                              digi->GetSettingFromMemory(RegList[ID], 0));
          }else{
            printf("Entered non numerical.\n");
            input = "-1";
          }
        }
      }

    }while( isNumeric( input) );
    uncooked(); 
  }

  if( c == 'c' ){
    cooked();
    std::string input = "-1";
    std::cout << "Enter channel number (# of ch = " << digi->GetNumRegChannels() << ") : ";
    std::getline(std::cin, input);

    if( !isNumeric(input) ) {
      uncooked();
      return;
    };

    int ch = atoi(input.c_str());

    if( ch < 0 || ch >= digi->GetNumRegChannels() ){
      printf("Input channel number = %d, outrange of the supported channel\n", ch);
      uncooked();
      return;
    }

    if( digi->GetDPPType() == DPPTypeCode::DPP_PHA_CODE ) RegList = RegisterChannelList_PHA;
    if( digi->GetDPPType() == DPPTypeCode::DPP_PSD_CODE ) RegList = RegisterChannelList_PSD;
    if( digi->GetDPPType() == DPPTypeCode::DPP_QDC_CODE ) RegList = RegisterChannelList_QDC;

    for( int i = 0; i < (int) RegList.size(); i++){
      std::string typeStr ;
      if( RegList[i].GetRWType() == RW::ReadWrite ) typeStr = "R/W";
      if( RegList[i].GetRWType() == RW::ReadONLY  ) typeStr = "R  ";
      if( RegList[i].GetRWType() == RW::WriteONLY ) typeStr = "  W";
      
      RegList[i].ActualAddress(ch);

      unsigned int value = digi->GetSettingFromMemory(RegList[i], ch);

      printf("%2d | 0x%04X %30s  %s 0x%08X = %10u : %d\n", i, 
                                               RegList[i].GetAddress(), 
                                               RegList[i].GetNameChar(), 
                                               typeStr.c_str(),
                                               value, 
                                               value,
                                               value * abs(RegList[i].GetPartialStep()));
    }

    do{
      std::cout << "Enter Setting ID to change the setting : ";
      std::getline(std::cin, input);

      if( !isNumeric(input) ) break;

      int ID = atoi(input.c_str());
      if( 0 <= ID && ID < (int) RegList.size() ){

        printf("\e[34mID=%d | ch-%d | Selected %s = 0x%08X = %u \e[0m\n", ID, ch, RegList[ID].GetNameChar(), 
                                               digi->GetSettingFromMemory(RegList[ID], ch), 
                                               digi->GetSettingFromMemory(RegList[ID], ch));

        if( RegList[ID].GetRWType() == RW::ReadONLY ) {
          printf("This register is READ-ONLY.\n");
        }else if (RegList[ID].GetRWType() == RW::WriteONLY){
          printf("This register is WRITE-ONLY, which is a command. no need to set value.\n");
        }else{
          std::cout << "What value (negative will change all channels) ? ";
          std::getline(std::cin, input);
          if( isNumeric(input) ){

            int value = atoi(input.c_str());

            printf(" input : %s  %d\n", input.c_str(), value);
            if( value < 0 ){
              value = abs(value);
              for( int i = 0; i < digi->GetNumRegChannels(); i++){
                digi->SetSettingToMemory(RegList[ID], value, i); 
                digi->SaveSettingToFile(RegList[ID], value, i);
                printf("\e[31mNow  ch-%2d %s = 0x%08X = %u \e[0m\n", i, RegList[ID].GetNameChar(), 
                                                  digi->GetSettingFromMemory(RegList[ID], i), 
                                                  digi->GetSettingFromMemory(RegList[ID], i));
              }
            }else{


              digi->SetSettingToMemory(RegList[ID], value, ch); 
              digi->SaveSettingToFile(RegList[ID], value, ch);
              printf("\e[31mNow ch-%2d %s = 0x%08X = %u \e[0m\n", ch, RegList[ID].GetNameChar(), 
                                                digi->GetSettingFromMemory(RegList[ID], ch), 
                                                digi->GetSettingFromMemory(RegList[ID], ch));

              if( RegList[ID].IsCoupled() ){
                int cpCh = (ch%2 == 0 ? ch + 1 : ch - 1);
                digi->SetSettingToMemory(RegList[ID], value, cpCh); 
                digi->SaveSettingToFile(RegList[ID], value, cpCh);
                printf("\e[31mNow ch-%2d %s = 0x%08X = %u \e[0m\n", cpCh, RegList[ID].GetNameChar(), 
                                                  digi->GetSettingFromMemory(RegList[ID], cpCh), 
                                                  digi->GetSettingFromMemory(RegList[ID], cpCh));
              }

            }
          }else{
            printf("Entered non numerical.\n");
            input = "-1";
          }
        }
      }

    }while( isNumeric( input) );


    uncooked();
  }

  if (c == 'x') { //========== 
    cooked(); ///set keyboard need enter to responds

    std::string input = "haha.txt";
    std::cout << "Eneter file name : ";
    std::getline(std::cin, input);
    digi->SaveAllSettingsAsText(input);
    uncooked();
  }
  
}


//################################################
int main(int argc, char **argv) {
  
  printf("=========================================\n");
  printf("===      Setting Binary Explorer      ===\n");
  printf("=========================================\n");  
  if (argc != 2)    {
    printf("Incorrect number of arguments:\n");
    printf("%s *bin \n", argv[0]);
    return 1;
  }

  digi = new Digitizer();
  digi->LoadSettingBinaryToMemory(argv[1]);

  if( !(digi->GetDPPType() == DPPTypeCode::DPP_PHA_CODE ||
        digi->GetDPPType() == DPPTypeCode::DPP_PSD_CODE ||
        digi->GetDPPType() == DPPTypeCode::DPP_QDC_CODE )){
    printf("DPP-type not supported. Or Binary file is not supported.\n");
    delete digi;
    return -1;
  }

  //digi->PrintSettingFromMemory();

  PrintCommands();
  
  do{

    if(keyboardhit()) {
      
      keyPressCommand();
      
      PrintCommands();
    }

  }while(!QuitFlag);

  delete digi;
  return 0;
  
}


//################################################

bool isNumeric(const std::string& str) {
    try {
        size_t pos = 0;
        std::stoi(str, &pos);
        return pos == str.size();  // Check if the entire string was used in conversion
    } catch (...) {
        return false;
    }
}

static void cooked(void){
  tcsetattr(0, TCSANOW, &g_old_kbd_mode);
}

static void uncooked(void){
  struct termios new_kbd_mode;
  /* put keyboard (stdin, actually) in raw, unbuffered mode */
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
  /* put keyboard (stdin, actually) in raw, unbuffered mode */
  uncooked();
  /* when we exit, go back to normal, "cooked" mode */
  atexit(cooked);

  init = 1;
}

int getch(void){
  unsigned char temp;
  raw();
  /* stdin = fd 0 */
  if(read(0, &temp, 1) != 1) return 0;
  //printf("%s", &temp);
  return temp;
}

int keyboardhit(){

  struct timeval timeout;
  fd_set read_handles;
  int status;

  raw();
  /* check stdin (fd 0) for activity */
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