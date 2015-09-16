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

#include <QApplication>
#include <QPrintPreviewDialog>

#include "../core/map_printer.h"


PrintProgressDialog::PrintProgressDialog(MapPrinter* map_printer, QWidget* parent, Qt::WindowFlags f)
 : QProgressDialog(parent, f)
 , map_printer(map_printer)
{
	setWindowModality(Qt::ApplicationModal);
	setRange(0, 100);
	setMinimumDuration(0);
	setValue(0);
	
	Q_ASSERT(map_printer);
	connect(map_printer, &MapPrinter::printProgress, this, &PrintProgressDialog::setProgress);
	connect(this, &PrintProgressDialog::canceled, map_printer, &MapPrinter::cancelPrintMap);
}

void PrintProgressDialog::paintRequested(QPrinter* printer)
{
	// Make sure that the dialog is on top of QPrintPreviewDialog.
	auto w = qobject_cast<QPrintPreviewDialog*>(sender());
	if (w && isHidden())
		setParent(w);
	
	map_printer->printMap(printer);
}

void PrintProgressDialog::setProgress(int value, QString status)
{
	if (isHidden())
	{
		show();
		raise();
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}
	
	setLabelText(status);
	setValue(value);
	QApplication::processEvents(); // Drawing and Cancel events
}

#endif
