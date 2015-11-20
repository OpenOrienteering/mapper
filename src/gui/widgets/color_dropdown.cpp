/*
 *    Copyright 2012, 2013 Thomas Schöps, Kai Pastor
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


#include "color_dropdown.h"

#include "../../map.h"
#include "../../core/map_color.h"


ColorDropDown::ColorDropDown(const Map* map, const MapColor* initial_color, bool spot_colors_only, QWidget* parent)
: QComboBox(parent)
{
	addItem(tr("- none -"), QVariant::fromValue<const MapColor*>(NULL));
	
	int icon_size = style()->pixelMetric(QStyle::PM_SmallIconSize);
	QPixmap pixmap(icon_size, icon_size);
	
	int initial_index = 0;
	int num_colors = map->getNumColors();
	for (int i = 0; i < num_colors; ++i)
	{
		const MapColor* color = map->getColor(i);
		if (spot_colors_only && color->getSpotColorMethod() != MapColor::SpotColor)
			continue;
		
		if (initial_color == color)
			initial_index = count();
		
		pixmap.fill(*color);
		QString name = spot_colors_only ? color->getSpotColorName() : color->getName();
		addItem(QIcon(pixmap), name, QVariant::fromValue(color));
	}
	if (!spot_colors_only)
	{
		const int count = this->count();
		if (count > 0)
		{
			insertSeparator(count);
		}
		const MapColor* color = Map::getRegistrationColor();
		pixmap.fill(*color);
		addItem(QIcon(pixmap), color->getName(), QVariant::fromValue(color));
	}
	setCurrentIndex(initial_index);

	if (!spot_colors_only)
	{
		// FIXME: these methods will not work when the box contains only spot colors
		connect(map, SIGNAL(colorAdded(int, const MapColor*)), this, SLOT(colorAdded(int, const MapColor*)));
		connect(map, SIGNAL(colorChanged(int, const MapColor*)), this, SLOT(colorChanged(int, const MapColor*)));
		connect(map, SIGNAL(colorDeleted(int, const MapColor*)), this, SLOT(colorDeleted(int, const MapColor*)));
	}
}

const MapColor* ColorDropDown::color() const
{
	return itemData(currentIndex()).value<const MapColor*>();
}

void ColorDropDown::setColor(const MapColor* color)
{
	setCurrentIndex(findData(QVariant::fromValue(color)));
}

void ColorDropDown::colorAdded(int pos, const MapColor* color)
{
	int icon_size = style()->pixelMetric(QStyle::PM_SmallIconSize);
	QPixmap pixmap(icon_size, icon_size);
	pixmap.fill(*color);
	insertItem(pos + 1, color->getName(), QVariant::fromValue(color));
	setItemData(pos + 1, pixmap, Qt::DecorationRole);
}

void ColorDropDown::colorChanged(int pos, const MapColor* color)
{
	int icon_size = style()->pixelMetric(QStyle::PM_SmallIconSize);
	QPixmap pixmap(icon_size, icon_size);
	pixmap.fill(*color);
	setItemText(pos + 1, color->getName());
	setItemData(pos + 1, pixmap, Qt::DecorationRole);
}

void ColorDropDown::colorDeleted(int pos, const MapColor* color)
{
	Q_UNUSED(color);
	
	if (currentIndex() == pos + 1)
		setCurrentIndex(0);
	removeItem(pos + 1);
}
