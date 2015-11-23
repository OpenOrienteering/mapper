This package is for building and installing GNU libstdc++ for the Android
NDK and for providing the corresponding source code.

It is to be used by doing an out-of-source build configured by qmake from the
supplied gnu-libstdc++.pro. You need to set ANDROID_NDK_ROOT, e.g. by choosing
an Android kit in Qt Creator.

The GCC 4.9 libstdc++ source code will be taken from

  https://android.googlesource.com/toolchain/gcc.git

The selected git revision will be documented in the file REVISION.

Build scripts and gcc patches will be taken from

  /PATH/TO/NDK/build/tools/

If it exists, a patch file build-tools-[NDK_REVISION].patch will be applied to
the build scripts and patches. Than, the gcc patches are applied to the source
code.
