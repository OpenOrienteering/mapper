#!/bin/bash
if [ -n "${MINGW}" ] ; then
  source /etc/profile
  unset CC
  unset PKG_CONFIG_PATH
fi

unset UNBUFFER
if [ -f /usr/bin/stdbuf ] ; then
  UNBUFFER="/usr/bin/stdbuf -oL"
elif [ -f /usr/bin/unbuffer ] ; then
  UNBUFFER="/usr/bin/unbuffer"
fi

set -o pipefail

${UNBUFFER} "$@" 2>&1 | ${UNBUFFER} sed -f "${SOURCE_DIR}/ci/filter-stderr.sed"
