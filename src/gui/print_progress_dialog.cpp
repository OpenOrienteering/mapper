/*
 *    Copyright 2013 Kai Pastor
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


#include "print_progress_dialog.h"

#include <QApplication>

#include "../core/map_printer.h"


PrintProgressDialog::PrintProgressDialog(QWidget* parent, Qt::WindowFlags f)
 : QProgressDialog(parent, f)
{
	setRange(0, 100);
	setMinimumDuration(0);
	setValue(0);
}

void PrintProgressDialog::attach(MapPrinter* printer)
{
#ifndef QT_NO_PRINTER
	connect(printer, SIGNAL(printProgress(int, QString)), this, SLOT(setProgress(int,QString)));
	connect(this, SIGNAL(canceled()), printer, SLOT(cancelPrintMap()));
#endif
}

void PrintProgressDialog::setProgress(int value, QString status)
{
	if (isHidden())
	{
		show();
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}
	setLabelText(status);
	setValue(value);
	QApplication::processEvents(); // Drawing and Cancel events
}
