#!/bin/bash
if [ -n "${MINGW}" ] ; then
  source /etc/profile
  unset CC
  unset PKG_CONFIG_PATH
fi
set -x
"$@" 3>&1 1>&2 2>&3 | sed -f "${SOURCE_DIR}/filter-stderr.sed" 3>&1 1>&2 2>&3
