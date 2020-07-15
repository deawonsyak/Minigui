source ../config.sh

echo "***** $INSTALL_DIR/include"
config(){
if [ ! -f configure ];then
		autoreconf -ivf
		automake --add-missing --foreign --copy
		clean
		autoreconf -ivf
	fi

	CC=$TOOLCHAN_DIR/bin/aarch64-linux-gcc \
	CXX=$TOOLCHAN_DIR/bin/aarch64-linux-g++ \
	CFLAGS="-I$TOOLCHAN_DIR/usr/include -I$TOOLCHAN_DIR/include -I$SYSROOT/usr/include -I$SYSROOT/usr/include/freetype2 -I$INSTALL_DIR/include" \
	CPPFLAGS="-I$TOOLCHAN_DIR/usr/include -I$TOOLCHAN_DIR/include -I$SYSROOT/usr/include -I$SYSROOT/usr/include/freetype2 -I$INSTALL_DIR/include" \
	PKG_CONFIG=$TOOLCHAN_DIR/bin/pkg-config \
	./configure \
		--host=arm-linux \
		--build=i386-linux  \
		--prefix=$INSTALL_DIR \

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
