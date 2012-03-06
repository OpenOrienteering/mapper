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
#include <assert.h>

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
    inline qint64 internalX() const {return x;}
    inline qint64 internalY() const {return y;}
    
    inline int getFlags() const {return (x & 15) | ((y & 15) << 4);}
    inline void setFlags(int flags) {x = (x & ~15) | (flags & 15); y = (y & ~15) | ((flags >> 4) & 15);}

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
	
	// Is this the start of a hole for a line?
	inline bool isHolePoint() const {return y & 1;}
	inline void setHolePoint(bool value) {y = (y & (~1)) | (value ? 1 : 0);}
	
	// Should a dash of a line be placed here?
	inline bool isDashPoint() const {return y & 2;}
	inline void setDashPoint(bool value) {y = (y & (~2)) | (value ? 2 : 0);}
	
	// Is this the last point of a closed path, which is at the same position as the first point? This is set in addition to isHolePoint().
	inline bool isClosePoint() const {return x & 2;}
	inline void setClosePoint(bool value) {x = (x & (~2)) | (value ? 2 : 0);}
	
	inline MapCoord& operator= (const MapCoord& other)
	{
		x = other.x;
		y = other.y;
		return *this;
	}
	inline bool operator== (const MapCoord& other) const
	{
		return (other.x == x) && (other.y == y);
	}
	inline bool operator != (const MapCoord& other) const
	{
		return (other.x != x) || (other.y != y);
	}
	
	inline MapCoord operator* (const double factor) const
	{
		MapCoord out(*this);
		out.setX(out.xd() * factor);
		out.setY(out.yd() * factor);
		return out;
	}
	inline MapCoord operator/ (const double scalar) const
	{
		assert(scalar != 0);
		MapCoord out(*this);
		out.setX(out.xd() / scalar);
		out.setY(out.yd() / scalar);
		return out;
	}
	inline MapCoord operator+ (const MapCoord& other) const
	{
		MapCoord out(*this);
		out.setRawX(out.rawX() + other.rawX());
		out.setRawY(out.rawY() + other.rawY());
		return out;
	}
	inline MapCoord operator- (const MapCoord& other) const
	{
		MapCoord out(*this);
		out.setRawX(out.rawX() - other.rawX());
		out.setRawY(out.rawY() - other.rawY());
		return out;
	}
	inline MapCoord& operator+ ()
	{
		return *this;
	}
	inline MapCoord operator- ()
	{
		MapCoord out(*this);
		out.setRawX(-out.rawX());
		out.setRawY(-out.rawY());
		return out;
	}
	
	inline friend MapCoord operator* (double scalar, const MapCoord& rhs_vector)
	{
		MapCoord out(rhs_vector);
		out.setRawX(qRound64(scalar * out.rawX()));
		out.setRawY(qRound64(scalar * out.rawY()));
		return out;
	}
	inline friend MapCoord operator/ (double scalar, const MapCoord& rhs_vector)
	{
		MapCoord out(rhs_vector);
		out.setRawX(qRound64(out.rawX() / scalar));
		out.setRawY(qRound64(out.rawY() / scalar));
		return out;
	}
	
	inline MapCoord& operator+= (const MapCoord& rhs_vector)
	{
		setRawX(rawX() + rhs_vector.rawX());
		setRawY(rawY() + rhs_vector.rawY());
		return *this;
	}
	inline MapCoord& operator-= (const MapCoord& rhs_vector)
	{
		setRawX(rawX() - rhs_vector.rawX());
		setRawY(rawY() - rhs_vector.rawY());
		return *this;
	}
	inline MapCoord& operator*= (const double factor)
	{
		setRawX(qRound64(factor * rawX()));
		setRawY(qRound64(factor * rawY()));
		return *this;
	}
	inline MapCoord& operator/= (const double scalar)
	{
		setRawX(qRound64(rawX() / scalar));
		setRawY(qRound64(rawY() / scalar));
		return *this;
	}
	
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
	inline void setY(double y) {this->y = y;};
	inline void setIntX(qint64 x) {this->x = 0.001 * x;}
	inline void setIntY(qint64 y) {this->y = 0.001 * y;}
	
	double getX() const {return x;}
	double getY() const {return y;}
	qint64 getIntX() const {return qRound64(1000 * x);}
	qint64 getIntY() const {return qRound64(1000 * y);}
	
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
	
	inline void toLength(float target_length)
	{
		double len = length();
		if (len > 1e-08)
		{
			double factor = target_length / len;
			x *= factor;
			y *= factor;
		}
	}
	
	inline double dot(const MapCoordF& other) const
	{
		return x * other.x + y * other.y;
	}
	
	/// Returns the angle of the vector relative to the vector (1, 0) in radians, range [-PI; +PI].
	/// Vector3(0, 1).getAngle() returns +PI/2, Vector3(0, -1).getAngle() returns -PI/2.
	inline double getAngle() const
	{
		return atan2(y, x);
	}
	
	/// Rotates the vector. Positive values for angle (in radians) result in a counter-clockwise rotation
	/// in a coordinate system where the x-axis is right and the y-axis is up.
	inline void rotate(double angle)
	{
		double new_angle = getAngle() + angle;
		double len = length();
		x = cos(new_angle) * len;
		y = sin(new_angle) * len;
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
	
	inline MapCoordF& operator= (const MapCoordF& other)
	{
		x = other.x;
		y = other.y;
		return *this;
	}
	inline bool operator== (const MapCoordF& other) const
	{
		return (other.x == x) && (other.y == y);
	}
	inline bool operator != (const MapCoordF& other) const
	{
		return (other.x != x) || (other.y != y);
	}
	
	inline MapCoordF operator* (const double factor) const
	{
		return MapCoordF(factor*x, factor*y);
	}
	inline MapCoordF operator/ (const double scalar) const
	{
		assert(scalar != 0);
		return MapCoordF(x/scalar, y/scalar);
	}
	inline MapCoordF operator+ (const MapCoordF& other) const
	{
		return MapCoordF(x+other.x, y+other.y);
	}
	inline MapCoordF operator- (const MapCoordF& other) const
	{
		return MapCoordF(x-other.x, y-other.y);
	}
	inline MapCoordF& operator+ ()
	{
		return *this;
	}
	inline MapCoordF operator- ()
	{
		return MapCoordF(-x, -y);
	}
	
	inline friend MapCoordF operator* (double scalar, const MapCoordF& rhs_vector)
	{
		return MapCoordF(scalar * rhs_vector.getX(), scalar * rhs_vector.getY());
	}
	inline friend MapCoordF operator/ (double scalar, const MapCoordF& rhs_vector)
	{
		return MapCoordF(scalar / rhs_vector.getX(), scalar / rhs_vector.getY());
	}
	
	inline MapCoordF& operator+= (const MapCoordF& rhs_vector)
	{
		x += rhs_vector.x;
		y += rhs_vector.y;
		return *this;
	}
	inline MapCoordF& operator-= (const MapCoordF& rhs_vector)
	{
		x -= rhs_vector.x;
		y -= rhs_vector.y;
		return *this;
	}
	inline MapCoordF& operator*= (const double factor)
	{
		x *= factor;
		y *= factor;
		return *this;
	}
	inline MapCoordF& operator/= (const double scalar)
	{
		assert(scalar != 0);
		x /= scalar;
		y /= scalar;
		return *this;
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
