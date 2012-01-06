/*
 *    Copyright 2011 Thomas Schöps
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


#ifndef _OPENORIENTEERING_MAP_COORD_H_
#define _OPENORIENTEERING_MAP_COORD_H_

#include <vector>
#include <math.h>

#include <QPointF>

/// Coordinates of a point in a map.
/// Saved as 64bit integers, where in addition some flags about the type of the point can be stored in the lowest 4 bits.
class MapCoord
{
public:
	/// Create coordinates from position given in millimeters
	inline MapCoord(double x, double y)
	{
		this->x = qRound64(x * 1000) << 4;
		this->y = qRound64(y * 1000) << 4;
	}
	
	// Get the coordinates as doubles
	inline double xd() {return (x >> 4) / 1000.0;}
	inline double yd() {return (y >> 4) / 1000.0;}
	
private:
	qint64 x;
	qint64 y;
};

/// Floating point map coordinates, only for rendering.
class MapCoordF
{
public:
	
	inline MapCoordF() {}
	inline MapCoordF(double x, double y) {setX(x); setY(y);}
	inline MapCoordF(const MapCoordF& copy) {x = copy.x; y = copy.y;}
	
    inline void setX(double x) {this->x = x;};
	double getX() const {return x;}
	
	inline void setY(double y) {this->y = y;};
	double getY() const {return y;}
	
	inline void normalize()
	{
		double length = sqrt(getX()*getX() + getY()*getY());
		if (length < 1e-08)
			return;
		
		setX(getX() / length);
		setY(getY() / length);
	}
	
	inline double length() const {return sqrt(x*x + y*y);}
	inline double lengthSquared() const {return x*x + y*y;}
	
	inline double lengthTo(const MapCoordF& to)
	{
		double dx = to.getX() - getX();
		double dy = to.getY() - getY();
		return sqrt(dx*dx + dy*dy);
	}
	
	inline double getAngle()
	{
		return atan2(y, x);
	}
	
	/// Get a perpendicular vector pointing to the right
	inline void perpRight()
	{
		double x = getX();
		setX(-getY());
		setY(x);
	}
	
	MapCoord toMapCoord()
	{
		return MapCoord(x, y);
	}
	
	inline bool operator== (const MapCoordF& other) const
	{
		return (other.x == x) && (other.y == y);
	}
	
    inline QPointF toQPointF()
	{
		return QPointF(x, y);
	}
	
private:
	double x;
	double y;
};

typedef std::vector<MapCoord> MapCoordVector;
typedef std::vector<MapCoordF> MapCoordVectorF;

#endif
