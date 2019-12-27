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

#ifndef __MAPCOLOR_H__
#define __MAPCOLOR_H__

#include <QColor>

#include "libvectorizer/KohonenMap.h"

namespace cove {
class MapColor : public OrganizableElement
{
private:
	typedef double (MapColor::*DistImplPtr)(double, double, double) const;
	DistImplPtr currentDistImpl;
	double distImplHamming(double y1, double y2, double y3) const;
	double distImplEuclid(double y1, double y2, double y3) const;
	inline double squaresImpl(double y1, double y2, double y3) const;
	double distImplChebyshev(double y1, double y2, double y3) const;
	double distImplMinkowski(double y1, double y2, double y3) const;

protected:
	double x1, x2, x3, p, p1;
	MapColor(double ip = 2);
	MapColor(int i1, int i2, int i3, double ip = 2);

public:
	virtual ~MapColor() override;
	virtual double distance(const OrganizableElement& y) const override;
	virtual double squares(const OrganizableElement& y) const override;
	virtual void add(const OrganizableElement& y) override;
	virtual void subtract(const OrganizableElement& y) override;
	virtual void multiply(const double y) override;
	virtual QRgb getRGBTriplet() const = 0;
	virtual void setRGBTriplet(const QRgb i) = 0;
	void setP(double p);
	double getP();
};

class MapColorRGB : public MapColor
{
public:
	MapColorRGB(double ip = 2);
	MapColorRGB(QRgb i, double ip = 2);
	virtual OrganizableElement* clone() const override;
	virtual QRgb getRGBTriplet() const override;
	virtual void setRGBTriplet(const QRgb i) override;
};

class MapColorHSV : public MapColor
{
public:
	MapColorHSV(double ip = 2);
	MapColorHSV(QRgb i, double ip = 2);
	virtual OrganizableElement* clone() const override;
	virtual QRgb getRGBTriplet() const override;
	virtual void setRGBTriplet(const QRgb i) override;
};
} // cove

#endif
