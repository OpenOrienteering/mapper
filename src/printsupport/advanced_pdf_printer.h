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

#ifndef OPENORIENTEERING_ADVANCED_PDF_H
#define OPENORIENTEERING_ADVANCED_PDF_H

#include <memory>

#include <QPaintEngine>
#include <QPrinter>


class AdvancedPdfPrintEngine;

class AdvancedPdfPrinter : public QPrinter
{
public:
	/** Creates a PdfFormat printer based on AdvancedPdfPrintEngine. */
	explicit AdvancedPdfPrinter(PrinterMode mode = ScreenResolution);
	
	/** Creates a PdfFormat printer based on AdvancedPdfPrintEngine. */
	explicit AdvancedPdfPrinter(const QPrinterInfo& printer, PrinterMode mode = ScreenResolution);
	
	/** Destructor. */
	~AdvancedPdfPrinter() override;
	
	/** Returns the paint engine type which is used for advanced pdf generation. */
	static QPaintEngine::Type paintEngineType();
	
private:
	void init();
	
	std::unique_ptr<AdvancedPdfPrintEngine> engine;
};

#endif
