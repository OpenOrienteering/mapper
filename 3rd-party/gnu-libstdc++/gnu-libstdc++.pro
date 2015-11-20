#
#    Copyright 2014 Kai Pastor
#    
#    This file is part of OpenOrienteering.
# 
#    OpenOrienteering is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
# 
#    OpenOrienteering is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# 
#    You should have received a copy of the GNU General Public License
#    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.

# This .pro file shall be used with a mkspec/kit for Android

TEMPLATE = aux

# Files to be listed in Qt Creator project tree
OTHER_FILES = \
    build-tools-r9c.patch \
    build.sh \
    init-r9c.sh \
    INSTALL.txt.in \
    README.txt

# Determine Android NDK root and release
ANDROID_NDK_ROOT = $$(ANDROID_NDK_ROOT)
isEmpty(ANDROID_NDK_ROOT) {
	ANDROID_NDK_ROOT = $$cat("$$OUT_PWD/ANDROID_NDK_ROOT.cache", true)
	ANDROID_NDK_ROOT = $$first(ANDROID_NDK_ROOT)
}

RELEASE_FILE = "$$ANDROID_NDK_ROOT/RELEASE.TXT"
RELEASE_FULL = $$cat("$$RELEASE_FILE", true)
RELEASE = $$split(RELEASE_FULL, " ")
RELEASE = $$first(RELEASE)

isEmpty(RELEASE) {
	warning("Unable to determine Android NDK release.")
	warning(" (ANDROID_NDK_ROOT: $$ANDROID_NDK_ROOT)")
	error("Building GNU libstdc++ library not supported for other systems.")
}

# Verify that init and build script exist for this release
init.script = $$PWD/init-$${RELEASE}.sh
exists($$init.script) {
	message("Android NDK Release: $$RELEASE_FULL")
} else {
	warning("Unsupported Android NDK Release: $$RELEASE_FULL")
}

# Save ANDROID_NDK_ROOT for reconfiguration
write_file($$OUT_PWD/ANDROID_NDK_ROOT.cache, ANDROID_NDK_ROOT)

# Download/src directory
GNU_LIBSTDCPP_DIR      = gnu-libstdc++-android-ndk-$$RELEASE-src

# Target "init"
init.target   = $$GNU_LIBSTDCPP_DIR/build.sh
init.depends  = \
    $$ANDROID_NDK_ROOT/RELEASE.TXT \
    $$PWD/build-tools-$${RELEASE}.patch \
    $$PWD/build.sh \
    $$PWD/INSTALL.txt.in \
    $$init.script
init.commands = @ \
    mkdir -p "$$GNU_LIBSTDCPP_DIR" && \
    echo "$$RELEASE" > "$$GNU_LIBSTDCPP_DIR/NDK-Release.txt" && \
    cp "$$PWD/build-tools-$${RELEASE}.patch" "$$GNU_LIBSTDCPP_DIR/build-tools.patch" && \
    cp "$$PWD/INSTALL.txt.in" "$$GNU_LIBSTDCPP_DIR/INSTALL.txt" && \
    ( test ! -f "$$init.target" || \
      rm "$$init.target" ) && \
    ( cd "$$GNU_LIBSTDCPP_DIR" && \
      "$$init.script" "$$ANDROID_NDK_ROOT" ) && \
    cp "$$PWD/build.sh" "$$init.target"

# Target "build"
build.target   = android-ndk-$$RELEASE-gnu-libstdc++.tgz
build.script   = $$init.target
build.depends  = $$build.script
build.commands = @ \
    ( cd "$$GNU_LIBSTDCPP_DIR" && \
      "../$$build.script" "$$ANDROID_NDK_ROOT" --package-dir="$$OUT_PWD" )

QMAKE_EXTRA_TARGETS += build init
PRE_TARGETDEPS      += $$build.target
QMAKE_CLEAN         += $$init.target $$build.target
