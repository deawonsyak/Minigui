SUBDIRS=io js

noinst_LTLIBRARIES=libemspider.la libnet.la

INCS = -I./ -I../ -I../include -I../include/js -I../glib -I./js -I../flash

AM_CFLAGS = $(INCS)
libemspider_la_SOURCES=browser.h dw_button.c dw_button.h \
             dw_viewport.c dw_viewport.h \
             dw_hruler.c dw_hruler.h \
             dw_list_item.c dw_list_item.h dw_bullet.c dw_bullet.h \
             dw_table.c dw_table.h dw_table_cell.c dw_table_cell.h \
             dw_aligned_page.c dw_aligned_page.h \
             dw_style.c dw_style.h dw_page.c dw_page.h \
             dw_container.c dw_container.h linktrave.c linktrave.h \
             dw_ext_iterator.c dw_ext_iterator.h findtext.c findtext.h \
             dw_marshal.c dw_marshal.h dw_widget.c dw_widget.h dw.c dw.h \
             html.c html.h binaryconst.h colors.c colors.h \
             progressbar.c progressbar.h \
             prefs.c prefs.h mgwidget.h mgwidget.c \
             msg.h list.h debug.h p_window.c p_window.h \
             image.c image.h dw_image.c dw_image.h \
             gif.c jpeg.c png.c bmp.c plain.c \
             emspider.c emspider.h interface.c interface.h frame.c frame.h\
             frameset.c frameset.h dw_tooltip.h dw_tooltip.c doc.c doc.h \
             marquee.c marquee.h mflash.c mflash.h \
             emspider_init.c


libemspider_la_LIBADD=libnet.la io/libio.la js/libejs.la

libnet_la_SOURCES= auth.c auth.h dns.c dns.h dicache.c dicache.h chain.c chain.h \
            cache.c cache.h url.c url.h web.c web.h \
            nav.c nav.h klist.c klist.h misc.c misc.h \
	        cookies.c cookies.h history.c history.h bitvec.c bitvec.h \
            capi.c capi.h



#noinst_PROGRAMS=nettrial
#nettrial_SOURCES=nettrial.c 
#nettrial_LDADD=libnet.la ../glib/libglib.a
