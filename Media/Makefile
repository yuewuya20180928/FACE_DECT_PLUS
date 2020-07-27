platform := hisi3516dv300
export platform

ifeq ($(platform), hisi3516cv500)
	#3516cv500
	CROSS=arm-himix200-linux-
	CFLAGS += -DHisi3516cv500=1 -DVDEC_FUNC=0 -DVI_FUNC=1 -DVENC_FUNC=0 -DVO_FUNC=1
else ifeq ($(platform), hisi3516dv300)
	#3516dv300
	CROSS=arm-himix200-linux-
	CFLAGS += -DHisi3516cv500=1 -DVDEC_FUNC=0 -DVI_FUNC=1 -DVENC_FUNC=0 -DVO_FUNC=1
endif

releaeType=debug

CC	= @echo " $(CROSS)gcc	$@"; $(CROSS)gcc
CPP	= @echo " $(CROSS)g++	$@"; $(CROSS)gcc
LD	= @echo " $(CROSS)LD	$@"; $(CROSS)ld
AR	= @echo " $(CROSS)AR	$@"; $(CROSS)ar
STRIP	= @echo " $(CROSS)STRIP $@"; $(CROSS)strip
RM	= @echo " RM $(OBJS)"; rm -rf

ifneq ($(releaeType), debug)
CFLAGS += -Wall -fopenmp -O2 -Wunused
else
CFLAGS += -Wall -fopenmp -O -g -Wunused
endif

LDFLAGS += "-Wl",-lpthread -ldl -lm -fopenmp -L
AFLAGS += -r
LIBDIR = ./Lib/$(platform)
BINDIR = ./Bin/$(platform)

# 获取GIT版本信息
GIT_SHA = $(shell git rev-list HEAD | awk 'NR==1')
GIT_SEQ = $(shell git rev-list HEAD | wc -l)
GIT_VER_INFO = $(GIT_SHA)-$(GIT_SEQ)
GIT_SVR_PATH = $(shell git remote -v | awk 'NR==1' | sed 's/[()]//g' | sed 's/\t/ /g' | cut -d " " -f2)

ifneq ($(SVN_VER_INFO),)
CFLAGS += -DGIT_VERSION=\"$(SVN_VER_INFO)\"
else
CFLAGS += -DGIT_VERSION=\"unknown\"
endif
CFLAGS += -DGIT_PATH=\"$(SVN_SVR_PATH)\"

# 目标依赖关系 
LIB_TARGET = $(LIBDIR)/libmedia.so

DEMO_MEDIA_LIB_PATH = ./Demo/Lib/media

MODULE_NAME = decTest

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

DEBUG_SUFFIX = debug
COMPILE_PATH = $(BINDIR)/.objs

# 所有.cpp .c 文件名
LIB_SRCS_C += $(foreach dir,$(SRCS_PATH),$(wildcard $(dir)/*.c))
SRCS = $(LIB_SRCS_C)

# 所有 .o 文件名 
LIB_C_OBJS =$(patsubst %.c,${COMPILE_PATH}/%.o, ${LIB_SRCS_C})
LIB_OBJS +=$(LIB_C_OBJS)

# 创建编译临时文件目录 
$(shell mkdir -p ${LIBDIR})
$(shell mkdir -p ${BINDIR})
$(foreach dir,$(SRCS_PATH),$(shell mkdir -p ${COMPILE_PATH}/${dir}))

all: $(LIB_TARGET) demo

$(LIB_TARGET): $(LIB_OBJS)
	$(CC) -shared -fPIC -o $@ $^
	# -Wl,--start-group $(LIBS) -Wl,--end-group

demo:
	$(shell cp ${LIB_TARGET} ${DEMO_MEDIA_LIB_PATH} -f)
	make -C Demo

#compile
${COMPILE_PATH}/%.$(DEBUG_SUFFIX).o:%.cpp 
	$(CPP) -c $(CFLAGS) $(CFLAGS_DBG) $< -o $@

${COMPILE_PATH}/%.$(DEBUG_SUFFIX).o:%.c 
	$(CC) -c $(CFLAGS) $(CFLAGS_DBG) $< -o $@

${COMPILE_PATH}/%.o:%.cpp
	$(CPP) -c $(CFLAGS) $(CFLAGS_NDBG) $< -o $@

${COMPILE_PATH}/%.o:%.c
	$(CC) -c $(CFLAGS) $(CFLAGS_NDBG) $< -o $@

${COMPILE_PATH}/%.o:$(dir)/%.cpp
	$(CPP) -c $(CFLAGS) $(CFLAGS_NDBG) $< -o $@

${COMPILE_PATH}/%.o:$(dir)/%.c
	$(CC) -c $(CFLAGS) $(CFLAGS_NDBG) $< -o $@

#depend 	
${COMPILE_PATH}/%.c.d:%.cpp
	$(CC) $(CFLAGS) ${CFLAGS_NDBG} -MM -E $^ > $@
	@sed 's/.*\.o/$(subst /,\/, $(dir $@))&/g' $@ >$@.tmp
	@mv $@.tmp $@

${COMPILE_PATH}/%.cpp.d:%.cpp
	$(CPP) $(CFLAGS) ${CFLAGS_NDBG} -MM -E $^ > $@
	@sed 's/.*\.o/${subst /,\/, $(dir $@)}&/g' $@ >$@.tmp
	@mv $@.tmp $@

${COMPILE_PATH}/%.c.d.debug:%.cpp
	$(CC) $(CFLAGS) ${CFLAGS_DBG} -MM -E $^ > $@
	@sed 's/\(.*\)\.o/$(subst /,\/, $(dir $@))\1.${DEBUG_SUFFIX}.o/g' $@ >$@.tmp
	@mv $@.tmp $@

${COMPILE_PATH}/%.cpp.d.debug:%.cpp
	$(CPP) $(CFLAGS) ${CFLAGS_DBG} -MM -E $^ > $@
	@sed 's/\(.*\)\.o/$(subst /,\/, $(dir $@))\1.${DEBUG_SUFFIX}.o/g' $@ >$@.tmp
	@mv $@.tmp $@

check:
	cppcheck --enable=all --inconclusive --std=posix --xml-version=2 $(SRCS) 2> $(MODULE_NAME)_$(shell date +"%Y%m%d%H%M%S")_$(GIT_VER_INFO).xls

clean:
	$(RM) $(LIB_TARGET) $(TARGET) $(LIB_OBJS);

cleanobj:
	$(RM) $(LIB_OBJS);
