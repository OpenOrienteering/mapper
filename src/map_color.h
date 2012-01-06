/*
 *    Copyright 2011 Thomas Schöps
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


#ifndef _OPENORIENTEERING_MAP_COLOR_H_
#define _OPENORIENTEERING_MAP_COLOR_H_

#include <QString>
#include <QColor>
#include <QComboBox>

class Map;

struct MapColor
{
	void updateFromCMYK();
	void updateFromRGB();
	
	QString name;
	int priority;
	
	// values are in range [0; 1]
	float c, m, y, k;
	float r, g, b;
	float opacity;
	
	QColor color;
};

class ColorDropDown : public QComboBox
{
Q_OBJECT
public:
	ColorDropDown(Map* map, MapColor* initial_color = NULL, QWidget* parent = NULL);
	
	/// Returns the selected color or NULL if no color selected
	MapColor* color();
	
protected slots:
	void colorAdded(int pos, MapColor* color);
	void colorChanged(int pos, MapColor* color);
	void colorDeleted(int pos, MapColor* color);
	
private:
	Map* map;
};

#endif
