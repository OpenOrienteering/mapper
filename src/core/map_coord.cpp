/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014-2021 Kai Pastor
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

#include <cstdlib>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <type_traits>

#include <QChar>
#include <QLatin1Char>
#include <QLatin1String>
#include <QLineF>
#include <QStringRef>

#include "util/xml_stream_util.h"


namespace OpenOrienteering {

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
static_assert(MapCoord(0.0, 0.0, MapCoord::Flags(MapCoord::CurveStart)).isCurveStart(),
              "MapCoord::CurveStart must result in MapCoord::isCurveStart() returning true.");

static_assert(!MapCoord(0.0, 0.0).isClosePoint(),
              "MapCoord::isClosePoint() must return false by default.");
static_assert(MapCoord(0.0, 0.0, MapCoord::Flags(MapCoord::ClosePoint)).isClosePoint(),
              "MapCoord::ClosePoint must result in MapCoord::isClosePoint() returning true.");

static_assert(!MapCoord(0.0, 0.0).isHolePoint(),
              "MapCoord::isHolePoint() must return false by default.");
static_assert(MapCoord(0.0, 0.0, MapCoord::Flags(MapCoord::HolePoint)).isHolePoint(),
              "MapCoord::HolePoint must result in MapCoord::isHolePoint() returning true.");

static_assert(!MapCoord(0.0, 0.0).isDashPoint(),
              "MapCoord::isDashPoint() must return false by default.");
static_assert(MapCoord(0.0, 0.0, MapCoord::Flags(MapCoord::DashPoint)).isDashPoint(),
              "MapCoord::DashPoint must result in MapCoord::isDashPoint() returning true.");

static_assert(!MapCoord(0.0, 0.0).isGapPoint(),
              "MapCoord::isGapPoint() must return false by default.");
static_assert(MapCoord(0.0, 0.0, MapCoord::Flags(MapCoord::GapPoint)).isGapPoint(),
              "MapCoord::GapPoint must result in MapCoord::isGapPoint() returning true.");

static_assert(MapCoord(-1.0, 2.0).x() == -1,
              "MapCoord::x() must return original value (w/o flags)");
static_assert(MapCoord(-1.0, 2.0, MapCoord::Flags{255}).x() == -1.0,
              "MapCoord::x() must return original value (with flags)");

static_assert(MapCoord::fromNative(-1, 2).nativeX() == -1,
              "MapCoord::nativeX() must return original value (w/o flags)");
static_assert(MapCoord::fromNative(-1, 2, MapCoord::Flags{255}).nativeX() == -1,
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
		throw std::range_error(QT_TRANSLATE_NOOP("OpenOrienteering::MapCoord", "Coordinates are out-of-bounds."));
	}
}

}  // namespace



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
	auto flags = Flags{element.attribute<Flags::Int>(literal::flags)};
	
	handleBoundsOffset(x64, y64);
	ensureBoundsForQint32(x64, y64);
	return MapCoord { static_cast<qint32>(x64), static_cast<qint32>(y64), flags };
}

MapCoord MapCoord::load(qreal x, qreal y, Flags flags)
{
	auto x64 = qRound64(x * 1000);
	auto y64 = qRound64(y * 1000);
	
	handleBoundsOffset(x64, y64);
	ensureBoundsForQint32(x64, y64);
	return MapCoord { static_cast<qint32>(x64), static_cast<qint32>(y64), flags };
}

MapCoord MapCoord::load(const QPointF& p, MapCoord::Flags flags)
{
	return MapCoord::load(p.x(), p.y(), flags);
}



QString MapCoord::toString() const
{
	auto buffer = MapCoord::StringBuffer<QChar>();
	auto string = toString(buffer);
	string.detach();  // Note: public in C++, but not documented API.
	return string;
}

QString MapCoord::toString(MapCoord::StringBuffer<QChar>& buffer) const
{
	static const QChar encoded[10] = {
	    QLatin1Char{'0'}, QLatin1Char{'1'},
	    QLatin1Char{'2'}, QLatin1Char{'3'},
	    QLatin1Char{'4'}, QLatin1Char{'5'},
	    QLatin1Char{'6'}, QLatin1Char{'7'},
	    QLatin1Char{'8'}, QLatin1Char{'9'}
	};
	
	// For efficiency, we construct the string from the back,
	// using a bidirectional iterator.
	auto const last = end(buffer) - 1;
	auto first = last;
	*first-- = QLatin1Char{';'};
	
	auto div = std::div_t{static_cast<int>(fp), 0};
	static_assert(sizeof(div.quot) >= sizeof(fp),
	              "div.quot must be large enough to hold the flags" );
	if (div.quot > 0)
	{
		do
		{
			div = std::div(div.quot, 10);
			*first-- = encoded[div.rem];
		}
		while (div.quot != 0);
		
		*first-- = QChar::Space;
	}
	
	static_assert(sizeof(div.quot) >= sizeof(MapCoord::yp),
	              "div.quot must be large enough to hold"
	              "-std::numeric_limits<decltype(MapCoord::yp)>::min()" );
	div.quot = yp;
	do
	{
		div = std::div(div.quot, 10);
		*first-- = encoded[std::abs(div.rem)];
	}
	while (div.quot != 0);
	if (yp < 0)
	{
		*first-- = QLatin1Char{'-'};
	}
	
	*first-- = QChar::Space;
	
	static_assert(sizeof(div.quot) >= sizeof(MapCoord::xp),
	              "div.quot must be large enough to hold"
	              "-std::numeric_limits<decltype(MapCoord::xp)>::min()" );
	div.quot = xp;
	do
	{
		div = std::div(div.quot, 10);
		*first-- = encoded[std::abs(div.rem)];
	}
	while (div.quot != 0);
	if (xp < 0)
	{
		*first-- = QLatin1Char{'-'};
	}
	
	return QString::fromRawData(&*(first+1), int(std::distance(first, last)));
}

QByteArray MapCoord::toUtf8(MapCoord::StringBuffer<char>& buffer) const
{
	static auto* encoded = "0123456789";
	
	// For efficiency, we construct the string from the back,
	// using a bidirectional iterator.
	auto const last = end(buffer) - 1;
	auto first = last;
	*first-- = ';';
	
	auto div = std::div_t{static_cast<int>(fp), 0};
	static_assert(sizeof(div.quot) >= sizeof(fp),
	              "div.quot must be large enough to hold the flags" );
	if (div.quot > 0)
	{
		do
		{
			div = std::div(div.quot, 10);
			*first-- = encoded[div.rem];
		}
		while (div.quot != 0);
		
		*first-- = ' ';
	}
	
	static_assert(sizeof(div.quot) >= sizeof(MapCoord::yp),
	              "div.quot must be large enough to hold"
	              "-std::numeric_limits<decltype(MapCoord::yp)>::min()" );
	div.quot = yp;
	do
	{
		div = std::div(div.quot, 10);
		*first-- = encoded[std::abs(div.rem)];
	}
	while (div.quot != 0);
	if (yp < 0)
	{
		*first-- = '-';
	}
	
	*first-- = ' ';
	
	static_assert(sizeof(div.quot) >= sizeof(MapCoord::xp),
	              "div.quot must be large enough to hold"
	              "-std::numeric_limits<decltype(MapCoord::xp)>::min()" );
	div.quot = xp;
	do
	{
		div = std::div(div.quot, 10);
		*first-- = encoded[std::abs(div.rem)];
	}
	while (div.quot != 0);
	if (xp < 0)
	{
		*first-- = '-';
	}
	
	return QByteArray::fromRawData(&*(first+1), int(std::distance(first, last)));
}

MapCoord::MapCoord(QStringRef& text)
: MapCoord{}
{
	const int len = text.length();
	if (Q_UNLIKELY(len < 2))
		throw std::invalid_argument("Premature end of data");
	
	auto const* data = text.constData();
	int i = 0;
	
	auto const is_whitespace = [](QChar const c) {
		return c == QChar::Space
		       || c == QChar::LineFeed
		       || c == QChar::CarriageReturn;
	};
	auto const skip_word_break = [data, len, is_whitespace](int i) -> int {
		++i;
		while (i < len && is_whitespace(data[i]))
			++i;
		return i;
	};
	
	qint64 x64 = data[0].unicode();
	if (x64 == '-')
	{
		x64 = '0' - data[1].unicode();
		for (i = 2; i != len; ++i)
		{
			auto c = data[i].unicode();
			if (c < '0' || c > '9')
				break;
			else
				x64 = 10*x64 + '0' - c;
		}
	}
	else if (x64 >= '0' && x64 <= '9')
	{
		x64 -= '0';
		for (i = 1; i != len; ++i)
		{
			auto c = data[i].unicode();
			if (c < '0' || c > '9')
				break;
			else
				x64 = 10*x64 + c - '0';
		}
	}
	
	i = skip_word_break(i);
	if (Q_UNLIKELY(i+1 >= len))
		throw std::invalid_argument("Premature end of data");
	
	qint64 y64 = data[i].unicode();
	if (y64 == '-')
	{
		++i;
		y64 = '0' - data[i].unicode();
		for (++i; i != len; ++i)
		{
			auto c = data[i].unicode();
			if (c < '0' || c > '9')
				break;
			else
				y64 = 10*y64 + '0' - c;
		}
	}
	else if (y64 >= '0' && y64 <= '9')
	{
		y64 -= '0';
		for (++i; i != len; ++i)
		{
			auto c = data[i].unicode();
			if (c < '0' || c > '9')
				break;
			else
				y64 = 10*y64 + c - '0';
		}
	}
	
	handleBoundsOffset(x64, y64);
	ensureBoundsForQint32(x64, y64);
	xp = static_cast<qint32>(x64);
	yp = static_cast<qint32>(y64);
	
	if (i < len && is_whitespace(data[i]))
	{
		i = skip_word_break(i);
		if (Q_UNLIKELY(i == len))
			throw std::invalid_argument("Premature end of data");
		
		// there are no negative flags
		fp = Flags(data[i].unicode() - '0');
		for (++i; i < len; ++i)
		{
			auto c = data[i].unicode();
			if (c < '0' || c > '9')
				break;
			else
				fp = Flags(10*int(fp) + c - '0');
		}
	}
	
	if (Q_UNLIKELY(i >= len || data[i] != QLatin1Char{';'}))
		throw std::invalid_argument("Invalid data");
	
	i = skip_word_break(i);
	text = text.mid(i, len-i);
}


}  // namespace OpenOrienteering
