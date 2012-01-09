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


#include "symbol_area.h"

#include <QFile>

#include "object.h"

AreaSymbol::AreaSymbol() : Symbol(Symbol::Area)
{
	color = NULL;
}
AreaSymbol::~AreaSymbol()
{
}

void AreaSymbol::createRenderables(Object* object, const MapCoordVectorF& coords, RenderableVector& output)
{
	if (coords.size() >= 3)
	{
		// The shape output is even created if the area is not filled with a color
		// because the QPainterPath created by it is needed as clip path for the fill objects
		output.push_back(new AreaRenderable(this, coords, object->getCoordinateVector()));
	}
}
void AreaSymbol::colorDeleted(int pos, MapColor* color)
{
    if (color == this->color)
		this->color = NULL;
}

void AreaSymbol::saveImpl(QFile* file, Map* map)
{
	int temp = map->findColorIndex(color);
	file->write((const char*)&temp, sizeof(int));
}
void AreaSymbol::loadImpl(QFile* file, Map* map)
{
	int temp;
	file->read((char*)&temp, sizeof(int));
	color = map->getColor(temp);
}
