platform := hisi3516dv300
export platform

CROSS = arm-himix200-linux-

releaeType=debug

CC	= @echo " $(CROSS)gcc	$@"; $(CROSS)gcc
LD	= @echo " $(CROSS)LD	$@"; $(CROSS)ld
AR	= @echo " $(CROSS)AR	$@"; $(CROSS)ar
STRIP	= @echo " $(CROSS)STRIP $@"; $(CROSS)strip
RM	= @echo " RM $(OBJS)"; rm -rf

CFLAGS += -Wall -O2 -Wunused -lpthread

ifeq ($(releaeType), debug)
CFLAGS += -g
endif

LIBDIR = ./output/
BINDIR = ./Bin/$(platform)

# 目标依赖关系 
LIB_TARGET = $(LIBDIR)/dspzx

# 头文件路径
LIBS_INC += -I ./Include \
			-I ./Include/hisi/$(platform)/mpp \
			-I ./Common \
			-I ./output \
			-I ./Src \
			-I ./Include/SDL2 \
			-I ./Include/SDL2_ttf \

CFLAGS += $(LIBS_INC)

# 源文件路径 
SRC_ROOT_PATH += ./Src 

SRCS_PATH = $(shell find $(SRC_ROOT_PATH) -maxdepth 3 -type d) 

COMPILE_PATH = $(BINDIR)/.objs

# 所有 .c 文件名
LIB_SRCS_C += $(foreach dir,$(SRCS_PATH),$(wildcard $(dir)/*.c))

# 所有 .o 文件名 
LIB_C_OBJS =$(patsubst %.c,${COMPILE_PATH}/%.o, ${LIB_SRCS_C})
LIB_OBJS +=$(LIB_C_OBJS)

# 创建编译临时文件目录 
$(shell mkdir -p ${LIBDIR})
$(shell mkdir -p ${BINDIR})
$(foreach dir,$(SRCS_PATH),$(shell mkdir -p ${COMPILE_PATH}/${dir}))

MPI_LIB_PATH := ./Lib/hisi3516dv300/hi_mpp

MPI_LIBS += -L $(MPI_LIB_PATH)/ -laaccomm
MPI_LIBS += -L $(MPI_LIB_PATH)/ -laacenc
MPI_LIBS += -L $(MPI_LIB_PATH)/ -laacdec
MPI_LIBS += -L $(MPI_LIB_PATH)/ -l_hicalcflicker
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lmd
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lnnie
MPI_LIBS += -L $(MPI_LIB_PATH)/ -l_hiae
MPI_LIBS += -L $(MPI_LIB_PATH)/ -l_hiawb_natura
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lhi_cipher
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lmpi
MPI_LIBS += -L $(MPI_LIB_PATH)/ -ltde
MPI_LIBS += -L $(MPI_LIB_PATH)/ -lhdmi
#MPI_LIBS += -L $(MPI_LIB_PATH)/ -lsvpruntime
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

LIBS += -L ./output -lsns_imx327
LIBS += -L ./output -lsns_imx327_2L
LIBS += -L ./output -lsns_gc2145
LIBS += -L ./Lib/SDL2_ttf -lSDL2_ttf
LIBS += -L ./Lib/SDL2 -lSDL2
LIBS += -L ./Lib/freetype -lfreetype
LIBS += -L ./Lib/png -lpng
LIBS += -L ./Lib/z -lz
LIBS += $(MPI_LIBS)

all: $(LIB_TARGET)

$(LIB_TARGET): $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -Wl,--start-group -lm -ldl $(LIBS) -Wl,--end-group
	# -Wl,--start-group $(LIBS) -Wl,--end-group

${COMPILE_PATH}/%.o:%.c
	$(CC) -c $(CFLAGS) $(CFLAGS_NDBG) $< -o $@

clean:
	$(RM) $(LIB_TARGET) $(TARGET) $(LIB_OBJS);

cleanobj:
	$(RM) $(LIB_OBJS);
