/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2015 Kai Pastor
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

#include <cmath>
#include <vector>

#include <QPointF>

#include "map_coord_p.h"

class QString;
class QTextStream;
class QXmlStreamReader;
class QXmlStreamWriter;

/**
 * Coordinates of a point in a map, with optional flags.
 * 
 * This coordinate uses what we call native map coordinates.
 * One unit in native map coordinates is 1/1000th of a millimeter on the map paper.
 * 
 * The possible flags are:
 * 
 * <ul>
 * <li><b>isCurveStart():</b> If set, this point is the first one in a sequence of
 *     four points which define a cubic bezier curve.</li>
 * <li><b>isHolePoint():</b> If set, this point marks the end of a distinct path.
 *     Only area objects may have more than one path. The first path can be
 *     understood as outline, while the remaining paths can be seen as holes in
 *     the area, subject to the filling rule.
 * <li><b>isDashPoint():</b> This flag marks special points in the path.
 *     The effect depends on the type of symbol.
 * <li><b>isClosePoint():</b> If this flag is set for the last point of a path,
 *     this path is treated as closed. The point's coordinates must be equal to
 *     the first point's coordinates in this path.</li>
 * <li><b>isGapPoint():</b> This flag is used to indicate the begin and the end
 *     of gaps in a path. This only used internally at the moment (for dealing
 *     with dashed lines.)
 * </ul>
 * 
 * These coordinates are implemented as 64 bit integers where lowest 4 bits are
 * used to store the flags.
 * 
 * MapCoord (and other parts of Mapper) rely on an arithmetic implementation of
 * the operator>>(). However, the behavior of operator>>() on negative integers
 * is implementation defined. This class comes with static and dynamic tests for
 * this prerequisite.
 */
class MapCoord
{
private:
	/** Benchmark */
	friend class CoordXmlTest;
	
	MapCoordElement x;
	MapCoordElement y;
	
public:
	enum Flag
	{
		CurveStart = 1 << 0,
		ClosePoint = 1 << 1,
		HolePoint  = 1 << 4,
		DashPoint  = 1 << 5,
		GapPoint   = 1 << 6,
	};
	
	/** Creates a MapCoord with position at the origin and without any flags set. */
	constexpr MapCoord();
	
	/** Copy constructor. */
	constexpr MapCoord(const MapCoord&) = default;
	
	/** Creates a MapCoord from a position given in millimeters on the map.
	 * 
	 * This is a convenience constructor for efficient construction of a point
	 * at the origin i.e. MapCoord(0, 0), or for simple vectors i.e. 
	 * MapCoord(1, 0). There intentionally is no version with flags - you need
	 * to use other argument types than int if the compiler complains about
	 * ambiguity.
	 */
	constexpr MapCoord(int x, int y);
	
	/** Creates a MapCoord from a position given in millimeters on the map. */
	constexpr MapCoord(qreal x, qreal y);
	
	/** Creates a MapCoord with the given flags from a position given in millimeters on the map. */
	constexpr MapCoord(qreal x, qreal y, int flags);
	
	/** Creates a MapCoord with the position taken from a QPointF. */
	explicit constexpr MapCoord(QPointF point);
	
	/** Creates a MapCoord with the given flags and with the position taken from a QPointF. */
	constexpr MapCoord(QPointF point, int flags);
	
private:
	/** Creates a MapCoord with internal-format elements. */
	constexpr MapCoord(MapCoordElement x, MapCoordElement y);
	
public:
	/** Creates a MapCoord from native map coordinates. */
	static constexpr MapCoord fromRaw(qint64 x, qint64 y);
	
	/** Creates a MapCoord from native map coordinates. */
	static constexpr MapCoord fromRaw(qint64 x, qint64 y, int flags);
	
	/** Assignment operator */
	MapCoord& operator= (const MapCoord& other) = default;
	
	
	/** Returns the coord's x position in millimeters on the map. */
	constexpr qreal xd() const;
	
	/** Returns the coord's y position in millimeters on the map. */
	constexpr qreal yd() const;
	
	/** Sets the coord's x position to a value in millimeters on the map. */
	inline void setX(qreal x);
	
	/** Sets the coord's y position to a value in millimeters on the map. */
	inline void setY(qreal y);
	
	
	/** Returns the coord's x position in native map coords. */
	constexpr qint64 rawX() const;
	
	/** Returns the coord's y position in native map coords. */
	constexpr qint64 rawY() const;
	
	/** Sets the coord's x position to a value in native map coords. */
	inline void setRawX(qint64 new_x);
	
	/** Sets the coord's y position to a value in native map coords. */
	inline void setRawY(qint64 new_y);
	
	
	/** Returns the coord's flags separately, merged into the lowest 8 bits of an int. */
	constexpr int getFlags() const;
	
	/** Sets the flags as retrieved by getFlags(). */
	void setFlags(int flags);
	
	
	/**
	 * Returns the length of this coord, seen as a vector from the origin
	 * to the given coordinate, in millimeters on the map.
	 */
	constexpr qreal length() const;
	
	/**
	 * Returns the squared length of this coord, seen as a vector from the origin
	 * to the given coordinate, in millimeters on the map. Faster than length().
	 */
	constexpr qreal lengthSquared() const;
	
	/**
	 * Returns the distance from this coord to the other
	 * in millimeters on the map.
	 */
	constexpr qreal distanceTo(const MapCoord& other) const;
	
	/**
	 * Returns the squared distance from this coord to the other
	 * in millimeters on the map. Faster than lengthTo().
	 */
	constexpr qreal distanceSquaredTo(const MapCoord& other) const;
	
	
	/**
	 * Returns if this coord's position is equal to that of the other one.
	 */
	constexpr bool isPositionEqualTo(const MapCoord& other) const;
	
	
	/** Is this point the first point of a cubic bezier curve segment? */
	constexpr bool isCurveStart() const;
	
	/** Sets the curve start flag. */
	void setCurveStart(bool value);
	
	/**
	 * Is this the last point of a closed path, which is at the same position
	 * as the first point? This is set in addition to isHolePoint().
	 */
	constexpr bool isClosePoint() const;
	
	/** Sets the close point flag. */
	void setClosePoint(bool value);
	
	
	/** Is this the start of a hole for a line? */
	constexpr bool isHolePoint() const;
	
	/** Sets the hole point flag. */
	void setHolePoint(bool value);
	
	
	/** Is this coordinate a special dash point? */
	constexpr bool isDashPoint() const;
	
	/** Sets the dash point flag. */
	void setDashPoint(bool value);
	
	
	/** Is this coordinate a gap point? */
	constexpr bool isGapPoint() const;
	
	/** Sets the gap point flag. */
	void setGapPoint(bool value);
	
	
	/** Additive inverse. */
	constexpr MapCoord operator-() const;
	
	/** Component-wise addition. */
	MapCoord& operator+= (const MapCoord& rhs_vector);
	
	/** Component-wise addition. */
	MapCoord& operator+= (const QPointF& rhs_vector);
	
	/** Component-wise subtraction. */
	MapCoord& operator-= (const MapCoord& rhs_vector);
	
	/** Component-wise subtraction. */
	MapCoord& operator-= (const QPointF& rhs_vector);
	
	/** Multiply with scalar factor. */
	MapCoord& operator*= (qreal factor);
	
	/** Divide by scalar factor. */
	MapCoord& operator/= (qreal scalar);
	
	
	/** Converts the coord's position to a QPointF. */
	constexpr explicit operator QPointF() const;
	
	
	/**
	 * Writes raw coordinates and flags to a string.
	 */
	QString toString() const;
	
	
	/** Saves the MapCoord in xml format to the stream. */
	void save(QXmlStreamWriter& xml) const;
	
	/** Loads the MapCoord in xml format from the stream. */
	static MapCoord load(QXmlStreamReader& xml);
	
	
	friend constexpr bool operator==(const MapCoord& lhs, const MapCoord& rhs);
	friend constexpr MapCoord operator+(const MapCoord& lhs, const MapCoord& rhs);
	friend constexpr MapCoord operator-(const MapCoord& lhs, const MapCoord& rhs);
	friend constexpr MapCoord operator*(const MapCoord& lhs, qreal factor);
	friend constexpr MapCoord operator*(qreal factor, const MapCoord& rhs);
	friend constexpr MapCoord operator/(const MapCoord& lhs, qreal divisor);
	
};

/** Compare MapCoord for equality. */
constexpr bool operator==(const MapCoord& lhs, const MapCoord& rhs);

/** Compare MapCoord for inequality. */
constexpr bool operator!=(const MapCoord& lhs, const MapCoord& rhs);


/** Component-wise addition of MapCoord. */
constexpr MapCoord operator+(const MapCoord& lhs, const MapCoord& rhs);

/** Component-wise subtraction of MapCoord. */
constexpr MapCoord operator-(const MapCoord& lhs, const MapCoord& rhs);

/** Multiply MapCoord with scalar factor. */
constexpr MapCoord operator*(const MapCoord& lhs, qreal factor);

/** Multiply MapCoord with scalar factor. */
constexpr MapCoord operator*(qreal factor, const MapCoord& rhs);

/** Divide MapCoord by scalar factor. */
constexpr MapCoord operator/(const MapCoord& lhs, qreal divisor);


/**
 * Reads raw coordinates and flags from a text stream.
 * @see MapCoord::toString()
 */
QTextStream& operator>>(QTextStream& stream, MapCoord& coord);



/**
 * Floating point map coordinates, only for rendering.
 * Store the coordinate position in doubles as millimeters on the map paper.
 * In contrast to MapCoord, does not store flags.
 * 
 * TODO: replace with QPointF everywhere? Derive from QPointF and add nice-to-have features?
 */
class MapCoordF
{
public:
	
	/** Creates an uninitialized MapCoordF. */
	constexpr MapCoordF() : x(0.0), y(0.0) {}
	/** Creates a MapCoordF with the given position in map coordinates. */
	constexpr MapCoordF(double x, double y) : x{x}, y{y} {}
	/** Creates a MapCoordF from a MapCoord, skipping its flags. */
	constexpr explicit MapCoordF(MapCoord coord) : x{coord.xd()}, y{coord.yd()} {}
	/** Creates a MapCoordF from a QPointF. */
	constexpr explicit MapCoordF(QPointF point) : x{point.x()}, y{point.y()} {}
	
	/** Copy constructor. */
	MapCoordF(const MapCoordF&) = default;
	
	
	/** Assignment operator. */
	MapCoordF& operator= (const MapCoordF&) = default;
	
	
	/** Sets the x coordinate to a new value in map coordinates. */
	inline void setX(double x) {this->x = x;}
	/** Sets the y coordinate to a new value in map coordinates. */
	inline void setY(double y) {this->y = y;}
	/** Sets the x coordinate to a new value in native map coordinates. */
	inline void setIntX(qint64 x) {this->x = 0.001 * x;}
	/** Sets the y coordinate to a new value in native map coordinates. */
	inline void setIntY(qint64 y) {this->y = 0.001 * y;}
	
	/** Returns the x coordinate in map coordinates. */
	double getX() const {return x;}
	/** Returns the y coordinate in map coordinates. */
	double getY() const {return y;}
	/** Returns the x coordinate in native map coordinates. */
	qint64 getIntX() const {return qRound64(1000 * x);}
	/** Returns the y coordinate in native map coordinates. */
	qint64 getIntY() const {return qRound64(1000 * y);}
	
	/** Adds the given offset (in map coordinates) to the position. */
	inline void move(double dx, double dy) {x += dx; y += dy;}
	/** Adds the given offset (in native map coordinates) to the position. */
	inline void moveInt(qint64 dx, qint64 dy) {x += 0.001 * dx; y += 0.001 * dy;}
	
	/**
	 * Normalizes the length of the vector from the origin
	 * to the coordinate position to one (in map coordinates).
	 */
	inline void normalize()
	{
		double length_squared = x*x + y*y;
		if (length_squared > 1e-16)
		{
			double length = sqrt(length_squared);
			x /= length;
			y /= length;
		}
	}
	
	/**
	 * Returns the length of the vector from the origin to the
	 * coordinate position (in map coordinates).
	 */
	inline double length() const {return sqrt(x*x + y*y);}
	/**
	 * Returns the squared length of the vector from the origin to the
	 * coordinate position (in map coordinates). Faster than length().
	 */
	inline double lengthSquared() const {return x*x + y*y;}
	
	/**
	 * Returns the length of the coordinate to the other coordinate
	 * (in map coordinates).
	 */
	inline double lengthTo(const MapCoordF& to) const
	{
		double dx = to.getX() - getX();
		double dy = to.getY() - getY();
		return sqrt(dx*dx + dy*dy);
	}
	/**
	 * Returns the squared length of the coordinate to the other coordinate
	 * (in map coordinates). Faster than lengthTo().
	 */
	inline double lengthToSquared(const MapCoordF& to) const
	{
		double dx = to.getX() - getX();
		double dy = to.getY() - getY();
		return dx*dx + dy*dy;
	}
	
	/**
	 * Treats this coordinate as vector and changes its length
	 * to the given target length, while keeping the vector's direction.
	 * Does nothing if the vector is very close to (0, 0).
	 */
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
	
	/** Returns the dot product between this and the other coordinate. */
	inline double dot(const MapCoordF& other) const
	{
		return x * other.x + y * other.y;
	}

	/** Returns the cross product between this and the other coordinate. */
    inline double cross(const MapCoordF& other) const
    {
        return x * other.y - y * other.x;
    }
	
	/**
	 * Returns the angle of the vector relative to the vector (1, 0) in radians,
	 * range [-PI; +PI].
	 * 
	 * Vector3(0, 1).getAngle() returns +PI/2,
	 * Vector3(0, -1).getAngle() returns -PI/2.
	 */
	inline double getAngle() const
	{
		return atan2(y, x);
	}
	
	/**
	 * Rotates the vector. Positive values for angle (in radians) result
	 * in a counter-clockwise rotation in a coordinate system where the x-axis
	 * is right and the y-axis is up.
	 */
	inline MapCoordF& rotate(double angle)
	{
		double new_angle = getAngle() + angle;
		double len = length();
		x = cos(new_angle) * len;
		y = sin(new_angle) * len;
		return *this;
	}
	
	/** Replaces this vector with a perpendicular vector pointing to the right. */
	inline void perpRight()
	{
		double x = getX();
		setX(-getY());
		setY(x);
	}
	
	/**
	 * Converts the MapCoordF to a MapCoord (without flags).
	 * See also toCurveStartMapCoord().
	 */
	constexpr MapCoord toMapCoord() const
	{
		return MapCoord(x, y);
	}
	
	/**
	 * Converts the MapCoordF to a MapCoord (with the curve start flag set).
	 */
	MapCoord toCurveStartMapCoord() const
	{
		MapCoord coord = MapCoord(x, y);
		coord.setCurveStart(true);
		return coord;
	}
	
	/**
	 * Converts the MapCoordF to a QPointf.
	 */
	inline explicit operator QPointF() const
	{
		return QPointF(x, y);
	}
	
	/** Tests for exact equality. Be aware of floating point inaccuracies! */
	inline bool operator== (const MapCoordF& other) const
	{
		return (other.x == x) && (other.y == y);
	}
	/** Tests for exact inequality. Be aware of floating point inaccuracies! */
	inline bool operator != (const MapCoordF& other) const
	{
		return (other.x != x) || (other.y != y);
	}
	/** Multiply with a scalar */
	inline MapCoordF operator* (const double factor) const
	{
		return MapCoordF(factor*x, factor*y);
	}
	/** Divide by a scalar */
	inline MapCoordF operator/ (const double scalar) const
	{
		Q_ASSERT(scalar != 0);
		return MapCoordF(x/scalar, y/scalar);
	}
	/** Component-wise addition */
	inline MapCoordF operator+ (const MapCoordF& other) const
	{
		return MapCoordF(x+other.x, y+other.y);
	}
	/** Component-wise subtraction */
	inline MapCoordF operator- (const MapCoordF& other) const
	{
		return MapCoordF(x-other.x, y-other.y);
	}
	/** no-operation */
	inline MapCoordF& operator+ ()
	{
		return *this;
	}
	/** Additive inverse */
	inline MapCoordF operator- ()
	{
		return MapCoordF(-x, -y);
	}
	
	/** Multiply with a scalar */
	inline friend MapCoordF operator* (double scalar, const MapCoordF& rhs_vector)
	{
		return MapCoordF(scalar * rhs_vector.getX(), scalar * rhs_vector.getY());
	}
	/** Divide by a scalar */
	inline friend MapCoordF operator/ (double scalar, const MapCoordF& rhs_vector)
	{
		return MapCoordF(scalar / rhs_vector.getX(), scalar / rhs_vector.getY());
	}
	
	/** Component-wise addition */
	inline MapCoordF& operator+= (const MapCoordF& rhs_vector)
	{
		x += rhs_vector.x;
		y += rhs_vector.y;
		return *this;
	}
	/** Component-wise subtraction */
	inline MapCoordF& operator-= (const MapCoordF& rhs_vector)
	{
		x -= rhs_vector.x;
		y -= rhs_vector.y;
		return *this;
	}
	/** Multiply with a scalar */
	inline MapCoordF& operator*= (const double factor)
	{
		x *= factor;
		y *= factor;
		return *this;
	}
	/** Divide by a scalar */
	inline MapCoordF& operator/= (const double scalar)
	{
		Q_ASSERT(scalar != 0);
		x /= scalar;
		y /= scalar;
		return *this;
	}
	
private:
	double x;
	double y;
};

QTextStream& operator>>(QTextStream& stream, MapCoord& coord);

typedef std::vector<MapCoord> MapCoordVector;
typedef std::vector<MapCoordF> MapCoordVectorF;



// ### MapCoord inline code ###

constexpr MapCoord::MapCoord()
 : x { 0 }
 , y { 0 }
{
	// nothing else
}

constexpr MapCoord::MapCoord(int x, int y)
 : x { x*1000 }
 , y { y*1000 }
{
	// nothing else
}

constexpr MapCoord::MapCoord(qreal x, qreal y)
 : x { qRound64(x*1000) }
 , y { qRound64(y*1000) }
{
	// nothing else
}

constexpr MapCoord::MapCoord(qreal x, qreal y, int flags)
 : x { qRound64(x*1000), flags }
 , y { qRound64(y*1000), flags >> MapCoordElement::bitsForFlags }
{
	// nothing else
}

constexpr MapCoord::MapCoord(QPointF point)
 : MapCoord { point.x(), point.y() }
{
	// nothing else
}

constexpr MapCoord::MapCoord(QPointF point, int flags)
 : MapCoord { point.x(), point.y(), flags }
{
	// nothing else
}

constexpr MapCoord::MapCoord(MapCoordElement x, MapCoordElement y)
 : x { x }
 , y { y }
{
	// nothing else
}

constexpr MapCoord MapCoord::fromRaw(qint64 x, qint64 y)
{
	return MapCoord{ MapCoordElement{ x }, MapCoordElement{ y } };
}

constexpr MapCoord MapCoord::fromRaw(qint64 x, qint64 y, int flags)
{
	return MapCoord{ MapCoordElement{ x, flags }, MapCoordElement{ y, flags >> MapCoordElement::bitsForFlags} };
}

constexpr qreal MapCoord::xd() const
{
	return rawX() / 1000.0;
}

constexpr qreal MapCoord::yd() const
{
	return rawY() / 1000.0;
}

inline
void MapCoord::setX(qreal x)
{
	this->x.setValue(qRound64(x * 1000));
}

inline
void MapCoord::setY(qreal y)
{
	this->y.setValue(qRound64(y * 1000));
}

constexpr qint64 MapCoord::rawX() const
{
	return x.value();
}

constexpr qint64 MapCoord::rawY() const
{
	return y.value();
}

inline
void MapCoord::setRawX(qint64 new_x)
{
	x.setValue(new_x);
}

inline
void MapCoord::setRawY(qint64 new_y)
{
	y.setValue(new_y);
}

constexpr int MapCoord::getFlags() const
{
	return x.flags() | (y.flags() << MapCoordElement::bitsForFlags);
}

inline
void MapCoord::setFlags(int flags)
{
	x.setFlags(flags);
	y.setFlags(flags >> MapCoordElement::bitsForFlags);
}

constexpr qreal MapCoord::length() const
{
	return sqrt(lengthSquared());
}

constexpr qreal MapCoord::lengthSquared() const
{
	return xd()*xd() + yd()*yd();
}

constexpr qreal MapCoord::distanceTo(const MapCoord& other) const
{
	return sqrt(distanceSquaredTo(other));
}

constexpr qreal MapCoord::distanceSquaredTo(const MapCoord& other) const
{
	return (*this - other).lengthSquared();
}

constexpr bool MapCoord::isPositionEqualTo(const MapCoord& other) const
{
	return (x.rawValue() == other.x.rawValue()) && (y.rawValue() == other.y.rawValue());
}

constexpr bool MapCoord::isCurveStart() const {
	return x.flags() & 1;
}

inline
void MapCoord::setCurveStart(bool value)
{
	x.setFlags((x.flags() & ~1) | (value<<0));
}

constexpr bool MapCoord::isClosePoint() const
{
	return x.flags() & 2;
}

inline
void MapCoord::setClosePoint(bool value)
{
	x.setFlags((x.flags() & ~2) | (value<<1));
}

constexpr bool MapCoord::isHolePoint() const
{
	return y.flags() & 1;
}

inline
void MapCoord::setHolePoint(bool value)
{
	y.setFlags((y.flags() & ~1) | (value<<0));
	//y = (y & (~1)) | (value ? 1 : 0);
}

constexpr bool MapCoord::isDashPoint() const
{
	return y.flags() & 2;
}

inline
void MapCoord::setDashPoint(bool value)
{
	y.setFlags((y.flags() & ~2) | (value<<1));
}

constexpr bool MapCoord::isGapPoint() const
{
	return y.flags() & 4;
}

inline
void MapCoord::setGapPoint(bool value)
{
	y.setFlags((y.flags() & ~4) | (value<<2));
}


constexpr MapCoord MapCoord::operator-() const
{
	return MapCoord { -x, -y };
}

inline
MapCoord& MapCoord::operator+=(const MapCoord& rhs_vector)
{
	x += rhs_vector.x;
	y += rhs_vector.y;
	return *this;
}

inline
MapCoord& MapCoord::operator-=(const MapCoord& rhs_vector)
{
	x -= rhs_vector.x;
	y -= rhs_vector.y;
	return *this;
}

inline
MapCoord& MapCoord::operator*=(qreal factor)
{
	x *= factor;
	y *= factor;
	return *this;
}

inline
MapCoord& MapCoord::operator/=(qreal divisor)
{
	x /= divisor;
	y /= divisor;
	return *this;
}

constexpr MapCoord::operator QPointF() const
{
	return QPointF(xd(), yd());
}



constexpr bool operator==(const MapCoord& lhs, const MapCoord& rhs)
{
	return (lhs.x == rhs.x) && (lhs.y == rhs.y);
}


constexpr bool operator!=(const MapCoord& lhs, const MapCoord& rhs)
{
	return !(lhs == rhs);
}


constexpr MapCoord operator+(const MapCoord& lhs, const MapCoord& rhs)
{
	return MapCoord{ lhs.x + rhs.x, lhs.y + rhs.y };
}

constexpr MapCoord operator-(const MapCoord& lhs, const MapCoord& rhs)
{
	return MapCoord{ lhs.x - rhs.x, lhs.y - rhs.y };
}

constexpr MapCoord operator*(const MapCoord& lhs, double factor)
{
	return MapCoord{ lhs.x * factor, lhs.y * factor };
}

constexpr MapCoord operator*(double factor, const MapCoord& rhs)
{
	return MapCoord{ factor * rhs.x, factor * rhs.y };
}

constexpr MapCoord operator/(const MapCoord& lhs, double divisor)
{
	return MapCoord{ lhs.x / divisor, lhs.y / divisor };
}



#endif
