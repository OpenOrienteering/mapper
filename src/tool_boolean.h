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

/**
 * Wraps some helper functions for boolean operations using the Clipper library
 * ( http://www.angusj.com/delphi/clipper.php ) which are presented as separate
 * tools in the UI.
 * 
 * As Clipper does not support bezier curves, the areas are clipped as polygonal
 * approximations and it is tried to rebuild the curves afterwards.
 */
class BooleanTool
{
public:
	typedef std::vector< PathObject* > PathObjects;
	
	enum Operation
	{
		Union = 0,
		Intersection,
		Difference,
		XOr
	};
	
	/** Constructs a boolean tool for the given map. */
	BooleanTool(Map* map);
	
	/** Executes the boolean tool on the selected objects in the map. */
	bool execute(Operation op);
	
	/**
	 * Executes the given operation on in_objects, returning the result in out_objects.
	 * subject is the main affected object and must be contained in in_objects.
	 * The symbol of the returned objects will be result_objects_symbol.
	 */
	bool executeForObjects(Operation op, PathObject* subject,
		Symbol* result_objects_symbol, PathObjects& in_objects,
		PathObjects& out_objects);
	
	/**
	 * Takes a line as subject.
	 * Only the Intersection and Difference operations are implemented for now.
	 */
	void executeForLine(Operation op, PathObject* area, PathObject* line,
		PathObjects& out_objects);
	
private:
	typedef QHash< Symbol*, PathObjects > ObjectGroups;
	typedef std::pair< PathObject::PathPart*, const PathCoord* > PathCoordInfo;
	
	/** Converts a PolyTree to a list of PathObjects. */
	void polyTreeToPathObjects(const ClipperLib::PolyTree& tree,
		PathObjects& out_objects, Symbol* result_objects_symbol,
		QHash< qint64, PathCoordInfo >& polymap);

	/** Converts a PolyNode representing an outer polygon (not a hole) to an object,
	 *  and recursively calls this method again for all outer children. */
	void outerPolyNodeToPathObjects(const ClipperLib::PolyNode& node,
		PathObjects& out_objects, Symbol* result_objects_symbol,
		QHash< qint64, PathCoordInfo >& polymap);
	
	/**
	 * Reconstructs the given clipper polygon, filling the object pointer
	 * with coordinates.
	 * 
	 * Curves are reconstructed with the help of the polymap, mapping locations
	 * to path coords of the original objects.
	 */
	void polygonToPathPart(const ClipperLib::Polygon& polygon,
		QHash< qint64, PathCoordInfo >& polymap, PathObject* object);
	
	/**
	 * Tries to reconstruct a straight or curved segment with given start and
	 * end indices from the polygon.
	 * The first coordinate of the segment is assumed to be already added.
	 */
	void rebuildSegment(int start_index, int end_index, bool have_sequence,
		bool sequence_increasing, const ClipperLib::Polygon& polygon,
		QHash< qint64, PathCoordInfo >& polymap, PathObject* object);
	
	/** Approximates a curved segment from the result polygon alone. */
	void rebuildSegmentFromPolygonOnly(const ClipperLib::IntPoint& start_point,
		const ClipperLib::IntPoint& second_point,
		const ClipperLib::IntPoint& second_last_point,
		const ClipperLib::IntPoint& end_point, PathObject* object);
	
	/** Special case of rebuildSegment() for straight or very short lines. */
	void rebuildTwoIndexSegment(int start_index, int end_index,
		bool have_sequence, bool sequence_increasing,
		const ClipperLib::Polygon& polygon,
		QHash< qint64, PathCoordInfo >& polymap,
		PathObject* object);
	
	/**
	 * Reconstructs one polygon coordinate and adds it to the object.
	 * Uses the polymap to check whether the coorinate should be a dash point.
	 */
	void rebuildCoordinate(int index, const ClipperLib::Polygon& polygon,
		QHash< qint64, PathCoordInfo >& polymap, PathObject* object,
		bool start_new_part = false);
	
	/** Removes flags from the coordinate to be able to use it in the reconstruction. */
	MapCoord convertOriginalCoordinate(MapCoord in);
	
	/**
	 * Compares the points between the given indices from the polygon to the original at coord_index.
	 * Returns true if the segments match. In this case, the out_... parameters are set.
	 */
	bool check_segment_match(int coord_index, PathObject* original,
		const ClipperLib::Polygon& polygon, int start_index, int end_index,
		bool& out_coords_increasing, bool& out_is_curve);
	
	/**
	 * Stores a Clipper IntPoint in a qint64. These are used as keys in a hash map.
	 * Assumes that the coordinates are in 32 bit range.
	 */
	inline qint64 intPointToQInt64(const ClipperLib::IntPoint& point)
	{
		return (point.X & 0xffffffff) | (point.Y << 32);
	}
	
	/** Objects to process, sorted by symbol */
	ObjectGroups object_groups;
	Map* map;
};

#endif
