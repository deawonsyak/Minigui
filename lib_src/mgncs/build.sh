source ../config.sh

config(){
	if [ ! -f configure ];then
		autoreconf -ivf
	fi
	CC=$TOOLCHAN_DIR/bin/aarch64-linux-gcc \
	CFLAGS="-I$TOOLCHAN_DIR/usr/include -I$TOOLCHAN_DIR/include -I$SYSROOT/usr/include " \
	LDFLAGS=-L$SYSROOT/usr/lib \
	CPPFLAGS="-I$TOOLCHAN_DIR/usr/include -I$TOOLCHAN_DIR/include -I$SYSROOT/usr/include " \
	PKG_CONFIG_PATH=$ROOTDIR/libs/lib/pkgconfig \
	./configure \
		--prefix=$INSTALL_DIR \
		--oldincludedir=$SYSROOT/usr/include \
		--host=aarch64-linux \
		--enable-dbxml=no \
		--build=i386-linux 

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


