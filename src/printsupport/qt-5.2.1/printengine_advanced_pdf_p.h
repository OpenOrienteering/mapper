/**
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
 *
 * Changes:
 * 2015-10-18 Kai Pastor <dg0yt@darc.de>
 * - Adjustment of legal information
 * - Modifications required for separate compilation:
 *   - Renaming of selected files, classes, members and macros
 *   - Adjustment of include statements
 *   - Removal of Q_XXX_EXPORT
 */
/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
****************************************************************************/

#ifndef PRINTENGINE_ADVANCED_PDF_P_H
#define PRINTENGINE_ADVANCED_PDF_P_H

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

#include "QtPrintSupport/qprintengine.h"

#ifndef QT_NO_PRINTER
#include "QtCore/qmap.h"
#include "QtGui/qmatrix.h"
#include "QtCore/qstring.h"
#include "QtCore/qvector.h"
#include "QtGui/qpaintengine.h"
#include "QtGui/qpainterpath.h"
#include "QtCore/qdatastream.h"

#include <private/qfontengine_p.h>
#include "advanced_pdf_p.h"
#include <private/qpaintengine_p.h>
#include "qprintengine.h"

QT_BEGIN_NAMESPACE

class QImage;
class QDataStream;
class QPen;
class QPointF;
class QRegion;
class QFile;
class AdvancedPdfPrintEngine;

namespace AdvancedPdf {

    struct PaperSize {
        int width, height; // in postscript points
    };
    PaperSize paperSize(QPrinter::PaperSize paperSize);
    const char *paperSizeToString(QPrinter::PaperSize paperSize);
}

class AdvancedPdfPrintEnginePrivate;

class AdvancedPdfPrintEngine : public AdvancedPdfEngine, public QPrintEngine
{
    Q_DECLARE_PRIVATE(AdvancedPdfPrintEngine)
public:
    AdvancedPdfPrintEngine(QPrinter::PrinterMode m);
    virtual ~AdvancedPdfPrintEngine();

    // reimplementations QPaintEngine
    bool begin(QPaintDevice *pdev);
    bool end();
    // end reimplementations QPaintEngine

    // reimplementations QPrintEngine
    bool abort() {return false;}
    QPrinter::PrinterState printerState() const {return state;}

    bool newPage();
    int metric(QPaintDevice::PaintDeviceMetric) const;
    virtual void setProperty(PrintEnginePropertyKey key, const QVariant &value);
    virtual QVariant property(PrintEnginePropertyKey key) const;
    // end reimplementations QPrintEngine

    QPrinter::PrinterState state;

protected:
    AdvancedPdfPrintEngine(AdvancedPdfPrintEnginePrivate &p);

private:
    Q_DISABLE_COPY(AdvancedPdfPrintEngine)
};

class AdvancedPdfPrintEnginePrivate : public AdvancedPdfEnginePrivate
{
    Q_DECLARE_PUBLIC(AdvancedPdfPrintEngine)
public:
    AdvancedPdfPrintEnginePrivate(QPrinter::PrinterMode m);
    ~AdvancedPdfPrintEnginePrivate();

    virtual bool openPrintDevice();
    virtual void closePrintDevice();

    virtual void updatePaperSize();

private:
    Q_DISABLE_COPY(AdvancedPdfPrintEnginePrivate)

    friend class QCupsPrintEngine;
    friend class QCupsPrintEnginePrivate;

    QString printerName;
    QString printProgram;
    QString selectionOption;

    QPrinter::DuplexMode duplex;
    bool collate;
    int copies;
    QPrinter::PageOrder pageOrder;
    QPrinter::PaperSource paperSource;

    QPrinter::PaperSize printerPaperSize;
    QSizeF customPaperSize; // in postscript points
    bool pageMarginsSet;
    int fd;
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTER

#endif // PRINTENGINE_ADVANCED_PDF_P_H
