/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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
 * the Free Software Foundation, or any later version approved
 * by the KDE Free Qt Foundation
 *
 * OpenOrienteering is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>
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
    AdvancedPdfPrintEngine(QPrinter::PrinterMode m, AdvancedPdfEngine::PdfVersion version = AdvancedPdfEngine::Version_1_4);
    ~AdvancedPdfPrintEngine() override;

    // reimplementations QPaintEngine
    bool begin(QPaintDevice *pdev) override;
    bool end() override;
    // end reimplementations QPaintEngine

    // reimplementations QPrintEngine
    bool abort() override {return false;}
    QPrinter::PrinterState printerState() const override {return state;}

    bool newPage() override;
    int metric(QPaintDevice::PaintDeviceMetric) const override;
    void setProperty(PrintEnginePropertyKey key, const QVariant &value) override;
    QVariant property(PrintEnginePropertyKey key) const override;
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
    ~AdvancedPdfPrintEnginePrivate() override;

    virtual bool openPrintDevice();
    virtual void closePrintDevice();

private:
    Q_DISABLE_COPY(AdvancedPdfPrintEnginePrivate)

    friend class QCupsPrintEngine;
    friend class QCupsPrintEnginePrivate;

    QString printerName;
    QString printProgram;
    QString selectionOption;

    bool collate;
    int copies;
    QPrinter::PageOrder pageOrder;
    QPrinter::PaperSource paperSource;

    int fd;
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTER

#endif // PRINTENGINE_ADVANCED_PDF_P_H
