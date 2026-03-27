#include "ClassDigitizerAPI.h"

DigitizerAPI::DigitizerAPI() : Digitizer(){

}

DigitizerAPI::DigitizerAPI(int boardID, int portID, bool program, bool verbose) : Digitizer(boardID, portID, false, verbose){

  //The Digitizer() filled 
  //handle
  //LinkType
  //NumRegChannel
  //NumInputCh
  //data
  //isConnected
  //---------- not filled
  // tick2ns
  // ModelType

  NumInputCh = 16;
  NumRegChannel = 16;
  NCoupledCh = 8;
  ModelType = ModelTypeCode::VME;

  if( program ) ProgramBoard_V1743();

}

DigitizerAPI::~DigitizerAPI(){

}

int DigitizerAPI::ProgramBoard_V1743(){

  int ret = 0;
	int groupsMask;

	//======================== reset the digitizer
  ret |= CAEN_DGTZ_Reset(handle);
  if (ret != 0) {
    ErrorMsg("===== Unable reset digitizer");
    printf("Please reset digitizer manually then restart the program\n");
    return -1;
  }

	//======================== Board Fail Status
	uint32_t d32 = 0;
	ret |= CAEN_DGTZ_ReadRegister(handle, 0x8178, &d32);
	if ((d32 & 0xF) != 0) {
		ErrorMsg("Error: Internal Communication Timeout occurred.");
		printf("Please reset digitizer manually then restart the program\n");
		return -1;
	}

	//======================== Set Group Enable Mask. all channel enabled.
	groupsMask = 0;
	for (int channel = 0; channel < NumInputCh; channel++) {
    groupsMask |= (1 << (channel / 2));
	}
	ret |= CAEN_DGTZ_SetGroupEnableMask(handle, groupsMask);

	//======================== Set Post Trigger Delay
	for (int gpIndex = 0; gpIndex < NumInputCh; gpIndex++) {
		ret |= CAEN_DGTZ_SetSAMPostTriggerSize(handle, gpIndex, 0x0F); // range from 1 to 255, 1 unit = 16 * sampling period
	}

	//======================== Set Sampling Frequency
	ret |= CAEN_DGTZ_SetSAMSamplingFrequency(handle, CAEN_DGTZ_SAM_3_2GHz);
  tick2ns = 0.3125;

	//======================== Set internal test Pulser Parameters
	for (int channel = 0; channel < NumInputCh; channel++) {
    // unsigend short pulsePattern = 0x00FF;
    //ret |= CAEN_DGTZ_EnableSAMPulseGen(handle, channel, pulsePattern, CAEN_DGTZ_SAMPulseCont);
    ret |= CAEN_DGTZ_DisableSAMPulseGen(handle, channel); //disable
	}

	//======================== Set Trigger Threshold, 0x0 = +1.25V, 0x7FFF = 0V, 0xFFFF = -1.25V
	for (int channel = 0; channel < NumInputCh; channel++) {
    float TriggerThreshold_V = 0.0;
    float DCOffset_V = 0.0;
		float valF = TriggerThreshold_V + DCOffset_V;
		int reg_val = (int)((MAX_DAC_RAW_VALUE - valF) / (MAX_DAC_RAW_VALUE - MIN_DAC_RAW_VALUE) * 65535);  // Inverted Range
		ret |= CAEN_DGTZ_SetChannelTriggerThreshold(handle, channel, reg_val);
	}

	//======================== Set Trigger Source
	// reset channel trigger
  regChannelMask = 0xFFFF; // all channel
	ret |= CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_DISABLED, regChannelMask); //disable self trigger on all channels

	// acalculate the channel mask0
  ret |= CAEN_DGTZ_SetSWTriggerMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_DISABLED);

	// switch (WDb->TriggerType) {
  //   case SYSTEM_TRIGGER_SOFT:
  //     ret |= CAEN_DGTZ_SetSWTriggerMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  //     ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_DISABLED);
  //     WDrun.ContinuousTrigger = 1;
  //     break;
  //   case SYSTEM_TRIGGER_NORMAL:
  //     ret |= CAEN_DGTZ_SetSWTriggerMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  //     ret |= CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY, channelsMask);
  //     ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_DISABLED);
  //     break;
  //   case SYSTEM_TRIGGER_EXTERNAL:
  //     ret |= CAEN_DGTZ_SetSWTriggerMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  //     ret |= CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_EXTOUT_ONLY, channelsMask);
  //     ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  //     break;
  //   case SYSTEM_TRIGGER_ADVANCED:
  //     ret |= CAEN_DGTZ_SetSWTriggerMode(handle, WDb->SwTrigger);
  //     ret |= CAEN_DGTZ_SetChannelSelfTrigger(handle, WDb->ChannelSelfTrigger, channelsMask);
  //     ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, WDb->ExtTrigger);
  // 		break;
	// }

	/* Set Trigger Polarity */
	for (int channel = 0; channel < NumInputCh; channel++) {
		ret |= CAEN_DGTZ_SetTriggerPolarity(handle, channel, CAEN_DGTZ_TriggerOnRisingEdge);
	}

	/* Set Channel DC Offset */
	for (int channel = 0; channel < NumInputCh; channel++) {
		float valF = 0.0;
		int reg_val = (int)((MAX_DAC_RAW_VALUE + valF) / (MAX_DAC_RAW_VALUE - MIN_DAC_RAW_VALUE) * 65535);  // Inverted Range
		ret |= CAEN_DGTZ_SetChannelDCOffset(handle, channel, reg_val);
	}

	/* Set Correction Level */
	ret |= CAEN_DGTZ_SetSAMCorrectionLevel(handle, CAEN_DGTZ_SAM_CORRECTION_DISABLED);

	/* Set MAX NUM EVENTS */
	ret |= CAEN_DGTZ_SetMaxNumEventsBLT(handle, MAX_NUM_EVENTS_BLT);

	/* Set Recording Depth */
	ret |= CAEN_DGTZ_SetRecordLength(handle, 16 * 100); // in sample, must be multiple of 16 and > 4 * 16

	/* Set Front Panel I/O Control */
	ret |= CAEN_DGTZ_SetIOLevel(handle, CAEN_DGTZ_IOLevel_NIM);

	ret |= CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);

	/* execute generic write commands */
	// for (int i = 0; i < WDb->GWn; i++)
	// 	ret |= WriteRegisterBitmask(handle, WDb->GWaddr[i], WDb->GWdata[i], WDb->GWmask[i]);

	if (ret) {
		printf("\n");
		printf("WARN: there were errors when configuring the digitizer.\n");
		printf("Some settings may not be executed\n\n");
	}

	return 0;
  


}

int DigitizerAPI::EventDecoding(){
	return 0;
}

int DigitizerAPI::WriteRegisterBitmask(int32_t handle, uint32_t address, uint32_t data, uint32_t mask) {
	int32_t ret = CAEN_DGTZ_Success;
	uint32_t d32 = 0xFFFFFFFF;

	ret = CAEN_DGTZ_ReadRegister(handle, address, &d32);
	if (ret != CAEN_DGTZ_Success)
		return ret;

	data &= mask;
	d32 &= ~mask;
	d32 |= data;
	ret = CAEN_DGTZ_WriteRegister(handle, address, d32);
	return ret;
}