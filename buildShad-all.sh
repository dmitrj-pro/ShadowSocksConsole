#!/bin/bash

BIN_NAME="ShadowSocksConsole"
PATH_FOLDER="Example/ShadowSocksGUI/"
ARCH=

while [ -n "$1" ]; do
	case "$1" in
		x86) 
			ARCH="x86" ;;
		--x86)
			ARCH="x86" ;;
		-h)
			printf " -h | --help\t\t write this information\n"
			printf " x86 | --x86\t\tSet x86 arch (Windows)"
			exit 0
		shift ;;
		*) echo "Unknow parametr $1"
		esac
		shift
done


if [ "$WINDIR" ]; then
	MAKE_BIN="mingw32-make.exe"
	export CC="gcc.exe"
	EXE_FORMAT=".exe"
	GIT_EXE="git.exe"
else
	MAKE_BIN="make"
	EXE_FORMAT=
	GIT_EXE="git"
fi

echo "....Update Source"
$GIT_EXE reset --hard
$GIT_EXE pull

echo "....Build Builder"
cd builder
rm -f ./pmake${EXE_FORMAT}
../build.sh release $ARCH
rm -f ../pmake${EXE_FORMAT}
mv ./pmake${EXE_FORMAT} ../
cd ..

echo "....Build DPLib"
rm -f libdp.a
./build.sh release $ARCH


echo "....Build www-generator"
cd Example/ShadowSocksGUI/www-generator
rm -f generate-html${EXE_FORMAT}
../../../pmake${EXE_FORMAT}
../../../build.sh release $ARCH
strip generate-html${EXE_FORMAT}
rm -f ../../../generate-html${EXE_FORMAT}
mv ./generate-html${EXE_FORMAT} ../../../
cd ../../../

echo "....Generate Makefile"
cd $PATH_FOLDER
rm -f ${BIN_NAME}${EXE_FORMAT}
../../pmake${EXE_FORMAT}

echo "....Build"
../../build.sh release $ARCH
strip ${BIN_NAME}${EXE_FORMAT}
mv ./${BIN_NAME}${EXE_FORMAT} ../../