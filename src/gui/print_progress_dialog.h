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

#ifndef _OPENORIENTEERING_PRINT_WIDGET_P_H_
#define _OPENORIENTEERING_PRINT_WIDGET_P_H_

#include <QProgressDialog>

class QPrinter;
class MapPrinter;

/**
 * PrintProgressDialog is a variation of QProgressDialog to be used with MapPrinter.
 * 
 * PrintProgressDialog connects to the MapPrint::printMapProgress() signal.
 * It provides a paintRequested slot which is to be connected to the
 * corresponding QPrintPreviewDialog signal.
 * 
 * This dialog is modal (for the application) by default.
 */
class PrintProgressDialog : public QProgressDialog
{
Q_OBJECT
public:
	/**
	 * Constructs a new dialog for the given MapPrinter.
	 * 
	 * map_printer must not be nullptr.
	 */
	PrintProgressDialog(MapPrinter* map_printer, QWidget* parent = nullptr, Qt::WindowFlags f = 0);
	
public slots:
	/**
	 * Listens to and forwards paint requests, and reparents the dialog.
	 * 
	 * Resets the parent to the current sender(), if the sender is a
	 * QPrintPreviewDialog and the dialog is not visible at the moment.
	 */
	void paintRequested(QPrinter* printer);
	
protected slots:
	/**
	 * Listens to printing progress messages.
	 * 
	 * Shows the dialog if it was hidden, and processes events before returning.
	 * This makes it possible to react on the dialog's Cancel button, and to
	 * draw UI updates.
	 */
	void setProgress(int value, QString status);
	
private:
	MapPrinter* const map_printer;
};

#endif

#endif // QT_PRINTSUPPORT_LIB
