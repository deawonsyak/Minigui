PWD=`pwd`
ROOTDIR=$PWD/../..
TOOLCHAN_DIR=$ROOTDIR/tools/toolchain
SYSROOT=$ROOTDIR/sysroot
INSTALL_DIR=$ROOTDIR/libs

clean(){
	make maintainer-clean
	rm -rf configure ltmain.sh *.m4 autom4te.cache libtool
}

