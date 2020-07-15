
#PWD="$( cd "$( dirname "$0"  )" && pwd  )"
PWD=`pwd`

ROOTDIR=$PWD

clean(){
	rm -rf out out.tar 
	cd app 
	make clean
	cd -

	cd lib_src
	for lib in `ls`
	do
		if [ -d $lib ];then
			cd $lib
			./build.sh clean
			cd -
		fi

	done

}

build_libs(){
	libs="minigui mgplus mgutils mgeff"
	for lib in $libs
	do
		echo $lib
		if [ -d $ROOTDIR/lib_src/$lib ];then
			echo "is dir"
			
			cd lib_src/$lib
			./build.sh
			ret=$?
			if [ "$ret" != "0" ];then
				echo "********** Build $lib fail!!!************"
				exit
			fi
			cd $ROOTDIR
		fi
	done
}

if [ "$1" == "all_libs" ];then
	build_libs
	exit
fi

if [ "$1" == "clean" ];then
	clean
	exit
fi

if [  -d "app/extern/session_manager" ];then
	cd app/extern/session_manager
	git pull
	cd -
else
	cd app/extern
	git clone https://gitee.com/zhuxunhua/session_manager.git
	cd -
fi

cd app
make
if [ "$?" != "0" ];then
	exit
fi
cd -

rm -rf out
mkdir out
mkdir out/lib
cp app/test out/gui
cp libs/lib/*.so* out/lib -d
cp ./res/config/* out
cp ./res/xml/ out -rf
cp ./res/res out -r

tar -cf out.tar out

cp out.tar /home/disk1/tftp
