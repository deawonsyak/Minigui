
source ../config.sh

config(){
	if [ ! -f configure ];then
		autoreconf -ivf
		automake --add-missing --foreign --copy
		clean
		autoreconf -ivf
	fi

	CC=$TOOLCHAN_DIR/bin/aarch64-linux-gcc \
	CXX=$TOOLCHAN_DIR/bin/aarch64-linux-g++ \
	CFLAGS="-I$TOOLCHAN_DIR/usr/include -I$TOOLCHAN_DIR/include -I$SYSROOT/usr/include -I$SYSROOT/usr/include/freetype2 " \
	LDFLAGS=-L$SYSROOT/usr/lib \
	CPPFLAGS="-I$TOOLCHAN_DIR/usr/include -I$TOOLCHAN_DIR/include -I$SYSROOT/usr/include -I$SYSROOT/usr/include/freetype2 " \
	MGPLUS_CFLAGS=-I$SYSROOT/usr/include \
	PKG_CONFIG_PATH=$ROOTDIR/libs/lib/pkgconfig \
	./configure \
		--prefix=$INSTALL_DIR \
		--oldincludedir=$SYSROOT/usr/include \
		--host=aarch64-linux \
		--build=i386-linux 

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
exit $ret
