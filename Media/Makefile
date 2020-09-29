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

LIBDIR = ./Lib/$(platform)
BINDIR = ./Bin/$(platform)

# 目标依赖关系 
LIB_TARGET = $(LIBDIR)/libmedia.so

DEMO_MEDIA_LIB_PATH = ./Demo/Lib/media

# 头文件路径
LIBS_INC += -I ./Include \
			-I ./Include/hisi/$(platform)/mpp \
			-I ./common \
			-I ./output \
			-I ./Src \

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

all: $(LIB_TARGET) demo

$(LIB_TARGET): $(LIB_OBJS)
	$(CC) -shared -fPIC $(CFLAGS) -o $@ $^
	# -Wl,--start-group $(LIBS) -Wl,--end-group

demo:
	$(shell cp ${LIB_TARGET} ${DEMO_MEDIA_LIB_PATH} -f)
	make -C Demo

${COMPILE_PATH}/%.o:%.c
	$(CC) -c $(CFLAGS) $(CFLAGS_NDBG) $< -o $@

clean:
	$(RM) $(LIB_TARGET) $(TARGET) $(LIB_OBJS);

cleanobj:
	$(RM) $(LIB_OBJS);
