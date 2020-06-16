platform := hisi3516dv300

CC=arm-himix200-linux-gcc
CXX=arm-himix200-linux-g++

INCLUDE = -I ./include/media \

#海思平台库
MPI_LIB_PATH := ./Lib/$(platform)/hi_mpp

MPI_LIBS += -L $(MPI_LIB_PATH)/ -laaccomm
MPI_LIBS += -L $(MPI_LIB_PATH)/ -laacenc
MPI_LIBS += -L $(MPI_LIB_PATH)/ -laacdec
MPI_LIBS += -L $(MPI_LIB_PATH)/ -l_hicalcflicker
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lmd
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lnnie
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lbin
MPI_LIBS += -L $(MPI_LIB_PATH)/ -l_hiae
MPI_LIBS += -L $(MPI_LIB_PATH)/ -l_hiawb_natura
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lhi_cipher
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lmpi
MPI_LIBS += -L $(MPI_LIB_PATH)/ -ltde
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lhdmi
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lsvpruntime
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lupvqe
MPI_LIBS += -L $(MPI_LIB_PATH)/ -l_hildci
MPI_LIBS += -L $(MPI_LIB_PATH)/ -live
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lisp
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lhifisheyecalibrate
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lVoiceEngine
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lsecurec
MPI_LIBS += -L $(MPI_LIB_PATH)/ -l_hidrc
MPI_LIBS += -L $(MPI_LIB_PATH)/ -l_hiawb
MPI_LIBS += -L $(MPI_LIB_PATH)/ -ldnvqe
MPI_LIBS += -L $(MPI_LIB_PATH)/ -l_hidehaze
MPI_LIBS += -L $(MPI_LIB_PATH)/ -l_hiir_auto


CFLAGS += -Wall -O0 $(INCLUDE) -Wmisleading-indentation
CLFLAGS += -lpthread -lm -ldl  -lstdc++ -fno-aggressive-loop-optimizations -ffunction-sections -fdata-sections -O0 -fstack-protector -fPIC -Wno-date-time 

CXXFLAGS += $(CFLAGS)
CXXFLAGS += -std=c++14

TARGET := $(SRCS:%.c=%)

DECODE_TEST_TARGET = test

LIBS += -L ./Lib/media -lmedia
LIBS += -L ./Lib/sensor -lsns_imx327
LIBS += -L ./Lib/sensor -lsns_imx327_2L
LIBS += $(MPI_LIBS)

.PHONY : clean $(DECODE_TEST_TARGET)

test:
	rm -f $(DECODE_TEST_TARGET)
	$(CXX) -o $@ demo.cpp $(CXXFLAGS) $(CLFLAGS) -Wl,--start-group -lm -ldl $(LIBS) -Wl,--end-group

clean:
	@echo "Removing linked and compiled files......"
	rm -f $(DECODE_TEST_TARGET)

.SUFFIXES: .c .o .cpp 
 
.c.o:
	$(CXX) $(CFLAGS) $(INCLUDE) -c $*.c -o $*.o
  
