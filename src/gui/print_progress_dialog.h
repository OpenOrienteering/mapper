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


#ifndef _OPENORIENTEERING_PRINT_WIDGET_P_H_
#define _OPENORIENTEERING_PRINT_WIDGET_P_H_

#include <QProgressDialog>

class MapPrinter;

/** 
 * PrintProgressDialog is a variation of the QProgressDialog which can be 
 * directly connected to the MapPrint::printMapProgress() signal.
 */
class PrintProgressDialog : public QProgressDialog
{
Q_OBJECT
public:
	/** Constructs a new dialog. */
	PrintProgressDialog(QWidget* parent = NULL, Qt::WindowFlags f = 0);
	
	/** Connects this progress dialog to the given printer. */
	void attach(MapPrinter* printer);
	
protected slots:
	/** Listens to printing progress messages. */
	void setProgress(int value, QString status);
	
};

#endif
