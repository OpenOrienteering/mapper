/*
 *    Copyright 2015 Kai Pastor
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


#ifndef OPENORIENTEERING_MAP_COORD_P_H
#define OPENORIENTEERING_MAP_COORD_P_H

#include <qglobal.h>

/**
 * The MapCoordElement stores a signed integer value together with some flags.
 */
class MapCoordElement
{
public:
	using value_type = qint64;
	
	/** The number of bits reserved for flags. */
	static const int bitsForFlags = 4;
	
private:
	/** The internal storage. */
	value_type internal_value;
	
	/** A mask for retrieving the flags from the internal value. */
	static constexpr int flagsMask();

public:
	/** Constructs an element with a value of zero and with flags unset. */
	constexpr MapCoordElement();
	
	/** Constructs an element with the same value and flags as another. */
	constexpr MapCoordElement(const MapCoordElement &) = default;
	
	/** Constructs an element with the given value and with flags unset. */
	explicit constexpr MapCoordElement(value_type value);
	
	/** Constructs an element with the given value and flags. */
	constexpr MapCoordElement(value_type value, int flags);
	
	/** Returns a MapCoordElement with the same flags but negated value. */
	constexpr MapCoordElement operator-() const;
	
	/** Returns the raw internal value with the flags bits unset. */
	constexpr value_type rawValue() const;
	
	/** Returns the stored value. */
	constexpr value_type value() const;
	
	/** Returns the stored flags. */
	constexpr int flags() const;
	
	/** Sets both the stored value and the stored flags. */
	void set(value_type value, int flags);
	
	/** Sets the stored value. */
	void setValue(value_type value);
	
	/** Sets the stored flags. */
	void setFlags(int flags);
	
	/** Sets the raw internal value, leaving the stored flags unchanged. */
	void setRawValue(value_type value);
	
	MapCoordElement& operator+=(const MapCoordElement &other);
	
	MapCoordElement& operator-=(const MapCoordElement &other);
	
	MapCoordElement& operator*=(qreal factor);
	
	MapCoordElement& operator/=(qreal divisor);
	
	friend class CoordXmlTest;
	
	friend constexpr bool operator==(const MapCoordElement lhs, const MapCoordElement rhs);
};

/** Compares two elements for equality. */
constexpr bool operator==(const MapCoordElement lhs, const MapCoordElement rhs);

/** Compares two elements for inequality. */
constexpr bool operator!=(const MapCoordElement lhs, const MapCoordElement rhs);

constexpr MapCoordElement operator+(MapCoordElement lhs, MapCoordElement rhs);

constexpr MapCoordElement operator-(MapCoordElement lhs, MapCoordElement rhs);

constexpr MapCoordElement operator*(MapCoordElement coord, qreal factor);

constexpr MapCoordElement operator*(qreal factor, const MapCoordElement coord);

constexpr MapCoordElement operator/(MapCoordElement coord, qreal divisor);



// ### MapCoordElement constexpr and inline implementation ###

constexpr int MapCoordElement::flagsMask()
{
	return (1 << bitsForFlags) - 1;
}

constexpr MapCoordElement::MapCoordElement()
 : internal_value{ 0 }
{
	// nothing
}

constexpr MapCoordElement::MapCoordElement(value_type value)
 : internal_value{ value << bitsForFlags }
{
	// nothing
}

constexpr MapCoordElement::MapCoordElement(value_type value, int flags)
 : internal_value{ (value << bitsForFlags) | (flags & flagsMask()) }
{
	// nothing
}

constexpr MapCoordElement MapCoordElement::operator-() const
{
	return MapCoordElement{ -value(), flags() };
}

constexpr MapCoordElement::value_type MapCoordElement::rawValue() const
{
	return internal_value & ~std::make_unsigned<value_type>::type(flagsMask());
}

constexpr MapCoordElement::value_type MapCoordElement::value() const
{
	return internal_value >> bitsForFlags;
}

constexpr int MapCoordElement::flags() const
{
	return internal_value & flagsMask();
}

inline
void MapCoordElement::set(value_type value, int flags)
{
	internal_value = (value << bitsForFlags) | (flags & flagsMask());
}

inline
void MapCoordElement::setValue(value_type value)
{
	internal_value = (value << bitsForFlags) | flags();
}

inline
void MapCoordElement::setFlags(int flags)
{
	internal_value = rawValue() | (flags & flagsMask());
}

inline
void MapCoordElement::setRawValue(value_type value)
{
	internal_value = (value & ~std::make_unsigned<value_type>::type(flagsMask())) | flags();
}

inline
MapCoordElement& MapCoordElement::operator+=(const MapCoordElement& other)
{
	setRawValue(rawValue() + other.rawValue());
	return *this;
}

inline
MapCoordElement& MapCoordElement::operator-=(const MapCoordElement& other)
{
	setRawValue(rawValue() - other.rawValue());
	return *this;
}

inline
MapCoordElement& MapCoordElement::operator*=(qreal factor)
{
	setValue(qRound64(value() * factor));
	return *this;
}

inline
MapCoordElement& MapCoordElement::operator/=(qreal divisor)
{
	setValue(qRound64(value() / divisor));
	return *this;
}

constexpr bool operator==(const MapCoordElement lhs, const MapCoordElement rhs)
{
	return lhs.internal_value == rhs.internal_value;
}

constexpr bool operator!=(const MapCoordElement lhs, const MapCoordElement rhs)
{
	return !(lhs == rhs);
}

constexpr MapCoordElement operator+(MapCoordElement lhs, MapCoordElement rhs)
{
	return MapCoordElement{ lhs.value() + rhs.value(), lhs.flags() };
}

constexpr MapCoordElement operator-(MapCoordElement lhs, MapCoordElement rhs)
{
	return MapCoordElement{ lhs.value() - rhs.value(), lhs.flags() };
}

constexpr MapCoordElement operator*(MapCoordElement coord, qreal factor)
{
	return MapCoordElement{ qRound64(coord.value() * factor), coord.flags() };
}

constexpr MapCoordElement operator*(qreal factor, MapCoordElement coord)
{
	return MapCoordElement{ qRound64(factor * coord.value()), coord.flags() };
}

constexpr MapCoordElement operator/(MapCoordElement coord, qreal divisor)
{
	return MapCoordElement{ qRound64(coord.value() / divisor), coord.flags() };
}



#endif
