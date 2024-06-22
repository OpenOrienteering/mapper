#!/bin/bash
if [ -n "${MINGW}" ] ; then
  source /etc/profile
  unset CC
  unset PKG_CONFIG_PATH
fi

unset UNBUFFER
if [ -n "${MINGW}" ] ; then
  : # msys stdbuf breaking msys binaries?
  : # C:\msys2\usr\bin\make.exe: *** fatal error - error while loading shared libraries: C: cannot open shared object file: No such file or directory
elif [ -f /usr/bin/stdbuf ] ; then
  UNBUFFER="/usr/bin/stdbuf -oL"
elif [ -f /usr/bin/unbuffer ] ; then
  UNBUFFER="/usr/bin/unbuffer"
fi

set -o pipefail

${UNBUFFER} "$@" 2>&1 | ${UNBUFFER} sed -f "${SOURCE_DIR}/ci/filter-stderr.sed"
