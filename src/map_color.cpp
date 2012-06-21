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


#include "map_color.h"

#include <assert.h>

#include "map.h"

void MapColor::updateFromCMYK()
{
	color.setCmykF(c, m, y, k);
	qreal rr, rg, rb;
	color.getRgbF(&rr, &rg, &rb);
	r = rr;
	g = rg;
	b = rb;
	assert(color.spec() == QColor::Cmyk);
}
void MapColor::updateFromRGB()
{
	color.setRgbF(r, g, b);
	color.convertTo(QColor::Cmyk);
	qreal rc, rm, ry, rk;
	color.getCmykF(&rc, &rm, &ry, &rk, NULL);
	c = rc;
	m = rm;
	y = ry;
	k = rk;
}

bool MapColor::equals(const MapColor& other, bool compare_priority) const
{
	return (name.compare(other.name, Qt::CaseInsensitive) == 0) &&
		   (!compare_priority || (priority == other.priority)) && 
		   (c == other.c) && (m == other.m) && (y == other.y) && (k == other.k) &&
		   (opacity == other.opacity);
}


// ### ColorDropDown ###

// allow explicit use of MapColor pointers in QVariant
Q_DECLARE_METATYPE(MapColor*)

ColorDropDown::ColorDropDown(Map* map, MapColor* initial_color, QWidget* parent) : QComboBox(parent)
{
	addItem(tr("- none -"), QVariant::fromValue<MapColor*>(NULL));
	
	int icon_size = style()->pixelMetric(QStyle::PM_SmallIconSize);
	QPixmap pixmap(icon_size, icon_size);
	
	int num_colors = map->getNumColors();
	for (int i = 0; i < num_colors; ++i)
	{
		MapColor* color = map->getColor(i);
		pixmap.fill(color->color);
		addItem(color->name, QVariant::fromValue(color));
		setItemData(i + 1, pixmap, Qt::DecorationRole);
	}
	setColor(initial_color);
	
	connect(map, SIGNAL(colorAdded(int,MapColor*)), this, SLOT(colorAdded(int,MapColor*)));
	connect(map, SIGNAL(colorChanged(int,MapColor*)), this, SLOT(colorChanged(int,MapColor*)));
	connect(map, SIGNAL(colorDeleted(int,MapColor*)), this, SLOT(colorDeleted(int,MapColor*)));
}

MapColor* ColorDropDown::color()
{
	return itemData(currentIndex()).value<MapColor*>();
}

void ColorDropDown::setColor(MapColor* color)
{
	setCurrentIndex(findData(QVariant::fromValue(color)));
}

void ColorDropDown::colorAdded(int pos, MapColor* color)
{
	int icon_size = style()->pixelMetric(QStyle::PM_SmallIconSize);
	QPixmap pixmap(icon_size, icon_size);
	pixmap.fill(color->color);
	insertItem(pos + 1, color->name, QVariant::fromValue(color));
	setItemData(pos + 1, pixmap, Qt::DecorationRole);
}

void ColorDropDown::colorChanged(int pos, MapColor* color)
{
	int icon_size = style()->pixelMetric(QStyle::PM_SmallIconSize);
	QPixmap pixmap(icon_size, icon_size);
	pixmap.fill(color->color);
	setItemText(pos + 1, color->name);
	setItemData(pos + 1, pixmap, Qt::DecorationRole);
}

void ColorDropDown::colorDeleted(int pos, MapColor* color)
{
	if (currentIndex() == pos + 1)
		setCurrentIndex(0);
	removeItem(pos + 1);
}
