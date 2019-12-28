/*
 * Copyright (c) 2005-2019 Libor Pecháček.
 *
 * This file is part of CoVe 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cmath>

#include <QColor>

#include "libvectorizer/MapColor.h"

namespace cove {
/*! \class MapColor
 * \brief This class represents a color layer in topographic map.
 *
 * It element of three-dimensional space and can represent a color
 * from RGB or HSV color space. */
/*! \var double MapColor::x1
  First coordinate. */
/*! \var double MapColor::x2
  Second coordinate. */
/*! \var double MapColor::x3
  Third coordinate. */
/*! \var double MapColor::p
  Parameter of Minkowski metrics. */
/*! \var double MapColor::p1
  Precomputed value of 1/p. \sa p */

//! Constructor, ip is the value of Minkowski metrics.
MapColor::MapColor(double ip)
{
	setP(ip);
}

//! Virtual destructor
MapColor::~MapColor()
{
}

//! Constructor, ip is the value of Minkowski metrics, i1..3 are the initial
//! values.
MapColor::MapColor(int i1, int i2, int i3, double ip)
	: x1(i1)
	, x2(i2)
	, x3(i3)
{
	setP(ip);
}

/*! Sets current value of p.  Valid values are from 1 to infinity. */
void MapColor::setP(double p)
{
	if (p == 1)
	{
		currentDistImpl = &MapColor::distImplHamming;
	}
	else if (p == 2)
	{
		currentDistImpl = &MapColor::distImplEuclid;
	}
	else if (std::isinf(p))
	{
		currentDistImpl = &MapColor::distImplChebyshev;
	}
	else if (p > 1)
	{
		currentDistImpl = &MapColor::distImplMinkowski;
	}
	else
		return;

	this->p = p;
	p1 = 1 / p;
}

/*! Current value of p. */
double MapColor::getP()
{
	return p;
}

double MapColor::distImplHamming(double y1, double y2, double y3) const
{
	return fabs(x1 - y1) + fabs(x2 - y2) + fabs(x3 - y3);
}

double MapColor::distImplEuclid(double y1, double y2, double y3) const
{
	return sqrt(squaresImpl(y1, y2, y3));
}

inline double MapColor::squaresImpl(double y1, double y2, double y3) const
{
	double result, d;
	d = x1 - y1;
	result = d * d;
	d = x2 - y2;
	result += d * d;
	d = x3 - y3;
	return result + d * d;
}

double MapColor::distImplChebyshev(double y1, double y2, double y3) const
{
	double result = fabs(x1 - y1), d;
	if ((d = fabs(x2 - y2)) > result) result = d;
	if ((d = fabs(x3 - y3)) > result) result = d;
	return result;
}

double MapColor::distImplMinkowski(double y1, double y2, double y3) const
{
	double result, d;
	d = fabs(x1 - y1);
	result = pow(d, p);
	d = fabs(x2 - y2);
	result += pow(d, p);
	d = fabs(x3 - y3);
	result += pow(d, p);
	return pow(result, p1);
}

//! \brief Minkowski distance to the next vector.
double MapColor::distance(const OrganizableElement& o) const
{
	// only static_cast due to performance reasons (inner loop)
	const MapColor& y = static_cast<const MapColor&>(o);
	return (this->*currentDistImpl)(y.x1, y.x2, y.x3);
}

//! \brief Sum of squares (x1-y1)^2+...+(x3-y3)^2, used for quality computation.
double MapColor::squares(const OrganizableElement& o) const
{
	const MapColor& y = static_cast<const MapColor&>(o);
	return squaresImpl(y.x1, y.x2, y.x3);
}

//! \brief Addition.
void MapColor::add(const OrganizableElement& o)
{
	const MapColor& y = static_cast<const MapColor&>(o);
	x1 += y.x1;
	x2 += y.x2;
	x3 += y.x3;
}

//! \brief Subtraction.
void MapColor::subtract(const OrganizableElement& o)
{
	const MapColor& y = static_cast<const MapColor&>(o);
	x1 -= y.x1;
	x2 -= y.x2;
	x3 -= y.x3;
}

//! \brief Multiplication by scalar value.
void MapColor::multiply(const double y)
{
	x1 *= y;
	x2 *= y;
	x3 *= y;
}

/*! \fn virtual QRgb MapColor::getRGBTriplet() const = 0;
 * \brief Returns RGB representation of this color. */

/*! \fn virtual void MapColor::setRGBTriplet(const QRgb i) = 0;
 * \brief Initializes this color to value equivalent to given RGB value. */

/*! \class MapColorRGB
 * \brief This class represents a color from RGB color space with (inherited)
 * Minkowski metric */

//! Constructor, calls parents' constructor.
MapColorRGB::MapColorRGB(double ip)
	: MapColor(ip)
{
}

//! Constructor, calls parents' constructor, then initializes its value to i.
MapColorRGB::MapColorRGB(QRgb i, double ip)
	: MapColor(ip)
{
	setRGBTriplet(i);
}

OrganizableElement* MapColorRGB::clone() const
{
	return new MapColorRGB(*this);
}

QRgb MapColorRGB::getRGBTriplet() const
{
	return qRgb((int)round(x1), (int)round(x2), (int)round(x3));
}

void MapColorRGB::setRGBTriplet(const QRgb i)
{
	x1 = qRed(i);
	x2 = qGreen(i);
	x3 = qBlue(i);
}

/*! \class MapColorHSV
 * \brief This class represents a color from HSV color space with (inherited)
 * Minkowski metric */

//! Constructor, calls parents' constructor.
MapColorHSV::MapColorHSV(double ip)
	: MapColor(ip)
{
}

//! Constructor, calls parents' constructor, then initializes its value to i.
MapColorHSV::MapColorHSV(QRgb i, double ip)
	: MapColor(ip)
{
	setRGBTriplet(i);
}

OrganizableElement* MapColorHSV::clone() const
{
	return new MapColorHSV(*this);
}

QRgb MapColorHSV::getRGBTriplet() const
{
	// the (float) is here because we need to keep hue == -1.0 true when hue is
	// -1.0
	return QColor::fromHsvF(float(x1), x2, x3).rgb();
}

void MapColorHSV::setRGBTriplet(const QRgb i)
{
	qreal qx1, qx2, qx3;
	QColor(i).getHsvF(&qx1, &qx2, &qx3);
	x1 = qx1;
	x2 = qx2;
	x3 = qx3;
}
} // cove
