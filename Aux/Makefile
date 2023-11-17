########################################################################
#
#
#########################################################################

CC	=	g++

#COPTS	=	-fPIC -DLINUX -O2 -std=c++17 -lpthread
COPTS	=	-fPIC -DLINUX -g -O2 -Wall -std=c++17 -lpthread

CAENLIBS	=	-lCAENDigitizer

ROOTLIBS = `root-config --cflags --glibs`

OBJS = ClassDigitizer.o MultiBuilder.o

ALL = test test_indep DataGenerator EventBuilder DataReader DumpFSU2ROOT

#########################################################################

all	:	$(ALL)

clean	:
		/bin/rm -f $(OBJS) $(ALL)

MultiBuilder.o : ../MultiBuilder.cpp ../MultiBuilder.h
		$(CC) $(COPTS) -c ../MultiBuilder.cpp

ClassDigitizer.o : ../ClassDigitizer.cpp ../ClassDigitizer.h ../RegisterAddress.h ../macro.h ../ClassData.h
		$(CC) $(COPTS) -c ../ClassDigitizer.cpp

test	:	test.cpp ../ClassDigitizer.o
		@echo "--------- making test"
		$(CC) $(COPTS) -o test test.cpp ../ClassDigitizer.o  $(CAENLIBS)  $(ROOTLIBS)

test_indep : test_indep.cpp ../RegisterAddress.h ../macro.h
		@echo "--------- making test_indep"
		$(CC) $(COPTS) -o test_indep test_indep.cpp $(CAENLIBS) 

DataGenerator : DataGenerator.cpp ../ClassDigitizer.o
		@echo "--------- making DataGenerator"
		$(CC) $(COPTS) -o DataGenerator DataGenerator.cpp ../ClassDigitizer.o $(CAENLIBS)

DataReader : DataReaderScript.cpp ../ClassData.h MultiBuilder.o
		@echo "--------- making DataReader"
		$(CC) $(COPTS) -o DataReader DataReaderScript.cpp ../ClassData.h MultiBuilder.o

EventBuilder : EventBuilder.cpp ../ClassData.h MultiBuilder.o
		@echo "--------- making EventBuilder"
		$(CC) $(COPTS) -o EventBuilder EventBuilder.cpp MultiBuilder.o $(ROOTLIBS) 

DumpFSU2ROOT : DumpFSU2ROOT.cpp ../ClassData.h MultiBuilder.o
		@echo "--------- making DumpFSU2ROOT"
		$(CC) $(COPTS) -o DumpFSU2ROOT DumpFSU2ROOT.cpp ../ClassData.h MultiBuilder.o $(ROOTLIBS)