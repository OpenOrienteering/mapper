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

#ifndef OPENORIENTEERING_PRINT_WIDGET_P_H
#define OPENORIENTEERING_PRINT_WIDGET_P_H

#include <Qt>
#include <QObject>
#include <QProgressDialog>
#include <QString>

class QPrinter;
class QWidget;

namespace OpenOrienteering {

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
	PrintProgressDialog(MapPrinter* map_printer, QWidget* parent = nullptr, Qt::WindowFlags f = {});
	
	/**
	 * Destructor.
	 */
	~PrintProgressDialog() override;
	
public slots:
	/**
	 * Listens to and forwards paint requests.
	 * 
	 * Shows an error message if printing fails.
	 */
	void paintRequested(QPrinter* printer);
	
protected slots:
	/**
	 * Listens to printing progress messages.
	 * 
	 * Shows the dialog if it was hidden, and processes events before returning.
	 * This makes it possible to react on the dialog's Cancel button, and to
	 * draw UI updates.
	 * 
	 * @param value   The progress, from 0 (not started) to 100 (finished).
	 * @param message The text to be shown as a label to the progress.
	 */
	void setProgress(int value, const QString& message);
	
private:
	MapPrinter* const map_printer;
};

#endif


}  // namespace OpenOrienteering

#endif // QT_PRINTSUPPORT_LIB
