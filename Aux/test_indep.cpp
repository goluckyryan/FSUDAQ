#include <stdio.h>
#include <string>
#include <sstream>
#include <cmath>
#include <cstring>  ///memset
#include <iostream> ///cout
#include <bitset>

#include "CAENDigitizer.h"
#include "CAENDigitizerType.h"
#include "../macro.h"
#include "../RegisterAddress.h"

using namespace std;

void PrintChannelSettingFromDigitizer(int handle, int ch, float tick2ns){
  
  printf("\e[33m================================================\n");
  printf("================ Setting for channel %d \n", ch);
  printf("================================================\e[0m\n");
  ///DPP algorithm Control
  uint32_t * value = new uint32_t[16];
  CAEN_DGTZ_ReadRegister(handle, DPP::DPPAlgorithmControl + (ch << 8), value);
  printf("                          32  28  24  20  16  12   8   4   0\n");
  printf("                           |   |   |   |   |   |   |   |   |\n");
  cout <<" DPP algorithm Control  : 0b" << bitset<32>(value[0]);
  printf(" = 0x%x\n", value[0]);

  int trapRescaling = int(value[0]) & 0x1f ;
  int polarity      = int(value[0] >> 16) & 0x1;  /// in bit[16]
  int baseline      = int(value[0] >> 20) & 0x7;  /// in bit[22:20]
  int NsPeak        = int(value[0] >> 12) & 0x3;  /// in bit[13:12]
  int rollOver      = int(value[0] >> 26) & 0x1;
  int pileUp        = int(value[0] >> 27) & 0x1;
  
  ///DPP algorithm Control 2
  CAEN_DGTZ_ReadRegister(handle, DPP::PHA::DPPAlgorithmControl2_G + (ch << 8), value);
  cout <<" DPP algorithm Control 2: 0b" << bitset<32>(value[0]) ;
  printf(" = 0x%x\n", value[0]);
  
  int extras2WordOption = int(value[0] >> 8) & 0x3;
  string extra2WordOptStr = "";
  switch (extras2WordOption){
    case 0 : extra2WordOptStr = "[0:15] Baseline *4 [16:31] Extended Time Stamp"; break;
    case 2 : extra2WordOptStr = "[0:9] Fine Time Stamp [10:15] Reserved [16:31] Extended Time Stamp"; break;
    case 4 : extra2WordOptStr = "[0:15] Total Trigger Counter [16:31] Lost Trigger Counter"; break;
    case 5 : extra2WordOptStr = "[0:15] Event After the Zero Crossing [16:31] Event Before the Zero Crossing"; break;
    default: extra2WordOptStr = "Reserved"; break;
  }

  printf(" tick2ns : %.0f ns\n", tick2ns);
  
  printf("==========----- input \n");
  CAEN_DGTZ_ReadRegister(handle, DPP::RecordLength_G + (ch << 8), value);    printf("%24s  %5d samples = %5.0f ns \n",   "Record Length",  ((value[0] * 8) & MaxRecordLength), ((value[0] * 8) & MaxRecordLength) * tick2ns); ///Record length
  CAEN_DGTZ_ReadRegister(handle, DPP::PreTrigger + (ch << 8), value);        printf("%24s  %5d samples = %5.0f ns \n",   "Pre-tigger",  value[0] * 4, value[0] * 4 * tick2ns);    ///Pre-trigger
                                                                                       printf("%24s  %5.0f samples, DPP-[20:22]\n", "baseline mean",  pow(4, 1 + baseline)); ///Ns baseline
  CAEN_DGTZ_ReadRegister(handle, DPP::ChannelDCOffset + (ch << 8), value);   printf("%24s  %.2f %% \n", "DC offset",  100.0 - value[0] * 100./ 0xFFFF); ///DC offset
  CAEN_DGTZ_ReadRegister(handle, DPP::InputDynamicRange + (ch << 8), value); printf("%24s  %.1f Vpp \n",     "input Dynamic",  value[0] == 0 ? 2 : 0.5); ///InputDynamic
                                                                                       printf("%24s  %s, DPP-[16]\n",           "polarity",  polarity ==  0 ? "Positive" : "negative"); ///Polarity

  printf("==========----- discriminator \n");
  CAEN_DGTZ_ReadRegister(handle, DPP::PHA::TriggerThreshold + (ch << 8), value);     printf("%24s  %4d LSB\n",     "Threshold",  value[0]); ///Threshold
  CAEN_DGTZ_ReadRegister(handle, DPP::PHA::TriggerHoldOffWidth + (ch << 8), value);  printf("%24s  %4d samples, %5.0f ns \n",   "trigger hold off", value[0],  value[0] * 4 * tick2ns); ///Trigger Hold off
  CAEN_DGTZ_ReadRegister(handle, DPP::PHA::RCCR2SmoothingFactor + (ch << 8), value); printf("%24s  %4d samples, %5.0f ns \n", "Fast Dis. smoothing",  (value[0] & 0x1f) * 2, (value[0] & 0x1f) * 2 * tick2ns ); ///Fast Discriminator smoothing
  CAEN_DGTZ_ReadRegister(handle, DPP::PHA::ShapedTriggerWidth + (ch << 8), value);   printf("%24s  %4d samples, %5.0f ns \n",   "Fast Dis. output width", value[0], value[0] * 4 * tick2ns); ///Fast Dis. output width
  CAEN_DGTZ_ReadRegister(handle, DPP::PHA::InputRiseTime + (ch << 8), value);        printf("%24s  %4d samples, %5.0f ns \n",   "Input rise time ", value[0],  value[0] * 4 * tick2ns); ///Input rise time

  printf("==========----- Trapezoid \n");
  CAEN_DGTZ_ReadRegister(handle, DPP::PHA::TrapezoidRiseTime + (ch << 8), value); printf("%24s  %4d samples, %5.0f ns \n", "Trap. rise time", value[0], value[0] * 4 * tick2ns); ///Trap. rise time, 2 for 1 ch to 2ns
  int riseTime = value[0] * 4 * tick2ns;
  CAEN_DGTZ_ReadRegister(handle, DPP::PHA::TrapezoidFlatTop + (ch << 8), value);  printf("%24s  %4d samples, %5.0f ns \n", "Trap. flat time", value[0], value[0] * 4 * tick2ns); ///Trap. flat time
  int flatTopTime = value[0] * 4 * tick2ns;
  double shift = log(riseTime * flatTopTime ) / log(2) - 2;
                                                                                            printf("%24s  %4d bit =? %.1f = Ceil( Log(rise [ns] x decay [ns])/Log(2) ), DPP-[0:5]\n", "Trap. Rescaling",  trapRescaling, shift ); ///Trap. Rescaling Factor
  CAEN_DGTZ_ReadRegister(handle, DPP::PHA::DecayTime + (ch << 8), value);         printf("%24s  %4d samples, %5.0f ns \n",    "Decay time", value[0], value[0] * 4 * tick2ns); ///Trap. pole zero
  CAEN_DGTZ_ReadRegister(handle, DPP::PHA::PeakingTime + (ch << 8), value);       printf("%24s  %4d samples, %5.0f ns = %.2f %% of FlatTop\n", "Peaking time",  value[0], value[0] * 4 * tick2ns, value[0] * 400. * tick2ns / flatTopTime ); ///Peaking time
  CAEN_DGTZ_ReadRegister(handle, DPP::PHA::PeakHoldOff + (ch << 8), value);       printf("%24s  %4d samples, %5.0f ns \n",    "Peak hole off", value[0], value[0] * 4 *tick2ns ); ///Peak hold off
                                                                                            printf("%24s  %4.0f samples, DPP-[12:13]\n", "Peak mean",  pow(4, NsPeak)); ///Ns peak

  printf("==========----- Other \n");
  CAEN_DGTZ_ReadRegister(handle, DPP::PHA::FineGain + (ch << 8), value);                 printf("%24s  %d = 0x%x\n",  "Energy fine gain",  value[0], value[0]); ///Energy fine gain
  CAEN_DGTZ_ReadRegister(handle, DPP::ChannelADCTemperature_R + (ch << 8), value);         printf("%24s  %d C\n",     "Temperature",  value[0]); ///Temperature
  CAEN_DGTZ_ReadRegister(handle, DPP::PHA::RiseTimeValidationWindow + (ch << 8), value); printf("%24s  %.0f ns \n", "RiseTime Vaild Win.",  value[0] * tick2ns); 
  CAEN_DGTZ_ReadRegister(handle, DPP::PHA::ChannelStopAcquisition + (ch << 8), value);   printf("%24s  %d = %s \n", "Stop Acq bit", value[0] & 1 , (value[0] & 1 ) == 0 ? "Run" : "Stop"); 
  CAEN_DGTZ_ReadRegister(handle, DPP::ChannelStatus_R + (ch << 8), value);                 printf("%24s  0x%x \n",    "Status bit",  (value[0] & 0xff) ); 
  CAEN_DGTZ_ReadRegister(handle, DPP::AMCFirmwareRevision_R + (ch << 8), value);           printf("%24s  0x%x \n",    "AMC firmware rev.",  value[0] ); 
  CAEN_DGTZ_ReadRegister(handle, DPP::VetoWidth  + (ch << 8), value);                    printf("%24s  0x%x \n",    "VetoWidth bit",  value[0] ); 
                                                                                                   printf("%24s  %d = %s\n",  "RollOverFlag, DPP-[26]",  rollOver, rollOver ? "enable" : "disable" ); 
                                                                                                   printf("%24s  %d = %s\n",  "Pile-upFlag, DPP-[27]",  pileUp, pileUp ? "enable" : "disable" ); 
                                                                                                   printf("%24s  %d, %s \n",  "Extra2 opt, DPP2-[8:10]",  extras2WordOption, extra2WordOptStr.c_str()); 
  printf("========= events storage and transfer\n");
  CAEN_DGTZ_ReadRegister(handle, DPP::NumberEventsPerAggregate_G + (ch << 8), value);    printf("%24s  %d \n",      "Event Aggregate",  value[0] & 0x3FF); 
  CAEN_DGTZ_ReadRegister(handle, DPP::AggregateOrganization, value);                     printf("%24s  %d \n",      "Buffer Division",  ((value[0] & 0x007) < 2 ? 0 : (int)pow(2, value[0] & 7))); 
  CAEN_DGTZ_ReadRegister(handle, DPP::MaxAggregatePerBlockTransfer , value);                  printf("%24s  %d \n",      "Num of Agg. / ReadData",  value[0] & 0x1FF); 
  
  printf("========================================= end of ch-%d\n", ch);

}


void PrintBoardConfiguration(int handle){
  printf("\e[33m================================================\n");
  printf("================ Setting for Board \n");
  printf("================================================\e[0m\n");
  uint32_t * value = new uint32_t[1];
  CAEN_DGTZ_ReadRegister(handle, (uint32_t) BoardConfiguration, value);
  printf("                        32  28  24  20  16  12   8   4   0\n");
  printf("                         |   |   |   |   |   |   |   |   |\n");
  cout <<" Board Configuration  : 0b" << bitset<32>(value[0]) << endl;
  printf("                      : 0x%x\n", value[0]);
  
  printf("             Bit[    0] = %d = Auto Data Flush   \n", value[0] & 0x1);
  printf("             Bit[    1] = %d = Decimated waveform  \n", (value[0] >> 1) & 0x1 );
  printf("             Bit[    2] = %d = Trigger propagation \n", (value[0] >> 2) & 0x1 );
  printf("             Bit[ 3:10] = %d = must be 001 0001 0 = 22 \n", (value[0] >> 3) & 0xFF );
  printf("             Bit[   11] = %d = Dual Trace \n", (value[0] >> 11) & 0x1 );
  printf("             Bit[12:13] = %d = Analog probe 1 : ",((value[0] >> 12) & 0x3  ));
  switch ( ((value[0] >> 12) & 0x3 ) ){
    case 0 : printf("input\n"); break;
    case 1 : printf("RC-CR (1st derivative)\n");break;
    case 2 : printf("RC-CR2 (2nd derivative)\n"); break;
    case 3 : printf("Trapezoid \n"); break;
  }
  printf("             Bit[14:15] = %d = Analog probe 2 : ", ((value[0] >> 14) & 0x3 ));
  switch ( ((value[0] >> 14) & 0x3 ) ){
    case 0 : printf("input\n"); break; 
    case 1 : printf("Threshold\n"); break; 
    case 2 : printf("Trapezoid - Baseline\n"); break; 
    case 3 : printf("baseline.\n"); break;
  }
  printf("             Bit[   16] = %d = WaveForm Recording \n",((value[0] >> 16) & 0x1 ) );
  printf("             Bit[   17] = %d = Extras 2 word enable   \n", ((value[0] >> 17) & 0x1 ));
  printf("             Bit[   18] = %d = Record Time Stamp   \n", ((value[0] >> 18) & 0x1 ));
  printf("             Bit[   19] = %d = Record Energy   \n", ((value[0] >> 19) & 0x1 ));
  printf("             Bit[20:23] = %d = Digital Virtual probe 1 : ", ((value[0] >> 20) & 0x7 ));
  switch (((value[0] >> 20) & 0xF )) {
    case  0: printf("Peaking, shows where the energy is calculated; \n"); break;
    case  1: printf("”Armed”, digital input showing where the RC‐CR2 crosses the Threshold\n"); 
    case  2: printf("”Peak Run”, starts with the trigger and last for the whole event\n");
    case  3: printf("”Pile‐up”, shows where a pile‐up event occurred\n");
    case  4: printf("”Peaking”, shows where the energy is calculated\n");
    case  5: printf("”TRG Validation Win”, digital input showing the trigger validation acceptance window TVAW\n");
    case  6: printf("”Baseline freeze”, shows where the algorithm stops calculating the baseline and its value is frozen\n");
    case  7: printf("”TRG Holdoff”, shows the trigger hold‐off parameter\n");
    case  8: printf("”TRG Validation”, shows the trigger validation signal TRG_VAL \n");
    case  9: printf("”Acq Busy”, this is 1 when the board is busy (saturated input signal or full memory board) or there is a veto\n");
    case 10: printf("”Zero Cross. Win.”, shows the RT Discrimination Width\n");
    case 11: printf("”Ext TRG”, shows the external trigger, when available\n");
    case 12: printf("”Busy”, shows when the memory board is full.\n");
  }
  printf("             Bit[26:28] = %d = Digital Virtual probe 2 : ", ((value[0] >> 26) & 0x7 ));
  if( ((value[0] >> 26) & 0x7 ) == 0 ) {
    printf("Trigger\n");
  }else{
    printf("Reserved\n");
  }
  printf("====================================== \n");
}

unsigned int ReadBuffer(unsigned int nWord, char * buffer, bool verbose = true){
  if( buffer == NULL ) return 0;
  
  unsigned int word = 0;
  for( int i = 0 ; i < 4 ; i++) word += ((buffer[i + 4 * nWord] & 0xFF) << 8*i);
  if( verbose) printf("%d | 0x%08x\n", nWord, word);
  return word;
}



int main(int argc, char* argv[]){
  
  
  ///============== open digitizer
  int handle;
  
  printf("======== open board\n");
  int ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_OpticalLink, 1, 0, 0, &handle);
  
  CAEN_DGTZ_BoardInfo_t BoardInfo;
  ret = (int) CAEN_DGTZ_GetInfo(handle, &BoardInfo);
  int NInputCh = BoardInfo.Channels;
  uint32_t regChannelMask = 0xFFFF;
  float tick2ns = 4.0;
  switch(BoardInfo.Model){
        case CAEN_DGTZ_V1730: tick2ns = 2.0; break; ///ns -> 500 MSamples/s
        case CAEN_DGTZ_V1725: tick2ns = 4.0; break; ///ns -> 250 MSamples/s
  }
  unsigned int ADCbits = BoardInfo.ADC_NBits;
  
  if( ret != 0 ) { printf("==== open digitizer\n"); return 0;}
  
  ///======= reset 
  ret = CAEN_DGTZ_Reset(handle);

  printf("======== program board\n");

  ///ret |= CAEN_DGTZ_SetDPPAcquisitionMode(handle, CAEN_DGTZ_DPP_ACQ_MODE_List, CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);
  ///ret |= CAEN_DGTZ_SetDPPAcquisitionMode(handle, CAEN_DGTZ_DPP_ACQ_MODE_Mixed, CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);  /// Board Configure can do that

  /// Set the number of samples for each waveform
  ret = CAEN_DGTZ_WriteRegister(handle, DPP::RecordLength_G + 0x7000, 625);  
  if( ret != 0 ) { printf("==== set Record Length.\n"); return 0;}


  //ret |= CAEN_DGTZ_WriteRegister(handle, DPP::BoardConfiguration, 0x4F8115);  // with wave
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::BoardConfiguration, 0x4E8115);  // with-out wave


  /// Set the digitizer acquisition mode (CAEN_DGTZ_SW_CONTROLLED or CAEN_DGTZ_S_IN_CONTROLLED)
  ret = CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED); /// software command

  /// Set the I/O level (CAEN_DGTZ_IOLevel_NIM or CAEN_DGTZ_IOLevel_TTL)
  ret |= CAEN_DGTZ_SetIOLevel(handle, CAEN_DGTZ_IOLevel_NIM);

  /** Set the digitizer's behaviour when an external trigger arrives:
  CAEN_DGTZ_TRGMODE_DISABLED: do nothing
  CAEN_DGTZ_TRGMODE_EXTOUT_ONLY: generate the Trigger Output signal
  CAEN_DGTZ_TRGMODE_ACQ_ONLY = generate acquisition trigger
  CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT = generate both Trigger Output and acquisition trigger
  see CAENDigitizer user manual, chapter "Trigger configuration" for details */
  ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  if( ret != 0 ) { printf("==== CAEN_DGTZ_SetExtTriggerInputMode.\n"); return 0;}

  ret = CAEN_DGTZ_SetChannelEnableMask(handle, 0xFFFF);
  if( ret != 0 ) { printf("==== CAEN_DGTZ_SetChannelEnableMask.\n"); return 0;}
  
  ret = CAEN_DGTZ_SetNumEventsPerAggregate(handle, 0);
  if( ret != 0 ) { printf("==== CAEN_DGTZ_SetNumEventsPerAggregate. %d\n", ret); return 0;}
  
  //ret = CAEN_DGTZ_SetDPPEventAggregation(handle, 0, 0);
  //if( ret != 0 ) { printf("==== CAEN_DGTZ_SetDPPEventAggregation. %d\n", ret); return 0;}
  
  
  /** Set the mode used to syncronize the acquisition between different boards.
  In this example the sync is disabled */
  ret = CAEN_DGTZ_SetRunSynchronizationMode(handle, CAEN_DGTZ_RUN_SYNC_Disabled);
  if( ret != 0 ) { printf("==== set board error.\n"); return 0;}
  
  printf("======== program Channels\n");
  ///CAEN_DGTZ_DPP_PHA_Params_t DPPParams;
  ///memset(&DPPParams, 0, sizeof(CAEN_DGTZ_DPP_PHA_Params_t));
  ///for(int i = 0; i < NInputCh; i++){
  ///  DPPParams.M[i] = 5000;        /// decay time [ns]
  ///  DPPParams.m[i] = 992;         /// flat-top [ns]
  ///  DPPParams.k[i] = 96;          /// rise-time [ns]
  ///  DPPParams.ftd[i] = 192;       /// flat-top delay, peaking time [ns]
  ///  DPPParams.a[i] = 4;           /// Trigger Filter smoothing factor, 1, 2, 3, 4, 16, 32
  ///  DPPParams.b[i] = 96;          /// input rise time [ns]
  ///  DPPParams.thr[i] = 100;       /// Threshold [LSB]
  ///  DPPParams.nsbl[i] = 3;        /// Baseline samples, 0 = 0, when > 0, pow(4, n+1)  /// in DPP Control
  ///  DPPParams.nspk[i] = 2;        /// peak samples, 4^n                               /// in DPP Control
  ///  DPPParams.pkho[i] = 992 ;     /// peak hold off [ns]
  ///  DPPParams.trgho[i] = 480 ;    /// trigger hold off [ns]
  ///  DPPParams.twwdt[i] = 0 ;      /// rise time validation window, 0x1070
  ///  DPPParams.trgwin[i] = 0 ;     /// trigger coincident window
  ///  DPPParams.dgain[i] = 0;       /// digial gain for digial probe, 2^n 
  ///  DPPParams.enf[i] = 1 ;        /// energy normalization factor (fine gain?)
  ///  DPPParams.decimation[i] = 0 ; /// waveform decimation, 2^n, when n = 0, disable
  ///  DPPParams.blho[i] = 0;        /// not use
  ///}
  ///ret = CAEN_DGTZ_SetDPPParameters(handle, regChannelMask, &DPPParams);
  
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::PHA::DecayTime + 0x7000 , 5000 ); 
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::PHA::TrapezoidFlatTop + 0x7000 , 62 ); 
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::PHA::TrapezoidRiseTime + 0x7000 , 6 ); 
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::PHA::PeakingTime + 0x7000 , 6 ); 
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::PHA::RCCR2SmoothingFactor + 0x7000 , 4 ); 
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::PHA::InputRiseTime + 0x7000 , 6 ); 
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::PHA::TriggerThreshold + 0x7000 , 64 );
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::PHA::PeakHoldOff + 0x7000 , 0x3E );
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::PHA::TriggerHoldOffWidth + 0x7000 , 0x3E );
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::PHA::RiseTimeValidationWindow + 0x7000 , 0x0 );
  
    
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::ChannelDCOffset + 0x7000 , 0xEEEE );
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::PreTrigger + 0x7000 , 124 );
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::InputDynamicRange + 0x7000 , 0x0 );
  
  
  //ret |= CAEN_DGTZ_WriteRegister(handle, DPP::BoardConfiguration , 0x10E0114 );
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::NumberEventsPerAggregate_G + 0x7000, 5);
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::AggregateOrganization, 0);
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::MaxAggregatePerBlockTransfer, 40);
  
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::DPPAlgorithmControl + 0x7000, 0xe30200f);
  
  
  if( ret != 0 ) { printf("==== set channels error.\n"); return 0;}
  
  printf("================ allowcate memory \n");
  
  int Nb;                                  /// number of byte
  char *buffer = NULL;                     /// readout buffer
  uint32_t DataIndex[MaxRegChannel];
  uint32_t AllocatedSize, BufferSize;
  CAEN_DGTZ_DPP_PHA_Event_t  *Events[MaxRegChannel];  /// events buffer
  CAEN_DGTZ_DPP_PHA_Waveforms_t   *Waveform[MaxRegChannel];     /// waveforms buffer
  
  ret = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer, &AllocatedSize);
  printf("allowcated %d byte ( %d words) for buffer\n", AllocatedSize, AllocatedSize/4); 
  ret |= CAEN_DGTZ_MallocDPPEvents(handle, reinterpret_cast<void**>(&Events), &AllocatedSize) ;
  printf("allowcated %d byte for Events\n", AllocatedSize); 
  for( int i = 0 ; i < NInputCh; i++){
    ret |= CAEN_DGTZ_MallocDPPWaveforms(handle, reinterpret_cast<void**>(&Waveform[i]), &AllocatedSize);
    printf("allowcated %d byte for waveform-%d\n", AllocatedSize, i); 
  }
  
  if( ret != 0 ) { printf("==== memory allocation error.\n"); return 0;}
  
  PrintBoardConfiguration(handle);
  PrintChannelSettingFromDigitizer(handle, 4, tick2ns);
  
  printf("============ Start ACQ \n");
  CAEN_DGTZ_SWStartAcquisition(handle);
  
  sleep(1);
  
  printf("============ Read Data \n");
  
  ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &BufferSize);
  if (ret) {
    printf("Error when reading data %d\n", ret);
    return 0;
  }
  Nb = BufferSize;
  if (Nb == 0 || ret) {
     return 0;
  }
  ret |= (CAEN_DGTZ_ErrorCode) CAEN_DGTZ_GetDPPEvents(handle, buffer, BufferSize, reinterpret_cast<void**>(&Events), DataIndex);
  if (ret) {
    printf("Error when getting events from data %d\n", ret);
    return 0;
  }

  for (int ch = 0; ch < NInputCh; ch++) {
    if( DataIndex[ch] > 0 ) printf("------------------------ %d, %d\n", ch, DataIndex[ch]);
    for (int ev = 0; ev < DataIndex[ch]; ev++) {
      ///TrgCnt[ch]++;
      
      if( ev == 0 ){
        printf("%3s, %6s, %13s | %5s | %13s | %13s \n", "ev", "energy", "timetag", "ex2", "rollover", "timeStamp");
      }
      
      if (Events[ch][ev].Energy > 0 && Events[ch][ev].TimeTag > 0 ) {
        ///ECnt[ch]++;
        unsigned long long timetag = (unsigned long long) Events[ch][ev].TimeTag;
        unsigned long long rollOver = Events[ch][ev].Extras2 >> 16;
        rollOver = rollOver << 31;
        timetag  += rollOver ;

        printf("%3d, %6d, %13lu | %5u | %13llu | %13llu \n", 
                   ev, Events[ch][ev].Energy, 
                       Events[ch][ev].TimeTag, 
                       Events[ch][ev].Extras2 , 
                       rollOver >> 32, timetag);

      } else { /// PileUp
          ///PurCnt[ch]++;
      }

    } /// loop on events
  } /// loop on channels

  printf("============ Stop ACQ \n");
  ret |= CAEN_DGTZ_ClearData(handle);
  
  
  printf("============= Read Buffer \n");
  
  unsigned int nw = 0;
      
  do{
    printf("######################################### Board Agg.\n");
    unsigned int word = ReadBuffer(nw, buffer);
    if( ( (word >> 28) & 0xF ) == 0xA ) { /// start of Board Agg
      unsigned int nWord = word & 0x0FFFFFFF ;
      printf(" number of words in this Agg : %d \n", nWord);
      
      nw = nw + 1; word = ReadBuffer(nw, buffer);
      unsigned int BoardID = ((word >> 27) & 0x1F);
      bool BoardFailFlag =  ((word >> 26) & 0x1 );
      unsigned int ChannelMask = ( word & 0xFF ) ;
      printf("Board ID : %d, FailFlag = %d, ChannelMask = 0x%x\n", BoardID, BoardFailFlag, ChannelMask);
      
      nw = nw + 2; 
      unsigned int AggCounter = ReadBuffer(nw, buffer);
      printf("Agg Counter : %d \n", AggCounter);
      

      
      for( int chMask = 0; chMask < 8 ; chMask ++ ){
        if( ((ChannelMask >> chMask) & 0x1 ) == 0 ) continue;
        printf("---------------------- Dual Channel Block : %d\n", chMask *2 );
        nw = nw + 1; word = ReadBuffer(nw, buffer);
        bool hasFormatInfo = ((word >> 31) & 0x1);
        unsigned int aggSize = ( word & 0x3FFFFFF ) ;
        printf(" size : %d \n",  aggSize);
        unsigned int nSample = 0; /// wave form;
        unsigned int nEvents = 0;
        if( hasFormatInfo ){
          nw = nw + 1; word = ReadBuffer(nw, buffer);
          nSample = ( word & 0xFFFF )  * 8;
          unsigned int digitalProbe = ( (word >> 16 ) & 0xF );
          unsigned int analogProbe2 = ( (word >> 20 ) & 0x3 );
          unsigned int analogProbe1 = ( (word >> 22 ) & 0x3 );
          unsigned int extra2Option = ( (word >> 24 ) & 0x7 );
          bool hasWaveForm  = ( (word >> 27 ) & 0x1 );
          bool hasExtra2    = ( (word >> 28 ) & 0x1 );
          bool hasTimeStamp = ( (word >> 29 ) & 0x1 );
          bool hasEnergy    = ( (word >> 30 ) & 0x1 );
          bool hasDualTrace = ( (word >> 31 ) & 0x1 );
          
          printf("dualTrace : %d, Energy : %d, Time: %d, Wave : %d, Extra2: %d, Extra2Option: %d \n", 
                  hasDualTrace, hasEnergy, hasTimeStamp, hasWaveForm, hasExtra2, extra2Option);
          printf("Ana Probe 1 & 2: %d %d , Digi Probe: %d, nSample : %d \n", 
                  analogProbe1, analogProbe2, digitalProbe, nSample);
        
          nEvents = aggSize / (nSample/2 + 2 + hasExtra2 );
          printf("=========== nEvents : %d \n", nEvents);
        }else{
          printf("does not has format info. unable to read buffer.\n");
          break;
        }
        
        for( int ev = 0; ev < nEvents ; ev++){
          printf("=================================== event : %d\n", ev);
          nw = nw +1 ; word = ReadBuffer(nw, buffer);
          bool channelTag = ((word >> 31) & 0x1);
          unsigned int timeStamp = (word & 0x7FFFFFFF);
          int channel = chMask*2 + channelTag;
          printf("ch : %d, timeStamp %u \n", channel, timeStamp);
          
          ///===== read waveform
          for( int wi = 0; wi < nSample/2; wi++){
            nw = nw +1 ; word = ReadBuffer(nw, buffer, false);
            bool isTrigger1 = (( word >> 31 ) & 0x1 );
            unsigned int wave1 = (( word >> 16) & 0x3FFF);
            
            bool isTrigger0 = (( word >> 15 ) & 0x1 );
            unsigned int wave0 = ( word & 0x3FFF);
            
            if( ev == 0 ){
              printf("%4d| %5d, %d \n",   2*wi, wave0, isTrigger0);
              printf("%4d| %5d, %d \n", 2*wi+1, wave1, isTrigger1);
            }
          }
          
          nw = nw +1 ; word = ReadBuffer(nw, buffer);
          unsigned int extra2 = word;
          
          nw = nw +1 ; word = ReadBuffer(nw, buffer);
          unsigned int extra = (( word >> 16) & 0x3FF);
          unsigned int energy = (word & 0x7FFF);
          bool pileUp = ((word >> 15) & 0x1);
          
          printf("PileUp : %d , extra : 0x%04x, energy : %d \n", pileUp, extra, energy);
          
        }
      }
    }else{
      printf("incorrect buffer header. \n");
      break;
    }
    nw++;
  }while(true);

  
  printf("=========== close Digitizer \n");
  CAEN_DGTZ_SWStopAcquisition(handle);
  CAEN_DGTZ_CloseDigitizer(handle);
  CAEN_DGTZ_FreeReadoutBuffer(&buffer);
  CAEN_DGTZ_FreeDPPEvents(handle, reinterpret_cast<void**>(&Events));
  CAEN_DGTZ_FreeDPPWaveforms(handle, Waveform);
      
  return 0;
}
