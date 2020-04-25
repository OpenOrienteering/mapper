/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2020 Kai Pastor
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


#ifndef OPENORIENTEERING_ROTATE_MAP_DIALOG_H
#define OPENORIENTEERING_ROTATE_MAP_DIALOG_H

#include <Qt>
#include <QDialog>
#include <QObject>
#include <QString>

class QDoubleSpinBox;
class QCheckBox;
class QRadioButton;
class QWidget;

namespace OpenOrienteering {

class Map;


/**
 * Dialog for rotating the whole map around a point.
 */
class RotateMapDialog : public QDialog
{
Q_OBJECT
public:
	/** Creates a new RotateMapDialog. */
	RotateMapDialog(Map& map, QWidget* parent = nullptr, Qt::WindowFlags f = {});
	
	/** Sets the rotation angle in degrees in the corresponding widget. */
	void setRotationDegrees(double rotation);
	/** Enables the setting to rotate around the georeferencing reference point. */
	void setRotateAroundGeorefRefPoint();
	/** Checks or unchecks the setting to adjust the georeferencing declination. */
	void setAdjustDeclination(bool adjust);
	/** Sets the visibility of the setting to adjust the georeferencing declination. */
	void showAdjustDeclination(bool show);
	
private slots:
	void updateWidgets();
	void okClicked();
	
private:
	QDoubleSpinBox* rotation_edit;
	QRadioButton* center_origin_radio;
	QRadioButton* center_georef_radio;
	QRadioButton* center_other_radio;
	QDoubleSpinBox* other_x_edit;
	QDoubleSpinBox* other_y_edit;
	
	QCheckBox* adjust_georeferencing_check;
	QCheckBox* adjust_declination_check;
	QCheckBox* adjust_templates_check;
	
	Map& map;
};


}  // namespace OpenOrienteering

#endif
