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
	/// Create null MapCoord
	inline MapCoord() : x(0), y(0) {}
	
	/// Create coordinates from position given in millimeters
	inline MapCoord(double x, double y)
	{
		this->x = qRound64(x * 1000) << 4;
		this->y = qRound64(y * 1000) << 4;
	}
	
	inline void setX(double x) {this->x = (qRound64(x * 1000) << 4) | (this->x & 15);}
	inline void setY(double y) {this->y = (qRound64(y * 1000) << 4) | (this->y & 15);}
	inline void setRawX(qint64 x) {this->x = (x << 4) | (this->x & 15);}
	inline void setRawY(qint64 y) {this->y = (y << 4) | (this->y & 15);}
	
	inline double xd() const {return (x >> 4) / 1000.0;}
	inline double yd() const {return (y >> 4) / 1000.0;}
	inline qint64 rawX() const {return x >> 4;}
	inline qint64 rawY() const {return y >> 4;}
	
	inline double lengthSquaredTo(const MapCoord& other)
	{
		double dx = xd() - other.xd();
		double dy = yd() - other.yd();
		return dx*dx + dy*dy;
	}
	inline double lengthTo(const MapCoord& other)
	{
		return sqrt(lengthSquaredTo(other));
	}
	inline bool isPositionEqualTo(const MapCoord& other)
	{
		return ((x >> 4) == (other.x >> 4)) && ((y >> 4) == (other.y >> 4));
	}
	
	// Is this point the first point of a cubic bezier curve segment?
	inline bool isCurveStart() const {return x & 1;}
	inline void setCurveStart(bool value) {x = (x & (~1)) | (value ? 1 : 0);}
	
	// Is this the first point of a hole in an area or the start of a hole for a line?
	inline bool isHolePoint() const {return y & 1;}
	inline void setHolePoint(bool value) {y = (y & (~1)) | (value ? 1 : 0);}
	
	// Should a dash of a line be placed here?
	inline bool isDashPoint() const {return y & 2;}
	inline void setDashPoint(bool value) {y = (y & (~2)) | (value ? 2 : 0);}
	
	inline QPointF toQPointF() const
	{
		return QPointF(xd(), yd());
	}
	
private:
	qint64 x;
	qint64 y;
};

/// Floating point map coordinates, only for rendering. TODO: replace with QPointF everywhere? Derive from QPointF and add nice-to-have features?
class MapCoordF
{
public:
	
	inline MapCoordF() {}
	inline MapCoordF(double x, double y) {setX(x); setY(y);}
	inline explicit MapCoordF(MapCoord coord) {setX(coord.xd()); setY(coord.yd());}
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
	
	inline double lengthTo(const MapCoordF& to) const
	{
		double dx = to.getX() - getX();
		double dy = to.getY() - getY();
		return sqrt(dx*dx + dy*dy);
	}
	inline double lengthToSquared(const MapCoordF& to) const
	{
		double dx = to.getX() - getX();
		double dy = to.getY() - getY();
		return dx*dx + dy*dy;
	}
	
	inline double dot(const MapCoordF& other) const
	{
		return x * other.x + y * other.y;
	}
	
	inline double getAngle() const
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
	
	MapCoord toMapCoord() const
	{
		return MapCoord(x, y);
	}
	
	inline bool operator== (const MapCoordF& other) const
	{
		return (other.x == x) && (other.y == y);
	}
	
	inline MapCoordF operator* (const float factor) const
	{
		return MapCoordF(factor*x, factor*y);
	}
	inline MapCoordF operator+ (const MapCoordF& other) const
	{
		return MapCoordF(x+other.x, y+other.y);
	}
	
	inline QPointF toQPointF() const
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
