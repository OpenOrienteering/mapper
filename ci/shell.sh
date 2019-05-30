#!/bin/bash
set -x
"$@" 3>&1 1>&2 2>&3 | sed -f "${BUILD_SOURCESDIRECTORY}/filter-stderr.sed" 3>&1 1>&2 2>&3
