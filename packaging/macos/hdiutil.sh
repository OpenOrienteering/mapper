#!/bin/sh

# Mitigate XProtect race condition for "hdiutil create"
# Cf. https://github.com/actions/runner-images/issues/7522

set -e
set -x

case "$1" in
create)
    max=5
    ;;
*)
    max=1
    ;;
esac

i=1
until hdiutil "$@"
do
    if [ $i -eq $max ]; then exit 1; fi
    sleep $max
    i=$((i+1))
done
