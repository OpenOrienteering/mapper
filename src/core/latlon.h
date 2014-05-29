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
 * 
 * Latitude is a geographic coordinate that specifies the north-south
 * position (φ, phi).
 * Longitude is a geographic coordinate that specifies the east-west 
 * position (λ, lambda).
 * 
 * @see QGeoCoordinate
 */
class LatLon
{
private:
	double latitude_value;
	double longitude_value;
	friend QDebug operator<<(QDebug dbg, const LatLon& lat_lon);
	
public:
	/**
	 * @brief Supported units of measurement for latitude and longitude.
	 */
	enum Unit
	{
		Radiant,
		Degrees
	};

	/**
	 * Constructs a new LatLon with latitude and longitude set to zero.
	 */
	LatLon();
	
	/**
	 * Constructs a new LatLon for the given latitude, longitude and unit.
	 */
	LatLon(double latitude_value, double longitude_value, Unit unit);
	
	/**
	 * Returns the latitude value in degrees.
	 */
	double getLatitudeInDegrees() const;
	
	/**
	 * Returns the longitude value in degrees.
	 */
	double getLongitudeInDegrees() const;
	
	/** 
	 * Returns the latitude value in radiant.
	 */
	double getLatitudeInRadiant() const;
	
	/**
	 * Returns the longitude value in radiant.
	 */
	double getLongitudeInRadiant() const;
	
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
: latitude_value(0.0)
, longitude_value(0.0)
{
	; // nothing
}

inline
LatLon::LatLon(double latitude, double longitude, Unit unit)
: latitude_value(latitude)
, longitude_value(longitude)
{
	if (unit == Degrees)
	{
		latitude_value  *= M_PI / 180;
		longitude_value *= M_PI / 180;
	}
}

inline
double LatLon::getLatitudeInRadiant() const
{
	return latitude_value;
}

inline
double LatLon::getLongitudeInRadiant() const
{
	return longitude_value;
}

inline
double LatLon::getLatitudeInDegrees() const
{
	return latitude_value * 180.0 / M_PI;
}

inline
double LatLon::getLongitudeInDegrees() const
{
	return longitude_value * 180.0 / M_PI;
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
