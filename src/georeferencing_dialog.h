/*
 *    Copyright 2012 Thomas Schöps
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


#ifndef _OPENORIENTEERING_GEOREFERENCING_DIALOG_H_
#define _OPENORIENTEERING_GEOREFERENCING_DIALOG_H_

#include <QDialog>
#include <QLocale>
#include <QString>

#include "gps_coordinates.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

class Georeferencing;
class Map;

class GeoreferencingDialog : public QDialog
{
Q_OBJECT
public:
	/// Construct a new Georeferencing for the given map and initial values
	GeoreferencingDialog(QWidget* parent, Map& map, const GPSProjectionParameters* initial_values = 0);
	
	/// TODO: Set the map coordinates of the reference point
	void setRefPoint(MapCoord coords);
	
	/// Update the CRS edit from the Georeferencing
	void updateCRS();
	/// Update the zone edit. This may enable or disable the input field and set an initial value.
	void updateZone();
	/// Update the reference point easting and northing from the Georeferencing
	void updateEastingNorthing();
	/// Update latitude and longitude from the GPSProjectionParameters (FIXME: from the Georeferencing)
	void updateLatLon();
	
public slots:
	/// Reset the inputs to the initial state
	void reset();
	/// Push the changes to the map and indicate success to the caller.
	void accept();
	
protected slots:
	/// Indicate a change in the grivation input
	void grivationChanged(double value);
	/// TODO: Indicate the wish to select another reference point
	void selectRefPoint();
	/// Indicate a change in the crs input
	void crsChanged();
	/// Indicate a change in the zone input
	void eastingNorthingChanged();
	/// Indicate a change in the lat/lon input
	void latLonChanged();
	
private:
	Map& map;
	
	/// @deprecated
	GPSProjectionParameters params;
	
	Georeferencing* georef;
	QString crs_spec_template;
	
	QLocale locale;
	
	QDoubleSpinBox* grivation_edit;
	QLabel* ref_point_edit;
	
	QComboBox* crs_edit;
	QLineEdit* zone_edit;
	QDoubleSpinBox* easting_edit;
	QDoubleSpinBox* northing_edit;
	
	QComboBox* ll_datum_edit;
	QDoubleSpinBox* lat_edit;
	QDoubleSpinBox* lon_edit;
	
	QLabel* link_label;
	
	QPushButton* reset_button;
	QPushButton* ok_button;
};

#endif
