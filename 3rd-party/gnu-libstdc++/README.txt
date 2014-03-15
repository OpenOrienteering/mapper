This package is for building and installing GNU libstdc++ for the Android
NDK and for providing the corresponding source code.

It is to be used by doing an out-of-source build configured by qmake from the
supplied gnu-libstdc++.pro. You need to set ANDROID_NDK_ROOT, e.g. by choosing
an Android kit in Qt Creator.

The GCC 4.8 libstdc++ source code is taken from

  https://android.googlesource.com/toolchain/gcc.git

The selected git revision is documented in the file REVISION.

Build scripts and patches are taken from

  /PATH/TO/NDK/build/tools/

The patch build-tools-r9c.patch is applied to the build scripts and patches.
Than, these patches are applied to the source code.

For building and installing libstdc++ and for creating the source package, 
see also INSTALL.txt.
