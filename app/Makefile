
#CC = ../tools/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc

#CC = ../tools/toolchain/bin/aarch64-linux-gcc
PWD=$(shell pwd)
TARGET_OUTPUT_DIR := $(PWD)/../tools/toolchain
COLOR_COMPILE=$(shell if [ -f /usr/local/bin/color_compile ];then echo "color_compile"; else echo ""; fi;)
CC = $(COLOR_COMPILE) $(TARGET_OUTPUT_DIR)/bin/aarch64-linux-gcc -g
AR=$(TARGET_OUTPUT_DIR)/bin/aarch64-linux-ar
SYSROOT=../sysroot
 
ifeq (,$(CC))
	CC = gcc
endif

MAKE = make
 
PWD = $(shell pwd)
SOURCE = $(wildcard src/*.c)                 # 获取当前文件夹下所有.c文件
SOURCE += $(wildcard src/widget/*.c)                 # 获取当前文件夹下所有.c文件
SOURCE += $(wildcard src/page/*.c)                 # 获取当前文件夹下所有.c文件
SOURCE += $(wildcard src/tools/*.c)                 # 获取当前文件夹下所有.c文件

export CC AR

OBJS = $(patsubst %.c,%.o,$(SOURCE))     # 把 SOURCE 中的.c 改成.o赋值给OBJS
 

CFLAGS = -I./inc 
CFLAGS += -I./inc/tools
CFLAGS += -I./inc/widget
CFLAGS += -I./inc/page
CFLAGS += -I./extern/session_manager/inc
CFLAGS += -I./extern/c_tools/out/include
CFLAGS += -I./extern/c_tools/out/include/orb_platform
CFLAGS += -I../libs/include
CFLAGS += -I../libs/include/orb_platform
CFLAGS += -I$(SYSROOT)/usr/include/libxml2/

LDFLAG = -L../libs/lib
LDFLAG += -L./extern/session_manager
LDFLAG += -L./extern/c_tools/out/lib
LDFLAG += -L$(SYSROOT)/usr/lib
LDFLAG += -lminigui_ths -lmgplus -lpthread -lpng -lm -lfreetype -ljpeg -lz -lrga -lxml2 -ldrm -lts -lorb_platform -lcJSON -zlog -lsn_script -lvicenter_service -lcJSON -lzlog -lorb_gdbus -lgmodule-2.0 -lglib-2.0 -lgio-2.0 -lgobject-2.0 -lcli
#LDFLAG += -lminigui_ths -lpthread -lpng -lm -lfreetype -ljpeg -lz -lrga -lmgutils -lxml2 -ldrm -lts
#LDFLAG += -lminigui_ths -lmgplus -lmgncs -lpthread -lpng -lm -lfreetype -ljpeg -lz -lrga -lmgutils -lxml2 -lpixman-1 -ldrm -lts
#LDFLAG += -lpthread -lminigui_ths -ljpeg -lpng -lm  -lfreetype -ldrm -lts

TARGET=test

 
all: $(TARGET)

.PHONY: memstat
memstat:
	make -C ./extern/memstattool
 
$(TARGET):   $(OBJS)
	make -C ./extern/session_manager
	$(CC) -o test $(OBJS) $(LDFLAG)
 
test.a: $(OBJS)
	$(AR) -rc $@ $^
 
%.o: %.c                                
	$(CC) -o $@ -c $< $(CFLAGS)                 
 
.PHONY: clean                              # 为目标
clean:
	make clean -C ./extern/session_manager
	@rm *.o -f
	@rm $(OBJS) -rf
	@rm $(TARGET)
