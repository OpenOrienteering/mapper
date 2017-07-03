/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2015, 2017 Kai Pastor
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


#ifndef OPENORIENTEERING_COLOR_DROPDOWN_H
#define OPENORIENTEERING_COLOR_DROPDOWN_H

#include <QtGlobal>
#include <QComboBox>
#include <QObject>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

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
	ColorDropDown(const Map* map, const MapColor* initial_color = nullptr, bool spot_colors_only = false, QWidget* parent = nullptr);
	
	/** Destructor. */
	~ColorDropDown() override;
	
	/** Returns the selected color or NULL if no color selected. */
	const MapColor* color() const;
	
	/** Sets the selection to the given color. */
	void setColor(const MapColor* color);
	
	/** Adds a color to the list. */
	void addColor(const MapColor* color);
	
	/** Updates a color in the list. */
	void updateColor(const MapColor* color);
	
	/** Removes a color from the list. */
	void removeColor(const MapColor* color);
	
protected:
	void onColorAdded(int, const MapColor* color);
	void onColorChanged(int, const MapColor* color);
	void onColorDeleted(int, const MapColor* color);
	
	const Map* map;
	const bool spot_colors_only;
};

#endif
