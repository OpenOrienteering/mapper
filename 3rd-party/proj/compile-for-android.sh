#
# Cross-compile script to compile Proj.4 as static libs for Android.
# Mostly taken from the QGIS mobile project.
#


# --- Settings ---

#WARNING: armeabi-v7a builds don't work on android emulators pre api 14
DEFAULT_ANDROID_ABI=armeabi-v7a #or armeabi
ARCH=arch-arm
#passing ANDROID_ABI=armeabi-v7a when calling a script will override the default
#for example ANDROID_ABI=armeabi-v7a ./build-all.sh
# Possible values:
# ANDROID_ABI : "armeabi-v7a", "armeabi", "armeabi-v7a with NEON", "armeabi-v7a with VFPV3", "armeabi-v6 with VFP", "x86", "mips"
#WARNING tested only with "armeabi-v7a", "armeabi"
#TODO somehow support ANDROID_ABI with spaces in names since $ANDROID_ABI is used often in mkdir
#see ascripts/android.toolchain.cmake for details 

export ANDROID_SDK_ROOT=/opt/android-sdk
export ANDROID_NDK_ROOT=/opt/android-ndk

export ANDROID_NDK_TOOLCHAIN_PREFIX=arm-linux-androideabi
export ANDROID_NDK_TOOLCHAIN_VERSION=4.8
export ANDROID_NDK_HOST=linux-x86_64 # e.g. linux-x86_64, darwin-x86_64

export ANDROID_LEVEL=9

# --- Settings end ---


# --- Settings processing ---

if [ ! -n "${ANDROID_ABI+x}" ]; then
  export ANDROID_ABI=$DEFAULT_ANDROID_ABI
fi


# --- Make standalone toolchain ---

ANDROID_STANDALONE_TOOLCHAIN=/tmp/my-android-$ANDROID_LEVEL-toolchain-$ANDROID_ABI
$ANDROID_NDK_ROOT/build/tools/make-standalone-toolchain.sh --system=$ANDROID_NDK_HOST --toolchain=$ANDROID_NDK_TOOLCHAIN_PREFIX-$ANDROID_NDK_TOOLCHAIN_VERSION --platform=android-$ANDROID_LEVEL --install-dir=$ANDROID_STANDALONE_TOOLCHAIN


# --- Set environment ---

export PATH=$ANDROID_STANDALONE_TOOLCHAIN/bin:$ANDROID_SDK_ROOT/tools:$ANDROID_SDK_ROOT/platform-tools:$PATH

export CC=$ANDROID_NDK_TOOLCHAIN_PREFIX-gcc
export CXX=$ANDROID_NDK_TOOLCHAIN_PREFIX-g++
export LD=$ANDROID_NDK_TOOLCHAIN_PREFIX-ld
export AR=$ANDROID_NDK_TOOLCHAIN_PREFIX-ar
export RANLIB=$ANDROID_NDK_TOOLCHAIN_PREFIX-ranlib
export AS=$ANDROID_NDK_TOOLCHAIN_PREFIX-as 

export INSTALL_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/out/$ANDROID_ABI"
export MY_STD_CONFIGURE_FLAGS="--prefix=$INSTALL_DIR --host=$ANDROID_NDK_TOOLCHAIN_PREFIX --enable-shared=false"

#FPU (floating point unit)FLAG:
#see http://gcc.gnu.org/onlinedocs/gcc-4.4.3/gcc/ARM-Options.html
#see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0408f/index.html
#see NDK docs/STANDALONE-TOOLCHAIN.html
#for generic MY_FPU=vfp
MY_FPU=vfp
#for tegra processors (xoom, transformer, iconia, galaxy tab)
#MY_FPU=vfpv3-d16
#for hummingbird and neon supporting processors (galaxy phone) -mfpu=neon
#MY_FPU=neon
if [ "$ANDROID_ABI" = "armeabi-v7a with VFPV3" ]; then
  MY_FPU=vfpv3
elif [ "$ANDROID_ABI" = "armeabi-v7a with NEON" ]; then
  MY_FPU=neon
fi
ARMV7A_flags=" -march=armv7-a -mfloat-abi=softfp -mfpu=$MY_FPU"

#build mthumb instructions only in non Debug mode
if [ "$BUILD_TYPE" == "Debug" ]; then
    extraFlags="-g -O0 -marm -finline-limit=300 -fno-strict-aliasing -fno-omit-frame-pointer -DDEBUG -D_DEBUG"
else
    extraFlags="-mthumb -O2"
fi

#-std=gnu99: http://comments.gmane.org/gmane.comp.handhelds.android.ndk/8732
#-std=c99: http://stackoverflow.com/questions/9247151/compiling-icu-for-android-uint64-t-does-not-name-a-type
#http://stackoverflow.com/questions/5135734/whats-the-difference-in-gcc-between-std-gnu0x-and-std-c0x-and-which-one-s
export MY_STD_CFLAGS="-DANDROID -Wno-psabi -std=c99 $extraFlags"
if [ "$ANDROID_ABI" = "armeabi-v7a" ]; then
  export MY_STD_CFLAGS=$MY_STD_CFLAGS$ARMV7A_flags
fi
export MY_STD_CXXFLAGS="-DANDROID -Wno-psabi -std=gnu++0x $extraFlags"
export MY_STD_LDFLAGS="-Wl,--fix-cortex-a8"


# Detect cpu cores
CORES=$(cat /proc/cpuinfo | grep processor | wc -l)
export CORES=$(($CORES+1))


# --- Information output ---

echo "BUILDING ANDROID LIBS"
echo "SRC location: " $SRC_DIR
echo "INSTALL location: " $INSTALL_DIR
echo "NDK location: " $ANDROID_NDK_ROOT
echo "Standalone toolchain location: " $ANDROID_STANDALONE_TOOLCHAIN
echo "PATH:"
echo $PATH
echo "CFLAGS: " $MY_STD_CFLAGS
echo "CXXFLAGS: " $MY_STD_CXXFLAGS
echo "LDFLAGS: " $MY_STD_LDFLAGS

echo "USING FOLLOWING TOOLCHAIN"
echo "CC: " `which $CC`
echo "CXX: " `which $CXX`
echo "LD: " `which $LD`
echo "AR: " `which $AR`
echo "RANLIB: " `which $RANLIB`
echo "AS: " `which $AS`
echo "GCC -v: " `$CC -v`


# --- Build Proj.4 (static libs) ---

mkdir -p build-$ANDROID_ABI
cd build-$ANDROID_ABI

CFLAGS=$MY_STD_CFLAGS \
CXXFLAGS=$MY_STD_CXXFLAGS \
LDFLAGS=$MY_STD_LDFLAGS \
../configure $MY_STD_CONFIGURE_FLAGS

make -j$CORES 2>&1 | tee make.out
make 2>&1 install | tee makeInstall.out