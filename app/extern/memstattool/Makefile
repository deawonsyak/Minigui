#
# unisound/Makefile
#
# Unisound 2013 Unisound, Inc..
# All Rights Reserved.
# Unisound Confidential and Proprietary.
#

PWD := $(shell pwd)
#DEMO_LIB = $(PWD)/lib/*.a
#$(PWD)/lib/libfireair.a
#LIB = $(PWD)/lib

default: all

DEMO_NAME = memstat

COPTS := -Wall -O2 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -Wpointer-arith -Wundef -Werror \
        -pipe -fno-caller-saves 

INCLUDES :=\
	-I. \
	-I$(GCFG_PATH)/inc \
	-I$(APP_PATH)/inc/session/ \
	-I$(APP_PATH)/inc/ \
	


CFLAGS = $(COPTS) $(INCLUDES)

UNI_CSRC := .
CSRCS := $(wildcard $(UNI_CSRC)/*.c) 
#CSRCS += $(wildcard $(GCFG_PATH)/src/*.c)
OBJS := $(CSRCS:%.c=%.o)
DEPS := $(CSRCS:%.c=%.d)

LDFLAGS =  \
	-L.\
	-L$(C600_SDK_LIB) \
	-Wl,--start-group \
	-L$(GCFG_PATH)/lib \


ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),mkdbg)
ifdef DEPS
sinclude $(DEPS)
endif
endif
endif

all:  $(DEMO_NAME)

$(DEMO_NAME): $(OBJS) 
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o:%.c
	$(CC) $(INCLUDES) -o $@ -c $<

clean:
	@$(foreach d, $(SUBDIRS), make -C $(d) clean;)
	@$(RM) $(UNI_CSRC)/*.o
	@$(RM) $(DEMO_NAME)
	@$(RM) $(GCFG_PATH)/src/*.o

