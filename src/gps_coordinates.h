/*
 *    Copyright 2012, 2013 Thomas Schöps
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

#include <qmath.h>

#include "map_coord.h"

class QDebug;

class Georeferencing;

/// Parameters for an ellipsoid and an orthographic projection of ellipsoid coordinates to 2D map (template) coordinates
/// @deprecated
struct GPSProjectionParameters
{
friend class LatLon;
friend class NativeFileImport;
friend class NativeFileExport;
private:
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

#endif
