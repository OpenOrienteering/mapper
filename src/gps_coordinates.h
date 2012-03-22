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


// For the source of the formulas, see:
//   http://www.epsg.org/guides/G7-2.html
//   Section: 1.3.18 Orthographic

#ifndef _OPENORIENTEERING_GPS_COORDINATES_H_
#define _OPENORIENTEERING_GPS_COORDINATES_H_

#include <QDialog>
#include <qmath.h>

#include "map.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

/// Parameters for an ellipsoid and an orthographic projection of ellipsoid coordinates to 2D map (template) coordinates
struct GPSProjectionParameters
{
	/// Call update after changing these parameters!
	double a;					// ellipsoidal semi-major axis
	double b;					// ellipsoidal semi-minor axis
	double e_sq;				// eccentricity of the ellipsoid (squared)
	double center_latitude;		// latitude which gives map coordinate zero
	double center_longitude;	// longitude which gives map coordinate zero
	double sin_center_latitude;	// sine of center_latitude
	double cos_center_latitude;	// cosine of center_latitude
	double v0;					// prime vertical radius of curvature at latitude of origin center_latitude
	
	GPSProjectionParameters();
	void update();
};

class GPSCoordinate
{
public:
	/// Coordinates in radiant; TODO: give origin and range in comments
	double latitude;	// Phi
	double longitude;	// Lambda
	
	GPSCoordinate();
	GPSCoordinate(double latitude, double longitude, bool given_in_degrees);
	GPSCoordinate(MapCoordF map_coord, const GPSProjectionParameters& params);
	
	MapCoordF toMapCoordF(const GPSProjectionParameters& params);
	void toCartesianCoordinates(const GPSProjectionParameters& params, double height, double& x, double& y, double& z);
	bool fromString(QString str);	// for example "53°48'33.82"N  2°07'46.38"E" or "N 48° 31.732 E 012° 08.422" or "48.52887 12.14037"
																													
	inline double getLatitudeInDegrees() {return latitude * 180 / M_PI;}
	inline double getLongitudeInDegrees() {return longitude * 180 / M_PI;}
};

class GPSProjectionParametersDialog : public QDialog
{
Q_OBJECT
public:
	GPSProjectionParametersDialog(QWidget* parent, const GPSProjectionParameters* initial_values = NULL);
	
	inline const GPSProjectionParameters& getParameters() const {return params;}
	
protected slots:
	void editChanged();
	
private:
	GPSProjectionParameters params;
	
	QLineEdit* lat_edit;
	QLineEdit* lon_edit;
	QPushButton* ok_button;
};

#endif
