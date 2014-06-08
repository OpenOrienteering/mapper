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
#include "map_undo.h"
#include "util.h"


//### Local helper functions

/**
 * Implements (specializes) qHash for ClipperLib::IntPoint.
 * 
 * This is needed to use ClipperLib::IntPoint as QHash key.
 */
template < >
uint qHash(const ClipperLib::IntPoint& point, uint seed)
{
	qint64 tmp = (point.X & 0xffffffff) | (point.Y << 32);
	return qHash(tmp, seed);
}

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
	// Objects to process, sorted by symbol
	typedef QHash< Symbol*, PathObjects > ObjectGroups;
	ObjectGroups object_groups;
	
	PathObjects result;
	AddObjectsUndoStep* add_step = new AddObjectsUndoStep(map);
	MapPart* part = map->getCurrentPart();

	if (op == Difference)
	{
		// Make a dummy group with all selected areas
		PathObjects new_vector;
		new_vector.reserve(map->getNumSelectedObjects());
		Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
		for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
			new_vector.push_back(reinterpret_cast<PathObject*>(*it));
		object_groups.insert(NULL, new_vector);
	}
	else
	{
		// Find all groups (of at least two areas with the same symbol) to process.
		// (If op == MergeHoles, do not enforce at least two objects.)
		Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
		for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
		{
			Symbol* symbol = (*it)->getSymbol();
			if (symbol->getContainedTypes() & Symbol::Area && (*it)->getType() == Object::Path)
			{
				if (object_groups.contains(symbol))
					object_groups.find(symbol).value().push_back(reinterpret_cast<PathObject*>(*it));
				else
				{
					PathObjects new_vector;
					new_vector.push_back(reinterpret_cast<PathObject*>(*it));
					object_groups.insert(symbol, new_vector);
				}
			}
		}
		if (op != MergeHoles)
		{
			for (ObjectGroups::iterator it = object_groups.begin(); it != object_groups.end();)
			{
				if (it.value().size() <= 1)
					it = object_groups.erase(it);
				else
					++it;
			}
		}
	}
	
	// For all groups of objects with the same symbol ...
	for (ObjectGroups::iterator it = object_groups.begin(); it != object_groups.end(); ++it)
	{
		// Do operation
		PathObjects group_result;
		if (!executeForObjects(map->getFirstSelectedObject()->asPath(), map->getFirstSelectedObject()->getSymbol(), it.value(), group_result))
		{
			delete add_step;
			for (int i = 0; i < (int)result.size(); ++i)
				delete result[i];
			return false;
		}
		
		// Add original objects to add step
		for (int i = 0; i < (int)it.value().size(); ++i)
		{
			Object* object = it.value().at(i);
			if (op == Difference && object != map->getFirstSelectedObject())
				continue;
			add_step->addObject(object, object);
		}
		
		// Collect resulting objects
		result.insert(result.end(), group_result.begin(), group_result.end());
	}
	
	// Remove original objects from map
	std::vector< Object* > in_objects;
	if (op == Difference)
		in_objects.push_back(map->getFirstSelectedObject());
	else
		add_step->getAffectedOutcome(in_objects);
	for (int i = 0; i < (int)in_objects.size(); ++i)
	{
		map->removeObjectFromSelection(in_objects[i], false);
		map->getCurrentPart()->deleteObject(in_objects[i], true);
		in_objects[i]->setMap(map); // necessary so objects are saved correctly
	}
	
	// Add resulting objects to map and create delete step for them
	for (int i = 0; i < (int)result.size(); ++i)
	{
		map->addObject(result[i]);
		map->addObjectToSelection(result[i], false);
	}
	DeleteObjectsUndoStep* delete_step = new DeleteObjectsUndoStep(map);
	for (int i = 0; i < (int)result.size(); ++i) // keep as separate loop to get the correct order
		delete_step->addObject(part->findObjectIndex(result[i]));
	
	CombinedUndoStep* undo_step = new CombinedUndoStep((void*)map);
	undo_step->addSubStep(delete_step);
	undo_step->addSubStep(add_step);
	map->objectUndoManager().addNewUndoStep(undo_step);
	map->setObjectsDirty();
	
	map->emitSelectionChanged();
	map->emitSelectionEdited();
	
	return true;
}

bool BooleanTool::executeForObjects(PathObject* subject, Symbol* result_objects_symbol, PathObjects& in_objects, PathObjects& out_objects)
{
	// Convert the objects to Clipper polygons and create a hash map, mapping point positions to the PathCoords
	ClipperLib::Paths subject_polygons;
	ClipperLib::Paths clip_polygons;
	PolyMap polymap;
	
	for (int object_number = 0; object_number < (int)in_objects.size(); ++object_number)
	{
		PathObject* object = in_objects[object_number];
		object->update();
		const PathCoordVector& path_coords = object->getPathCoordinateVector();
		
		for (int part_number = 0; part_number < object->getNumParts(); ++part_number)
		{
			ClipperLib::Path* polygon;
			if (object == subject)
			{
				subject_polygons.push_back(ClipperLib::Path());
				polygon = &subject_polygons.back();
			}
			else
			{
				clip_polygons.push_back(ClipperLib::Path());
				polygon = &clip_polygons.back();
			}
			
			PathObject::PathPart& part = object->getPart(part_number);
			
			for (int i = part.path_coord_start_index + (part.isClosed() ? 1 : 0); i <= part.path_coord_end_index; ++i)
			{
				polygon->push_back(ClipperLib::IntPoint(path_coords[i].pos.getIntX(), path_coords[i].pos.getIntY()));
				polymap.insertMulti(polygon->back(), std::make_pair(&part, &path_coords[i]));
			}
			
			bool orientation = Orientation(*polygon);
			if ((part_number == 0 && !orientation) || (part_number > 0 && orientation))
				std::reverse(polygon->begin(), polygon->end());
		}
	}
	
	// Do the operation. Add the first selected object as subject, the rest as clip polygons
	// These paths are to be regarded as closed.
	ClipperLib::Clipper clipper;
	clipper.AddPaths(subject_polygons, ClipperLib::ptSubject, true);
	clipper.AddPaths(clip_polygons, ClipperLib::ptClip, true);
	
	ClipperLib::ClipType clip_type;
	if (op == Union) clip_type = ClipperLib::ctUnion;
	else if (op == Intersection) clip_type = ClipperLib::ctIntersection;
	else if (op == Difference) clip_type = ClipperLib::ctDifference;
	else if (op == XOr) clip_type = ClipperLib::ctXor;
	else if (op == MergeHoles) clip_type = ClipperLib::ctUnion;
	else return false;
	
	ClipperLib::PolyFillType fill_type;
	if (op == MergeHoles)
		fill_type = ClipperLib::pftPositive;
	else
		fill_type = ClipperLib::pftNonZero;
	
	ClipperLib::PolyTree solution;
	if (!clipper.Execute(clip_type, solution, fill_type, fill_type))
		return false;
	
	// Try to convert the solution polygons to objects again
	polyTreeToPathObjects(solution, out_objects, result_objects_symbol, polymap);
	
	return true;
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
	if (op != BooleanTool::Intersection && op == BooleanTool::Difference)
	{
		Q_ASSERT(false && "Only intersection and difference are supported.");
		return; // no-op in release build
	}
	if (line->getNumParts() == 1)
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

void BooleanTool::polygonToPathPart(const ClipperLib::Path& polygon, PolyMap& polymap, PathObject* object)
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

void BooleanTool::rebuildSegment(int start_index, int end_index, bool have_sequence, bool sequence_increasing, const ClipperLib::Path& polygon, PolyMap& polymap, PathObject* object)
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
	PolyMap::iterator second_it = polymap.find(second_point);
	PolyMap::iterator second_last_it = polymap.find(second_last_point);
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
	
	PolyMap::iterator start_it = polymap.find(start_point);
	while (start_it != polymap.end())
	{
		if (start_it->first == original_path)
			break;
		++start_it;
	}
	if (start_it != polymap.end())
		start_info = *start_it;
	
	PolyMap::iterator end_it = polymap.find(end_point);
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

void BooleanTool::rebuildTwoIndexSegment(int start_index, int end_index, bool sequence_increasing, const ClipperLib::Path& polygon, PolyMap& polymap, PathObject* object)
{
	// sequence_increasing is only used in Q_ASSERT.
	// This won't be checked in release builds so the compiler is right to issue a warning...
	/* Q_UNUSED(sequence_increasing); */ 
	
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

void BooleanTool::rebuildCoordinate(int index, const ClipperLib::Path& polygon, PolyMap& polymap, PathObject* object, bool start_new_part)
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
	bool is_curve = first.isCurveStart();
	MapCoord& other = original->getCoordinate(coord_index + is_curve ? 3 : 1);
	
	bool found = true;
	if (first == polygon.at(start_index) && other == polygon.at(end_index))
	{
		out_coords_increasing = true;
		out_is_curve = is_curve;
	}
	else if (first == polygon.at(end_index) && other == polygon.at(start_index))
	{
		out_coords_increasing = false;
		out_is_curve = is_curve;
	}
	else
	{
		found = false;
	}
	
	return found;
}

