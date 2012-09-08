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


#include "path_coord.h"

#include "assert.h"

#include "symbol_line.h"

const float PathCoord::bezier_error = 0.0005f;

void PathCoord::calculatePathCoords(const MapCoordVector& flags, const MapCoordVectorF& coords, PathCoordVector* path_coords)
{
	int part_start = 0;
	int part_end = 0;
	while (getNextPathPart(flags, coords, part_start, part_end, path_coords, false, true));
}
bool PathCoord::getNextPathPart(const MapCoordVector& flags, const MapCoordVectorF& coords, int& part_start, int& part_end, PathCoordVector* path_coords, bool break_at_dash_points, bool append_path_coords)
{
	int size = (int)coords.size();
	if (part_end >= size - 1)
		return false;
	if (!append_path_coords)
		path_coords->clear();
	
	part_start = part_end;
	if (flags[part_start].isHolePoint())
		++part_start;
	
	if (!append_path_coords || path_coords->empty() || (part_start > 0 && flags[part_start-1].isHolePoint()))
	{
		PathCoord start;
		start.clen = 0;
		start.index = part_start;
		start.pos = coords[part_start];
		start.param = 0;
		path_coords->push_back(start);
	}
	
	for (int i = part_start + 1; i < size; ++i)
	{
		if (flags[i-1].isCurveStart())
		{
			if (i == size - 2)
				curveToPathCoord(coords[i-1], coords[i], coords[i+1], coords[0], i-1, bezier_error, path_coords);
			else
				curveToPathCoord(coords[i-1], coords[i], coords[i+1], coords[i+2], i-1, bezier_error, path_coords);
			i += 2;
		}
		else
		{
			PathCoord coord;
			coord.clen = path_coords->at(path_coords->size() - 1).clen + coords[i].lengthTo(coords[i-1]);
			coord.index = i - 1;
			coord.pos = coords[i];
			coord.param = 1;
			path_coords->push_back(coord);
		}
		
		if (i < size && (flags[i].isHolePoint() || (break_at_dash_points && flags[i].isDashPoint())))
		{
			part_end = i;
			return true;
		}
	}
	
	part_end = size - 1;
	return true;
}
PathCoord PathCoord::findPathCoordForCoorinate(const PathCoordVector* path_coords, int index)
{
	int path_coords_size = path_coords->size();
	for (int i = path_coords_size - 1; i >= 0; --i)
	{
		if (path_coords->at(i).param == 0 && index == path_coords->at(i).index)
			return path_coords->at(i);
		else if (path_coords->at(i).param == 1 && index > path_coords->at(i).index)
			return path_coords->at(i);
	}
	assert(false);
	return path_coords->at(0);
}
void PathCoord::curveToPathCoordRec(MapCoordF c0, MapCoordF c1, MapCoordF c2, MapCoordF c3, int coord_index, float max_error, PathCoordVector* path_coords, float p0, float p1)
{
	float outer_len = c0.lengthTo(c1) + c1.lengthTo(c2) + c2.lengthTo(c3);
	float inner_len = c0.lengthTo(c3);
	float p_half = 0.5f * (p0 + p1);
	
	if (outer_len - inner_len <= max_error)
	{
		PathCoord coord;
		PathCoord& prev = path_coords->at(path_coords->size() - 1);
		coord.pos.setX(0.5f * (c1.getX() + c2.getX()));
		coord.pos.setY(0.5f * (c1.getY() + c2.getY()));
		coord.param = p_half;
		coord.clen = coord.pos.lengthTo(prev.pos) + prev.clen;
		coord.index = coord_index;
		
		path_coords->push_back(coord);
		return;
	}
	else
	{
		// Split in two
		MapCoordF c01((c0.getX() + c1.getX()) * 0.5f, (c0.getY() + c1.getY()) * 0.5f);
		MapCoordF c12((c1.getX() + c2.getX()) * 0.5f, (c1.getY() + c2.getY()) * 0.5f);
		MapCoordF c23((c2.getX() + c3.getX()) * 0.5f, (c2.getY() + c3.getY()) * 0.5f);
		MapCoordF c012((c01.getX() + c12.getX()) * 0.5f, (c01.getY() + c12.getY()) * 0.5f);
		MapCoordF c123((c12.getX() + c23.getX()) * 0.5f, (c12.getY() + c23.getY()) * 0.5f);
		MapCoordF c0123((c012.getX() + c123.getX()) * 0.5f, (c012.getY() + c123.getY()) * 0.5f);
		
		curveToPathCoordRec(c0, c01, c012, c0123, coord_index, max_error, path_coords, p0, p_half);
		curveToPathCoordRec(c0123, c123, c23, c3, coord_index, max_error, path_coords, p_half, p1);
	}
}
void PathCoord::curveToPathCoord(MapCoordF c0, MapCoordF c1, MapCoordF c2, MapCoordF c3, int coord_index, float max_error, PathCoordVector* path_coords)
{
	// Add curve coordinates
	curveToPathCoordRec(c0, c1, c2, c3, coord_index, max_error, path_coords, 0, 1);
	
	// Add end point
	PathCoord end;
	end.clen = path_coords->at(path_coords->size() - 1).clen + c3.lengthTo(path_coords->at(path_coords->size() - 1).pos);
	end.index = coord_index;
	end.pos = c3;
	end.param = 1;
	path_coords->push_back(end);
}
void PathCoord::calculatePositionAt(const MapCoordVector& flags, const MapCoordVectorF& coords, const PathCoordVector& path_coords, float length, int& line_coord_search_start, MapCoordF* out_pos, MapCoordF* out_right_vector)
{
	int size = (int)path_coords.size();
	for (int i = qMax(1, line_coord_search_start); i < size; ++i)	// i may not be 0 because the previous LineParam is accessed (and it is unnecessary to look at the 0th line ;coord)
	{
		if (path_coords[i].clen < length)
			continue;
		int index = path_coords[i].index;
		
		if (0 == path_coords[i].clen - path_coords[i-1].clen)
		{
			// Zero-length segment
			out_pos->setX(path_coords[i].pos.getX());
			out_pos->setY(path_coords[i].pos.getY());
		}
		else if (flags[index].isCurveStart())
		{
			float factor = (length - path_coords[i-1].clen) / (path_coords[i].clen - path_coords[i-1].clen);
			//assert(factor >= -0.01f && factor <= 1.01f); happens when using large break lengths as these are not adjusted
			if (factor > 1)
				factor = 1;
			else if (factor < 0)
				factor = 0;
			float prev_param = (path_coords[i-1].index == path_coords[i].index) ? path_coords[i-1].param : 0;
			assert(prev_param <= path_coords[i].param);
			float p = prev_param + (path_coords[i].param - prev_param) * factor;
			assert(p >= 0 && p <= 1);
			MapCoordF o0, o1, o3, o4;
			if (index < (int)flags.size() - 3)
				splitBezierCurve(coords[index], coords[index+1], coords[index+2], coords[index+3], p, o0, o1, *out_pos, o3, o4);
			else
				splitBezierCurve(coords[index], coords[index+1], coords[index+2], coords[0], p, o0, o1, *out_pos, o3, o4);
			
			if (out_right_vector)
			{
				out_right_vector->setX(o3.getX() - o1.getX());
				out_right_vector->setY(o3.getY() - o1.getY());
				out_right_vector->perpRight();
			}
		}
		else
		{
			float factor = (length - path_coords[i-1].clen) / (path_coords[i].clen - path_coords[i-1].clen);
			//assert(factor >= -0.01f && factor <= 1.01f); happens when using large break lengths as these are not adjusted
			if (factor > 1)
				factor = 1;
			else if (factor < 0)
				factor = 0;
			MapCoordF to_next(path_coords[i].pos.getX() - path_coords[i-1].pos.getX(), path_coords[i].pos.getY() - path_coords[i-1].pos.getY());
			out_pos->setX(path_coords[i-1].pos.getX() + factor * to_next.getX());
			out_pos->setY(path_coords[i-1].pos.getY() + factor * to_next.getY());
			if (out_right_vector)
			{
				*out_right_vector = to_next;
				out_right_vector->perpRight();
			}
		}
		
		line_coord_search_start = i;
		return;
	}
	
	//assert(length < path_coords[path_coords.size() - 1].clen + 0.01f); perhaps same problem as the commented assert above?
	*out_pos = path_coords[path_coords.size() - 1].pos;
	if (out_right_vector)
	{
		out_right_vector->setX(path_coords[path_coords.size() - 1].pos.getX() - path_coords[path_coords.size() - 2].pos.getX());
		out_right_vector->setY(path_coords[path_coords.size() - 1].pos.getY() - path_coords[path_coords.size() - 2].pos.getY());
		out_right_vector->perpRight();
	}
}
void PathCoord::splitBezierCurve(MapCoordF c0, MapCoordF c1, MapCoordF c2, MapCoordF c3, float p, MapCoordF& o0, MapCoordF& o1, MapCoordF& o2, MapCoordF& o3, MapCoordF& o4)
{
	if (p >= 1)
	{
		o0 = c1;
		o1 = c2;
		o2 = c3;
		o3 = c3;
		o4 = c3;
		return;
	}
	else if (p <= 0)
	{
		o0 = c0;
		o1 = c0;
		o2 = c0;
		o3 = c1;
		o4 = c2;
		return;
	}
	
	o0 = MapCoordF(c0.getX() + (c1.getX() - c0.getX()) * p, c0.getY() + (c1.getY() - c0.getY()) * p);
	MapCoordF c12(c1.getX() + (c2.getX() - c1.getX()) * p, c1.getY() + (c2.getY() - c1.getY()) * p);
	o4 = MapCoordF(c2.getX() + (c3.getX() - c2.getX()) * p, c2.getY() + (c3.getY() - c2.getY()) * p);
	o1 = MapCoordF(o0.getX() + (c12.getX() - o0.getX()) * p, o0.getY() + (c12.getY() - o0.getY()) * p);
	o3 = MapCoordF(c12.getX() + (o4.getX() - c12.getX()) * p, c12.getY() + (o4.getY() - c12.getY()) * p);
	o2 = MapCoordF(o1.getX() + (o3.getX() - o1.getX()) * p, o1.getY() + (o3.getY() - o1.getY()) * p);
}
MapCoordF PathCoord::calculateRightVector(const MapCoordVector& flags, const MapCoordVectorF& coords, bool path_closed, int i, float* scaling)
{
	bool ok;
	
	if ((i == 0 && !path_closed)) // || (i > 0 && flags[i-1].isHolePoint()))
	{
		if (scaling)
			*scaling = 1;
		MapCoordF right = calculateTangent(coords, i, false, ok);
		right.perpRight();
		right.normalize();
		return right;
	}
	else if ((i == (int)coords.size() - 1 && !path_closed)) // || flags[i].isHolePoint())
	{
		if (scaling)
			*scaling = 1;
		MapCoordF right = calculateTangent(coords, i, true, ok);
		right.perpRight();
		right.normalize();
		return right;
	}
	
	MapCoordF to_coord = calculateTangent(coords, i, true, ok);
	if (!ok)
	{
		if (path_closed)
			to_coord = calculateTangent(coords, (int)coords.size() - 1, true, ok);
		else
			to_coord = calculateTangent(coords, i, false, ok);
	}
	to_coord.normalize();
	MapCoordF to_next = calculateTangent(coords, i, false, ok);
	if (!ok)
	{
		if (path_closed)
			to_next = calculateTangent(coords, 0, false, ok);
		else
			to_next = calculateTangent(coords, i, true, ok);
	}
	to_next.normalize();
	MapCoordF right = MapCoordF(-to_coord.getY() - to_next.getY(), to_next.getX() + to_coord.getX());
	right.normalize();
	if (scaling)
		*scaling = qMin(1.0f / sin(acos(right.getX() * to_coord.getX() + right.getY() * to_coord.getY())), 2.0 * LineSymbol::miterLimit());
	
	return right;
}
MapCoordF PathCoord::calculateTangent(const MapCoordVector& coords, int i, bool backward, bool& ok)
{
	// TODO: this is a copy of the code below, adjusted for MapCoord (without F) and handling of closed paths
	
	ok = true;
	MapCoordF tangent;
	if (backward)
	{
		//assert(i >= 1);
		int k = i-1;
		for (; k >= 0; --k)
		{
			if (coords[k].isClosePoint())
				break;
			tangent = MapCoordF(coords[i].xd() - coords[k].xd(), coords[i].yd() - coords[k].yd());
			if (tangent.lengthSquared() > 0.01f*0.01f)
				return tangent;
		}
		
		++k;
		int size = (int)coords.size();
		while (k < size && !coords[k].isClosePoint())
			++k;
		if (k < size)
		{
			// This is a closed part, wrap around
			--k;
			for (; k > i; --k)
			{
				tangent = MapCoordF(coords[i].xd() - coords[k].xd(), coords[i].yd() - coords[k].yd());
				if (tangent.lengthSquared() > 0.01f*0.01f)
					return tangent;
			}
		}
		
		ok = false;
	}
	else
	{
		int size = (int)coords.size();
		//assert(i < size - 1);
		int k = i+1;
		for (; k < size; ++k)
		{
			tangent = MapCoordF(coords[k].xd() - coords[i].xd(), coords[k].yd() - coords[i].yd());
			if (tangent.lengthSquared() > 0.01f*0.01f)
				return tangent;
			if (coords[k].isClosePoint())
				break;
		}
		
		if (k >= size)
			--k;
		if (coords[k].isClosePoint())
		{
			--k;
			// This is a closed part, wrap around
			while (k >= 0 && !coords[k].isClosePoint())
				--k;
			k += 2;
			for (; k < i; ++k)
			{
				tangent = MapCoordF(coords[k].xd() - coords[i].xd(), coords[k].yd() - coords[i].yd());
				if (tangent.lengthSquared() > 0.01f*0.01f)
					return tangent;
			}
		}
		
		ok = false;
	}
	return tangent;
}
MapCoordF PathCoord::calculateTangent(const MapCoordVectorF& coords, int i, bool backward, bool& ok)
{
	// TODO: use this for tangent calculation in LineSymbol or for objects in general
	
	ok = true;
	MapCoordF tangent;
	if (backward)
	{
		//assert(i >= 1);
		int k = i-1;
		for (; k >= 0; --k)
		{
			tangent = MapCoordF(coords[i].getX() - coords[k].getX(), coords[i].getY() - coords[k].getY());
			if (tangent.lengthSquared() > 0.01f*0.01f)
				return tangent;
		}
		
		ok = false;
	}
	else
	{
		int size = (int)coords.size();
		//assert(i < size - 1);
		int k = i+1;
		for (; k < size; ++k)
		{
			tangent = MapCoordF(coords[k].getX() - coords[i].getX(), coords[k].getY() - coords[i].getY());
			if (tangent.lengthSquared() > 0.01f*0.01f)
				return tangent;
		}
		
		ok = false;
	}
	return tangent;
}
