
source ../config.sh

config(){
	if [ ! -f configure ];then
		autoreconf -ivf
	fi

	CC=$TOOLCHAN_DIR/bin/aarch64-linux-gcc \
	CFLAGS="-I$TOOLCHAN_DIR/usr/include -I$TOOLCHAN_DIR/include -I$SYSROOT/usr/include -DENABLE_RGA=1 " \
	PKG_CONFIG=$TOOLCHAN_DIR/bin/pkg-config \
	PKG_CONFIG_PATH=$SYSROOT/usr/lib/pkgconfig \
	./configure \
		--host=aarch64-linux \
		--prefix=$INSTALL_DIR \
		--build=i386-linux \
		--with-osname=linux \
		--disable-videopcxvfb \
		--with-ttfsupport=ft2 \
		--with-ft2-includes=$SYSROOT/usr/include/freetype2 \
		--disable-autoial \
		--disable-vbfsupport \
		--disable-textmode \
		--enable-vbfsupport \
		--disable-pcxvfb \
		--disable-dlcustomial \
		--disable-dummyial \
		--disable-jpgsupport \
		--disable-fontcourier \
		--disable-screensaver \
		--enable-jpgsupport \
		--enable-pngsupport=yes \
		--disable-fontsserif \
		--disable-fontsystem \
		--disable-flatlf \
		--disable-skinlfi \
		--disable-dblclk \
		--disable-consoleps2 \
		--disable-consolems \
		--disable-consolems3 \
		--disable-rbfterminal \
		--disable-rbffixedsys \
		--disable-vbfsupport \
		--disable-splash \
		--enable-videoshadow \
		--disable-static \
		--enable-shared \
		--disable-procs \
		--with-runmode=ths \
		--disable-incoreres \
		--disable-cursor \
		--enable-mousecalibrate \
		--with-pic \
		--enable-videodrmcon \
		--disable-videofbcon \
		--enable-pixman \
		--with-targetname=drmcon \
		--enable-tslibial=yes \
		--enable-detaildebug=yes

	ret=$?
	if [ "$ret" != "0" ];then
		exit $ret
	fi
}

if [ "$1" == "clean" ];then
	clean
	exit
fi

if [ "$1" == "config" ];then
	config
	exit
fi

if [ ! -f Makefile ];then
	config
fi

make install
ret=$?
echo "ret:$ret"
exit $ret

