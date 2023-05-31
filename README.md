# Introduction

This is a DAQ for 1st gen CAEN digitizer for V1725, V17255S, V1230 with PHA and PSD firmware.

It has scope (updated every half-sec), allow full control of the digitizer (except LVDS), and allow saving waveform.

It can be connected to InfluxDB v1.8 and Elog.

# Undergoing

the following additional functions are planned and I am working on them

- 1-D and 2-D histogram
- Online Analyzer
- support V1740 DPP-QDC
- synchronization helper

# Required / Development enviroment

Ubuntu 22.04

- CAENVMELib_v3.3
- CAENCOmm_v1.5.3
- CAENDigitizer_v2.17.1
- CAEN A3818 Driver

- qt6-base-dev
- libqt6charts6-dec
- libcurl4-openssl-dev
- elog

The CAEN Libraries need to download and install manually. The other libraries can be installed using the following command:

`sudo apt install qt6-base-dev libqt6charts6-dev libcurl4-openssl-dev elog`

The elog installed using apt is 3.1.3. If a higher version is needed. Please go to https://elog.psi.ch/elog/

The libcurl4 is need for pushing data to InfluxDB v1.8

The QCustomPlot (https://www.qcustomplot.com/index.php/introduction) source files are already included in the repository.

# Compile

use `qmake6 -project ` to generate the *.pro

in the *.pro, add 

` QT += core widgets charts printsupport`

` LIBS += -lCAENDigitizer -lcurl`

then run ` qmake6 *.pro` it will generate Makefile

then  ` make`

if you want to use GDB debugger, in the *.pro file add

` QMAKE_CXXFLAGS += -g`

## exclude some files from the auto-gen *.pro

The following files must be excluded from the *.pro, as they are not related to the GUI

- DataGenerator.cpp
- DataReaderScript.cpp
- EventBuilder.cpp
- test.cpp
- test_indep.cpp

Those file can be compiled using 

`make -f Makefile_test`