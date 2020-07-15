#prefix=/opt/em86/arm-elf-uclibc.69/arm-elf/lib
#path= /opt/em86/arm-elf-uclibc.69/lib/gcc-lib/arm-elf/2.95.3
AM_CFLAGS=-I../../include/ -I../../glib
noinst_PROGRAMS=mspider mspider_stb

mspider_SOURCES=emspider_example.c emspider_proxy.c  emspider-res.h new_toolbar.c proxy_box_res.c \
                promptbox.c
mspider_stb_SOURCES=emspider_stb.c emspider-res.h new_toolbar.c promptbox.c

if BUILD_FLASH
flash_LDADD =  ../../flash/libflash.la ../../mpegsound/libmpegsound.la -lstdc++
else
flash_LDADD = 
endif

mspider_LDADD = ../../src/libemspider.la ../../glib/libglib.a $(flash_LDADD) $(SYSTEM_LIBS)
 
mspider_stb_LDADD = ../../src/libemspider.la ../../glib/libglib.a $(flash_LDADD) $(SYSTEM_LIBS)

# the following is for porting to em86
#mspider_LDADD= ../../src/libemspider.la ../../src/libnet.la ../../glib/libglib.a ${prefix}/libminigui.a ${prefix}/libmgext.a ${prefix}/libjpeg.a  ${prefix}/libjs.a ${path}/libstdc++.a -lm -lpthread  -lhz

#mspider_LDFLAGS=-static

