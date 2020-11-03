CC=arm-himix200-linux-gcc
CXX=arm-himix200-linux-g++

INCLUDE = -I ../output \
		  -I ./include/json \
		  -I ./include \

#海思平台库

OPENLIBS += -L ./Lib/json -ljsoncpp

CFLAGS += -Wall -O2 $(INCLUDE) -Wmisleading-indentation
CLFLAGS += -lpthread -lm -ldl  -lstdc++ -fno-aggressive-loop-optimizations -ffunction-sections -fdata-sections -O0 -fstack-protector -fPIC -Wno-date-time 

CXXFLAGS += $(CFLAGS)
CXXFLAGS += -std=c++14

TARGET := $(SRCS:%.c=%)

DECODE_TEST_TARGET = test

LIBS += -L ./Lib/media -lDspApi
LIBS += $(MPI_LIBS) $(OPENLIBS)

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
  
