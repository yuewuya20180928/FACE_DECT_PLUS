AR = arm-himix200-linux-ar
CC = arm-himix200-linux-gcc

LIBPATH := ./lib
OBJPATH := ./obj

ARFLAGS = rcv
ARFLAGS_SO += -shared -fPIC -o
CFLAGS  += -Wall -fPIC -O2 -g -lm

CUR_INC := ./

REL_INC := ../../Include/hisi/hisi3516dv300/mpp

$(warning $(REL_INC))
 
INC := -I$(CUR_INC) -I$(REL_INC)

DEPEND_FILE := $(foreach file,$(subst -I, ,$(CUR_INC)), $(wildcard $(file)/*.h))

COMPILE = $(CC) $(CFLAGS)

$(OBJPATH)/%.o: ./%.c $(DEPEND_FILE)
	@[ -e $(LIBPATH) ] || mkdir -p $(LIBPATH)
	@[ -e $(OBJPATH) ] || mkdir -p $(OBJPATH)
	@$(COMPILE) -o $@ -c $< $(INC)

SRCS = $(wildcard ./*.c)
OBJS = $(SRCS:%.c=%.o)
OBJS := $(OBJS:./%=obj/%)

TARGETLIB := $(LIBPATH)/libsns_imx327_2L.a
TARGETLIB_SO := $(LIBPATH)/libsns_imx327_2L.so

all:$(TARGETLIB)
$(TARGETLIB):$(OBJS)
	@($(AR) $(ARFLAGS) $(TARGETLIB) $(OBJS))
	@($(CC) $(ARFLAGS_SO) $(TARGETLIB_SO) $(OBJS))

clean:
	@$(RM) -rf $(TARGETLIB) $(OBJS)
	@$(RM) -rf $(LIBPATH) $(OBJPATH)

aa:
	echo "$(INC)"
