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


#ifndef _OPENORIENTEERING_TOOL_BOOLEAN_H_
#define _OPENORIENTEERING_TOOL_BOOLEAN_H_

#include <vector>

#include <QHash>

#include <clipper.hpp>
#include "object.h"

class Symbol;
class PathObject;
class PathCoord;

/// Wraps some helper functions for boolean operations using the Clipper library ( http://www.angusj.com/delphi/clipper.php ) which are presented as separate tools in the UI.
/// As Clipper does not support bezier curves, the areas are clipped as polygonal approximations and it is tried to rebuild the curves afterwards.
class BooleanTool
{
public:
	enum Operation
	{
		Union = 0,
		Intersection,
		Difference,
		XOr
	};
	
	BooleanTool(Map* map);
	bool execute(Operation op);
	
private:
	typedef std::vector< PathObject* > PathObjects;
	typedef QHash< Symbol*, PathObjects > ObjectGroups;
	typedef std::pair< PathObject::PathPart*, const PathCoord* > PathCoordInfo;
	
	bool executeImpl(Operation op, PathObjects& in_objects, PathObjects& out_objects);
	void polygonToPathPart(ClipperLib::Polygon& polygon, QHash< qint64, PathCoordInfo >& polymap, PathObject* object);
	void rebuildSegment(int start_index, int end_index, bool have_sequence, bool sequence_increasing, ClipperLib::Polygon& polygon, QHash< qint64, PathCoordInfo >& polymap, PathObject* object);
	void rebuildCoordinate(int index, ClipperLib::Polygon& polygon, QHash< qint64, PathCoordInfo >& polymap, PathObject* object, bool start_new_part = false);
	MapCoord convertOriginalCoordinate(MapCoord in);
	bool check_segment_match(int coord_index, PathObject* original, ClipperLib::Polygon& polygon, int start_index, int end_index, bool& out_coords_increasing, bool& out_is_curve);
	
	// Assumes that the coordinates are in 32 bit range
	inline qint64 intPointToQInt64(const ClipperLib::IntPoint& point) {return (point.X & 0xffffffff) | (point.Y << 32);}
	
	ObjectGroups object_groups;	// Objects to process, sorted by symbol
	Map* map;
};

#endif
