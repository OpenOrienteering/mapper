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


#include "object.h"

#include <QFile>

#include "symbol.h"
#include "symbol_point.h"
#include "util.h"

Object::Object(Object::Type type, Symbol* symbol) : type(type), symbol(symbol), map(NULL)
{
	output_dirty = true;
	path_closed = false;
	extent = QRectF();
}
Object::~Object()
{
	int size = (int)output.size();
	for (int i = 0; i < size; ++i)
		delete output[i];
}

void Object::save(QFile* file)
{
	int symbol_index = -1;
	if (map)
		symbol_index = map->findSymbolIndex(symbol);
	file->write((const char*)&symbol_index, sizeof(int));
	
	int num_coords = (int)coords.size();
	file->write((const char*)&num_coords, sizeof(int));
	file->write((const char*)&coords[0], num_coords * sizeof(MapCoord));
	
	file->write((const char*)&path_closed, sizeof(bool));
}
void Object::load(QFile* file)
{
	int symbol_index;
	file->read((char*)&symbol_index, sizeof(int));
	if (symbol_index >= 0)
		symbol = map->getSymbol(symbol_index);
	
	int num_coords;
	file->read((char*)&num_coords, sizeof(int));
	coords.resize(num_coords);
	file->read((char*)&coords[0], num_coords * sizeof(MapCoord));
	
	file->read((char*)&path_closed, sizeof(bool));
}

bool Object::update(bool force)
{
	if (!force && !output_dirty)
		return false;
	
	if (map)
		map->removeRenderablesOfObject(this, false);
	clearOutput();
	
	// Calculate float coordinates
	MapCoordVectorF coordsF;
	int size = coords.size();
	coordsF.resize(size);
	for (int i = 0; i < size; ++i)
		coordsF[i] = MapCoordF(coords[i].xd(), coords[i].yd());
	
	// Create renderables
	symbol->createRenderables(this, coords, coordsF, path_closed, output);
	
	// Calculate extent and set this object as creator of the renderables
	extent = QRectF();
	RenderableVector::const_iterator end = output.end();
	for (RenderableVector::const_iterator it = output.begin(); it != end; ++it)
	{
		(*it)->setCreator(this);
		if ((*it)->getClipPath() == NULL)
		{
			if (extent.isValid())
				rectInclude(extent, (*it)->getExtent());
			else
				extent = (*it)->getExtent();
			assert(extent.right() < 999999);	// assert if bogus values are returned
		}
	}
	
	output_dirty = false;
	
	if (map)
	{
		map->insertRenderablesOfObject(this);
		if (extent.isValid())
			map->setObjectAreaDirty(extent);
	}
	
	return true;
}
void Object::clearOutput()
{
	if (map && extent.isValid())
		map->setObjectAreaDirty(extent);
	
	int size = output.size();
	for (int i = 0; i < size; ++i)
		delete output[i];
	
	output.clear();
	output_dirty = true;
}

void Object::scale(double factor)
{
	int coords_size = coords.size();
	for (int c = 0; c < coords_size; ++c)
	{
		MapCoord coord = coords[c];
		coord.setX(factor * coord.xd());
		coord.setY(factor * coord.yd());
		coords[c] = coord;
	}
	
	setOutputDirty();
}

void Object::setPathClosed(bool value)
{
	if (!path_closed && value)
	{
		if (!coords.empty())
			coords.push_back(coords[0]);
		
		path_closed = true;
		setOutputDirty();
	}
	else if (path_closed && !value)
	{
		if (!coords.empty())
			coords.pop_back();
		
		path_closed = false;
		setOutputDirty();
	}
}

bool Object::setSymbol(Symbol* new_symbol, bool no_checks)
{
	if (!no_checks && new_symbol && symbol)
	{
		if ((new_symbol->getType() == Symbol::Point || symbol->getType() == Symbol::Point) && !(new_symbol->getType() == Symbol::Point && symbol->getType() == Symbol::Point))
			return false;	// Changing something from or to point is not allowed
		if ((new_symbol->getType() == Symbol::Text || symbol->getType() == Symbol::Text) && !(new_symbol->getType() == Symbol::Text && symbol->getType() == Symbol::Text))
			return false;	// Changing something from or to text is not allowed
	}
	
	symbol = new_symbol;
	setOutputDirty();
	return true;
}

Object* Object::getObjectForType(Object::Type type, Symbol* symbol)
{
	if (type == Point)
		return new PointObject(symbol);
	else if (type == Path)
		return new PathObject(symbol);
	else
	{
		assert(false);
		return NULL;
	}
}

// ### PathObject ###

PathObject::PathObject(Symbol* symbol) : Object(Object::Path, symbol)
{
	assert(!symbol || (symbol->getType() == Symbol::Line || symbol->getType() == Symbol::Area || symbol->getType() == Symbol::Combined));
}
Object* PathObject::duplicate()
{
	PathObject* new_path = new PathObject(symbol);
	new_path->coords = coords;
	new_path->path_closed = path_closed;
	return new_path;
}

void PathObject::setCoordinate(int pos, MapCoord c)
{
	assert(pos >= 0 && pos < getCoordinateCount());
	coords[pos] = c;
	
	if (path_closed && pos == 0)
		coords[coords.size() - 1] = c;
}
void PathObject::addCoordinate(int pos, MapCoord c)
{
	assert(pos >= 0 && pos <= getCoordinateCount());
	coords.insert(coords.begin() + pos, c);
	
	if (path_closed && coords.size() == 1)
		coords.push_back(coords[0]);
}
void PathObject::addCoordinate(MapCoord c)
{
	if (path_closed)
	{
		if (coords.empty())
		{
			coords.push_back(c);
			coords.push_back(c);
		}
		else
			coords.insert(coords.begin() + (coords.size() - 1), c);
	}
	else
		coords.push_back(c);
}
void PathObject::deleteCoordinate(int pos)
{
	assert(pos >= 0 && pos < getCoordinateCount());
	coords.erase(coords.begin() + pos);
	
	if (path_closed && coords.size() == 1)
		coords.clear();
}

// ### PointObject ###

PointObject::PointObject(Symbol* symbol) : Object(Object::Point, symbol)
{
	assert(!symbol || (symbol->getType() == Symbol::Point));
	rotation = 0;
	coords.push_back(MapCoord(0, 0));
}
Object* PointObject::duplicate()
{
	PointObject* new_point = new PointObject(symbol);
	new_point->coords = coords;
	new_point->path_closed = path_closed;
	
	new_point->rotation = rotation;
	return new_point;
}

void PointObject::setPosition(MapCoord position)
{
	coords[0] = position;
	setOutputDirty();
}
MapCoord PointObject::getPosition()
{
	return coords[0];
}
void PointObject::setRotation(float new_rotation)
{
	PointSymbol* point = reinterpret_cast<PointSymbol*>(symbol);
	assert(point && point->isRotatable());
	
	rotation = new_rotation;
	setOutputDirty();
}

// ### TextObject ###

TextObject::TextObject(Symbol* symbol): Object(Object::Text, symbol)
{
	assert(!symbol || (symbol->getType() == Symbol::Text));
	coords.push_back(MapCoord(0, 0));
	text = "";
	h_align = AlignHCenter;
	v_align = AlignVCenter;
	rotation = 0;
}
Object* TextObject::duplicate()
{
	TextObject* new_text = new TextObject(symbol);
	new_text->coords = coords;
	new_text->path_closed = path_closed;
	
	new_text->text = text;
	new_text->h_align = h_align;
	new_text->v_align = v_align;
	new_text->rotation = rotation;
	return new_text;
}

void TextObject::setAnchorPosition(MapCoord position)
{
	coords.resize(1);
	coords[0] = position;
	setOutputDirty();
}
MapCoord TextObject::getAnchorPosition()
{
	return coords[0];
}
void TextObject::setBox(MapCoord midpoint, double width, double height)
{
	coords.resize(2);
	coords[0] = midpoint;
	coords[1] = MapCoord(width, height);
	setOutputDirty();
}

void TextObject::setText(const QString& text)
{
	this->text = text;
	setOutputDirty();
}

void TextObject::setHorizontalAlignment(TextObject::HorizontalAlignment h_align)
{
	this->h_align = h_align;
	setOutputDirty();
}
void TextObject::setVerticalAlignment(TextObject::VerticalAlignment v_align)
{
	this->v_align = v_align;
	setOutputDirty();
}

void TextObject::setRotation(float new_rotation)
{
	rotation = new_rotation;
	setOutputDirty();
}
