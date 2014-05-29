/*
 *    Copyright 2012, 2013, 2014 Kai Pastor
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


#ifndef _OPENORIENTEERING_LATLON_H_
#define _OPENORIENTEERING_LATLON_H_

#include <qmath.h>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE


/**
 * @brief Specifies geographic location by latitude and longitude.
 * 
 * Latitude is a geographic coordinate that specifies the north-south
 * position (φ, phi).
 * 
 * Longitude is a geographic coordinate that specifies the east-west 
 * position (λ, lambda).
 * 
 * LatLon is meant to be similar to QGeoCoordinate (part of Qt since 5.2).
 * This Qt class might eventually replace LatLon. QGeoCoordinate has altitude
 * as an additional property which is rarely used in Mapper at the moment.
 * 
 * @see QGeoCoordinate (http://qt-project.org/doc/qt-5/qgeocoordinate.html)
 */
class LatLon
{
private:
	double latitude_value;
	double longitude_value;
	friend QDebug operator<<(QDebug dbg, const LatLon& lat_lon);
	
public:
	/**
	 * Constructs a new LatLon for the latitude and longitude given in degrees.
	 */
	LatLon(double latitude = 0.0, double longitude = 0.0);
	
	/**
	 * Returns a new LatLon for the latitude and longitude given in radiant.
	 */
	static LatLon fromRadiant(double latitude, double longitude);
	
	/**
	 * Returns the latitude value in degrees.
	 */
	double latitude() const;
	
	/**
	 * Returns the longitude value in degrees.
	 */
	double longitude() const;
	
	/**
	 * Returns true if this object has exactly the same latitude and longitude
	 * like another. FP precision issues are not taken into account.
	 */
	bool operator==(const LatLon& rhs) const;
	
	/**
	 * Returns true if this object has not exactly the same latitude and longitude
	 * like another. FP precision issues are not taken into account.
	 */
	bool operator!=(const LatLon& rhs) const;
};

/**
 * Dumps a LatLon to the debug output.
 * 
 * Note that this requires a *reference*, not a pointer.
 */
QDebug operator<<(QDebug dbg, const LatLon &lat_lon);



//### LatLon inline code ###

inline
LatLon::LatLon(double latitude, double longitude)
: latitude_value(latitude)
, longitude_value(longitude)
{
	; // nothing
}

inline
LatLon LatLon::fromRadiant(double latitude, double longitude)
{
	return LatLon(latitude * 180.0 / M_PI, longitude * 180.0 / M_PI);
}

inline
double LatLon::latitude() const
{
	return latitude_value;
}

inline
double LatLon::longitude() const
{
	return longitude_value;
}

inline
bool LatLon::operator==(const LatLon& rhs) const
{
	return (this->latitude_value == rhs.latitude_value) && (this->longitude_value == rhs.longitude_value);
}

inline
bool LatLon::operator!=(const LatLon& rhs) const
{
	return (this->latitude_value != rhs.latitude_value) || (this->longitude_value != rhs.longitude_value);
}

#endif
