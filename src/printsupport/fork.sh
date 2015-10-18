#!/bin/bash

SUBDIR=qt-5.4.2

if [ -n "$1" -a -d "$1/qtbase/src" ] ; then
	SOURCE_DIR=$1/qtbase/src
elif [ -n "$1" -a -d "$1/src" ] ; then
	SOURCE_DIR=$1/src
elif [ -d "$1" ] ; then
	SOURCE_DIR=$1
else
	echo "Usage: $0 QTBASE_SRC_DIR"
	exit 1
fi

if [ ! -d "$SUBDIR" ] ; then
	mkdir "$SUBDIR"
fi

for I in \
  printsupport/kernel/qprintengine_pdf_p.h \
  printsupport/kernel/qprintengine_pdf.cpp \
  gui/painting/qpdf_p.h \
  gui/painting/qpdf.cpp \
  gui/text/qfontsubset_p.h \
  gui/text/qfontsubset_agl.cpp \
  gui/text/qfontsubset.cpp
do
	if [ ! -f "$SOURCE_DIR/$I" ]; then
		echo "No $I in $SOURCE_DIR"
		exit 1
	fi
	NEW=$(basename $I)
	NEW=${NEW/qpdf/advanced_pdf}
	NEW=${NEW/qprintengine_pdf/printengine_advanced_pdf}
	cp "$SOURCE_DIR/$I" "$SUBDIR/$NEW"
done

exit 0
