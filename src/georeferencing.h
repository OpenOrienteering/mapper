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


#ifndef _OPENORIENTEERING_GEOREFERENCING_H_
#define _OPENORIENTEERING_GEOREFERENCING_H_

#include <QDialog>

#include "gps_coordinates.h"

class QLineEdit;
class QPushButton;

class Map;

/** A class for georeferencing a map in cartesian coordinates */
class CartesianGeoreferencing
{
public:
	/** Construct an undefined georeferencing for a particular map */
	CartesianGeoreferencing(const Map& map);
	/** Construct a georeferencing for a defined origin and (optional) declination */
	CartesianGeoreferencing(const Map& map, double easting, double northing, double declination = 0.0);
	/** Construct a georeferencing which is a copy of an existing georeferencing */
	CartesianGeoreferencing(const CartesianGeoreferencing& georeferencing);
	/** Cleanup memory allocated by the georeferencing */
	~CartesianGeoreferencing();
	
	/** Returns true if easting and northing have been defined for this georeferencing.
	 *  You must not use a georeferencing for any transformation before it is defined. */
	inline bool isDefined() const { return paperToGeo != NULL; }
	
	/** Get the easting of the map origin point */
	inline double getEasting() const { return easting; }
	/** Get the northing of the map origin point */
	inline double getNorthing() const { return northing; }
	/** Get the declination of the map */
	inline double getDeclination() const { return declination; }
	/** Set easting and northing of the map origin point.
	 *  The declination is not modified. (The declination defaults to zero. */
	void set(double easting, double northing);
	/** Set easting and northing of the map origin point and the declination of the map */
	void set(double easting, double northing, double declination);
	/** Set the declination of the map */
	void setDeclination(double declination);
	
	/** Transform map (paper) coordinates to geographic coordinates */
	QPointF toGeoCoords(const MapCoord& paper_coords) const;
	/** Transform map (paper) coordinates to geographic coordinates */
	QPointF toGeoCoords(const MapCoordF& paper_coords) const;
	/** Transform geographic coordinates to map (paper) coordinates */
	MapCoord toPaperCoords(const QPointF& geo_coords) const;
	
protected:
	/** Update the stored transforms from the current easting/northing/declination */
	void updateTransforms();
	
private:
	const Map& map;
	double easting;
	double northing;
	double declination;
	
	QTransform* paperToGeo;
	QTransform* geoToPaper;
};


class GeoreferencingDialog : public QDialog
{
Q_OBJECT
public:
	GeoreferencingDialog(QWidget* parent, const Map& map, const GPSProjectionParameters* initial_values = NULL);
	
	inline const GPSProjectionParameters& getParameters() const {return params;}
	inline const CartesianGeoreferencing& getGeoreferencing() const {return *cartesian; }
	
protected slots:
	void editChanged();
	
	void cartesianChanged();
	
private:
	GPSProjectionParameters params;
	
	CartesianGeoreferencing* cartesian;
	
	QLineEdit* lat_edit;
	QLineEdit* lon_edit;
	QLineEdit* datum_edit;
	QLineEdit* zone_edit;
	QLineEdit* easting_edit;
	QLineEdit* northing_edit;
	QLineEdit* declination_edit;
	QPushButton* ok_button;
};

#endif
