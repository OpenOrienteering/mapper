#!/bin/sh
set -e

SKIP_LIST=
while read WORD; do SKIP_LIST="${SKIP_LIST:+$SKIP_LIST,}$WORD"; done \
<<END_SKIP_LIST
  3rd-party/cove/potrace
  packaging/linux/Mapper.desktop
  src/gdal/mapper-osmconf.ini
  src/libocad
  src/printsupport/qt-5.5.1
  src/printsupport/qt-5.12.4
END_SKIP_LIST

codespell -q 7 \
  'CMakeLists.txt' \
  'INSTALL.md' \
  'README.md' \
  '3rd-party/cove' \
  'android' \
  'doc/api' \
  'doc/licensing' \
  'doc/man' \
  'doc/manual' \
  'doc/tip-of-the-day/tips_en.txt' \
  'examples/src' \
  'packaging' \
  'src' \
  'symbol sets/src' \
  'translations/future_translations.cpp' \
  -S "$SKIP_LIST" \
  -L copyable \
  -L endwhile \
  -L intrest \
  "$@"

git log -n 20 | codespell -q 7 - | sed 's/^/git log: /'
