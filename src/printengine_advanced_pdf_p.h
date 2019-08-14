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
 *
 */

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
#include <private/qprint_p.h>

QT_BEGIN_NAMESPACE

class QImage;
class QDataStream;
class QPen;
class QPointF;
class QRegion;
class QFile;

class AdvancedPdfPrintEnginePrivate;

class AdvancedPdfPrintEngine : public AdvancedPdfEngine, public QPrintEngine
{
    Q_DECLARE_PRIVATE(AdvancedPdfPrintEngine)
public:
    AdvancedPdfPrintEngine(QPrinter::PrinterMode m);
    ~AdvancedPdfPrintEngine() Q_DECL_OVERRIDE;

    // reimplementations QPaintEngine
    bool begin(QPaintDevice *pdev) Q_DECL_OVERRIDE;
    bool end() Q_DECL_OVERRIDE;
    // end reimplementations QPaintEngine

    // reimplementations QPrintEngine
    bool abort() Q_DECL_OVERRIDE {return false;}
    QPrinter::PrinterState printerState() const Q_DECL_OVERRIDE {return state;}

    bool newPage() Q_DECL_OVERRIDE;
    int metric(QPaintDevice::PaintDeviceMetric) const Q_DECL_OVERRIDE;
    void setProperty(PrintEnginePropertyKey key, const QVariant &value) Q_DECL_OVERRIDE;
    QVariant property(PrintEnginePropertyKey key) const Q_DECL_OVERRIDE;
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
    ~AdvancedPdfPrintEnginePrivate() Q_DECL_OVERRIDE;

    virtual bool openPrintDevice();
    virtual void closePrintDevice();

private:
    Q_DISABLE_COPY(AdvancedPdfPrintEnginePrivate)

    friend class QCupsPrintEngine;
    friend class QCupsPrintEnginePrivate;

    QString printerName;
    QString printProgram;
    QString selectionOption;

    QPrint::DuplexMode duplex;
    bool collate;
    int copies;
    QPrinter::PageOrder pageOrder;
    QPrinter::PaperSource paperSource;

    int fd;
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTER

#endif // PRINTENGINE_ADVANCED_PDF_P_H
