/*
 *    Copyright 2012, 2013 Kai Pastor
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
 * Specifies geographic coordinates by latitude and longitude.
 */
class LatLon
{
public:
	/** 
	 * Latitude is a geographic coordinate that specifies the north-south
	 * position (φ, phi). The unit is radiant.
	 */
	double latitude;
	
	/**
	 * Longitude is a geographic coordinate that specifies the east-west 
	 * position (λ, lambda). The unit is radiant.
	 */
	double longitude;
	
	/**
	 * Constructs a new LatLon with latitude and longitude set to zero.
	 */
	LatLon();
	
	/**
	 * Constructs a new LatLon for the latitude and longitude.
	 * If given_in_degrees is true, the parameters' unit is degree, otherwise 
	 * the unit is radiant.
	 */
	LatLon(double latitude, double longitude, bool given_in_degrees = false);
	
	/**
	 * Returns the latitude value in degrees.
	 */
	double getLatitudeInDegrees() const;
	
	/**
	 * Returns the longitude value in degrees.
	 */
	double getLongitudeInDegrees() const;
	
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
LatLon::LatLon()
: latitude(0.0)
, longitude(0.0)
{
	; // nothing
}

inline
LatLon::LatLon(double latitude, double longitude, bool given_in_degrees)
: latitude(latitude)
, longitude(longitude)
{
	if (given_in_degrees)
	{
		this->latitude  *= M_PI / 180;
		this->longitude *= M_PI / 180;
	}
}

inline
double LatLon::getLatitudeInDegrees() const
{
	return latitude * 180.0 / M_PI;
}

inline
double LatLon::getLongitudeInDegrees() const
{
	return longitude * 180.0 / M_PI;
}

inline
bool LatLon::operator==(const LatLon& rhs) const
{
	return (this->latitude == rhs.latitude) && (this->longitude == rhs.longitude);
}

inline
bool LatLon::operator!=(const LatLon& rhs) const
{
	return (this->latitude != rhs.latitude) || (this->longitude != rhs.longitude);
}

#endif
