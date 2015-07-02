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


static_assert(std::is_nothrow_move_constructible<MapCoord>::value, "MapCoord must be nothrow move constructible.");
static_assert(std::is_nothrow_move_assignable<MapCoord>::value, "MapCoord must be nothrow move assignable.");

static_assert(MapCoordF(1.0, 1.0) == QPointF(1.0, 1.0),
              "MapCoordF and QPointF constructors must use the same unit of measurement");
static_assert(MapCoord(1, 1) == MapCoord(MapCoordF(1.0, 1.0)),
              "MapCoord and MapCoordF constructors must use the same unit of measurement");
static_assert(MapCoord(1.0, 1.0) == MapCoord(QPointF(1.0, 1.0)),
              "MapCoord and QPointF constructors must use the same unit of measurement");
static_assert(MapCoord(1, 1) == MapCoord::fromRaw(1000, 1000),
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



void MapCoord::save(QXmlStreamWriter& xml) const
{
	XmlElementWriter element(xml, XmlStreamLiteral::coord);
	element.writeAttribute(literal::x, rawX());
	element.writeAttribute(literal::y, rawY());
	const int flags = getFlags();
	if (flags)
	{
		element.writeAttribute(literal::flags, flags);
	}
}

MapCoord MapCoord::load(QXmlStreamReader& xml)
{
	XmlElementReader element(xml);
	qint64 x = element.attribute<qint64>(literal::x);
	qint64 y = element.attribute<qint64>(literal::y);
	int flags = element.attribute<int>(literal::flags);
	return MapCoord::fromRaw(x, y, flags);
}

QString MapCoord::toString() const
{
	/* The buffer size must allow for
	 *  1x ';':   1
	 *  2x '-':   2
	 *  2x ' ':   2
	 *  2x the decimal digits for values up to 0..2^59-1:
	 *           34
	 *  1x the decimal digits for 0..2^8-1:
	 *            3
	 *  Total:   42 */
	static const std::size_t buf_size = 48;
	static char encoded[11] = "0123456789";
	char buffer[buf_size];
	
	// For efficiency, we construct the string from the back.
	int j = buf_size - 1;
	buffer[j] = ';';
	--j;
	
	int flags = getFlags();
	if (flags > 0)
	{
		do
		{
			buffer[j] = encoded[flags % 10];
			flags = flags / 10;
			--j;
		}
		while (flags != 0);
		
		buffer[j] = ' ';
		--j;
	}
	
	qint64 tmp = rawY();
	char sign = 0;
	if (tmp < 0)
	{
		sign = '-';
		tmp = -tmp; // NOTE: tmp never exceeds -2^60
	}
	do
	{
		buffer[j] = encoded[tmp % 10];
		tmp = tmp / 10;
		--j;
	}
	while (tmp != 0);
	if (sign)
	{
		buffer[j] = sign;
		--j;
		sign = 0;
	}
	
	buffer[j] = ' ';
	--j;
	
	tmp = rawX();
	if (tmp < 0)
	{
		sign = '-';
		tmp = -tmp; // NOTE: tmp never exceeds -2^60
	}
	do
	{
		buffer[j] = encoded[tmp % 10];
		tmp = tmp / 10;
		--j;
	}
	while (tmp != 0);
	if (sign)
	{
		buffer[j] = sign;
		--j;
	}
	
	++j;
	return QString::fromUtf8(buffer+j, buf_size-j);
}



QTextStream& operator>>(QTextStream& stream, MapCoord& coord)
{
	qint64 x, y;
	int flags = 0;
	char separator;
	stream >> x >> y >> separator;
	coord.setRawX(x);
	coord.setRawY(y);
	if (separator != ';')
	{
		stream >> flags >> separator;
	}
	coord.setFlags(flags);
	return stream;
}
