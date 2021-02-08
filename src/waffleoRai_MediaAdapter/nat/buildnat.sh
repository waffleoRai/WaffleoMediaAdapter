#!/bin/bash

echo -e "Setting environment variables..."

includedir=./include
srcdir=./src/waffleoRai_MediaAdapter/nat
objdir=./obj
bindir=./bin

MINGW32=i686-w64-mingw32-g++
MINGW64=x86_64-w64-mingw32-g++

MINGW64_DIR="/usr/x86_64-w64-mingw32/sys-root/mingw"
WINLIB_DIR="/home/bghospe/winlib/10/Lib/10.0.18362.0/um"

CXXFLAGS_LIN32="-Wall -O3 -m32 -std=c++11 -pedantic"
CXXFLAGS_LINUX="-Wall -O3 -m64 -std=c++11 -pedantic"
CXXFLAGS_WIN32="-Wall -O3 -m32 -std=c++11 -pedantic"
CXXFLAGS_WIN64="-Wall -O3 -m64 -std=c++11 -pedantic"

#-Wl,-out_implib,libwrma.a
LINKFLAGS_GCC="-Wall -m64 -pedantic -shared -fpic"
LINKFLAGS_MINGW64="-Wall -m64 -pedantic -shared -Wl,-export-all-symbols -Wl,-enable-auto-image-base -L ${MINGW64_DIR}/lib -L ${WINLIB_DIR}/x64" 

LIBLIST_LINUX="-lFLAC++"
LIBLIST_MINGW="-lFLAC++"

LINUX_LIB_DIR=/usr/lib/x86_64-w64-linux-gnu

FLAC_FILENAME=waffleoRai_MediaAdapter_audio_FLACEncoder
MAUTILS_FILENAME=mediaadapterutils

JNI_INCLUDEDIR=/usr/lib/jvm/java-8-openjdk-amd64/include
if [ -n ${CPATH} ]; then
	CPATH=${CPATH}${JNI_INCLUDEDIR}
else
	CPATH={JNI_INCLUDEDIR}
fi
echo -e ${CPATH}

#--- Linux 64
echo -e "Building Linux x86(64-bit)"
#Variables
OBJ_TARG_DIR=${objdir}/linux64
LIB_TARG_DIR=${bindir}/linux64

#Build
g++ ${CXXFLAGS_LINUX} -I ${includedir} -I ${includedir}/jni -I ${JNI_INCLUDEDIR} -I ${JNI_INCLUDEDIR}/linux -c ${srcdir}/${MAUTILS_FILENAME}.cpp -o ${OBJ_TARG_DIR}/${MAUTILS_FILENAME}.o
g++ ${CXXFLAGS_LINUX} -I ${includedir} -I ${includedir}/jni -I ${JNI_INCLUDEDIR} -I ${JNI_INCLUDEDIR}/linux -c ${srcdir}/${FLAC_FILENAME}.cpp -o ${OBJ_TARG_DIR}/${FLAC_FILENAME}.o

#Link
g++ ${LINKFLAGS_GCC} -o ${LIB_TARG_DIR}/libwrma.so ${OBJ_TARG_DIR}/${FLAC_FILENAME}.o ${LIBLIST_LINUX}
ldd ${LIB_TARG_DIR}/libwrma.so

#--- Linux 32
#echo -e "Building Linux x86(32-bit)"
#Variables
OBJ_TARG_DIR=${objdir}/linux32
LIB_TARG_DIR=${bindir}/linux32

#Build
g++ ${CXXFLAGS_LIN32} -I ${includedir} -I ${includedir}/jni -I ${JNI_INCLUDEDIR} -I ${JNI_INCLUDEDIR}/linux -c ${srcdir}/${MAUTILS_FILENAME}.cpp -o ${OBJ_TARG_DIR}/${MAUTILS_FILENAME}.o
g++ ${CXXFLAGS_LIN32} -I ${includedir} -I ${includedir}/jni -I ${JNI_INCLUDEDIR} -I ${JNI_INCLUDEDIR}/linux -c ${srcdir}/${FLAC_FILENAME}.cpp -o ${OBJ_TARG_DIR}/${FLAC_FILENAME}.o

#Link
#Hm, I don't have 32-bit libraries to work with.
#g++ ${CXXFLAGS_LIN32} -shared -fpic -lFLAC++ -o ${LIB_TARG_DIR}/libwrma.so ${OBJ_TARG_DIR}/${FLAC_FILENAME}.o
#ldd ${LIB_TARG_DIR}/libwrma.so

#--- Win 64
echo -e "Building Windows x86(64-bit)"
#Variables
OBJ_TARG_DIR=${objdir}/win64
LIB_TARG_DIR=${bindir}/win64
INCLUDEDIR_MINGW64=/usr/x86_64-w64-mingw32/sys-root/mingw/include
JNI_INCLUDEDIR_WIN=${INCLUDEDIR_MINGW64}/jni

#Build
x86_64-w64-mingw32-g++ ${CXXFLAGS_WIN64} -I ${includedir} -I ${includedir}/jni -I ${JNI_INCLUDEDIR_WIN} -I ${JNI_INCLUDEDIR_WIN}/win32 -I ${INCLUDEDIR_MINGW64} -c ${srcdir}/${MAUTILS_FILENAME}.cpp -o ${OBJ_TARG_DIR}/${MAUTILS_FILENAME}.o
x86_64-w64-mingw32-g++ ${CXXFLAGS_WIN64} -I ${includedir} -I ${includedir}/jni -I ${JNI_INCLUDEDIR_WIN} -I ${JNI_INCLUDEDIR_WIN}/win32 -I ${INCLUDEDIR_MINGW64} -c ${srcdir}/${FLAC_FILENAME}.cpp -o ${OBJ_TARG_DIR}/${FLAC_FILENAME}.o

#Link
x86_64-w64-mingw32-g++ ${LINKFLAGS_MINGW64} -v -o libwrma.dll ${OBJ_TARG_DIR}/${FLAC_FILENAME}.o ${OBJ_TARG_DIR}/${MAUTILS_FILENAME}.o ${LIBLIST_MINGW}

#--- Win 32
echo -e "Building Windows x86(32-bit)"
#Variables
OBJ_TARG_DIR=${objdir}/win32
LIB_TARG_DIR=${bindir}/win32
INCLUDEDIR_MINGW32=/usr/i686-w64-mingw32/sys-root/mingw/include
JNI_INCLUDEDIR_WIN=${INCLUDEDIR_MINGW32}/jni

#Build
i686-w64-mingw32-g++ ${CXXFLAGS_WIN32} -I ${includedir} -I ${includedir}/jni -I ${JNI_INCLUDEDIR_WIN} -I ${JNI_INCLUDEDIR_WIN}/win32 -I ${INCLUDEDIR_MINGW32} -c ${srcdir}/${MAUTILS_FILENAME}.cpp -o ${OBJ_TARG_DIR}/${MAUTILS_FILENAME}.o
i686-w64-mingw32-g++ ${CXXFLAGS_WIN32} -I ${includedir} -I ${includedir}/jni -I ${JNI_INCLUDEDIR_WIN} -I ${JNI_INCLUDEDIR_WIN}/win32 -I ${INCLUDEDIR_MINGW32} -c ${srcdir}/${FLAC_FILENAME}.cpp -o ${OBJ_TARG_DIR}/${FLAC_FILENAME}.o

#Link