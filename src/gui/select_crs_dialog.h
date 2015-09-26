/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2015 Kai Pastor
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


#ifndef _OPENORIENTEERING_SELECT_CRS_DIALOG_H_
#define _OPENORIENTEERING_SELECT_CRS_DIALOG_H_

#include <QDialog>
#include <QString>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QRadioButton;
QT_END_NAMESPACE

class Georeferencing;
class Map;
class CRSSelector;


/** Dialog to select a coordinate reference system (CRS) */
class SelectCRSDialog : public QDialog
{
Q_OBJECT
public:
	/**
	 * Creates a SelectCRSDialog.
	 * 
	 * @param map The map to create the dialog for.
	 * @param parent The parent widget.
	 * @param show_take_from_map Toggle whether to show the "Take the map's CRS" option.
	 * @param show_local Toggle whether to show the "Local" option.
	 * @param show_geographic Toggle whether to show the "Geographic (WGS84)" option.
	 * @param desc_text Optional description text for the dialog. Should explain
	 *                  for what the selected CRS will be used for.
	 */
	SelectCRSDialog(Map* map, QWidget* parent, bool show_take_from_map,
					bool show_local, bool show_geographic, const QString& desc_text = QString());
	
	/** Returns the chosen CRS spec after the dialog has completed. */
	QString getCRSSpec() const;
	
private slots:
	void crsSpecEdited(QString text);
	void updateWidgets();
	
private:
	/* Internal state */
	Map* const map;
	
	/* GUI elements */
	QRadioButton* map_radio;
	QRadioButton* local_radio;
	QRadioButton* geographic_radio;
	QRadioButton* projected_radio;
	QRadioButton* spec_radio;
	CRSSelector* crs_edit;
	QFormLayout* crs_spec_layout;
	QLineEdit* crs_spec_edit;
	QLabel* status_label;
	QDialogButtonBox* button_box;
};

#endif
