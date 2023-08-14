#ifndef MACRO_H
#define MACRO_H

#define MaxNPorts 4   //for optical link
#define MaxNBoards 4  //for both optical link and usb

#define MaxNDigitizer MaxNPorts * MaxNBoards

#define MaxNChannels 16
#define MaxRecordLength 0x3fff * 8 
#define MaxSaveFileSize  1024 * 1024 * 1024 * 2

#define MaxDisplayTraceDataLength 2000 //data point, 
#define MaxNumberOfTrace  4   // in an event

#define SETTINGSIZE 2048

#include <sys/time.h> /** struct timeval, select() */

inline unsigned int get_time(){
  unsigned int time_us;
  struct timeval t1;
  struct timezone tz;
  gettimeofday(&t1, &tz);
  time_us = (t1.tv_sec) * 1000 * 1000 + t1.tv_usec;
  return time_us;
}

#endif
