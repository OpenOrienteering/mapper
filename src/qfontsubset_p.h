/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
****************************************************************************/
/**
 * Copyright 2015 Kai Pastor
 *
 * This file is part of OpenOrienteering.
 *
 * This is a modified version of a file from the Qt Toolkit.
 * You can redistribute it and/or modify it under the terms of
 * the GNU General Public License, version 3, as published by
 * the Free Software Foundation.
 *
 * OpenOrienteering is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef QFONTSUBSET_P_H
#define QFONTSUBSET_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qfontengine_p.h>

QT_BEGIN_NAMESPACE

class QFontSubset
{
public:
    explicit QFontSubset(QFontEngine *fe, int obj_id = 0)
        : object_id(obj_id), noEmbed(false), fontEngine(fe), downloaded_glyphs(0), standard_font(false)
    {
        fontEngine->ref.ref();
#ifndef QT_NO_PDF
        addGlyph(0);
#endif
    }
    ~QFontSubset() {
        if (!fontEngine->ref.deref())
            delete fontEngine;
    }

    QByteArray toTruetype() const;
#ifndef QT_NO_PDF
    QByteArray widthArray() const;
    QByteArray createToUnicodeMap() const;
    QVector<int> getReverseMap() const;
    QByteArray glyphName(unsigned int glyph, const QVector<int> &reverseMap) const;

    static QByteArray glyphName(unsigned short unicode, bool symbol);

    int addGlyph(int index);
#endif
    const int object_id;
    bool noEmbed;
    QFontEngine *fontEngine;
    QList<int> glyph_indices;
    mutable int downloaded_glyphs;
    mutable bool standard_font;
    int nGlyphs() const { return glyph_indices.size(); }
    mutable QFixed emSquare;
    mutable QVector<QFixed> widths;
};

QT_END_NAMESPACE

#endif // QFONTSUBSET_P_H
