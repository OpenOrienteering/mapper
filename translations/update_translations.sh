#!/bin/sh

APPNAME="OpenOrienteering"
SOURCES="../src/*.cpp ../src/*.h"
LANGUAGES="de ja sv uk"

cd $(dirname "$0") || exit 1

lupdate ${SOURCES} -ts ${APPNAME}_template.ts

cat >../translations.qrc <<QRC_HEADER
<!DOCTYPE RCC><RCC version="1.0">
 <qresource>
QRC_HEADER

for I in ${LANGUAGES} ; do
	lupdate ${SOURCES} -ts ${APPNAME}_${I}.ts
	cat >>../translations.qrc <<QRC_LINE
  <file alias="translations/${I}.qm">build/${APPNAME}_${I}.qm</file>
QRC_LINE
done

cat >>../translations.qrc <<QRC_FOOTER
 </qresource>
</RCC>
QRC_FOOTER

