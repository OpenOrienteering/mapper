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
	Symbol::Type contained_types = symbol->getContainedTypes();
	
	// Special case for points
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
	
	// Special case for texts
	if (type == Symbol::Text)
	{
		TextObject* text_object = reinterpret_cast<TextObject*>(this);
		return (text_object->calcTextPositionAt(coord, true) != -1) ? Symbol::Text : Symbol::NoSymbol;
	}

	int coords_size = (int)coords.size();

	// Check for line selection
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

int PathObject::calcNumRegularPoints()
{
	int num_regular_points = 0;
	int size = (int)coords.size();
	for (int i = 0; i < size; ++i)
	{
		++num_regular_points;
		if (coords[i].isCurveStart())
			i += 2;
	}
	if (path_closed)
		--num_regular_points;
	return num_regular_points;
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
	
	if (path_closed)
	{
		if (coords.size() == 1)
			coords.push_back(coords[0]);
		else if (pos == 0)
			coords[coords.size() - 1] = c;
	}
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
void PathObject::deleteCoordinate(int pos, bool adjust_other_coords)
{
	assert(pos >= 0 && pos < getCoordinateCount());
	MapCoord old_coord = coords[pos];
	coords.erase(coords.begin() + pos);
	
	if (path_closed)
	{
		if (coords.size() == 1)
			coords.clear();
		else if (pos == 0)
			coords[coords.size() - 1] = coords[0];
	}
	
	if (adjust_other_coords && coords.size() >= 2)
	{
		// Deleting points at line ends
		if (!path_closed)
		{
			if (pos == 0 && old_coord.isCurveStart())
			{
				deleteCoordinate(1, false);
				deleteCoordinate(0, false);
				return;
			}
			else if (pos == getCoordinateCount() && pos >= 3 && coords[pos - 3].isCurveStart())
			{
				deleteCoordinate(pos - 1, false);
				deleteCoordinate(pos - 2, false);
				coords[pos - 3].setCurveStart(false);
				return;
			}
		}
		
		// Too few points for a closed path? Open the path
		if (path_closed && coords.size() < 4)
			setPathClosed(false);
		
		int coord_count = getCoordinateCount();
		if (old_coord.isHolePoint())
			coords[(pos - 1 + coord_count) % coord_count].setHolePoint(true);
		if (old_coord.isCurveStart())
		{
			if ((pos >= 3 && coords[pos - 3].isCurveStart()) || (path_closed && getCoordinateCount() >= 3 && coords[(pos - 3 + coord_count) % coord_count].isCurveStart()))
			{
				// Bezier curves on both sides. Merge curves to one and adjust handle positions to preserve the original shape somewhat (of course, in general it is impossible to do this)
				MapCoord& p0 = coords[(pos - 3 + coord_count) % coord_count];
				MapCoord& p1 = coords[(pos - 2 + coord_count) % coord_count];
				MapCoord& p2 = coords[(pos - 1 + coord_count) % coord_count];
				MapCoord& q0 = old_coord;
				MapCoord& q1 = coords[pos];
				MapCoord& q2 = coords[pos + 1];
				MapCoord& q3 = coords[pos + 2];
				
				// Trying to invert the de Casteljau subdivision, bad results
				/*float t;
				if (p2.xd() - q1.xd() == 0)
				{
					if (q1.yd() - p2.yd() == 0)
						t = 0.5;
					else
						t = (q0.yd() - p2.yd())/(q1.yd() - p2.yd());
				}
				else if (q1.yd() - p2.yd() == 0)
					t = (q0.xd() - p2.xd())/(q1.xd() - p2.xd());
				else
					t = (p2.xd() - q0.xd())/(2*(p2.xd() - q1.xd())) + (p2.yd() - q0.yd())/(2*(p2.yd() - q1.yd()));
				
				MapCoord r1 = MapCoord((p1.xd() - p0.xd()*(1-t))/t , (p1.yd() - p0.yd()*(1-t))/t);
				MapCoord r2 = MapCoord((q2.xd() - q3.xd()*t)/(1-t), (q2.yd() - q3.yd()*t)/(1-t));*/
				
				/*
				// Least squares curve fitting with projection of resulting handles onto original end point tangents
				MapCoord r1 = MapCoord( (7*p1.xd())/8 - (35*p0.xd())/48 + (7*p2.xd())/8 + (7*q0.xd())/12 - (3*q1.xd())/8 - q2.xd()/2 + (13*q3.xd())/48,
										(7*p1.yd())/8 - (35*p0.yd())/48 + (7*p2.yd())/8 + (7*q0.yd())/12 - (3*q1.yd())/8 - q2.yd()/2 + (13*q3.yd())/48 );
				MapCoord r2 = MapCoord( (13*p0.xd())/48 - p1.xd()/2 - (3*p2.xd())/8 + (7*q0.xd())/12 + (7*q1.xd())/8 + (7*q2.xd())/8 - (35*q3.xd())/48,
										(13*p0.yd())/48 - p1.yd()/2 - (3*p2.yd())/8 + (7*q0.yd())/12 + (7*q1.yd())/8 + (7*q2.yd())/8 - (35*q3.yd())/48 );
				
				MapCoordF right = MapCoordF(p1) - MapCoordF(p0);
				right.perpRight();
				right.normalize();
				MapCoordF p0r1 = MapCoordF(r1) - MapCoordF(p0);
				float right_amount = right.dot(p0r1);
				r1 = MapCoord(r1.xd() - right_amount * right.getX(), r1.yd() - right_amount * right.getY());
				
				right = MapCoordF(q3) - MapCoordF(q2);
				right.perpRight();
				right.normalize();
				MapCoordF q3r2 = MapCoordF(r2) - MapCoordF(q3);
				right_amount = right.dot(q3r2);
				r2 = MapCoord(r2.xd() - right_amount * right.getX(), r2.yd() - right_amount * right.getY());*/
				
				// Least squares curve fitting with the constraint that handles are on the same line as before.
				// The weighting is only a heuristic but seems to work ok.
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
				coord_count -= 2;
				coords[(pos - 2 + coord_count) % coord_count] = r1;
				coords[(pos - 1 + coord_count) % coord_count] = r2;
			}
			else
			{
				coords[(pos - 1 + coord_count) % coord_count].setCurveStart(true);
				if (path_closed && pos == 0)
				{
					// Zero is at the wrong position - can't start on a bezier curve handle! Switch the handles over to the end of the path
					coords[coords.size() - 1] = coords[0];
					coords.push_back(coords[1]);
					coords.erase(coords.begin() + 1);
					coords.erase(coords.begin());
					coords.push_back(coords[0]);
				}
			}
		}
	}
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
