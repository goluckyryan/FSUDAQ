# Introduction

This is a DAQ for 1st gen CAEN digitizer for V1725, V17255S, V1230 with PHA and PSD firmware.

It has scope (updated every half-sec), allow full control of the digitizer (except LVDS), and allow saving waevform.

# Required / Development enviroment

Ubuntu 22.04

CAENVMELib_v3.3

CAENCOmm_v1.5.3

CAENDigitizer_v2.17.1

`sudo apt install qt6-base-dev libcurl4-openssl-dev libqt6charts6-dev`

# Compile

use `qmake6 -project ` to generate the *.pro

in the *.pro, add 

` QT += core widgets charts`

` LIBS += -lCAENDigitizer -lcurl`

then run ` qmake6 *.pro` it will generate Makefile

then  ` make`