#
#    Copyright 2015 Kai Pastor
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


# Update of legal information

1 {
  i  /**
  i\\ * This file is part of OpenOrienteering.
  i\\ *
  i\\ * This is a modified version of a file from the Qt Toolkit.
  i\\ * You can redistribute it and/or modify it under the terms of
  i\\ * the GNU General Public License, version 3, as published by
  i\\ * the Free Software Foundation.
  i\\ *
  i\\ * OpenOrienteering is distributed in the hope that it will be useful,
  i\\ * but WITHOUT ANY WARRANTY; without even the implied warranty of
  i\\ * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  i\\ * GNU General Public License for more details.
  i\\ *
  i\\ * You should have received a copy of the GNU General Public License
  i\\ * along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>
  i\\ *
  i\\ * Changes:
  i\\ * %DATE% Kai Pastor <dg0yt@darc.de>
  i\\ * - Adjustment of legal information
  i\\ * - Modifications required for separate compilation:
  i\\ *   - Renaming of selected files, classes, members and macros
  i\\ *   - Adjustment of include statements
  i\\ *   - Removal of Q_XXX_EXPORT
  i\\ */
}
/\*\* *This file is part of.*Qt/ {
: cleanup
  /\*\//! { N; b cleanup }
  s/.*\n//
}

# Renaming of classes, members and macros

s/QPdf/AdvancedPdf/g
s/qpdf/advanced_pdf/
s/QPDF/ADVANCED_PDF/
s/QPRINTENGINE_PDF/PRINTENGINE_ADVANCED_PDF/

# Adjustment of include statements

s/<advanced_pdfwriter.h>/\"advanced_pdfwriter.h\"/
s/qprintengine_pdf/printengine_advanced_pdf/
s/private\/advanced_pdf_p.h/advanced_pdf_p.h/
s/<private\/qfontsubset_p.h>/\"qfontsubset_p.h\"/
s/QtCore\/private/private/
/qfontsubset/!s/\"\(q\w*\)_p.h\"/<private\/\1_p.h>/
/qfontsubset/!s/\"\(private\/q\w*\)_p.h\"/<\1_p.h>/

# Removal of Q_XXX_EXPORT

s/Q_\([A-Z]*\)_EXPORT //
