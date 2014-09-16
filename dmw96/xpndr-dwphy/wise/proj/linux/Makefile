# Makefile for WiSE
# Copyright 2002 Bergana Communications, Inc. All rights reserved.
#    Also Copyright 2005-2011 DSP Group, Inc. All rights reserved.

# Define the shell for make
SHELL = /bin/sh

# Executable Filename
OUTPUT_FNAME = ../../wise.linux

# Complier and build options

CC  = gcc -pthread -O3 -ffast-math -Wall -m64
CXX = g++ -pthread -O3 -ffast-math -Wall

CC_DEFINES += -DWISE_BUILD_MULTITHREADED -DWISE_USE_PTHREADS
CC_DEFINES += -DWISE_BUILD_INCLUDE_CALCEVM
CC_DEFINES += -DWISE_BUILD_INCLUDE_NEVADAPHY
#CC_DEFINES += -DWISE_BUILD_INCLUDE_TAMALEPHY
CC_DEFINES += -DWISE_BUILD_INCLUDE_PHY11N
CC_DEFINES += -DWISE_MAX_THREADS=9

# uncomment to disable Wfixp precision limiting
#CC_DEFINES += -DWIBFFIXPT_DISABLE

#LDFLAGS += -lpthread -D_REENTRANT
VPATH = ../../source
#INCLUDE = -I../../source

### List object files to include in the build ##############################

WISE_LIB  += wiArray.o
WISE_LIB  += wiChanl.o
WISE_LIB  += wiFilter.o
WISE_LIB  += wiMAC.o
WISE_LIB  += wiMain.o
WISE_LIB  += wiMath.o
WISE_LIB  += wiMatrix.o
WISE_LIB  += wiMemory.o
WISE_LIB  += wiParse.o
WISE_LIB  += wiProcess.o
WISE_LIB  += wiPHY.o
WISE_LIB  += wiRnd.o
WISE_LIB  += wiTest.o
WISE_LIB  += wiTGnChanl.o
WISE_LIB  += wiUtil.o

CALCEVM    = CalcEVM.o
CALCEVM   += CalcEVM_OFDM.o

DW52       = DW52.o
DW52      += Mojave.o
DW52      += Mojave_DigitalFrontEnd.o
DW52      += Mojave_FFT.o
DW52      += MojaveTest.o
DW52      += bMojave.o
DW52      += RF52.o

NEVADAPHY  = Nevada.o
NEVADAPHY += Nevada_DFS.o
NEVADAPHY += Nevada_DigitalFrontEnd.o
NEVADAPHY += Nevada_dpdBlocks.o
NEVADAPHY += Nevada_FFT.o
NEVADAPHY += Nevada_RX.o
NEVADAPHY += Nevada_RX1.o
NEVADAPHY += Nevada_RX2.o
NEVADAPHY += Nevada_TX.o
NEVADAPHY += Nevada_TX_DPD.o
NEVADAPHY += Nevada_b.o
NEVADAPHY += NevadaPHY.o
NEVADAPHY += Coltrane.o

TAMALEPHY  = Tamale.o
TAMALEPHY += Tamale_DigitalFrontEnd.o
TAMALEPHY += Tamale_dpdBlocks.o
TAMALEPHY += Tamale_FFT.o
TAMALEPHY += Tamale_RX.o
TAMALEPHY += Tamale_RX1.o
TAMALEPHY += Tamale_RX2.o
TAMALEPHY += Tamale_TX.o
TAMALEPHY += Tamale_TX_DPD.o
TAMALEPHY += Tamale_b.o
TAMALEPHY += TamalePHY.o
#TAMALEPHY += Coltrane.o # Coltrane is shared with NEVADAPHY

PHY11N  = Phy11n.o
PHY11N += Phy11n_TX.o
PHY11N += Phy11n_RX.o
PHY11N += Phy11n_Demapper.o
PHY11N += Phy11n_SphereDecoder.o
PHY11N += Phy11n_SphereDecoderFxP.o
PHY11N += Phy11n_BFDemapper.o
PHY11N += wiBFfixpt.o
PHY11N += wiBFformat.o
PHY11N += wiBFkbsd.o
PHY11N += wiBFkvsd.o
PHY11N += wiBFlfsd.o
PHY11N += wiBFmatrix.o
PHY11N += wiBFsesd.o
PHY11N += wiBFutil.o


OBJECTS = main.o $(WISE_LIB) $(CALCEVM) $(NEVADAPHY) $(PHY11N)


### BUILD RULES ############################################################

all: wise


wise: $(OBJECTS)
	$(CXX) $(LDFLAGS) -lm -o $(OUTPUT_FNAME) $(OBJECTS)

%.o: %.c
	$(CC) -c $(CC_DEFINES) $(INCLUDE) $+

%.o: %.cpp
	$(CC) -c $(CC_DEFINES) $(INCLUDE) $+

clean:
	rm -f *.o	

