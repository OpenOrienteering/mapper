/*
 *    Copyright 2015 Kai Pastor
 *
 *    This file is part of OpenOrienteering.
 *
 *    OpenOrienteering is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    OpenOrienteering is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "advanced_pdf_printer.h"

#include <advanced_pdf_p.h>
#include <printengine_advanced_pdf_p.h>


AdvancedPdfPrinter::AdvancedPdfPrinter(QPrinter::PrinterMode mode)
 : QPrinter{ mode }
 , engine{ new AdvancedPdfPrintEngine{ mode} }
{
	init();
}

AdvancedPdfPrinter::AdvancedPdfPrinter(const QPrinterInfo& printer, QPrinter::PrinterMode mode)
 : QPrinter{ printer, mode }
 , engine{ new AdvancedPdfPrintEngine{ mode} }
{
	init();
}

AdvancedPdfPrinter::~AdvancedPdfPrinter() = default;

void AdvancedPdfPrinter::init()
{
	setOutputFormat(PdfFormat);
	setEngines(engine.get(), engine.get());
}


QPaintEngine::Type AdvancedPdfPrinter::paintEngineType()
{
	return AdvancedPdfEngine::PaintEngineType;
}
