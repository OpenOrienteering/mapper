/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014, 2015 Kai Pastor
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

#include "map_coord.h"

#include <type_traits>

#include <QLineF>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "../util/xml_stream_util.h"


static_assert(sizeof(qint32) <= sizeof(int), 
              "MapCoord::setX/Y uses qRound() returning int, xp/yp is of type qint32");

static_assert(!MapCoord::BoundsOffset().check_for_offset,
              "Default-constructed BoundsOffset must have check_for_offset == false.");
static_assert(MapCoord::BoundsOffset().x == 0,
              "Default-constructed BoundsOffset must have x == 0.");
static_assert(MapCoord::BoundsOffset().y == 0,
              "Default-constructed BoundsOffset must have y == 0.");
static_assert(MapCoord::BoundsOffset().isZero(),
              "Default-constructed BoundsOffset must be null.");

static_assert(std::is_nothrow_move_constructible<MapCoord>::value, "MapCoord must be nothrow move constructible.");
static_assert(std::is_nothrow_move_assignable<MapCoord>::value, "MapCoord must be nothrow move assignable.");

static_assert(MapCoordF(1.0, 1.0) == QPointF(1.0, 1.0),
              "MapCoordF and QPointF constructors must use the same unit of measurement");
static_assert(MapCoord(1, 1) == MapCoord(MapCoordF(1.0, 1.0)),
              "MapCoord and MapCoordF constructors must use the same unit of measurement");
static_assert(MapCoord(1.0, 1.0) == MapCoord(QPointF(1.0, 1.0)),
              "MapCoord and QPointF constructors must use the same unit of measurement");
static_assert(MapCoord(1, 1) == MapCoord::fromNative(1000, 1000),
              "MapCoord::fromRaw must use the native unit of measurement");

static_assert(MapCoord(1, 1) + MapCoord (2, -1) == MapCoord(3, 0),
              "MapCoord has component-wise operator+");
static_assert(MapCoord(1, 1) - MapCoord (2, -1) == MapCoord(-1, 2),
              "MapCoord has component-wise operator-");
static_assert(MapCoord(1, 1) * 2.1 == MapCoord(2.1, 2.1),
              "MapCoord has component-wise operator*(coord, factor)");
static_assert(0.5 * MapCoord(1, 1) == MapCoord(0.5, 0.5),
              "MapCoord has component-wise operator*(factor, coord)");
static_assert(MapCoord(1, 1) / 4 == MapCoord(0.25, 0.25),
              "MapCoord has component-wise operator/(coord, divisor");
static_assert(-MapCoord(-2, 1) == MapCoord(2, -1),
              "MapCoord has unary operator-()");

static_assert(!MapCoord(0.0, 0.0).isCurveStart(),
              "MapCoord::isCurveStart() must return false by default.");
static_assert(MapCoord(0.0, 0.0, MapCoord::CurveStart).isCurveStart(),
              "MapCoord::CurveStart must result in MapCoord::isCurveStart() returning true.");

static_assert(!MapCoord(0.0, 0.0).isClosePoint(),
              "MapCoord::isClosePoint() must return false by default.");
static_assert(MapCoord(0.0, 0.0, MapCoord::ClosePoint).isClosePoint(),
              "MapCoord::ClosePoint must result in MapCoord::isClosePoint() returning true.");

static_assert(!MapCoord(0.0, 0.0).isHolePoint(),
              "MapCoord::isHolePoint() must return false by default.");
static_assert(MapCoord(0.0, 0.0, MapCoord::HolePoint).isHolePoint(),
              "MapCoord::HolePoint must result in MapCoord::isHolePoint() returning true.");

static_assert(!MapCoord(0.0, 0.0).isDashPoint(),
              "MapCoord::isDashPoint() must return false by default.");
static_assert(MapCoord(0.0, 0.0, MapCoord::DashPoint).isDashPoint(),
              "MapCoord::DashPoint must result in MapCoord::isDashPoint() returning true.");

static_assert(!MapCoord(0.0, 0.0).isGapPoint(),
              "MapCoord::isGapPoint() must return false by default.");
static_assert(MapCoord(0.0, 0.0, MapCoord::GapPoint).isGapPoint(),
              "MapCoord::GapPoint must result in MapCoord::isGapPoint() returning true.");

static_assert(MapCoord(-1.0, 2.0).x() == -1,
              "MapCoord::x() must return original value (w/o flags)");
static_assert(MapCoord(-1.0, 2.0, 255).x() == -1.0,
              "MapCoord::x() must return original value (with flags)");

static_assert(MapCoord::fromNative(-1, 2).nativeX() == -1,
              "MapCoord::nativeX() must return original value (w/o flags)");
static_assert(MapCoord::fromNative(-1, 2, 255).nativeX() == -1,
              "MapCoord::nativeX() must return original value (with flags)");

#ifndef MAPPER_NO_QREAL_CHECK
// Check that QPointF/MapCoorF will actually use double.
static_assert(std::is_same<qreal, double>::value, "qreal is not double. This could work but was not tested.");
#endif

static_assert(std::is_nothrow_move_constructible<MapCoordF>::value, "MapCoord must be nothrow move constructible.");
static_assert(std::is_nothrow_move_assignable<MapCoordF>::value, "MapCoord must be nothrow move assignable.");

static_assert(QLineF(QPointF(0,0), -MapCoordF(1,0).perpRight()) == QLineF(QPointF(0,0), QPointF(1,0)).normalVector(),
              "MapCoordF::perpRight() must return a vector in opposite direction of QLineF::normalVector().");
static_assert(QLineF(QPointF(0,0), MapCoordF(1,0).normalVector()) == QLineF(QPointF(0,0), QPointF(1,0)).normalVector(),
              "MapCoordF::normalVector() must behave like QLineF::normalVector().");



namespace literal
{
	static const QLatin1String x("x");
	static const QLatin1String y("y");
	static const QLatin1String flags("flags");
}



namespace
{

// Acceptable coord bounds on import, derived from printing UI bounds
constexpr qint64 min_coord = -50000000;
constexpr qint64 max_coord = +50000000;

MapCoord::BoundsOffset bounds_offset;

inline
void applyBoundsOffset(qint64& x64, qint64& y64)
{
	x64 -= bounds_offset.x;
	y64 -= bounds_offset.y;
}
	
inline
void handleBoundsOffset(qint64& x64, qint64& y64)
{
	if (bounds_offset.check_for_offset)
	{
		bounds_offset.check_for_offset = false;
		if (x64 < min_coord || x64 > max_coord)
		{
			bounds_offset.x = x64;
			x64 = 0;
		}
		if (y64 < min_coord || y64 > max_coord)
		{
			bounds_offset.y = y64;
			y64 = 0;
		}
	}
	else
	{
		applyBoundsOffset(x64, y64);
	}
}

inline
void ensureBoundsForQint32(qint64 x64, qint64 y64)
{
	if ( x64 < std::numeric_limits<qint32>::min()
		 || x64 > std::numeric_limits<qint32>::max()
		 || y64 < std::numeric_limits<qint32>::min()
		 || y64 > std::numeric_limits<qint32>::max() )
	{
		throw std::range_error(QT_TRANSLATE_NOOP("MapCoord", "Coordinates are out-of-bounds."));
	}
}

} // namespace



MapCoord::BoundsOffset& MapCoord::boundsOffset()
{
	return bounds_offset;
}

bool MapCoord::isRegular() const
{
	return (xp > 2 * min_coord
	        && xp < 2 * max_coord
	        && yp > 2 * min_coord
	        && yp < 2 * max_coord );
}

MapCoord MapCoord::fromNative64(qint64 x64, qint64 y64)
{
	// We only need to check the storage bounds here because 
	// this is the technical invariant.
	// Printing bounds will be checked during map loading ATM.
	ensureBoundsForQint32(x64, y64);
	return MapCoord{ static_cast<qint32>(x64), static_cast<qint32>(y64), Flags() };
}

MapCoord MapCoord::fromNative64withOffset(qint64 x64, qint64 y64)
{
	applyBoundsOffset(x64, y64);
	ensureBoundsForQint32(x64, y64);
	return MapCoord { static_cast<qint32>(x64), static_cast<qint32>(y64), Flags() };
}

void MapCoord::save(QXmlStreamWriter& xml) const
{
	XmlElementWriter element(xml, XmlStreamLiteral::coord);
	element.writeAttribute(literal::x, xp);
	element.writeAttribute(literal::y, yp);
	if (fp)
	{
		element.writeAttribute(literal::flags, Flags::Int(fp));
	}
}

MapCoord MapCoord::load(QXmlStreamReader& xml)
{
	XmlElementReader element(xml);
	auto x64 = element.attribute<qint64>(literal::x);
	auto y64 = element.attribute<qint64>(literal::y);
	auto flags = element.attribute<Flags::Int>(literal::flags);
	
	handleBoundsOffset(x64, y64);
	ensureBoundsForQint32(x64, y64);
	return MapCoord { static_cast<qint32>(x64), static_cast<qint32>(y64), flags };
}

#ifndef NO_NATIVE_FILE_FORMAT
	
MapCoord::MapCoord(const LegacyMapCoord& coord)
 : xp{ decltype(xp)(coord.x >> 4) }
 , yp{ decltype(yp)(coord.y >> 4) }
 , fp{ Flags::Int((coord.x & 0xf) | ((coord.y & 0xf) << 4)) }
{
	//nothing
}

#endif

QString MapCoord::toString() const
{
	/* The buffer size must allow for
	 *  1x ';':   1
	 *  2x '-':   2
	 *  2x ' ':   2
	 *  2x the decimal digits for values up to 0..2^31:
	 *           20
	 *  1x the decimal digits for 0..2^8-1:
	 *            3
	 *  Total:   28 */
	constexpr std::size_t buf_size = 1+2+2+20+3;
	static const QChar encoded[10] = {
	    QLatin1Char{'0'}, QLatin1Char{'1'},
	    QLatin1Char{'2'}, QLatin1Char{'3'},
	    QLatin1Char{'4'}, QLatin1Char{'5'},
	    QLatin1Char{'6'}, QLatin1Char{'7'},
	    QLatin1Char{'8'}, QLatin1Char{'9'}
	};
	QChar buffer[buf_size];
	
	// For efficiency, we construct the string from the back.
	std::size_t j = buf_size - 1;
	buffer[j] = QLatin1Char{';'};
	--j;
	
	int flags = fp;
	if (flags > 0)
	{
		do
		{
			buffer[j] = encoded[flags % 10];
			flags = flags / 10;
			--j;
		}
		while (flags != 0);
		
		buffer[j] = QChar::Space;
		--j;
	}
	
	qint64 tmp = yp;
	static_assert(sizeof(decltype(tmp)) > sizeof(decltype(MapCoord::yp)),
	              "decltype(tmp) must be large enough to hold"
	              "-std::numeric_limits<decltype(MapCoord::yp)>::min()" );
	QChar sign { QChar::Null };
	if (tmp < 0)
	{
		sign = QLatin1Char{'-'};
		tmp = -tmp;
	}
	do
	{
		buffer[j] = encoded[tmp % 10];
		tmp = tmp / 10;
		--j;
	}
	while (tmp != 0);
	if (!sign.isNull())
	{
		buffer[j] = sign;
		--j;
		sign = QChar::Null;
	}
	
	buffer[j] = QChar::Space;
	--j;
	
	static_assert(sizeof(decltype(tmp)) > sizeof(decltype(MapCoord::xp)),
	              "decltype(tmp) must be large enough to hold"
	              "-std::numeric_limits<decltype(MapCoord::xp)>::min()" );
	tmp = xp;
	if (tmp < 0)
	{
		sign = QLatin1Char{'-'};
		tmp = -tmp;
	}
	do
	{
		buffer[j] = encoded[tmp % 10];
		tmp = tmp / 10;
		--j;
	}
	while (tmp != 0);
	if (!sign.isNull())
	{
		buffer[j] = sign;
		--j;
	}
	
	++j;
	Q_ASSERT(j < buf_size);
	j = qMin(j, buf_size);
	return QString(buffer+j, buf_size-j);
}



QTextStream& operator>>(QTextStream& stream, MapCoord& coord)
{
	// Always update all MapCoord members here (xp, yp, flags).
	qint64 x64, y64;
	QChar separator;
	stream >> x64 >> y64 >> separator;
	
	handleBoundsOffset(x64, y64);
	ensureBoundsForQint32(x64, y64);
	
	coord.xp = static_cast<qint32>(x64);
	coord.yp = static_cast<qint32>(y64);
	
	int flags = 0;
	if (separator == QChar::Space)
	{
		stream >> flags >> separator;
	}
	coord.setFlags(flags);
	
	return stream;
}
