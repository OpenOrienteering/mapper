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


#ifndef _OPENORIENTEERING_MAP_COORD_H_
#define _OPENORIENTEERING_MAP_COORD_H_

#include <cassert>
#include <cmath>
#include <vector>

#include <QPointF>
#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

/**
 * Coordinates of a point in a map, defined in native map coordinates,
 * with optional flags about the type of the point.
 * Saved as 64bit integers, where the flags are be stored in the lowest 4 bits.
 * 
 * One unit in native map coordinates equals 1/1000th of a millimeter on the
 * map paper.
 * 
 * The possible flags are:
 * 
 * <ul>
 * <li><b>isCurveStart():</b> if set, this coord is the first of a sequence of
 *     four points which define a cubic bezier curve.</li>
 * <li><b>isHolePoint():</b> if set, this point marks the beginning of a hole in
 *     a line (i.e. is the last point of a segment), or is the last point of
 *     a component of an area object (i.e. the next point begins a new hole
 *     in the area). This is usually always set at the end of a path object's
 *     coordinate list, if the object is not closed.<li>
 * <li><b>isDashPoint():</b> marks special points which are interpreted differently
 *     depending on the object's symbol.</li>
 * <li><b>isClosePoint():</b> is set for the last point in closed parts in
 *     objects. Its position must be equal to the first point in the path part.</li>
 * </ul>
 */
class MapCoord
{
public:
	/** Creates a MapCoord with position at the origin and without any flags set. */
	inline MapCoord() : x(0), y(0) {}
	
	/** Creates coordinates from a position given in millimeters on the map. */
	inline MapCoord(double x, double y)
	{
		this->x = qRound64(x * 1000) << 4;
		this->y = qRound64(y * 1000) << 4;
	}
	
	/** Creates a MapCoord with the position taken from a QPointF. */
	inline explicit MapCoord(QPointF point)
	{
		this->x = qRound64(point.x() * 1000) << 4;
		this->y = qRound64(point.y() * 1000) << 4;
	}
	
	/** Creates a MapCoord from native map coordinates. */
	static inline MapCoord fromRaw(qint64 x, qint64 y)
	{
		MapCoord new_coord;
		new_coord.x = x << 4;
		new_coord.y = y << 4;
		return new_coord;
	}
	
	/** Sets the coord's x position to a value in millimeters on the map. */
	inline void setX(double x) {this->x = (qRound64(x * 1000) << 4) | (this->x & 15);}
	/** Sets the coord's y position to a value in millimeters on the map. */
	inline void setY(double y) {this->y = (qRound64(y * 1000) << 4) | (this->y & 15);}
	/** Sets the coord's x position to a value in native map coords. */
	inline void setRawX(qint64 new_x) {this->x = (new_x << 4) | (this->x & 15);}
	/** Sets the coord's y position to a value in native map coords. */
	inline void setRawY(qint64 new_y) {this->y = (new_y << 4) | (this->y & 15);}
	
	/** Returns the coord's x position in millimeters on the map. */
	inline double xd() const {return (x >> 4) / 1000.0;}
	/** Returns the coord's y position in millimeters on the map. */
	inline double yd() const {return (y >> 4) / 1000.0;}
	/** Returns the coord's x position in native map coords. */
	inline qint64 rawX() const {return x >> 4;}
	/** Returns the coord's y position in native map coords. */
	inline qint64 rawY() const {return y >> 4;}
	/** Returns the coord's x position as is, i.e. with flags in the lowest 4 bits. */
	inline qint64 internalX() const {return x;}
	/** Returns the coord's y position as is, i.e. with flags in the lowest 4 bits. */
	inline qint64 internalY() const {return y;}

	/** Returns the coord's flags separately, merged into the lowest 8 bits of an int. */
	inline int getFlags() const {return (x & 15) | ((y & 15) << 4);}
	/** Sets the flags as retrieved by getFlags(). */
	inline void setFlags(int flags) {x = (x & ~15) | (flags & 15); y = (y & ~15) | ((flags >> 4) & 15);}

	/**
	 * Returns the length of this coord, seen as a vector from the origin
	 * to the given coordinate, in millimeters on the map.
	 */
	inline double length() const {return sqrt(xd()*xd() + yd()*yd());}
	/**
	 * Returns the squared length of this coord, seen as a vector from the origin
	 * to the given coordinate, in millimeters on the map. Faster than length().
	 */
	inline double lengthSquared() const {return xd()*xd() + yd()*yd();}

	/**
	 * Returns the length from this coord to the other
	 * in millimeters on the map.
	 */
	inline double lengthTo(const MapCoord& other)
	{
		return sqrt(lengthSquaredTo(other));
	}
	/**
	 * Returns the squared length from this coord to the other
	 * in millimeters on the map. Faster than lengthTo().
	 */
	inline double lengthSquaredTo(const MapCoord& other)
	{
		double dx = xd() - other.xd();
		double dy = yd() - other.yd();
		return dx*dx + dy*dy;
	}
	
	/**
	 * Returns if this coord's position is equal to that of the other one.
	 */
	inline bool isPositionEqualTo(const MapCoord& other)
	{
		return ((x >> 4) == (other.x >> 4)) && ((y >> 4) == (other.y >> 4));
	}
	
	/** Is this point the first point of a cubic bezier curve segment? */
	inline bool isCurveStart() const {return x & 1;}
	/** Sets the curve start flag. */
	inline void setCurveStart(bool value) {x = (x & (~1)) | (value ? 1 : 0);}
	
	/** Is this the start of a hole for a line? */
	inline bool isHolePoint() const {return y & 1;}
	/** Sets the hole point flag. */
	inline void setHolePoint(bool value) {y = (y & (~1)) | (value ? 1 : 0);}
	
	/** Is this coordinate a special dash point? */
	inline bool isDashPoint() const {return y & 2;}
	/** Sets the dash point flag. */
	inline void setDashPoint(bool value) {y = (y & (~2)) | (value ? 2 : 0);}
	
	/**
	 * Is this the last point of a closed path, which is at the same position
	 * as the first point? This is set in addition to isHolePoint().
	 */
	inline bool isClosePoint() const {return x & 2;}
	/** Sets the close point flag. */
	inline void setClosePoint(bool value) {x = (x & (~2)) | (value ? 2 : 0);}
	
	/** Assignment operator */
	inline MapCoord& operator= (const MapCoord& other)
	{
		x = other.x;
		y = other.y;
		return *this;
	}
	/** Compare for equality */
	inline bool operator== (const MapCoord& other) const
	{
		return (other.x == x) && (other.y == y);
	}
	/** Compare for inequality */
	inline bool operator != (const MapCoord& other) const
	{
		return (other.x != x) || (other.y != y);
	}
	
	/** Multiply with scalar factor */
	inline MapCoord operator* (const double factor) const
	{
		MapCoord out(*this);
		out.setX(out.xd() * factor);
		out.setY(out.yd() * factor);
		return out;
	}
	/** Divide by scalar factor */
	inline MapCoord operator/ (const double scalar) const
	{
		assert(scalar != 0);
		MapCoord out(*this);
		out.setX(out.xd() / scalar);
		out.setY(out.yd() / scalar);
		return out;
	}
	/** Component-wise addition */
	inline MapCoord operator+ (const MapCoord& other) const
	{
		MapCoord out(*this);
		out.setRawX(out.rawX() + other.rawX());
		out.setRawY(out.rawY() + other.rawY());
		return out;
	}
	/** Component-wise subtraction */
	inline MapCoord operator- (const MapCoord& other) const
	{
		MapCoord out(*this);
		out.setRawX(out.rawX() - other.rawX());
		out.setRawY(out.rawY() - other.rawY());
		return out;
	}
	/** no-operation */
	inline MapCoord& operator+ ()
	{
		return *this;
	}
	/** additive inverse */
	inline MapCoord operator- ()
	{
		MapCoord out(*this);
		out.setRawX(-out.rawX());
		out.setRawY(-out.rawY());
		return out;
	}
	
	/** Multiply with scalar factor */
	inline friend MapCoord operator* (double scalar, const MapCoord& rhs_vector)
	{
		MapCoord out(rhs_vector);
		out.setRawX(qRound64(scalar * out.rawX()));
		out.setRawY(qRound64(scalar * out.rawY()));
		return out;
	}
	/** Divide by scalar factor */
	inline friend MapCoord operator/ (double scalar, const MapCoord& rhs_vector)
	{
		MapCoord out(rhs_vector);
		out.setRawX(qRound64(out.rawX() / scalar));
		out.setRawY(qRound64(out.rawY() / scalar));
		return out;
	}
	
	/** Component-wise addition */
	inline MapCoord& operator+= (const MapCoord& rhs_vector)
	{
		setRawX(rawX() + rhs_vector.rawX());
		setRawY(rawY() + rhs_vector.rawY());
		return *this;
	}
	/** Component-wise subtraction */
	inline MapCoord& operator-= (const MapCoord& rhs_vector)
	{
		setRawX(rawX() - rhs_vector.rawX());
		setRawY(rawY() - rhs_vector.rawY());
		return *this;
	}
	/** Multiply with scalar factor */
	inline MapCoord& operator*= (const double factor)
	{
		setRawX(qRound64(factor * rawX()));
		setRawY(qRound64(factor * rawY()));
		return *this;
	}
	/** Divide by scalar factor */
	inline MapCoord& operator/= (const double scalar)
	{
		setRawX(qRound64(rawX() / scalar));
		setRawY(qRound64(rawY() / scalar));
		return *this;
	}
	
	/** Converts the coord's position to a QPointF. */
	inline QPointF toQPointF() const
	{
		return QPointF(xd(), yd());
	}
	
	/** Saves the MapCoord in xml format to the stream. */
	void save(QXmlStreamWriter& xml) const
	{
		xml.writeStartElement("coord");
		xml.writeAttribute("x", QString::number(rawX()));
		xml.writeAttribute("y", QString::number(rawY()));
		int flags = getFlags();
		if (flags != 0)
			xml.writeAttribute("flags", QString::number(flags));
		xml.writeEndElement(/*coord*/);
	}
	
	/** Loads the MapCoord in xml format from the stream. */
	static MapCoord load(QXmlStreamReader& xml)
	{
		Q_ASSERT(xml.name() == "coord");
		MapCoord coord;
		coord.setRawX(xml.attributes().value("x").toString().toLongLong());
		coord.setRawY(xml.attributes().value("y").toString().toLongLong());
		coord.setFlags(xml.attributes().value("flags").toString().toInt());
		xml.skipCurrentElement();
		return coord;
	}

private:
	qint64 x;
	qint64 y;
};

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
	inline MapCoordF() {}
	/** Creates a MapCoordF with the given position in map coordinates. */
	inline MapCoordF(double x, double y) {setX(x); setY(y);}
	/** Creates a MapCoordF from a MapCoord, skipping its flags. */
	inline explicit MapCoordF(MapCoord coord) {setX(coord.xd()); setY(coord.yd());}
	/** Creates a MapCoordF from a QPointF. */
	inline explicit MapCoordF(QPointF point) {setX(point.x()); setY(point.y());}
	/** Copy constructor. */
	inline MapCoordF(const MapCoordF& copy) {x = copy.x; y = copy.y;}
	
	/** Sets the x coordinate to a new value in map coordinates. */
    inline void setX(double x) {this->x = x;};
	/** Sets the y coordinate to a new value in map coordinates. */
	inline void setY(double y) {this->y = y;};
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
		double length = sqrt(getX()*getX() + getY()*getY());
		if (length < 1e-08)
			return;
		
		setX(getX() / length);
		setY(getY() / length);
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
	 * Changes the length of the vector from the origin to this position
	 * to the given target length, while keeping the vector's direction.
	 * Does nothing if the position is very close to (0, 0).
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
	 * Scales the vector to a given length
	 * 
	 * TODO: this is a duplicate of toLength(), delete one variant!
	 */
	inline void setLength(double length)
	{
		double cur_length = sqrt(getX()*getX() + getY()*getY());
		if (cur_length < 1e-08)
			return;
		
		setX(getX() * length / cur_length);
		setY(getY() * length / cur_length);
	}
	
	/**
	 * Converts the MapCoordF to a MapCoord (without flags).
	 * See also toCurveStartMapCoord().
	 */
	MapCoord toMapCoord() const
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
	inline QPointF toQPointF() const
	{
		return QPointF(x, y);
	}
	
	/** Assignment operator. */
	inline MapCoordF& operator= (const MapCoordF& other)
	{
		x = other.x;
		y = other.y;
		return *this;
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
		assert(scalar != 0);
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
		assert(scalar != 0);
		x /= scalar;
		y /= scalar;
		return *this;
	}
	
private:
	double x;
	double y;
};

typedef std::vector<MapCoord> MapCoordVector;
typedef std::vector<MapCoordF> MapCoordVectorF;

/**
 * Converts a vector of MapCoords to a vector of MapCoordFs.
 */
inline void mapCoordVectorToF(const MapCoordVector& coords, MapCoordVectorF& out_coordsF)
{
	int size = coords.size();
	out_coordsF.resize(size);
	for (int i = 0; i < size; ++i)
		out_coordsF[i] = MapCoordF(coords[i]);
}

#endif
