#!/bin/bash

SUBDIR=qt-5.5.1
PRETTY_DATE=$(date +%Y-%m-%d)

for I in $SUBDIR/*.h $SUBDIR/*.cpp
do
	sed -f patches/headers.sed -i -- "$I" && rm -f "$I--"
	sed -e "s/%DATE%/$PRETTY_DATE/" -i -- "$I" && rm -f "$I--"
done

cd $SUBDIR
patch -p1 < ../patches/producer.diff || exit 1
patch -p1 < ../patches/devicecmyk.diff || exit 1
patch -p1 < ../patches/enginetype.diff || exit 1
cd

exit 0
