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

// ### ColorDropDown ###

ColorDropDown::ColorDropDown(Map* map, MapColor* initial_color, QWidget* parent) : QComboBox(parent)
{
	addItem(tr("- none -"), qVariantFromValue<void*>(NULL));
	
	int size = map->getNumColors();
	for (int i = 0; i < size; ++i)
	{
		MapColor* color = map->getColor(i);
		addItem(color->name, qVariantFromValue<void*>(color));
		setItemData(i + 1, color->color, Qt::DecorationRole);
	}
	setColor(initial_color);
	
	connect(map, SIGNAL(colorAdded(int,MapColor*)), this, SLOT(colorAdded(int,MapColor*)));
	connect(map, SIGNAL(colorChanged(int,MapColor*)), this, SLOT(colorChanged(int,MapColor*)));
	connect(map, SIGNAL(colorDeleted(int,MapColor*)), this, SLOT(colorDeleted(int,MapColor*)));
}
MapColor* ColorDropDown::color()
{
	return reinterpret_cast<MapColor*>(itemData(currentIndex()).value<void*>());
}
void ColorDropDown::setColor(MapColor* color)
{
	setCurrentIndex(findData(qVariantFromValue<void*>(color)));
}

void ColorDropDown::colorAdded(int pos, MapColor* color)
{
	insertItem(pos + 1, color->name, qVariantFromValue<void*>(color));
	setItemData(pos + 1, color->color, Qt::DecorationRole);
}
void ColorDropDown::colorChanged(int pos, MapColor* color)
{
	setItemData(pos + 1, color->color, Qt::DecorationRole);
}
void ColorDropDown::colorDeleted(int pos, MapColor* color)
{
	if (currentIndex() == pos + 1)
		setCurrentIndex(0);
	removeItem(pos + 1);
}

#include "moc_map_color.cpp"
