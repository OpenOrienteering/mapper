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


#include "tool_boolean.h"

#include <algorithm>

#include <QDebug>

#include "map.h"
#include "symbol.h"
#include "object.h"
#include "object_undo.h"
#include "util.h"


//### Local helper functions

/*
 * NB: qHash must be in the same namespace as the QHash key.
 * This is a C++ language issue, not a Qt issue.
 * Cf. https://bugreports.qt-project.org/browse/QTBUG-34912
 */
namespace ClipperLib
{

/**
 * Implements qHash for ClipperLib::IntPoint.
 * 
 * This is needed to use ClipperLib::IntPoint as QHash key.
 */
uint qHash(const IntPoint& point, uint seed)
{
	qint64 tmp = (point.X & 0xffffffff) | (point.Y << 32);
	return ::qHash(tmp, seed); // must use :: namespace to prevent endless recursion
}

} // namespace ClipperLib

/**
 * Removes flags from the coordinate to be able to use it in the reconstruction.
 */
static MapCoord resetCoordinate(MapCoord in)
{
	in.setHolePoint(false);
	in.setClosePoint(false);
	in.setCurveStart(false);
	return in;
}

/**
 * Compares a MapCoord and ClipperLib::IntPoint.
 */
bool operator==(const MapCoord& lhs, const ClipperLib::IntPoint& rhs)
{
	return lhs.rawX() == rhs.X && lhs.rawY() == rhs.Y;
}

/**
 * Compares a MapCoord and ClipperLib::IntPoint.
 */
bool operator==(const ClipperLib::IntPoint& lhs, const MapCoord& rhs)
{
	return rhs == lhs;
}



//### BooleanTool ###

BooleanTool::BooleanTool(Operation op, Map* map)
: op(op)
, map(map)
{
	; // nothing
}

bool BooleanTool::execute()
{
	// Check basic prerequisite
	Object* const primary_object = map->getFirstSelectedObject();
	if (primary_object->getType() != Object::Path)
	{
		Q_ASSERT(false && "The first selected object must be a path.");
		return false; // in release build
	}

	// Filter selected objects into in_objects
	PathObjects in_objects;
	in_objects.reserve(map->getNumSelectedObjects());
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		Object* const object = *it;
		if (object->getType() == Object::Path)
		{
			PathObject* const path = object->asPath();
			if (op != MergeHoles || (object->getSymbol()->getContainedTypes() & Symbol::Area && path->getNumParts() > 1 ))
			{
				in_objects.push_back(path);
			}
		}
	}
	
	// Perform the core operation
	PathObjects out_objects;
	if (!executeForObjects(primary_object->asPath(), primary_object->getSymbol(), in_objects, out_objects))
	{
		Q_ASSERT(out_objects.size() == 0);
		return false; // in release build
	}
	
	// Add original objects to undo step, and remove them from map.
	AddObjectsUndoStep* add_step = new AddObjectsUndoStep(map);
	for (int i = 0; i < (int)in_objects.size(); ++i)
	{
		PathObject* object = in_objects.at(i);
		if (op != Difference || object == primary_object)
		{
			add_step->addObject(object, object);
		}
	}
	// Keep as separate loop to get the correct index in the previous loop
	for (int i = 0; i < (int)in_objects.size(); ++i)
	{
		PathObject* object = in_objects.at(i);
		if (op != Difference || object == primary_object)
		{
			map->removeObjectFromSelection(object, false);
			map->getCurrentPart()->deleteObject(object, true);
			object->setMap(map); // necessary so objects are saved correctly
		}
	}
	
	// Add resulting objects to map, and create delete step for them
	DeleteObjectsUndoStep* delete_step = new DeleteObjectsUndoStep(map);
	MapPart* part = map->getCurrentPart();
	for (int i = 0; i < (int)out_objects.size(); ++i)
	{
		map->addObject(out_objects[i]);
		map->addObjectToSelection(out_objects[i], false);
	}
	// Keep as separate loop to get the correct index
	for (int i = 0; i < (int)out_objects.size(); ++i)
	{
		delete_step->addObject(part->findObjectIndex(out_objects[i]));
	}
	
	CombinedUndoStep* undo_step = new CombinedUndoStep(map);
	undo_step->push(add_step);
	undo_step->push(delete_step);
	map->push(undo_step);
	map->setObjectsDirty();
	
	map->emitSelectionChanged();
	map->emitSelectionEdited();
	
	return true;
}

bool BooleanTool::executeForObjects(PathObject* subject, Symbol* result_objects_symbol, PathObjects& in_objects, PathObjects& out_objects)
{
	
	// Convert the objects to Clipper polygons and
	// create a hash map, mapping point positions to the PathCoords.
	// These paths are to be regarded as closed.
	PolyMap polymap;
	
	ClipperLib::Paths subject_polygons;
	PathObjectToPolygons(subject, subject_polygons, polymap);
	
	ClipperLib::Paths clip_polygons;
	for (int object_number = 0; object_number < (int)in_objects.size(); ++object_number)
	{
		PathObject* object = in_objects[object_number];
		if (object != subject)
		{
			PathObjectToPolygons(object, clip_polygons, polymap);
		}
	}
	
	// Do the operation.
	ClipperLib::Clipper clipper;
	clipper.AddPaths(subject_polygons, ClipperLib::ptSubject, true);
	clipper.AddPaths(clip_polygons, ClipperLib::ptClip, true);
	
	ClipperLib::ClipType clip_type;
	ClipperLib::PolyFillType fill_type = ClipperLib::pftNonZero;
	switch (op)
	{
	case Union:         clip_type = ClipperLib::ctUnion;
	                    break;
	case Intersection:  clip_type = ClipperLib::ctIntersection;
	                    break;
	case Difference:    clip_type = ClipperLib::ctDifference;
	                    break;
	case XOr:           clip_type = ClipperLib::ctXor;
	                    break;
	case MergeHoles:    clip_type = ClipperLib::ctUnion;
	                    fill_type = ClipperLib::pftPositive;
	                    break;
	default:            Q_ASSERT(false && "Undefined operation");
	                    return false;
	}

	ClipperLib::PolyTree solution;
	bool success = clipper.Execute(clip_type, solution, fill_type, fill_type);
	if (success)
	{
		// Try to convert the solution polygons to objects again
		polyTreeToPathObjects(solution, out_objects, result_objects_symbol, polymap);
	}
	
	return success;
}

void BooleanTool::polyTreeToPathObjects(const ClipperLib::PolyTree& tree, PathObjects& out_objects, Symbol* result_objects_symbol, PolyMap& polymap)
{
	for (int i = 0, count = tree.ChildCount(); i < count; ++i)
		outerPolyNodeToPathObjects(*tree.Childs[i], out_objects, result_objects_symbol, polymap);
}

void BooleanTool::outerPolyNodeToPathObjects(const ClipperLib::PolyNode& node, PathObjects& out_objects, Symbol* result_objects_symbol, PolyMap& polymap)
{
	PathObject* object = new PathObject();
	object->setSymbol(result_objects_symbol, true);
	
	polygonToPathPart(node.Contour, polymap, object);
	for (int i = 0, i_count = node.ChildCount(); i < i_count; ++i)
	{
		polygonToPathPart(node.Childs[i]->Contour, polymap, object);
		
		// Add outer polygons contained by (nested within) holes ...
		for (int j = 0, j_count = node.Childs[i]->ChildCount(); j < j_count; ++j)
			outerPolyNodeToPathObjects(*node.Childs[i]->Childs[j], out_objects, result_objects_symbol, polymap);
	}
	
	out_objects.push_back(object);
}



void BooleanTool::executeForLine(PathObject* area, PathObject* line, BooleanTool::PathObjects& out_objects)
{
	if (op != BooleanTool::Intersection && op != BooleanTool::Difference)
	{
		Q_ASSERT(false && "Only intersection and difference are supported.");
		return; // no-op in release build
	}
	if (line->getNumParts() != 1)
	{
		Q_ASSERT(false && "Only single-part lines are supported.");
		return; // no-op in release build
	}
	
	// Calculate all intersection points of line with the area path
	PathObject::Intersections intersections;
	line->calcAllIntersectionsWith(area, intersections);
	intersections.clean();
	
	// For every segment, check whether it is inside or outside of the area.
	// Keep only segments inside the area.
	int line_coord_search_start = 0;
	MapCoordVectorF mapCoordVectorF;
	mapCoordVectorToF(line->getRawCoordinateVector(), mapCoordVectorF);
	MapCoordF middle_pos;
	
	PathObject* first_segment = NULL;
	PathObject* last_segment = NULL;
	
	// Only one segment?
	if (intersections.size() == 0)
	{
		double middle_length = line->getPart(0).getLength();
		PathCoord::calculatePositionAt(line->getRawCoordinateVector(), mapCoordVectorF, line->getPathCoordinateVector(), middle_length, line_coord_search_start, &middle_pos, NULL);
		if (area->isPointInsideArea(middle_pos) == (op == BooleanTool::Intersection))
			out_objects.push_back(line->duplicate()->asPath());
		return;
	}
	
	// First segment
	double middle_length = intersections[0].length / 2;
	PathCoord::calculatePositionAt(line->getRawCoordinateVector(), mapCoordVectorF, line->getPathCoordinateVector(), middle_length, line_coord_search_start, &middle_pos, NULL);
	if (area->isPointInsideArea(middle_pos) == (op == BooleanTool::Intersection))
	{
		PathObject* part = line->duplicate()->asPath();
		part->changePathBounds(0, 0, intersections[0].length);
		first_segment = part;
	}
	
	// Middle segments
	for (int i = 0; i < (int)intersections.size() - 1; ++i)
	{
		middle_length = (intersections[i].length + intersections[i+1].length) / 2;
		PathCoord::calculatePositionAt(line->getRawCoordinateVector(), mapCoordVectorF, line->getPathCoordinateVector(), middle_length, line_coord_search_start, &middle_pos, NULL);
		if (area->isPointInsideArea(middle_pos) == (op == BooleanTool::Intersection))
		{
			PathObject* part = line->duplicate()->asPath();
			part->changePathBounds(0, intersections[i].length, intersections[i+1].length);
			out_objects.push_back(part);
		}
	}
	
	// Last segment
	middle_length = (line->getPart(0).getLength() + intersections[intersections.size() - 1].length) / 2;
	PathCoord::calculatePositionAt(line->getRawCoordinateVector(), mapCoordVectorF, line->getPathCoordinateVector(), middle_length, line_coord_search_start, &middle_pos, NULL);
	if (area->isPointInsideArea(middle_pos) == (op == BooleanTool::Intersection))
	{
		PathObject* part = line->duplicate()->asPath();
		part->changePathBounds(0, intersections[intersections.size() - 1].length, line->getPart(0).getLength());
		last_segment = part;
	}
	
	// If the line was closed at the beginning, merge the first and last segments if they were kept both.
	if (line->getPart(0).isClosed() && first_segment && last_segment)
	{
		last_segment->connectPathParts(0, first_segment, 0, false);
		delete first_segment;
		out_objects.push_back(last_segment);
	}
	else
	{
		if (first_segment)
			out_objects.push_back(first_segment);
		if (last_segment)
			out_objects.push_back(last_segment);
	}
}

void BooleanTool::PathObjectToPolygons(PathObject* object, ClipperLib::Paths& polygons, PolyMap& polymap)
{
	object->update();
	
	int part_count = object->getNumParts();
	polygons.clear();
	polygons.resize(part_count);
	
	const PathCoordVector& path_coords = object->getPathCoordinateVector();
	
	for (int part_number = 0; part_number < part_count; ++part_number)
	{
		PathObject::PathPart& part = object->getPart(part_number);
		
		ClipperLib::Path& polygon = polygons[part_number];
		for (int i = part.path_coord_start_index + (part.isClosed() ? 1 : 0); i <= part.path_coord_end_index; ++i)
		{
			polygon.push_back(ClipperLib::IntPoint(path_coords[i].pos.getIntX(), path_coords[i].pos.getIntY()));
			polymap.insertMulti(polygon.back(), std::make_pair(&part, &path_coords[i]));
		}
		
		bool orientation = Orientation(polygon);
		if ((part_number == 0 && !orientation) || (part_number > 0 && orientation))
		{
			std::reverse(polygon.begin(), polygon.end());
		}
	}
}

void BooleanTool::polygonToPathPart(const ClipperLib::Path& polygon, const PolyMap& polymap, PathObject* object)
{
	if (polygon.size() < 3)
		return;
	
	// Check if we can find either an unknown intersection point or a path coord with parameter 0 or 1.
	// This gives a starting point to search for curves to rebuild (because we cannot start in the middle of a curve)
	// Number of points in the polygon
	int num_points = (int)polygon.size();
	// Index of first used point in polygon
	int part_start_index = 0;
	
	for (; part_start_index < num_points; ++part_start_index)
	{
		if (!polymap.contains(polygon.at(part_start_index)))
			break;
		
		const PathCoord* path_coord = polymap.value(polygon.at(part_start_index)).second;
		if (path_coord->param <= 0 || path_coord->param >= 1)
			break;
	}
	
	if (part_start_index == num_points)
	{
		// Did not find a valid starting point. Return the part as a polygon.
		for (int i = 0; i < num_points; ++i)
			object->addCoordinate(MapCoord(0.001 * polygon.at(i).X, 0.001 * polygon.at(i).Y), (i == 0));
		object->getPart(object->getNumParts() - 1).setClosed(true, true);
		return;
	}
	
	// Add the first point to the object
	rebuildCoordinate(part_start_index, polygon, polymap, object, true);
	
	// Advance along the boundary and, for every sequence of path coord pointers with the same path and index, rebuild the curve
	PathCoordInfo cur_info;
	if (polymap.contains(polygon.at(part_start_index)))
		cur_info = polymap.value(polygon.at(part_start_index));
	else
	{
		cur_info.first = NULL;
		cur_info.second = NULL;
	}
	// Index of first segment point in polygon
	int segment_start_index = part_start_index;
	bool have_sequence = false;
	bool sequence_increasing = false;
	bool stop_before = false;
	
	int i = part_start_index + 1;
	while (true)
	{
		if (i >= num_points)
			i = 0;
		
		PathCoordInfo new_info;
		if (polymap.contains(polygon.at(i)))
			new_info = polymap.value(polygon.at(i));
		else
		{
			new_info.first = NULL;
			new_info.second = NULL;
		}
		
		/*qDebug() << new_info.first << " " << i;
		if (new_info.second)
		{
			qDebug() << "  " << new_info.second->index << " " << new_info.second->param << " " << new_info.second->pos.getX() << " " << new_info.second->pos.getY();
			qDebug() << "  " << polygon.at(i).X << " " << polygon.at(i).Y;
		}*/
		
		if (cur_info.first == new_info.first && cur_info.first)
		{
			int cur_coord_index = cur_info.second->index;
			MapCoord& cur_coord = cur_info.first->path->getCoordinate(cur_coord_index);
			int new_coord_index = new_info.second->index;
			MapCoord& new_coord = new_info.first->path->getCoordinate(new_coord_index);
			if (cur_coord_index == new_coord_index)
			{
				if (have_sequence && sequence_increasing != (new_info.second->param > cur_info.second->param))
					stop_before = true;
				else
				{
					have_sequence = true;
					sequence_increasing = new_info.second->param > cur_info.second->param;
				}
			}
			else if (((cur_coord.isCurveStart() && new_coord_index == cur_coord_index + 3) ||
					 (!cur_coord.isCurveStart() && new_coord_index == cur_coord_index + 1) ||
					 new_coord_index == 0) && cur_info.second->param >= 1)
			{
				if (have_sequence && sequence_increasing != true)
					stop_before = true;
				else
				{
					have_sequence = true;
					sequence_increasing = true;
				}
			}
			else if (((new_coord.isCurveStart() && new_coord_index == cur_coord_index - 3) ||
					 (!new_coord.isCurveStart() && new_coord_index == cur_coord_index - 1) ||
					 cur_coord_index == 0) && new_info.second->param >= 1)
			{
				if (have_sequence && sequence_increasing != false)
					stop_before = true;
				else
				{
					have_sequence = true;
					sequence_increasing = false;
				}
			}
			else if ((segment_start_index + 1) % num_points != i)
				stop_before = true;
		}
		
		if ((i == part_start_index) || stop_before || (new_info.second && (new_info.second->param >= 1 || new_info.second->param <= 0)) ||
			(cur_info.first && (cur_info.first != new_info.first || cur_info.second->index != new_info.second->index) && (segment_start_index + 1) % num_points != i) ||
			(!new_info.first))
		{
			if (stop_before)
			{
				--i;
				if (i < 0)
					i = num_points - 1;
			}
			
			rebuildSegment(segment_start_index, i, have_sequence, sequence_increasing, polygon, polymap, object);
			
			if (stop_before)
			{
				++i;
				if (i >= num_points)
					i = 0;
				rebuildCoordinate(i, polygon, polymap, object);
				stop_before = false;
			}
			
			segment_start_index = i;
			have_sequence = false;
		}
		
		if (i == part_start_index)
			break;
		++i;
		cur_info = new_info;
	}
	
	object->getPart(object->getNumParts() - 1).connectEnds();
}

void BooleanTool::rebuildSegment(int start_index, int end_index, bool have_sequence, bool sequence_increasing, const ClipperLib::Path& polygon, const PolyMap& polymap, PathObject* object)
{
	int num_points = (int)polygon.size();
	
	// Do we have a sequence of at least two points belonging to the same curve?
	if (!have_sequence)
	{
		// No, add a straight line segment only
		rebuildCoordinate(end_index, polygon, polymap, object);
		return;
	}
	
	object->getCoordinate(object->getCoordinateCount() - 1).setCurveStart(true);
	
	if ((start_index + 1) % num_points == end_index)
	{
		// This could happen for a straight line or a very flat curve - take coords directly from original
		rebuildTwoIndexSegment(start_index, end_index, sequence_increasing, polygon, polymap, object);
		return;
	}

	// Get polygon point coordinates
	const ClipperLib::IntPoint& start_point = polygon.at(start_index);
	PathCoordInfo start_info;
	start_info.first = NULL;
	
	const ClipperLib::IntPoint& second_point = polygon.at((start_index + 1) % num_points);
	PathCoordInfo second_info;
	
	const ClipperLib::IntPoint& end_point = polygon.at(end_index);
	PathCoordInfo end_info;
	end_info.first = NULL;
	
	int second_last_index = end_index - 1;
	if (second_last_index < 0)
		second_last_index = num_points - 1;
	const ClipperLib::IntPoint& second_last_point = polygon.at(second_last_index);
	PathCoordInfo second_last_info;
	
	// Try to find a consistent set of path coord infos for the middle coordinates
	bool found = false;
	PolyMap::const_iterator second_it = polymap.find(second_point);
	PolyMap::const_iterator second_last_it = polymap.find(second_last_point);
	while (second_it != polymap.end())
	{
		while (second_last_it != polymap.end())
		{
			if (second_it->first == second_last_it->first)
			{
				found = true;
				break;
			}
			++second_last_it;
		}
		if (found)
			break;
		++second_it;
		second_last_it = polymap.find(second_last_point);
	}
	if (!found)
	{
		// Need unambiguous path part information to find the original object with high probability
		qDebug() << "BooleanTool::rebuildSegment: cannot identify original object!";
		rebuildSegmentFromPathOnly(start_point, second_point, second_last_point, end_point, object);
		return;
	}
	
	second_info = *second_it;
	second_last_info = *second_last_it;
	
	// Also try to find consistent outer coordinates
	PathObject::PathPart* original_path = second_info.first;
	PathObject* original = original_path->path;
	int original_index = second_info.second->index;
	
	PolyMap::const_iterator start_it = polymap.find(start_point);
	while (start_it != polymap.end())
	{
		if (start_it->first == original_path)
			break;
		++start_it;
	}
	if (start_it != polymap.end())
		start_info = *start_it;
	
	PolyMap::const_iterator end_it = polymap.find(end_point);
	while (end_it != polymap.end())
	{
		if (end_it->first == original_path)
			break;
		++end_it;
	}
	if (end_it != polymap.end())
		end_info = *end_it;
	
	// Find out start tangent
	float start_param;
	MapCoord start_coord = MapCoord(0.001 * start_point.X, 0.001 * start_point.Y);
	MapCoord start_tangent;
	MapCoord original_end_tangent;
	
	double start_error_sq, end_error_sq;
	// Maximum difference in mm from reconstructed start and end coords to the
	// intersection points returned by Clipper
	const double error_bound = 0.4;
	
	if (start_info.first && start_info.first == second_info.first &&
		((!sequence_increasing && start_info.second->param == 1) || (sequence_increasing && start_info.second->param == 0)) &&
		((start_info.second->index == second_info.second->index) ||
			(sequence_increasing && start_info.second->param >= 1))) //&& start_info.second->index + 1 == second_info.second->index
	{
		// Take coordinates directly
		start_param = sequence_increasing ? 0 : 1;
		start_tangent = sequence_increasing ? original->getCoordinate(original_index + 1) : original->getCoordinate(original_index + 2);
		original_end_tangent = sequence_increasing ? original->getCoordinate(original_index + 2) : original->getCoordinate(original_index + 1);
		
		const MapCoord& assumed_start = sequence_increasing ? original->getCoordinate(original_index + 0) : original->getCoordinate(original_index + 3);
		start_error_sq = start_coord.lengthSquaredTo(assumed_start);
		if (start_error_sq > error_bound)
			qDebug() << "BooleanTool::rebuildSegment: start error too high in direct case: " << sqrt(start_error_sq);
	}
	else
	{
		// Approximate coords
		start_param = second_info.second->param;
		int dx = second_point.X - start_point.X;
		int dy = second_point.Y - start_point.Y;
		float point_dist = 0.001 * sqrt(dx*dx + dy*dy);
		
		if (sequence_increasing)
		{
			const PathCoord* prev_coord = (second_info.second->param <= 0) ? second_info.second : (second_info.second - 1);
			float prev_param = (prev_coord->param == 1) ? 0 : prev_coord->param;
			start_param -= (second_info.second->param - prev_param) * point_dist / qMax(1e-7f, (second_info.second->clen - prev_coord->clen));
			if (start_param < 0)
				start_param = 0;
			
			MapCoordF o0, o1, o2, o3, o4;
			PathCoord::splitBezierCurve(MapCoordF(original->getCoordinate(original_index + 0)), MapCoordF(original->getCoordinate(original_index + 1)),
										MapCoordF(original->getCoordinate(original_index + 2)), MapCoordF(original->getCoordinate(original_index + 3)),
										start_param,
										o0, o1, o2, o3, o4);
			start_tangent = o3.toMapCoord();
			original_end_tangent = o4.toMapCoord();
			start_error_sq = start_coord.lengthSquaredTo(o2.toMapCoord());
			if (start_error_sq > error_bound)
				qDebug() << "BooleanTool::rebuildSegment: start error too high in increasing general case: " << sqrt(start_error_sq);
		}
		else
		{
			const PathCoord* next_coord = (second_info.second->param >= 1) ? second_info.second : (second_info.second + 1);
			float next_param = next_coord->param;
			start_param += (next_param - second_info.second->param) * point_dist / qMax(1e-7f, (next_coord->clen - second_info.second->clen));
			if (start_param > 1)
				start_param = 1;
			
			MapCoordF o0, o1, o2, o3, o4;
			PathCoord::splitBezierCurve(MapCoordF(original->getCoordinate(original_index + 3)), MapCoordF(original->getCoordinate(original_index + 2)),
										MapCoordF(original->getCoordinate(original_index + 1)), MapCoordF(original->getCoordinate(original_index + 0)),
										1 - start_param,
							o0, o1, o2, o3, o4);
			start_tangent = o3.toMapCoord();
			original_end_tangent = o4.toMapCoord();
			start_error_sq = start_coord.lengthSquaredTo(o2.toMapCoord());
			if (start_error_sq > error_bound)
				qDebug() << "BooleanTool::rebuildSegment: start error too high in decreasing general case: " << sqrt(start_error_sq);
		}
	}
	
	// Find better end point approximation and its tangent
	MapCoord end_tangent;
	MapCoord end_coord;
	
	if (end_info.first && end_info.first == second_last_info.first &&
		((!sequence_increasing && end_info.second->param == 0) || (sequence_increasing && end_info.second->param == 1)) &&
		((end_info.second->index == second_last_info.second->index) ||
		(!sequence_increasing && end_info.second->param >= 1))) //&& end_info.second->index + 1 == second_last_info.second->index
	{
		// Take coordinates directly
		end_coord = sequence_increasing ? original->getCoordinate(original_index + 3) : original->getCoordinate(original_index + 0);
		end_tangent = original_end_tangent;
		
		qint64 test_x = end_point.X - end_coord.rawX();
		qint64 test_y = end_point.Y - end_coord.rawY();
		end_error_sq = 0.001 * sqrt(test_x*test_x + test_y*test_y);
		if (end_error_sq > error_bound)
			qDebug() << "BooleanTool::rebuildSegment: end error too high in direct case: " << sqrt(end_error_sq);
	}
	else
	{
		// Approximate coords
		float end_param = second_last_info.second->param;
		int dx = end_point.X - second_last_point.X;
		int dy = end_point.Y - second_last_point.Y;
		float point_dist = 0.001 * sqrt(dx*dx + dy*dy);
		if (sequence_increasing)
		{
			const PathCoord* next_coord = (second_last_info.second->param >= 1) ? second_last_info.second : (second_last_info.second + 1);
			float next_param = next_coord->param;
			end_param += (next_param - second_last_info.second->param) * point_dist / qMax(1e-7f, (next_coord->clen - second_last_info.second->clen));
			if (end_param > 1)
				end_param = 1;
			
			MapCoordF o0, o1, o2, o3, o4;
			PathCoord::splitBezierCurve(MapCoordF(start_coord), MapCoordF(start_tangent),
										MapCoordF(original_end_tangent), MapCoordF(original->getCoordinate(original_index + 3)),
										(end_param - start_param) / (1 - start_param),
										o0, o1, o2, o3, o4);
			start_tangent = o0.toMapCoord();
			end_tangent = o1.toMapCoord();
			end_coord = o2.toMapCoord();
			
			qint64 test_x = end_point.X - end_coord.rawX();
			qint64 test_y = end_point.Y - end_coord.rawY();
			end_error_sq = 0.001 * sqrt(test_x*test_x + test_y*test_y);
			if (end_error_sq > error_bound)
				qDebug() << "BooleanTool::rebuildSegment: end error too high in increasing general case: " << sqrt(end_error_sq);
		}
		else
		{
			const PathCoord* prev_coord = (second_last_info.second->param <= 0) ? second_last_info.second : (second_last_info.second - 1);
			float prev_param = (prev_coord->param == 1) ? 0 : prev_coord->param;
			end_param -= (second_last_info.second->param - prev_param) * point_dist / qMax(1e-7f, (second_last_info.second->clen - prev_coord->clen));
			if (end_param < 0)
				end_param = 0;
			
			MapCoordF o0, o1, o2, o3, o4;
			PathCoord::splitBezierCurve(MapCoordF(start_coord), MapCoordF(start_tangent),
											MapCoordF(original_end_tangent), MapCoordF(original->getCoordinate(original_index + 0)),
											1 - (end_param / start_param),
											o0, o1, o2, o3, o4);
			start_tangent = o0.toMapCoord();
			end_tangent = o1.toMapCoord();
			end_coord = o2.toMapCoord();
			
			qint64 test_x = end_point.X - end_coord.rawX();
			qint64 test_y = end_point.Y - end_coord.rawY();
			end_error_sq = 0.001 * sqrt(test_x*test_x + test_y*test_y);
			if (end_error_sq > error_bound)
				qDebug() << "BooleanTool::rebuildSegment: end error too high in decreasing general case: " << sqrt(end_error_sq);
		}
	}
	
	if (start_error_sq <= error_bound && end_error_sq <= error_bound)
	{
		// Rebuild bezier curve using information from original curve
		object->addCoordinate(start_tangent);
		object->addCoordinate(end_tangent);
		object->addCoordinate(resetCoordinate(end_coord));
	}
	else
	{
		// Rebuild bezier curve approximately using tangents derived from result polygon
		rebuildSegmentFromPathOnly(start_point, second_point, second_last_point, end_point, object);
	}
}

void BooleanTool::rebuildSegmentFromPathOnly(const ClipperLib::IntPoint& start_point, const ClipperLib::IntPoint& second_point, const ClipperLib::IntPoint& second_last_point, const ClipperLib::IntPoint& end_point, PathObject* object)
{
	MapCoord start_point_c = MapCoord::fromRaw(start_point.X, start_point.Y);
	MapCoord second_point_c = MapCoord::fromRaw(second_point.X, second_point.Y);
	MapCoord second_last_point_c = MapCoord::fromRaw(second_last_point.X, second_last_point.Y);
	MapCoord end_point_c = MapCoord::fromRaw(end_point.X, end_point.Y);
	
	MapCoordF polygon_start_tangent = MapCoordF(second_point_c - start_point_c);
	polygon_start_tangent.normalize();
	MapCoordF polygon_end_tangent = MapCoordF(second_last_point_c - end_point_c);
	polygon_end_tangent.normalize();
	
	double tangent_length = BEZIER_HANDLE_DISTANCE * start_point_c.lengthTo(end_point_c);
	object->addCoordinate((MapCoordF(start_point_c) + tangent_length * polygon_start_tangent).toMapCoord());
	object->addCoordinate((MapCoordF(end_point_c) + tangent_length * polygon_end_tangent).toMapCoord());
	object->addCoordinate(end_point_c);
}

void BooleanTool::rebuildTwoIndexSegment(int start_index, int end_index, bool sequence_increasing, const ClipperLib::Path& polygon, const PolyMap& polymap, PathObject* object)
{
	// sequence_increasing is only used in Q_ASSERT.
#ifndef NDEBUG
	Q_UNUSED(sequence_increasing);
#endif
	
	PathCoordInfo start_info = polymap.value(polygon.at(start_index));
	PathCoordInfo end_info = polymap.value(polygon.at(end_index));
	PathObject* original = end_info.first->path;
	
	bool coords_increasing;
	bool is_curve;
	int coord_index;
	if (start_info.second->index == end_info.second->index)
	{
		coord_index = end_info.second->index;
		bool found = checkSegmentMatch(original, coord_index, polygon, start_index, end_index, coords_increasing, is_curve);
		if (!found)
		{
			object->getCoordinate(object->getCoordinateCount() - 1).setCurveStart(false);
			rebuildCoordinate(end_index, polygon, polymap, object);
			return;
		}
		Q_ASSERT(coords_increasing == sequence_increasing);
	}
	else
	{
		coord_index = end_info.second->index;
		bool found = checkSegmentMatch(original, coord_index, polygon, start_index, end_index, coords_increasing, is_curve);
		if (!found)
		{
			coord_index = start_info.second->index;
			found = checkSegmentMatch(original, coord_index, polygon, start_index, end_index, coords_increasing, is_curve);
			if (!found)
			{
				object->getCoordinate(object->getCoordinateCount() - 1).setCurveStart(false);
				rebuildCoordinate(end_index, polygon, polymap, object);
				return;
			}
		}
	}
	
	if (!is_curve)
		object->getCoordinate(object->getCoordinateCount() - 1).setCurveStart(false);
	if (coords_increasing)
	{
		object->addCoordinate(resetCoordinate(original->getCoordinate(coord_index + 1)));
		if (is_curve)
		{
			object->addCoordinate(original->getCoordinate(coord_index + 2));
			object->addCoordinate(resetCoordinate(original->getCoordinate(coord_index + 3)));
		}
	}
	else
	{
		if (is_curve)
		{
			object->addCoordinate(resetCoordinate(original->getCoordinate(coord_index + 2)));
			object->addCoordinate(original->getCoordinate(coord_index + 1));
		}
		object->addCoordinate(resetCoordinate(original->getCoordinate(coord_index + 0)));
	}
}

void BooleanTool::rebuildCoordinate(int index, const ClipperLib::Path& polygon, const PolyMap& polymap, PathObject* object, bool start_new_part)
{
	MapCoord coord(0.001 * polygon.at(index).X, 0.001 * polygon.at(index).Y);
	if (polymap.contains(polygon.at(index)))
	{
		PathCoordInfo info = polymap.value(polygon.at(index));
		MapCoord& original = info.first->path->getCoordinate(info.second->index);
		
		if (original.isDashPoint())
			coord.setDashPoint(true);
	}
	object->addCoordinate(coord, start_new_part);
}

bool BooleanTool::checkSegmentMatch(
        PathObject* original,
        int coord_index,
        const ClipperLib::Path& polygon,
        int start_index,
        int end_index,
        bool& out_coords_increasing,
        bool& out_is_curve )
{
	
	MapCoord& first = original->getCoordinate(coord_index);
	out_is_curve = first.isCurveStart();
	MapCoord& other = original->getCoordinate(coord_index + (out_is_curve ? 3 : 1));
	
	bool found = true;
	if (first == polygon.at(start_index) && other == polygon.at(end_index))
	{
		out_coords_increasing = true;
	}
	else if (first == polygon.at(end_index) && other == polygon.at(start_index))
	{
		out_coords_increasing = false;
	}
	else
	{
		found = false;
	}
	
	return found;
}

