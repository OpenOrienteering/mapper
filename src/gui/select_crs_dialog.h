/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2020 Kai Pastor
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


#ifndef OPENORIENTEERING_SELECT_CRS_DIALOG_H
#define OPENORIENTEERING_SELECT_CRS_DIALOG_H

#include <QDialog>
#include <QFlags>
#include <QObject>
#include <QString>

class QDialogButtonBox;
class QLabel;
class QWidget;

namespace OpenOrienteering {

class CRSSelector;
class Georeferencing;


/** Dialog to select a coordinate reference system (CRS) */
class SelectCRSDialog : public QDialog
{
Q_OBJECT
public:
	/**
	 * Creates a SelectCRSDialog.
	 * 
	 * @param georef       A default georeferencing (usually the map's one).
	 * @param parent       The parent widget.
	 * @param alternatives The georeferencing alternatives to be offered.
	 * @param description  Optional description text for the dialog.
	 *                     Should explain what the selected CRS will be used for.
	 */
	SelectCRSDialog(
	        const Georeferencing& georef,
	        QWidget* parent,
	        const QString& description = QString()
	);
	
	/** 
	 * Returns the current CRS specification string.
	 */
	QString currentCRSSpec() const;
	
protected:
	/** 
	 * Update the status field and enables/disables the OK button.
	 */
	void updateWidgets();
	
private:
	const Georeferencing& georef;
	CRSSelector* crs_selector;
	QLabel* status_label;
	QDialogButtonBox* button_box;
};


}  // namespace OpenOrienteering

#endif
