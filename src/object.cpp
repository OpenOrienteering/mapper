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

#include "util.h"
#include "symbol.h"
#include "symbol_point.h"
#include "symbol_text.h"
#include "map_editor.h"

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
	
	// Central handling of sub-types here to avoid virtual methods
	if (type == Point)
	{
		PointObject* point = reinterpret_cast<PointObject*>(this);
		PointSymbol* point_symbol = reinterpret_cast<PointSymbol*>(point->getSymbol());
		if (point_symbol->isRotatable())
		{
			float rotation = point->getRotation();
			file->write((const char*)&rotation, sizeof(float));
		}
	}
	else if (type == Text)
	{
		TextObject* text = reinterpret_cast<TextObject*>(this);
		float rotation = text->getRotation();
		file->write((const char*)&rotation, sizeof(float));
		int temp = (int)text->getHorizontalAlignment();
		file->write((const char*)&temp, sizeof(int));
		temp = (int)text->getVerticalAlignment();
		file->write((const char*)&temp, sizeof(int));
		saveString(file, text->getText());
	}
}
void Object::load(QFile* file, int version, Map* map)
{
	this->map = map;
	
	int symbol_index;
	file->read((char*)&symbol_index, sizeof(int));
	if (symbol_index >= 0)
		symbol = map->getSymbol(symbol_index);
	
	int num_coords;
	file->read((char*)&num_coords, sizeof(int));
	coords.resize(num_coords);
	file->read((char*)&coords[0], num_coords * sizeof(MapCoord));
	
	file->read((char*)&path_closed, sizeof(bool));
	
	if (version >= 8)
	{
		if (type == Point)
		{
			PointObject* point = reinterpret_cast<PointObject*>(this);
			PointSymbol* point_symbol = reinterpret_cast<PointSymbol*>(point->getSymbol());
			if (point_symbol->isRotatable())
			{
				float rotation;
				file->read((char*)&rotation, sizeof(float));
				point->setRotation(rotation);
			}
		}
		else if (type == Text)
		{
			TextObject* text = reinterpret_cast<TextObject*>(this);
			float rotation;
			file->read((char*)&rotation, sizeof(float));
			text->setRotation(rotation);
			
			int temp;
			file->read((char*)&temp, sizeof(int));
			text->setHorizontalAlignment((TextObject::HorizontalAlignment)temp);
			file->read((char*)&temp, sizeof(int));
			text->setVerticalAlignment((TextObject::VerticalAlignment)temp);
			
			QString str;
			loadString(file, str);
			text->setText(str);
		}
	}
	
	output_dirty = true;
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
	
	// If the symbol contains a line or area symbol, calculate path coordinates
	path_coords.clear();
	if (symbol->getContainedTypes() & (Symbol::Area | Symbol::Line))
		PathCoord::calculatePathCoords(coords, coordsF, &path_coords);
	
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

void Object::move(qint64 dx, qint64 dy)
{
	int coords_size = coords.size();
	
	if (type == Text && coords_size == 2)
		coords_size = 1;	// don't touch box width / height for box texts
	
	for (int c = 0; c < coords_size; ++c)
	{
		MapCoord coord = coords[c];
		coord.setRawX(dx + coord.rawX());
		coord.setRawY(dy + coord.rawY());
		coords[c] = coord;
	}
	
	setOutputDirty();
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

void Object::reverse()
{
	int coords_size = coords.size();
	for (int c = 0; c < coords_size; ++c)
	{
		MapCoord coord = coords[c];
		if (c < coords_size  / 2)
		{
			coords[c] = coords[coords_size - 1 - c];
			coords[coords_size - 1 - c] = coord;
		}
		
		if (!(c == 0 && path_closed) && coords[c].isCurveStart())
		{
			assert(c >= 3);
			coords[c - 3].setCurveStart(true);
			if (!(c == coords_size - 1 && path_closed))
				coords[c].setCurveStart(false);
		}
		else if (coords[c].isHolePoint())
		{
			assert(c >= 1);
			coords[c - 3].setHolePoint(true);
			coords[c].setHolePoint(false);
		}
	}
	
	setOutputDirty();
}

int Object::isPointOnObject(MapCoordF coord, float tolerance, bool extended_selection)
{
	Symbol::Type type = symbol->getType();
	
	// Special case for points
	if (type == Symbol::Point)
	{
		if (!extended_selection)
			return (coord.lengthToSquared(MapCoordF(coords[0])) <= tolerance) ? Symbol::Point : Symbol::NoSymbol;
		else
			return extent.contains(coord.toQPointF());
	}
	
	// First check using extent
	if (coord.getX() < extent.left() - tolerance) return Symbol::NoSymbol;
	if (coord.getY() < extent.top() - tolerance) return Symbol::NoSymbol;
	if (coord.getX() > extent.right() + tolerance) return Symbol::NoSymbol;
	if (coord.getY() > extent.bottom() + tolerance) return Symbol::NoSymbol;

	// Use only the extent for now
	// TODO: more precise logic for lines and areas!
	
	return symbol->getType();
}
bool Object::isPathPointInBox(QRectF box)
{
	int size = coords.size();
	for (int i = 0; i < size; ++i)
	{
		if (box.contains(coords[i].toQPointF()))
			return true;
		
		if (coords[i].isCurveStart())
			i += 2;
	}
	return false;
}

void Object::takeRenderables()
{
	output.clear();
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
	if (!no_checks && new_symbol)
	{
		if (!new_symbol->isTypeCompatibleTo(this))
			return false;
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
	else if (type == Text)
		return new TextObject(symbol);
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

QTransform TextObject::calcTextToMapTransform()
{
	TextSymbol* text_symbol = reinterpret_cast<TextSymbol*>(symbol);
	
	QTransform transform;
	double scaling = 1.0f / text_symbol->calculateInternalScaling();
	transform.translate(coords[0].xd(), coords[0].yd());
	if (rotation != 0)
		transform.rotate(-rotation * 180 / M_PI);
	transform.scale(scaling, scaling);
	
	return transform;
}
QTransform TextObject::calcMapToTextTransform()
{
	TextSymbol* text_symbol = reinterpret_cast<TextSymbol*>(symbol);
	
	QTransform transform;
	double scaling = 1.0f / text_symbol->calculateInternalScaling();
	transform.scale(1.0f / scaling, 1.0f / scaling);
	if (rotation != 0)
		transform.rotate(rotation * 180 / M_PI);
	transform.translate(-coords[0].xd(), -coords[0].yd());
	
	return transform;
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

int TextObject::calcTextPositionAt(MapCoordF coord, bool find_line_only)
{
	TextSymbol* text_symbol = reinterpret_cast<TextSymbol*>(symbol);
	QFontMetricsF metrics(text_symbol->getQFont());
	
	QTransform transform = calcMapToTextTransform();
	QPointF point = transform.map(coord.toQPointF());
	
	for (int line = 0; line < getNumLineInfos(); ++line)
	{
		TextObjectLineInfo* line_info = getLineInfo(line);
		if (line_info->line_y - metrics.ascent() > point.y())
			return -1;	// NOTE: Only true as long as every line has a bigger or equal y value than the line before
		
		if (point.x() < line_info->line_x - MapEditorTool::click_tolerance) continue;
		if (point.y() > line_info->line_y + metrics.descent()) continue;
		if (point.x() > line_info->line_x + metrics.width(line_info->text) + MapEditorTool::click_tolerance) continue;
		
		// Position in the line rect.
		if (find_line_only)
			return line;
		else
			return line_info->start_index + findLetterPosition(line_info, point, metrics);
	}
	return -1;
}
int TextObject::findLetterPosition(TextObjectLineInfo* line_info, QPointF point, const QFontMetricsF& metrics)
{
	int left = 0;
	int right = line_info->text.length();
	while (right != left)	
	{
		int middle = (left + right) / 2;
		float x = line_info->line_x + metrics.width(line_info->text.left(middle));
		if (point.x() >= x)
		{
			if (middle >= right)
				return right;
			float next = line_info->line_x + metrics.width(line_info->text.left(middle + 1));
			if (point.x() < next)
				if (point.x() < (x + next) / 2)
					return middle;
				else
					return middle + 1;
			else
				left = middle + 1;
		}
		else // if (point.x() < x)
		{
			if (middle <= 0)
				return 0;
			float prev = line_info->line_x + metrics.width(line_info->text.left(middle - 1));
			if (point.x() > prev)
				if (point.x() > (x + prev) / 2)
					return middle;
				else
					return middle - 1;
			else
				right = middle - 1;
		}
	}
	return right;
}
TextObjectLineInfo* TextObject::findLineInfoForIndex(int index)
{
	for (int line = 0; line < getNumLineInfos() - 1; ++line)
	{
		TextObjectLineInfo* line_info = getLineInfo(line);
		if (index <= line_info->end_index + (text[line_info->start_index] == '\n' ? 0 : 1))
			return line_info;
	}
	return getLineInfo(getNumLineInfos() - 1);
}
