#
#    Copyright 2016 Kai Pastor
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

TEMPLATE = aux
CONFIG  -= debug_and_release
INSTALLS = translations

qtPrepareTool(LRELEASE, lrelease)
qtPrepareTool(LCONVERT, lconvert)

exists($$LRELEASE):exists($$LCONVERT) {
    translations.CONFIG += no_check_exist # undocumented qmake
    android: translations.path = /assets/translations
    win32:   translations.path = /translations
    
    lrelease.input         = TRANSLATIONS
    lrelease.output        = ${QMAKE_FILE_BASE}.qm
    lrelease.commands      = $${LRELEASE} -qm ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    lrelease.CONFIG        = no_link
    lrelease.variable_out  = translations.files
    QMAKE_EXTRA_COMPILERS += lrelease
    
    LANGUAGES = cs de en es fi fr hu it ja lv nb pl ru sv uk
    QT_TRANSLATIONS_PREFIXES = qtbase
    for(lang, LANGUAGES) {
        TRANSLATIONS += OpenOrienteering_$${lang}.ts
        translations.depends += OpenOrienteering_$${lang}.qm
        
        qt_qm_files =
        for(qt_prefix, QT_TRANSLATIONS_PREFIXES) {
            qt_qm_file = $$[QT_INSTALL_TRANSLATIONS]/$${qt_prefix}_$${lang}.qm
            exists($$qt_qm_file): qt_qm_files += $$qt_qm_file
        }
        !isEmpty(qt_qm_files) {
            qm_file = qt_$${lang}.qm
            qt_$${lang}.target    = $$qm_file
            qt_$${lang}.depends   = $$qt_qm_files
            qt_$${lang}.commands  = $$LCONVERT -o $$qm_file $$qt_qm_files
            QMAKE_EXTRA_TARGETS  += qt_$${lang}
            QMAKE_CLEAN          += $$qm_file
            translations.depends += $$qm_file
            translations.files   += $$OUT_PWD/$$qm_file
        } else:exists($$PWD/qt_$${lang}.ts) {
            TRANSLATIONS += qt_$${lang}.ts
            translations.depends += qt_$${lang}.qm
        }
    }
}
