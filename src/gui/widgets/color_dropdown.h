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


#ifndef _OPENORIENTEERING_COLOR_DROPDOWN_H_
#define _OPENORIENTEERING_COLOR_DROPDOWN_H_

#include <QComboBox>

class Map;
class MapColor;

/**
 * A combobox which lets the user select a map color.
 */
class ColorDropDown : public QComboBox
{
Q_OBJECT
public:
	/**
	 * Constructs a new ColorDropDown for the colors of the given map.
	 * If spot_colors_only is true, it will only display fulltone spot colors.
	 */
	ColorDropDown(const Map* map, const MapColor* initial_color = 0, bool spot_colors_only = false, QWidget* parent = 0);
	
	/** Returns the selected color or NULL if no color selected. */
	const MapColor* color() const;
	
	/** Sets the selection to the given color. */
	void setColor(const MapColor* color);
	
protected slots:
	void colorAdded(int pos, MapColor* color);
	void colorChanged(int pos, MapColor* color);
	void colorDeleted(int pos, const MapColor* color);
};

#endif
