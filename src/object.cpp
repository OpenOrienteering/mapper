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

#include <qmath.h>
#include <QFile>

#include "util.h"
#include "symbol.h"
#include "symbol_point.h"
#include "symbol_text.h"
#include "map_editor.h"
#include "object_text.h"

Object::Object(Object::Type type, Symbol* symbol) : type(type), symbol(symbol), map(NULL)
{
	output_dirty = true;
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
	
	if (version <= 8)
	{
		bool path_closed;
		file->read((char*)&path_closed, sizeof(bool));
		if (path_closed)
			coords[coords.size() - 1].setClosePoint(true);
	}
	
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
	
	if (type == Path)
	{
		PathObject* path = reinterpret_cast<PathObject*>(this);
		path->recalculateParts();
	}
	
	output_dirty = true;
}

bool Object::update(bool force, bool remove_old_renderables)
{
	if (!force && !output_dirty)
		return false;
	
	if (map && remove_old_renderables)
		map->removeRenderablesOfObject(this, false);
	clearOutput();
	
	// Calculate float coordinates
	MapCoordVectorF coordsF;
	int size = coords.size();
	coordsF.resize(size);
	for (int i = 0; i < size; ++i)
		coordsF[i] = MapCoordF(coords[i].xd(), coords[i].yd());
	
	// If the symbol contains a line or area symbol, calculate path coordinates
	if (symbol->getContainedTypes() & (Symbol::Area | Symbol::Line))
	{
		PathObject* path = reinterpret_cast<PathObject*>(this);
		path->updatePathCoords(coordsF);
	}
	
	// Create renderables
	symbol->createRenderables(this, coords, coordsF, output);
	
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
		coords[c].setX(coords[c].xd() * factor);
		coords[c].setY(coords[c].yd() * factor);
	}
	
	setOutputDirty();
}

void Object::rotateAround(MapCoordF center, double angle)
{
	double sin_angle = sin(angle);
	double cos_angle = cos(angle);
	
	int coords_size = coords.size();
	if (type == Text && coords_size == 2)
		coords_size = 1;	// don't touch box width / height for box texts
	for (int c = 0; c < coords_size; ++c)
	{
		MapCoordF center_to_coord = MapCoordF(coords[c].xd() - center.getX(), coords[c].yd() - center.getY());
		coords[c].setX(center.getX() + cos_angle * center_to_coord.getX() + sin_angle * center_to_coord.getY());
		coords[c].setY(center.getY() - sin_angle * center_to_coord.getX() + cos_angle * center_to_coord.getY());
	}
	
	if (type == Point)
	{
		PointObject* point = reinterpret_cast<PointObject*>(this);
		PointSymbol* point_symbol = reinterpret_cast<PointSymbol*>(point->getSymbol());
		if (point_symbol->isRotatable())
			point->setRotation(point->getRotation() + angle);
	}
	else if (type == Text)
	{
		TextObject* text = reinterpret_cast<TextObject*>(this);
		text->setRotation(text->getRotation() + angle);
	}
}

int Object::isPointOnObject(MapCoordF coord, float tolerance, bool extended_selection)
{
	Symbol::Type type = symbol->getType();
	Symbol::Type contained_types = symbol->getContainedTypes();
	
	// Points
	if (type == Symbol::Point)
	{
		if (!extended_selection)
			return (coord.lengthToSquared(MapCoordF(coords[0])) <= tolerance) ? Symbol::Point : Symbol::NoSymbol;
		else
			return extent.contains(coord.toQPointF()) ? Symbol::Point : Symbol::NoSymbol;
	}
	
	// First check using extent
	float extent_extension = (contained_types & Symbol::Line) ? tolerance : 0;
	if (coord.getX() < extent.left() - extent_extension) return Symbol::NoSymbol;
	if (coord.getY() < extent.top() - extent_extension) return Symbol::NoSymbol;
	if (coord.getX() > extent.right() + extent_extension) return Symbol::NoSymbol;
	if (coord.getY() > extent.bottom() + extent_extension) return Symbol::NoSymbol;
	
	if (type == Symbol::Text)
	{
		// Texts
		TextObject* text_object = reinterpret_cast<TextObject*>(this);
		return (text_object->calcTextPositionAt(coord, true) != -1) ? Symbol::Text : Symbol::NoSymbol;
	}
	else
	{
		// Path objects
		PathObject* path = reinterpret_cast<PathObject*>(this);
		return path->isPointOnPath(coord, tolerance);
	}
}
bool Object::isPathPointInBox(QRectF box)
{
	int size = coords.size();
	
	if (type == Text)
		return extent.intersects(box);
	
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

// ### PathObject::PathPart ###

void PathObject::PathPart::setClosed(bool closed)
{
	if (!isClosed() && closed)
	{
		if (getNumCoords() == 1 || path->coords[start_index].rawX() != path->coords[end_index].rawX() || path->coords[start_index].rawY() != path->coords[end_index].rawY())
			path->addCoordinate(end_index + 1, path->coords[start_index]);
		path->setClosingPoint(end_index, path->coords[start_index]);
		
		path->setOutputDirty();
	}
	else if (isClosed() && !closed)
	{
		path->coords[end_index].setClosePoint(false);
		
		path->setOutputDirty();
	}
}
void PathObject::PathPart::connectEnds()
{
	if (isClosed())
		return;
	
	path->coords[start_index].setRawX(qRound64((path->coords[end_index].rawX() + path->coords[start_index].rawX()) / 2));
	path->coords[start_index].setRawY(qRound64((path->coords[end_index].rawY() + path->coords[start_index].rawY()) / 2));
	path->setClosingPoint(end_index, path->coords[start_index]);
	path->setOutputDirty();
}
int PathObject::PathPart::calcNumRegularPoints()
{
	int num_regular_points = 0;
	for (int i = start_index; i <= end_index; ++i)
	{
		++num_regular_points;
		if (path->coords[i].isCurveStart())
			i += 2;
	}
	if (isClosed())
		--num_regular_points;
	return num_regular_points;
}

// ### PathObject ###

PathObject::PathObject(Symbol* symbol) : Object(Object::Path, symbol)
{
	assert(!symbol || (symbol->getType() == Symbol::Line || symbol->getType() == Symbol::Area || symbol->getType() == Symbol::Combined));
}
PathObject::PathObject(Symbol* symbol, const MapCoordVector& coords, Map* map) : Object(Object::Path, symbol)
{
	assert(!symbol || (symbol->getType() == Symbol::Line || symbol->getType() == Symbol::Area || symbol->getType() == Symbol::Combined));
	this->coords = coords;
	recalculateParts();
	if (map)
		setMap(map);
}
Object* PathObject::duplicate()
{
	PathObject* new_path = new PathObject(symbol);
	new_path->coords = coords;
	new_path->parts = parts;
	int parts_size = parts.size();
	for (int i = 0; i < parts_size; ++i)
		new_path->parts[i].path = new_path;
	return new_path;
}
PathObject* PathObject::duplicateFirstPart()
{
	PathObject* new_path = new PathObject(symbol);
	new_path->coords.assign(coords.begin() + parts[0].start_index, coords.begin() + (parts[0].end_index + 1));
	new_path->parts.push_back(parts[0]);
	new_path->parts[0].path = new_path;
	return new_path;
}

MapCoord& PathObject::shiftedCoord(int base_index, int offset, PathObject::PathPart& part)
{
	int index = shiftedCoordIndex(base_index, offset, part);
	assert(index >= 0);
	return coords[index];
}
int PathObject::shiftedCoordIndex(int base_index, int offset, PathObject::PathPart& part)
{
	if (part.isClosed())
	{
		if (offset >= part.getNumCoords() - 1)
			return -1;
		return ((base_index + offset - part.start_index + (part.getNumCoords() - 1)) % (part.getNumCoords() - 1)) + part.start_index;
	}
	else
	{
		base_index += offset;
		return (base_index < 0 || base_index > part.end_index) ? -1 : base_index;
	}
}
PathObject::PathPart& PathObject::findPartForIndex(int coords_index)
{
	int num_parts = (int)parts.size();
	for (int i = 0; i < num_parts; ++i)
	{
		if (coords_index >= parts[i].start_index && coords_index <= parts[i].end_index)
			return parts[i];
	}
	assert(false);
	return parts[0];
}
int PathObject::findPartIndexForIndex(int coords_index)
{
	int num_parts = (int)parts.size();
	for (int i = 0; i < num_parts; ++i)
	{
		if (coords_index >= parts[i].start_index && coords_index <= parts[i].end_index)
			return i;
	}
	assert(false);
	return 0;
}
void PathObject::deletePart(int part_index)
{
	coords.erase(coords.begin() + parts[part_index].start_index, coords.begin() + (parts[part_index].end_index + 1));
	int num_part_coords = parts[part_index].end_index - parts[part_index].start_index + 1;
	parts.erase(parts.begin() + part_index);
	for (int i = part_index; i < (int)parts.size(); ++i)
	{
		parts[i].start_index -= num_part_coords;
		parts[i].end_index -= num_part_coords;
	}
}
void PathObject::partSizeChanged(int part_index, int change)
{
	parts[part_index].end_index += change;
	for (int i = part_index + 1; i < (int)parts.size(); ++i)
	{
		parts[i].start_index += change;
		parts[i].end_index += change;
	}
}

void PathObject::calcClosestPointOnPath(MapCoordF coord, float& out_distance_sq, PathCoord& out_path_coord)
{
	update(false);
	
	int coords_size = (int)coords.size();
	if (coords_size == 0)
	{
		out_distance_sq = -1;
		return;	
	}
	else if (coords_size == 1)
	{
		out_distance_sq = coord.lengthToSquared(MapCoordF(coords[0]));
		out_path_coord.clen = 0;
		out_path_coord.index = 0;
		out_path_coord.param = 0;
		out_path_coord.pos = MapCoordF(coords[0]);
		return;
	}
	
	out_distance_sq = 999999;
	int path_size = (int)path_coords.size();
	for (int i = 0; i < path_size - 1; ++i)
	{
		assert(path_coords[i].index < path_size);
		if (coords[path_coords[i].index].isHolePoint())
			continue;
		
		MapCoordF to_coord = MapCoordF(coord.getX() - path_coords[i].pos.getX(), coord.getY() - path_coords[i].pos.getY());
		MapCoordF to_next = MapCoordF(path_coords[i+1].pos.getX() - path_coords[i].pos.getX(), path_coords[i+1].pos.getY() - path_coords[i].pos.getY());
		MapCoordF tangent = to_next;
		tangent.normalize();
		
		float dist_along_line = to_coord.dot(tangent);
		if (dist_along_line <= 0)
		{
			if (to_coord.lengthSquared() < out_distance_sq)
			{
				out_distance_sq = to_coord.lengthSquared();
				out_path_coord = path_coords[i];
			}
			continue;
		}
		
		float line_length = path_coords[i+1].clen - path_coords[i].clen;
		if (dist_along_line >= line_length)
		{
			if (coord.lengthToSquared(path_coords[i+1].pos) < out_distance_sq)
			{
				out_distance_sq = coord.lengthToSquared(path_coords[i+1].pos);
				out_path_coord = path_coords[i+1];
			}
			continue;
		}
		
		MapCoordF right = tangent;
		right.perpRight();
		
		float dist_from_line = qAbs(right.dot(to_coord));
		if (dist_from_line*dist_from_line < out_distance_sq)
		{
			float factor = (dist_along_line / line_length);
			out_distance_sq = dist_from_line*dist_from_line;
			out_path_coord.clen = path_coords[i].clen + dist_along_line;
			out_path_coord.index = path_coords[i+1].index;
			if (path_coords[i+1].index == path_coords[i].index)
				out_path_coord.param = path_coords[i].param + (path_coords[i+1].param - path_coords[i].param) * factor;
			else
				out_path_coord.param = path_coords[i+1].param * factor;
			if (coords[out_path_coord.index].isCurveStart())
			{
				MapCoordF o0, o1, o3, o4;
				PathCoord::splitBezierCurve(MapCoordF(coords[out_path_coord.index]), MapCoordF(coords[out_path_coord.index+1]),
											MapCoordF(coords[out_path_coord.index+2]), MapCoordF(coords[out_path_coord.index+3]),
											out_path_coord.param, o0, o1, out_path_coord.pos, o3, o4);
			}
			else
			{
				out_path_coord.pos = MapCoordF(path_coords[i].pos.getX() + (path_coords[i+1].pos.getX() - path_coords[i].pos.getX()) * factor,
											   path_coords[i].pos.getY() + (path_coords[i+1].pos.getY() - path_coords[i].pos.getY()) * factor);
			}
		}
	}
}
int PathObject::subdivide(int index, float param)
{
	update(false);
	
	if (coords[index].isCurveStart())
	{
		MapCoordF o0, o1, o2, o3, o4;
		PathCoord::splitBezierCurve(MapCoordF(coords[index]), MapCoordF(coords[index+1]),
									MapCoordF(coords[index+2]), MapCoordF(coords[index+3]),
									param, o0, o1, o2, o3, o4);
		coords[index + 1] = o0.toMapCoord();
		coords[index + 2] = o4.toMapCoord();
		addCoordinate(index + 2, o3.toMapCoord());
		MapCoord middle_coord = o2.toMapCoord();
		middle_coord.setCurveStart(true);
		addCoordinate(index + 2, middle_coord);
		addCoordinate(index + 2, o1.toMapCoord());
		return index + 3;
	}
	else
	{
		addCoordinate(index + 1, (MapCoordF(coords[index]) + (MapCoordF(coords[index+1]) - MapCoordF(coords[index])) * param).toMapCoord());
		return index + 1;
	}
}

bool PathObject::canBeConnected(PathObject* other, double connect_threshold_sq)
{
	int num_parts = getNumParts();
	for (int i = 0; i < num_parts; ++i)
	{
		if (parts[i].isClosed())
			continue;
		
		int num_other_parts = other->getNumParts();
		for (int k = 0; k < num_other_parts; ++k)
		{
			if (other->parts[k].isClosed())
				continue;
			
			if (coords[parts[i].start_index].lengthSquaredTo(other->coords[other->parts[k].start_index]) <= connect_threshold_sq)
				return true;
			else if (coords[parts[i].start_index].lengthSquaredTo(other->coords[other->parts[k].end_index]) <= connect_threshold_sq)
				return true;
			else if (coords[parts[i].end_index].lengthSquaredTo(other->coords[other->parts[k].start_index]) <= connect_threshold_sq)
				return true;
			else if (coords[parts[i].end_index].lengthSquaredTo(other->coords[other->parts[k].end_index]) <= connect_threshold_sq)
				return true;
		}
	}
	
	return false;
}
bool PathObject::connectIfClose(PathObject* other, double connect_threshold_sq)
{
	bool did_connect_path = false;
	
	int num_parts = getNumParts();
	int num_other_parts = other->getNumParts();
	std::vector<bool> other_parts;	// Which parts have not been connected to this part yet?
	other_parts.assign(num_other_parts, true);
	for (int i = 0; i < num_parts; ++i)
	{
		if (parts[i].isClosed())
			continue;
		
		for (int k = 0; k < num_other_parts; ++k)
		{
			if (!other_parts[k] || other->parts[k].isClosed())
				continue;
	
			if (coords[parts[i].start_index].lengthSquaredTo(other->coords[other->parts[k].start_index]) <= connect_threshold_sq)
			{
				other->reversePart(k);
				connectPathParts(i, other, k, true);
			}
			else if (coords[parts[i].start_index].lengthSquaredTo(other->coords[other->parts[k].end_index]) <= connect_threshold_sq)
				connectPathParts(i, other, k, true);
			else if (coords[parts[i].end_index].lengthSquaredTo(other->coords[other->parts[k].start_index]) <= connect_threshold_sq)
				connectPathParts(i, other, k, false);
			else if (coords[parts[i].end_index].lengthSquaredTo(other->coords[other->parts[k].end_index]) <= connect_threshold_sq)
			{
				other->reversePart(k);
				connectPathParts(i, other, k, false);
			}
			else
				continue;
			
			if (coords[parts[i].start_index].lengthSquaredTo(coords[parts[i].end_index]) <= connect_threshold_sq)
				parts[i].connectEnds();
			
			did_connect_path = true;
			
			other_parts[k] = false;
		}
	}
	
	if (did_connect_path)
	{
		// Copy over all remaining parts of the other object
		getCoordinate(getCoordinateCount() - 1).setHolePoint(true);
		for (int i = 0; i < num_other_parts; ++i)
		{
			if (other_parts[i])
				appendPathPart(other, i);
		}
	}
	
	return did_connect_path;
}
void PathObject::connectPathParts(int part_index, PathObject* other, int other_part_index, bool prepend)
{
	PathPart& part = parts[part_index];
	PathPart& other_part = other->parts[other_part_index];
	assert(!part.isClosed() && !other_part.isClosed());
	
	int other_part_size = other_part.getNumCoords();
	coords.resize(coords.size() + other_part_size - 1);
	
	if (prepend)
	{
		for (int i = (int)coords.size() - 1; i >= part.start_index + (other_part_size - 1); --i)
			coords[i] = coords[i - (other_part_size - 1)];
		
		MapCoord& join_coord = coords[part.start_index + other_part_size - 1];
		join_coord.setRawX((join_coord.rawX() + other->coords[other_part.end_index].rawX()) / 2);
		join_coord.setRawY((join_coord.rawY() + other->coords[other_part.end_index].rawY()) / 2);
		join_coord.setHolePoint(false);
		join_coord.setClosePoint(false);
		
		for (int i = part.start_index; i < part.start_index + other_part_size - 1; ++i)
			coords[i] = other->coords[i - part.start_index + other_part.start_index];
	}
	else
	{
		MapCoord coord = other->coords[other_part.start_index];	// take flags from first coord of path to append
		coord.setRawX((coords[part.end_index].rawX() + coord.rawX()) / 2);
		coord.setRawY((coords[part.end_index].rawY() + coord.rawY()) / 2);
		coords[part.end_index] = coord;
		
		for (int i = (int)coords.size() - 1; i > part.end_index + (other_part_size - 1); --i)
			coords[i] = coords[i - (other_part_size - 1)];
		for (int i = part.end_index + 1; i <= part.end_index + (other_part_size - 1); ++i)
			coords[i] = other->coords[i - part.end_index + other_part.start_index];
	}
	
	partSizeChanged(part_index, other_part_size - 1);
}

void PathObject::splitAt(const PathCoord& split_pos, Object*& out1, Object*& out2)
{
	out1 = NULL;
	out2 = NULL;
	
	int part_index = findPartIndexForIndex(split_pos.index);
	PathPart& part = parts[part_index];
	if (part.isClosed())
	{
		PathObject* path1 = new PathObject(symbol, coords, map);
		out1 = path1;
		
		if (split_pos.clen == path_coords[part.path_coord_start_index].clen || split_pos.clen == path_coords[part.path_coord_end_index].clen)
		{
			(path1->parts[part_index]).setClosed(false);
			return;
		}
		
		path1->changePathBounds(part_index, split_pos.clen, split_pos.clen);
		return;
	}
	
	if (split_pos.clen == path_coords[part.path_coord_start_index].clen)
	{
		if (part.calcNumRegularPoints() > 2)
		{
			PathObject* path1 = new PathObject(symbol, coords, map);
			out1 = path1;
			
			if (coords[part.start_index].isCurveStart())
			{
				path1->deleteCoordinate(part.start_index + 2, false);
				path1->deleteCoordinate(part.start_index + 1, false);
				path1->deleteCoordinate(part.start_index + 0, false);
			}
			else
				path1->deleteCoordinate(part.start_index + 0, false);
		}
	}
	else if (split_pos.clen == path_coords[part.path_coord_end_index].clen)
	{
		if (part.calcNumRegularPoints() > 2)
		{
			PathObject* path1 = new PathObject(symbol, coords, map);
			out1 = path1;
			
			if (part.getNumCoords() >= 4 && coords[part.end_index - 3].isCurveStart())
			{
				path1->deleteCoordinate(path1->parts[part_index].end_index, false);
				path1->deleteCoordinate(path1->parts[part_index].end_index, false);
				path1->deleteCoordinate(path1->parts[part_index].end_index, false);
				coords[path1->parts[part_index].end_index].setCurveStart(false);
			}
			else
				path1->deleteCoordinate(path1->parts[part_index].end_index, false);
		}
	}
	else
	{
		PathObject* path1 = new PathObject(symbol, coords, map);
		out1 = path1;
		PathObject* path2 = new PathObject(symbol, coords, map);
		out2 = path2;
		
		path1->changePathBounds(part_index, path_coords[part.path_coord_start_index].clen, split_pos.clen);
		path2->changePathBounds(part_index, split_pos.clen, path_coords[part.path_coord_end_index].clen);
	}
}
void PathObject::changePathBounds(int part_index, double start_len, double end_len)
{
	//if (start_len == end_len)
	//	return;
	
	update();
	PathPart& part = parts[part_index];
	int part_size = part.end_index - part.start_index + 1;
	
	MapCoordVector* p_coords = &coords;
	MapCoordVector temp_coords;
	if (getNumParts() > 1)
	{
		// TODO: optimize this HACK-ed case
		temp_coords.resize(part_size);
		for (int i = 0; i < part_size; ++i)
			temp_coords[i] = coords[part.start_index + i];
		
		p_coords = &temp_coords;
	}
	
	MapCoordVectorF coordsF;
	coordsF.resize(part_size);
	for (int i = 0; i < part_size; ++i)
		coordsF[i] = MapCoordF(coords[part.start_index + i]);
	
	MapCoordVector out_coords;
	out_coords.reserve(part_size + 2);
	MapCoordVectorF out_coordsF;
	out_coordsF.reserve(part_size + 2);
	
	if (end_len == 0)
		end_len = path_coords[part.path_coord_end_index].clen;
	
	int cur_path_coord = part.path_coord_start_index + 1;
	while (cur_path_coord < part.path_coord_end_index && path_coords[cur_path_coord].clen < start_len)
		++cur_path_coord;
	
	// Start position
	int start_bezier_index = -1;		// if the range starts at a bezier curve, this is the curve's index, otherwise -1
	float start_bezier_split_param = 0;	// the parameter value where the split of the curve for the range start was made
	MapCoordF o3, o4;					// temporary bezier control points
	if (p_coords->at(path_coords[cur_path_coord].index).isCurveStart())
	{
		int index = path_coords[cur_path_coord].index;
		float factor = (start_len - path_coords[cur_path_coord-1].clen) / (path_coords[cur_path_coord].clen - path_coords[cur_path_coord-1].clen);
		assert(factor >= -0.01f && factor <= 1.01f);
		if (factor > 1)
			factor = 1;
		else if (factor < 0)
			factor = 0;
		float prev_param = (path_coords[cur_path_coord-1].index == path_coords[cur_path_coord].index) ? path_coords[cur_path_coord-1].param : 0;
		assert(prev_param <= path_coords[cur_path_coord].param);
		float p = prev_param + (path_coords[cur_path_coord].param - prev_param) * factor;
		assert(p >= 0 && p <= 1);
		
		MapCoordF o0, o1;
		out_coordsF.push_back(MapCoordF(0, 0));
		PathCoord::splitBezierCurve(coordsF[index], coordsF[index+1], coordsF[index+2], (index < (int)coordsF.size() - 3) ? coordsF[index+3] : coordsF[0], p, o0, o1, out_coordsF[out_coordsF.size() - 1], o3, o4);
		MapCoord flag = out_coordsF[out_coordsF.size() - 1].toMapCoord();
		flag.setCurveStart(true);
		out_coords.push_back(flag);
		
		start_bezier_split_param = p;
		start_bezier_index = index;
	}
	else
	{
		out_coordsF.push_back(MapCoordF(0, 0));
		PathCoord::calculatePositionAt(*p_coords, coordsF, path_coords, start_len, cur_path_coord, &out_coordsF[out_coordsF.size() - 1], NULL);
		out_coords.push_back(out_coordsF[out_coordsF.size() - 1].toMapCoord());
	}
	
	int current_index = path_coords[cur_path_coord].index;
	if (start_len == path_coords[cur_path_coord].clen && path_coords[cur_path_coord].param == 1)
	{
		if (p_coords->at(current_index).isCurveStart())
			current_index += 3;
		else
			++current_index;
		if (current_index > part.end_index)
			current_index = 0;
		out_coords[out_coords.size() - 1].setFlags(p_coords->at(current_index).getFlags());
		out_coords[out_coords.size() - 1].setHolePoint(false);
		out_coords[out_coords.size() - 1].setClosePoint(false);
		++cur_path_coord;
	}
	else if (start_len == path_coords[cur_path_coord - 1].clen && path_coords[cur_path_coord - 1].param == 0)
	{
		out_coords[out_coords.size() - 1].setFlags(p_coords->at(current_index).getFlags());
		out_coords[out_coords.size() - 1].setHolePoint(false);
		out_coords[out_coords.size() - 1].setClosePoint(false);
	}
	
	// End position
	bool enforce_wrap = (end_len <= path_coords[cur_path_coord].clen && end_len <= start_len);
	advanceCoordinateRangeTo(*p_coords, coordsF, path_coords, cur_path_coord, current_index, end_len, enforce_wrap, start_bezier_index, out_coords, out_coordsF, o3, o4);
	if (current_index < part.end_index)
	{
		out_coordsF.push_back(MapCoordF(0, 0));
		if (p_coords->at(current_index).isCurveStart())
		{
			int index = path_coords[cur_path_coord].index - part.start_index;
			float factor = (end_len - path_coords[cur_path_coord-1].clen) / (path_coords[cur_path_coord].clen - path_coords[cur_path_coord-1].clen);
			assert(factor >= -0.01f && factor <= 1.01f);
			if (factor > 1)
				factor = 1;
			else if (factor < 0)
				factor = 0;
			float prev_param = (path_coords[cur_path_coord-1].index == path_coords[cur_path_coord].index) ? path_coords[cur_path_coord-1].param : 0;
			assert(prev_param <= path_coords[cur_path_coord].param);
			float p = prev_param + (path_coords[cur_path_coord].param - prev_param) * factor;
			assert(p >= 0 && p <= 1);
			
			out_coordsF.push_back(MapCoordF(0, 0));
			out_coordsF.push_back(MapCoordF(0, 0));
			MapCoordF unused, unused2;
			
			if (start_bezier_index == current_index && !enforce_wrap)
			{
				// The dash end is in the same curve as the start, need to make a second split with the correct parameter
				p = (p - start_bezier_split_param) / (1 - start_bezier_split_param);
				assert(p >= 0 && p <= 1);
				
				PathCoord::splitBezierCurve(out_coordsF[out_coordsF.size() - 4], o3, o4, (index < (int)coordsF.size() - 3) ? coordsF[index+3] : coordsF[0],
											p, out_coordsF[out_coordsF.size() - 3], out_coordsF[out_coordsF.size() - 2],
											out_coordsF[out_coordsF.size() - 1], unused, unused2);
			}
			else
			{
				PathCoord::splitBezierCurve(coordsF[index], coordsF[index+1], coordsF[index+2], (index < (int)coordsF.size() - 3) ? coordsF[index+3] : coordsF[0],
											p, out_coordsF[out_coordsF.size() - 3], out_coordsF[out_coordsF.size() - 2],
											out_coordsF[out_coordsF.size() - 1], unused, unused2);
			}
			
			out_coords.push_back(out_coordsF[out_coordsF.size() - 3].toMapCoord());
			out_coords.push_back(out_coordsF[out_coordsF.size() - 2].toMapCoord());
			out_coords.push_back(out_coordsF[out_coordsF.size() - 1].toMapCoord());
		}
		else
		{
			PathCoord::calculatePositionAt(*p_coords, coordsF, path_coords, end_len, cur_path_coord, &out_coordsF[out_coordsF.size() - 1], NULL);
			out_coords.push_back(out_coordsF[out_coordsF.size() - 1].toMapCoord());
		}
	}
	
	if (end_len == path_coords[cur_path_coord].clen && path_coords[cur_path_coord].param == 0)
	{
		out_coords[out_coords.size() - 1].setFlags(p_coords->at(current_index).getFlags());
		out_coords[out_coords.size() - 1].setClosePoint(false);
	}
	if (end_len == path_coords[cur_path_coord].clen && path_coords[cur_path_coord].param == 1)
	{
		if (p_coords->at(current_index).isCurveStart())
			current_index += 3;
		else
			++current_index;
		out_coords[out_coords.size() - 1].setFlags(p_coords->at(current_index).getFlags());
		out_coords[out_coords.size() - 1].setClosePoint(false);
	}
	
	out_coords[out_coords.size() - 1].setCurveStart(false);
	out_coords[out_coords.size() - 1].setHolePoint(true);
	
	if ((int)out_coords.size() > part_size)
		coords.insert(coords.begin() + part.start_index, (int)out_coords.size() - part_size, MapCoord());
	else if ((int)out_coords.size() < part_size)
		coords.erase(coords.begin() + part.start_index, coords.begin() + (part.start_index + part_size - (int)out_coords.size()));
	
	for (int i = part.start_index; i < part.start_index + (int)out_coords.size(); ++i)
		coords[i] = out_coords[i - part.start_index];
	//partSizeChanged(part_index, out_coords.size() - part_size);
	recalculateParts();
	setOutputDirty();
}
void PathObject::advanceCoordinateRangeTo(const MapCoordVector& flags, const MapCoordVectorF& coords, const PathCoordVector& path_coords, int& cur_path_coord, int& current_index, float cur_length,
										  bool enforce_wrap, int start_bezier_index, MapCoordVector& out_flags, MapCoordVectorF& out_coords, const MapCoordF& o3, const MapCoordF& o4)
{
	PathPart& part = findPartForIndex(path_coords[cur_path_coord].index);
	
	int path_coords_size = (int)path_coords.size();
	bool have_to_wrap = enforce_wrap;
	while (cur_length > path_coords[cur_path_coord].clen || cur_length < path_coords[cur_path_coord - 1].clen || have_to_wrap)
	{
		++cur_path_coord;
		if (cur_path_coord > part.path_coord_end_index || flags[path_coords[cur_path_coord].index - part.start_index].isClosePoint())
			cur_path_coord = part.path_coord_start_index + 1;
		assert(cur_path_coord < path_coords_size);
		
		int index = path_coords[cur_path_coord].index - part.start_index;
		
		if (index != current_index)
		{
			if (current_index == start_bezier_index && !(enforce_wrap && !have_to_wrap))
			{
				current_index += 3;
				assert(current_index <= part.end_index);
				if (flags[current_index].isClosePoint())
					current_index = 0;
				assert(current_index == index);
				
				out_flags.push_back(o3.toMapCoord());
				out_coords.push_back(o3);
				out_flags.push_back(o4.toMapCoord());
				out_coords.push_back(o4);
				out_flags.push_back(flags[current_index]);
				out_flags[out_flags.size() - 1].setClosePoint(false);
				out_flags[out_flags.size() - 1].setHolePoint(false);
				out_coords.push_back(coords[current_index]);
			}
			else
			{
				//assert((!flags[current_index].isCurveStart() && current_index + 1 == line_coords[cur_line_coord].index) ||
				//       (flags[current_index].isCurveStart() && current_index + 3 == line_coords[cur_line_coord].index) ||
				//       (flags[current_index+1].isHolePoint() && current_index + 2 == line_coords[cur_line_coord].index));
				do
				{
					++current_index;
					assert(current_index <= part.end_index);
					if (flags[current_index].isClosePoint())
						current_index = 0;
					out_flags.push_back(flags[current_index]);
					out_flags[out_flags.size() - 1].setClosePoint(false);
					out_flags[out_flags.size() - 1].setHolePoint(false);
					out_coords.push_back(coords[current_index]);
				} while (current_index != index);
			}
			
			have_to_wrap = false;
		}
	}	
}

void PathObject::appendPath(PathObject* other)
{
	int coords_size = coords.size();
	int other_coords_size = (int)other->coords.size();
	
	coords.resize(coords_size + other_coords_size);
	
	for (int i = 0; i < other_coords_size; ++i)
		coords[coords_size + i] = other->coords[i];
	
	recalculateParts();
	setOutputDirty();
}
void PathObject::appendPathPart(PathObject* other, int part_index)
{
	int coords_size = coords.size();
	PathPart& part = other->getPart(part_index);
	
	coords.resize(coords_size + part.getNumCoords());
	
	for (int i = 0; i < part.getNumCoords(); ++i)
		coords[coords_size + i] = other->coords[part.start_index + i];
	
	recalculateParts();
	setOutputDirty();
}

void PathObject::reverse()
{
	int parts_size = parts.size();
	for (int i = 0; i < parts_size; ++i)
		reversePart(i);
}

void PathObject::reversePart(int part_index)
{
	PathPart& part = parts[part_index];
	
	bool set_last_hole_point = false;
	int half = (part.end_index + part.start_index + 1) / 2;
	for (int c = part.start_index; c <= part.end_index; ++c)
	{
		MapCoord coord = coords[c];
		if (c < half)
		{
			int mirror_index = part.end_index - (c - part.start_index);
			coords[c] = coords[mirror_index];
			coords[mirror_index] = coord;
		}
		
		if (!(c == part.start_index && part.isClosed()) && coords[c].isCurveStart())
		{
			assert((c - part.start_index) >= 3);
			coords[c - 3].setCurveStart(true);
			if (!(c == part.end_index && part.isClosed()))
				coords[c].setCurveStart(false);
		}
		else if (coords[c].isHolePoint())
		{
			coords[c].setHolePoint(false);
			if (c >= part.start_index + 1)
				coords[c - 1].setHolePoint(true);
			else
				set_last_hole_point = true;
		}
	}
	
	if (coords[part.start_index].isClosePoint())
	{
		coords[part.start_index].setClosePoint(false);
		coords[part.end_index].setClosePoint(true);
	}
	if (set_last_hole_point)
		coords[part.end_index].setHolePoint(true);
	
	setOutputDirty();
}

int PathObject::isPointOnPath(MapCoordF coord, float tolerance)
{
	Symbol::Type contained_types = symbol->getContainedTypes();
	int coords_size = (int)coords.size();
	
	if (contained_types & Symbol::Line)
	{
		update(false);
		int size = (int)path_coords.size();
		for (int i = 0; i < size - 1; ++i)
		{
			assert(path_coords[i].index < coords_size);
			if (coords[path_coords[i].index].isHolePoint())
				continue;
			
			MapCoordF to_coord = MapCoordF(coord.getX() - path_coords[i].pos.getX(), coord.getY() - path_coords[i].pos.getY());
			MapCoordF to_next = MapCoordF(path_coords[i+1].pos.getX() - path_coords[i].pos.getX(), path_coords[i+1].pos.getY() - path_coords[i].pos.getY());
			MapCoordF tangent = to_next;
			tangent.normalize();
			
			float dist_along_line = to_coord.dot(tangent);
			if (dist_along_line < -tolerance)
				continue;
			else if (dist_along_line < 0 && to_coord.lengthSquared() <= tolerance*tolerance)
				return Symbol::Line;
			
			float line_length = path_coords[i+1].clen - path_coords[i].clen;
			if (line_length < 1e-7)
				continue;
			if (dist_along_line > line_length + tolerance)
				continue;
			else if (dist_along_line > line_length && coord.lengthToSquared(path_coords[i+1].pos) <= tolerance*tolerance)
				return Symbol::Line;
			
			MapCoordF right = tangent;
			right.perpRight();
			
			float dist_from_line = qAbs(right.dot(to_coord));
			if (dist_from_line <= tolerance)
				return Symbol::Line;
		}
	}
	
	// Check for area selection
	if (contained_types & Symbol::Area)
	{
		update(false);
		bool inside = false;
		int size = (int)path_coords.size();
		int i, j;
		for (i = 0, j = size - 1; i < size; j = i++)
		{
			if ( ((path_coords[i].pos.getY() > coord.getY()) != (path_coords[j].pos.getY() > coord.getY())) &&
				 (coord.getX() < (path_coords[j].pos.getX() - path_coords[i].pos.getX()) *
				 (coord.getY() - path_coords[i].pos.getY()) / (path_coords[j].pos.getY() - path_coords[i].pos.getY()) + path_coords[i].pos.getX()) )
				inside = !inside;
		}
		if (inside)
			return Symbol::Area;
	}
	
	return Symbol::NoSymbol;
}

void PathObject::setCoordinate(int pos, MapCoord c)
{
	assert(pos >= 0 && pos < getCoordinateCount());
	
	PathPart& part = findPartForIndex(pos);
	if (part.isClosed() && pos == part.end_index)
		pos = part.start_index;
	coords[pos] = c;
	if (part.isClosed() && pos == part.start_index)
		setClosingPoint(part.end_index, c);
	
	setOutputDirty();
}
void PathObject::addCoordinate(int pos, MapCoord c)
{
	assert(pos >= 0 && pos <= getCoordinateCount());
	int part_index = coords.empty() ? -1 : findPartIndexForIndex(qMin(pos, (int)coords.size() - 1));
	coords.insert(coords.begin() + pos, c);
	
	if (part_index >= 0)
	{
		++parts[part_index].end_index;
		for (int i = part_index + 1; i < (int)parts.size(); ++i)
		{
			++parts[i].start_index;
			++parts[i].end_index;
		}
		
		if (parts[part_index].isClosed() && parts[part_index].start_index == pos)
			setClosingPoint(parts[part_index].end_index, c);
	}
	else
	{
		PathPart part;
		part.start_index = 0;
		part.end_index = 0;
		part.path = this;
		parts.push_back(part);
	}
	
	setOutputDirty();
}
void PathObject::addCoordinate(MapCoord c)
{
	if (!parts.empty() && parts[parts.size() - 1].isClosed())
	{
		coords.insert(coords.begin() + (coords.size() - 1), c);
		++parts[parts.size() - 1].end_index;
	}
	else
	{
		coords.push_back(c);
		if (parts.empty())
		{
			PathPart part;
			part.start_index = 0;
			part.end_index = 0;
			part.path = this;
			parts.push_back(part);
		}
		else
			++parts[parts.size() - 1].end_index;
	}
	
	setOutputDirty();
}
void PathObject::deleteCoordinate(int pos, bool adjust_other_coords)
{
	assert(pos >= 0 && pos < getCoordinateCount());
	MapCoord old_coord = coords[pos];
	coords.erase(coords.begin() + pos);
	
	int part_index = findPartIndexForIndex(pos);
	PathPart& part = parts[part_index];
	partSizeChanged(part_index, -1);
	
	if (parts[part_index].isClosed())
	{
		if (parts[part_index].end_index - parts[part_index].start_index == 0)
		{
			deletePart(part_index);
			return;
		}
		else if (pos == parts[part_index].start_index)
			setClosingPoint(parts[part_index].end_index, coords[parts[part_index].start_index]);
	}
	else if (parts[part_index].end_index - parts[part_index].start_index == -1)
	{
		deletePart(part_index);
		return;
	}
	
	if (adjust_other_coords && part.getNumCoords() >= 2)
	{
		// Deleting points at line ends
		if (!part.isClosed())
		{
			if (pos == part.start_index && old_coord.isCurveStart())
			{
				deleteCoordinate(part.start_index + 1, false);
				deleteCoordinate(part.start_index, false);
				return;
			}
			else if (pos == part.end_index + 1 && pos >= part.start_index + 3 && coords[pos - 3].isCurveStart())
			{
				deleteCoordinate(pos - 1, false);
				deleteCoordinate(pos - 2, false);
				coords[pos - 3].setCurveStart(false);
				return;
			}
		}
		
		// Too few points for a closed path? Open the path
		if (part.isClosed() && part.getNumCoords() < 4)
		{
			part.setClosed(false);
			deleteCoordinate(part.end_index, false);
			getCoordinate(part.end_index).setHolePoint(true);
		}
		
		if (old_coord.isHolePoint())
			shiftedCoord(pos, -1, part).setHolePoint(true);
		if (old_coord.isCurveStart())
		{
			if ((pos - part.start_index >= 3 && coords[pos - 3].isCurveStart()) || (part.isClosed() && part.getNumCoords() >= 4 && shiftedCoord(pos, -3, part).isCurveStart()))
			{
				// Bezier curves on both sides. Merge curves to one and adjust handle positions to preserve the original shape somewhat (of course, in general it is impossible to do this)
				MapCoord& p0 = shiftedCoord(pos, -3, part);
				MapCoord& p1 = shiftedCoord(pos, -2, part);
				MapCoord& p2 = shiftedCoord(pos, -1, part);
				MapCoord& q0 = old_coord;
				MapCoord& q1 = coords[pos];
				MapCoord& q2 = coords[pos + 1];
				MapCoord& q3 = coords[pos + 2];
				
				// Least squares curve fitting with the constraint that handles are on the same line as before.
				// The weighting for x / y is only a heuristic but seems to work ok.
				// TODO: Should the resulting factors be bounded (not -> infinite, how to better deal with negative factors)?
				/*
				Matlab:
				>> X = (1-u)^3*P0+3*(1-u)^2*u*P1+3*(1-u)*u^2*P2+u^3*P3;
				>> Y = (1-v)^3*P3+3*(1-v)^2*v*Q1+3*(1-v)*v^2*Q2+v^3*Q3;
				>> Z = (1-w)^3*P0+3*(1-w)^2*w*(P0+pfactor*(P1-P0))+3*(1-w)*w^2*(Q3+qfactor*(Q2-Q3))+w^3*Q3;
				>> E = int((Z - subs(X, u, 2*w))^2, w, 0, 1/2) + int((Z - subs(Y, v, 2*w - 1))^2, w, 1/2, 1);
				>> Epfactor = diff(E, pfactor);
				>> Eqfactor = diff(E, qfactor);
				>> S = solve(Epfactor, Eqfactor, pfactor, qfactor);
				>> S = [S.pfactor, S.qfactor]
				 */
				
				float gxweight = qAbs(p0.yd() - q3.yd());
				float gyweight = qAbs(p0.xd() - q3.xd());
				
				float xweight = gxweight * qAbs(p0.xd() - p1.xd());
				float yweight = gyweight * qAbs(p0.yd() - p1.yd());
				float pfactorx = -(42*p1.xd() - 83*p0.xd() + 42*p2.xd() + 28*q0.xd() - 18*q1.xd() - 24*q2.xd() + 13*q3.xd())/(48*(p0.xd() - p1.xd()));
				float pfactory = -(42*p1.yd() - 83*p0.yd() + 42*p2.yd() + 28*q0.yd() - 18*q1.yd() - 24*q2.yd() + 13*q3.yd())/(48*(p0.yd() - p1.yd()));
				float pfactor = (xweight * pfactorx + yweight * pfactory) / (xweight + yweight);
				if (pfactor < 0)
					pfactor = -pfactor;
				
				xweight = gxweight * qAbs(q2.xd() - q3.xd());
				yweight = gyweight * qAbs(q2.yd() - q3.yd());
				float qfactorx = (13*p0.xd() - 24*p1.xd() - 18*p2.xd() + 28*q0.xd() + 42*q1.xd() + 42*q2.xd() - 83*q3.xd())/(48*(q2.xd() - q3.xd()));
				float qfactory = (13*p0.yd() - 24*p1.yd() - 18*p2.yd() + 28*q0.yd() + 42*q1.yd() + 42*q2.yd() - 83*q3.yd())/(48*(q2.yd() - q3.yd()));
				float qfactor = (xweight * qfactorx + yweight * qfactory) / (xweight + yweight);
				if (qfactor < 0)
					qfactor = -qfactor;
				
				MapCoordF p0p1 = MapCoordF(p1) - MapCoordF(p0);
				MapCoord r1 = MapCoord(p0.xd() + pfactor * p0p1.getX(), p0.yd() + pfactor * p0p1.getY());
				
				MapCoordF q3q2 = MapCoordF(q2) - MapCoordF(q3);
				MapCoord r2 = MapCoord(q3.xd() + qfactor * q3q2.getX(), q3.yd() + qfactor * q3q2.getY());
				
				deleteCoordinate(pos + 1, false);
				deleteCoordinate(pos, false);
				shiftedCoord(pos, -2, part) = r1;
				shiftedCoord(pos, -1, part) = r2;
			}
			else
			{
				shiftedCoord(pos, -1, part).setCurveStart(true);
				if (part.isClosed() && pos == 0)
				{
					// Zero is at the wrong position - can't start on a bezier curve handle! Switch the handles over to the end of the path
					coords[part.end_index] = coords[part.start_index];
					coords.insert(coords.begin() + (part.end_index + 1), coords[part.start_index + 1]);
					coords.erase(coords.begin() + part.start_index, coords.begin() + (part.start_index + 2));
					coords.insert(coords.begin() + part.end_index, coords[part.start_index]);
					setClosingPoint(part.end_index, coords[part.start_index]);
				}
			}
		}
	}
	
	setOutputDirty();
}

void PathObject::updatePathCoords(MapCoordVectorF& float_coords)
{
	path_coords.clear();
	
	int part_index = 0;
	int part_start = 0;
	int part_end = 0;
	int path_coord_start = 0;
	while (PathCoord::getNextPathPart(coords, float_coords, part_start, part_end, &path_coords, false, true))
	{
		assert(parts[part_index].start_index == part_start);
		assert(parts[part_index].end_index == part_end);
		parts[part_index].path_coord_start_index = path_coord_start;
		parts[part_index].path_coord_end_index = path_coords.size() - 1;
		path_coord_start = parts[part_index].path_coord_end_index + 1;
		
		++part_index;
	}
}

void PathObject::recalculateParts()
{
	parts.clear();
	
	int start = 0;
	int coords_size = coords.size();
	for (int i = 0; i < coords_size - 1; ++i)
	{
		if (coords[i].isHolePoint())
		{
			PathPart part;
			part.start_index = start;
			part.end_index = i;
			part.path = this;
			parts.push_back(part);
			start = i + 1;
			++i;	// assume that there are never two hole points in a row
		}
	}
	
	PathPart part;
	part.start_index = start;
	part.end_index = coords_size - 1;
	part.path = this;
	parts.push_back(part);
}
void PathObject::setClosingPoint(int index, MapCoord coord)
{
	coord.setCurveStart(false);
	coord.setHolePoint(true);
	coord.setClosePoint(true);
	coords[index] = coord;
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
	new_point->rotation = rotation;
	return new_point;
}

void PointObject::setPosition(qint64 x, qint64 y)
{
	coords[0].setRawX(x);
	coords[0].setRawY(y);
	setOutputDirty();
}
void PointObject::setPosition(MapCoordF coord)
{
	coords[0].setX(coord.getX());
	coords[0].setY(coord.getY());
	setOutputDirty();
}
void PointObject::getPosition(qint64& x, qint64& y) const
{
	x = coords[0].rawX();
	y = coords[0].rawY();
}
MapCoordF PointObject::getCoordF() const
{
	return MapCoordF(coords[0]);
}
void PointObject::setRotation(float new_rotation)
{
	PointSymbol* point = reinterpret_cast<PointSymbol*>(symbol);
	assert(point && point->isRotatable());
	
	rotation = new_rotation;
	setOutputDirty();
}
