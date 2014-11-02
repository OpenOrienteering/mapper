/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014 Kai Pastor
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

#undef CLIPPER_VERSION
#include <clipper.hpp>
#include "object.h"

class CombinedUndoStep;
class PathCoord;
class PathObject;
class Symbol;

/**
 * Wraps some helper functions for boolean operations.
 * 
 * The implementation of the boolean operation tools is based on the Clipper
 * library (aka libpolyclipping) (http://www.angusj.com/delphi/clipper.php).
 * 
 * Because Clipper does not support bezier curves, areas are clipped as
 * polygonal approximations, and after clipping we try to rebuild the curves.
 * 
 * @todo This class should get unit tests.
 */
class BooleanTool
{
public:
	/**
	 * A list of PathObject elements.
	 */
	typedef std::vector< PathObject* > PathObjects;
	
	/**
	 * Types of boolean operation.
	 */
	enum Operation
	{
		Union,
		Intersection,
		Difference,
		XOr,
		MergeHoles
	};
	
	/**
	 * Constructs a tool for the given operation and map.
	 * 
	 * map must be NULL.
	 */
	BooleanTool(Operation op, Map* map);
	
	/**
	 * Executes the operation on the selected objects in the map.
	 * 
	 * The first selected object is treated special and must be a path.
	 * 
	 * @return True on success, false on error
	 */
	bool execute();
	
	/**
	 * Executes the operation per symbol on the selected objects in the map.
	 * 
	 * Executes the operation independently for every group of path objects
	 * which have got the same symbol. Objects which are not of type
	 * Object::Path are ignored.
	 * 
	 * Errors during the operation are ignored, too. The original objects the
	 * operation failed for remain unchanged. The operation continues for other
	 * groups of objects.
	 * 
	 * @return True if the map was changed, false otherwise.
	 */
	bool executePerSymbol();
	
	/**
	 * Executes the operation on particular objects.
	 * 
	 * This function does not (actively) change the collection of objects in the map
	 * or the selection.
	 * 
	 * @param subject               The primary affected object.
	 * @param result_objects_symbol Determines the symbol of the returned objects.
	 * @param in_objects            All objects to operate on. Must contain subject.
	 * @param out_objects           The resulting collection of objects.
	 */
	bool executeForObjects(PathObject* subject,
	        const Symbol* result_objects_symbol,
	        PathObjects& in_objects,
	        PathObjects& out_objects );
	
	/**
	 * Executes the Intersection and Difference operation on the given line object.
	 * 
	 * @param area                  The primary object, here defining a boundary.
	 * @param line                  The line object to operate on.
	 * @param out_objects           The resulting collection of objects.
	 */
	void executeForLine(
	        PathObject* area,
	        PathObject* line,
	        PathObjects& out_objects );
	
private:
	typedef std::pair< PathObject::PathPart*, const PathCoord* > PathCoordInfo;
	
	typedef QHash< ClipperLib::IntPoint, PathCoordInfo > PolyMap;
	
	/**
	 * Executes the operation on particular objects, and provides undo steps.
	 * 
	 * This function changes the collection of objects in the map and the selection.
	 * 
	 * @param subject               The primary affected object.
	 * @param result_objects_symbol Determines the symbol of the returned objects.
	 * @param in_objects            All objects to operate on. Must contain subject.
	 * @param out_objects           The resulting collection of objects.
	 * @param undo_step             A combined undo step which will be filled with sub steps.
	 */
	bool executeForObjects(PathObject* subject,
	        const Symbol* result_objects_symbol,
	        PathObjects& in_objects,
	        PathObjects& out_objects,
	        CombinedUndoStep& undo_step);
	
	/**
	 * Converts a ClipperLib::PolyTree to PathObjects.
	 * 
	 * @see BooleanTool::outerPolyNodeToPathObjects()
	 */
	void polyTreeToPathObjects(
	        const ClipperLib::PolyTree& tree,
	        PathObjects& out_objects,
	        const Symbol* result_objects_symbol,
	        PolyMap& polymap );

	/**
	 * Converts a ClipperLib::PolyNode to PathObjects.
	 * 
	 * The given ClipperLib::PolyNode must represent an outer polygon, not a hole.
	 * 
	 * This method operates recursively on all outer children.
	 */
	void outerPolyNodeToPathObjects(const ClipperLib::PolyNode& node,
	        PathObjects& out_objects,
	        const Symbol* result_objects_symbol,
	        PolyMap& polymap );
	
	/**
	 * Constructs ClipperLib::Paths from a PathObject.
	 */
	static void PathObjectToPolygons(
	        PathObject* object,
	        ClipperLib::Paths& polygons,
	        PolyMap& polymap );
	
	/**
	 * Reconstructs a PathObject from a polygon given as ClipperLib::Path.
	 * 
	 * Curves are reconstructed with the help of the polymap, mapping locations
	 * to path coords of the original objects.
	 */
	static void polygonToPathPart(const ClipperLib::Path& polygon,
	        const PolyMap& polymap,
	        PathObject* object );
	
	/**
	 * Tries to reconstruct a straight or curved segment with given start and
	 * end indices from the polygon.
	 * The first coordinate of the segment is assumed to be already added.
	 */
	static void rebuildSegment(
	        int start_index,
	        int end_index,
	        bool have_sequence,
	        bool sequence_increasing,
	        const ClipperLib::Path& polygon,
	        const PolyMap& polymap,
	        PathObject* object );
	
	/**
	 * Approximates a curved segment from the result polygon alone.
	 */
	static void rebuildSegmentFromPathOnly(
	        const ClipperLib::IntPoint& start_point,
	        const ClipperLib::IntPoint& second_point,
	        const ClipperLib::IntPoint& second_last_point,
	        const ClipperLib::IntPoint& end_point,
	        PathObject* object );
	
	/**
	 * Special case of rebuildSegment() for straight or very short lines.
	 */
	static void rebuildTwoIndexSegment(
	        int start_index,
	        int end_index,
	        bool sequence_increasing,
	        const ClipperLib::Path& polygon,
	        const PolyMap& polymap,
	        PathObject* object );
	
	/**
	 * Reconstructs one polygon coordinate and adds it to the object.
	 * 
	 * Uses the polymap to check whether the coorinate should be a dash point.
	 */
	static void rebuildCoordinate(
	        int index,
	        const ClipperLib::Path& polygon,
	        const PolyMap& polymap,
	        PathObject* object,
	        bool start_new_part = false );
	
	/**
	 * Compares a PathObject segment to a ClipperLib::Path polygon segment.
	 * 
	 * Returns true if the segments match. In this case, the out_... parameters are set.
	 * 
	 * @param original      The original PathObject.
	 * @param coord_index   The index of the segment start at the original.
	 * @param polygon       The ClipperLib::Path polygon.
	 * @param start_index   The start of the segement at the polygon.
	 * @param end_index     The end of the segment at the polygon.
	 * @param out_coords_increasing If the segments match, will be set to
	 *                      either true if a matching segment's point at coord_index corresponds to the point at start_index,
	 *                      or false otherwise.
	 * @param out_is_curve  If the segments match, will be set to
	 *                      either true if the original segment is a curve,
	 *                      or false otherwise.
	 */
	static bool checkSegmentMatch(
	        PathObject* original,
	        int coord_index,
	        const ClipperLib::Path& polygon,
	        int start_index,
	        int end_index,
	        bool& out_coords_increasing,
	        bool& out_is_curve );
	
	const Operation op;
	Map* const map;
};

#endif
