/*
 *    Copyright 2013-2015 Kai Pastor
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


#ifdef QT_PRINTSUPPORT_LIB

#include "print_progress_dialog.h"

#include <QtGlobal>
#include <QApplication>
#include <QMessageBox>

#include "core/map_printer.h"


namespace OpenOrienteering {

PrintProgressDialog::PrintProgressDialog(MapPrinter* map_printer, QWidget* parent, Qt::WindowFlags f)
 : QProgressDialog(parent, f)
 , map_printer(map_printer)
{
	setWindowModality(Qt::ApplicationModal); // Required for OSX, cf. QTBUG-40112
	setRange(0, 100);
	setMinimumDuration(0);
	setValue(0);
	
	Q_ASSERT(map_printer);
	connect(map_printer, &MapPrinter::printProgress, this, &PrintProgressDialog::setProgress);
	connect(this, &PrintProgressDialog::canceled, map_printer, &MapPrinter::cancelPrintMap);
}

PrintProgressDialog::~PrintProgressDialog()
{
	// nothing, not inlined
}

void PrintProgressDialog::paintRequested(QPrinter* printer)
{
	if (!map_printer->printMap(printer))
	{
		QMessageBox::warning(
		  parentWidget(), tr("Printing", "PrintWidget"),
		  tr("An error occurred during processing.", "PrintWidget"),
		  QMessageBox::Ok, QMessageBox::Ok );
	}
}

void PrintProgressDialog::setProgress(int value, const QString& status)
{
	setLabelText(status);
	setValue(value);
	if (!isVisible() && value < maximum())
	{
		show();
	}
	
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 100 /* ms */); // Drawing and Cancel events
}


}  // namespace OpenOrienteering

#endif
