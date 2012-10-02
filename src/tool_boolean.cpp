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


#include "tool_boolean.h"

#include <algorithm>

#include "map.h"
#include "symbol.h"
#include "object.h"
#include "map_undo.h"

using namespace ClipperLib;

BooleanTool::BooleanTool(Map* map) : map(map)
{
}

bool BooleanTool::execute(BooleanTool::Operation op)
{
	PathObjects result;
	AddObjectsUndoStep* add_step = new AddObjectsUndoStep(map);
	MapPart* part = map->getCurrentPart();

	if (op != Difference)
	{
		// Find all groups (of at least two areas with the same symbol) to process
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
		for (ObjectGroups::iterator it = object_groups.begin(); it != object_groups.end();)
		{
			if (it.value().size() <= 1)
				it = object_groups.erase(it);
			else
				++it;
		}
	}
	else
	{
		// Make a dummy group with all selected areas
		PathObjects new_vector;
		new_vector.reserve(map->getNumSelectedObjects());
		Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
		for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
			new_vector.push_back(reinterpret_cast<PathObject*>(*it));
		object_groups.insert(NULL, new_vector);
	}
	
	// For all groups of objects with the same symbol ...
	for (ObjectGroups::iterator it = object_groups.begin(); it != object_groups.end(); ++it)
	{
		// Do operation
		PathObjects group_result;
		if (!executeImpl(op, it.value(), group_result))
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
	
	map->emitSelectionChanged();
	map->emitSelectionEdited();
	
	return true;
}

bool BooleanTool::executeImpl(BooleanTool::Operation op, PathObjects& in_objects, PathObjects& out_objects)
{
	// Convert the objects to Clipper polygons and create a hash map, mapping point positions to the PathCoords
	Polygons subject_polygons;
	Polygons clip_polygons;
	QHash< qint64, PathCoordInfo > polymap;
	
	for (int object_number = 0; object_number < (int)in_objects.size(); ++object_number)
	{
		PathObject* object = in_objects[object_number];
		object->update();
		const PathCoordVector& path_coords = object->getPathCoordinateVector();
		
		for (int part_number = 0; part_number < object->getNumParts(); ++part_number)
		{
			Polygon* polygon;
			if (object == map->getFirstSelectedObject())
			{
				subject_polygons.push_back(Polygon());
				polygon = &subject_polygons.back();
			}
			else
			{
				clip_polygons.push_back(Polygon());
				polygon = &clip_polygons.back();
			}
			
			PathObject::PathPart& part = object->getPart(part_number);
			
			for (int i = part.path_coord_start_index + (part.isClosed() ? 1 : 0); i <= part.path_coord_end_index; ++i)
			{
				polygon->push_back(IntPoint(path_coords[i].pos.getIntX(), path_coords[i].pos.getIntY()));
				polymap.insert(intPointToQInt64(polygon->back()), std::make_pair(&part, &path_coords[i]));	// FIXME: would it be possible to fix potential collisions here?
			}
			
			bool orientation = Orientation(*polygon);
			if ((part_number == 0 && !orientation) || (part_number > 0 && orientation))
				std::reverse(polygon->begin(), polygon->end());
		}
	}
	
	// Do the operation. Add the first selected object as subject, the rest as clip polygons
	Clipper clipper;
	clipper.AddPolygons(subject_polygons, ptSubject);
	clipper.AddPolygons(clip_polygons, ptClip);
	
	ClipType clip_type;
	if (op == Union) clip_type = ctUnion;
	else if (op == Intersection) clip_type = ctIntersection;
	else if (op == Difference) clip_type = ctDifference;
	else if (op == XOr) clip_type = ctXor;
	ExPolygons solution;
	if (!clipper.Execute(clip_type, solution, pftNonZero, pftNonZero))
		return false;
	
	// Try to convert the solution polygons to objects again
	for (int polygon_number = 0; polygon_number < (int)solution.size(); ++polygon_number)
	{
		ExPolygon& expolygon = solution.at(polygon_number);
		
		PathObject* object = new PathObject();
		object->setSymbol(map->getFirstSelectedObject()->getSymbol(), true);
		
		polygonToPathPart(expolygon.outer, polymap, object);
		for (int i = 0; i < (int)expolygon.holes.size(); ++i)
			polygonToPathPart(expolygon.holes.at(i), polymap, object);
		
		out_objects.push_back(object);
	}
	
	return true;
}

void BooleanTool::polygonToPathPart(Polygon& polygon, QHash< qint64, PathCoordInfo >& polymap, PathObject* object)
{
	if (polygon.size() < 3)
		return;
	
	// Check if we can find either an unknown intersection point or a path coord with parameter 0 or 1.
	// This gives a starting point to search for curves to rebuild (because we cannot start in the middle of a curve)
	int num_points = (int)polygon.size();
	int part_start_index = 0;
	
	for (; part_start_index < num_points; ++part_start_index)
	{
		if (!polymap.contains(intPointToQInt64(polygon.at(part_start_index))))
			break;
		
		const PathCoord* path_coord = polymap.value(intPointToQInt64(polygon.at(part_start_index))).second;
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
	cur_info.first = NULL;
	cur_info.second = NULL;
	if (polymap.contains(intPointToQInt64(polygon.at(part_start_index))))
		cur_info = polymap.value(intPointToQInt64(polygon.at(part_start_index)));
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
		new_info.first = NULL;
		new_info.second = NULL;
		if (polymap.contains(intPointToQInt64(polygon.at(i))))
			new_info = polymap.value(intPointToQInt64(polygon.at(i)));
		
		/*qDebug() << new_info.first << " " << i;
		if (new_info.second)
		{
			qDebug() << "  " << new_info.second->index << " " << new_info.second->param << " " << new_info.second->pos.getX() << " " << new_info.second->pos.getY();
			qDebug() << "  " << polygon.at(i).X << " " << polygon.at(i).Y;
		}*/
		
		if (cur_info.first == new_info.first && cur_info.first)
		{
			MapCoord& cur_coord = cur_info.first->path->getCoordinate(cur_info.second->index);
			MapCoord& new_coord = new_info.first->path->getCoordinate(new_info.second->index);
			if (cur_info.second->index == new_info.second->index)
			{
				if (have_sequence && sequence_increasing != (new_info.second->param > cur_info.second->param))
					stop_before = true;
				else
				{
					have_sequence = true;
					sequence_increasing = new_info.second->param > cur_info.second->param;
				}
			}
			else if (((cur_coord.isCurveStart() && new_info.second->index == cur_info.second->index + 3) ||
					 (!cur_coord.isCurveStart() && new_info.second->index == cur_info.second->index + 1) ||
					 new_info.second->index == 0) && cur_info.second->param >= 1)
			{
				if (have_sequence && sequence_increasing != true)
					stop_before = true;
				else
				{
					have_sequence = true;
					sequence_increasing = true;
				}
			}
			else if (((new_coord.isCurveStart() && new_info.second->index == cur_info.second->index - 3) ||
					 (!new_coord.isCurveStart() && new_info.second->index == cur_info.second->index - 1) ||
					 cur_info.second->index == 0) && new_info.second->param >= 1)
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

void BooleanTool::rebuildSegment(int start_index, int end_index, bool have_sequence, bool sequence_increasing, Polygon& polygon, QHash< qint64, PathCoordInfo >& polymap, PathObject* object)
{
	int num_points = (int)polygon.size();
	
	// Is there a sequence of at least two points belonging to the same curve?
	if (have_sequence)
	{
		object->getCoordinate(object->getCoordinateCount() - 1).setCurveStart(true);
		
		if ((start_index + 1) % num_points == end_index)
		{
			// This could happen for a straight line or a very flat curve - take coords directly from original
			PathCoordInfo start_info = polymap.value(intPointToQInt64(polygon.at(start_index)));
			PathCoordInfo end_info = polymap.value(intPointToQInt64(polygon.at(end_index)));
			PathObject* original = end_info.first->path;
			
			bool coords_increasing;
			bool is_curve;
			int coord_index;
			if (start_info.second->index == end_info.second->index)
			{
				coord_index = end_info.second->index;
				bool found = check_segment_match(coord_index, original, polygon, start_index, end_index, coords_increasing, is_curve);
				if (!found)
				{
					object->getCoordinate(object->getCoordinateCount() - 1).setCurveStart(false);
					rebuildCoordinate(end_index, polygon, polymap, object);
					return;
				}
				assert(coords_increasing == sequence_increasing);
			}
			else
			{
				coord_index = end_info.second->index;
				bool found = check_segment_match(coord_index, original, polygon, start_index, end_index, coords_increasing, is_curve);
				if (!found)
				{
					coord_index = start_info.second->index;
					found = check_segment_match(coord_index, original, polygon, start_index, end_index, coords_increasing, is_curve);
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
				object->addCoordinate(convertOriginalCoordinate(original->getCoordinate(coord_index + 1)));
				if (is_curve)
				{
					object->addCoordinate(original->getCoordinate(coord_index + 2));
					object->addCoordinate(convertOriginalCoordinate(original->getCoordinate(coord_index + 3)));
				}
			}
			else
			{
				if (is_curve)
				{
					object->addCoordinate(convertOriginalCoordinate(original->getCoordinate(coord_index + 2)));
					object->addCoordinate(original->getCoordinate(coord_index + 1));
				}
				object->addCoordinate(convertOriginalCoordinate(original->getCoordinate(coord_index + 0)));
			}
		}
		else
		{
			// Find out start tangent
			float start_param;
			MapCoord start_coord = MapCoord(0.001 * polygon.at(start_index).X, 0.001 * polygon.at(start_index).Y);
			MapCoord start_tangent;
			MapCoord original_end_tangent;
			
			PathCoordInfo start_info;
			start_info.first = NULL;
			IntPoint& start_point = polygon.at(start_index);
			if (polymap.contains(intPointToQInt64(start_point)))
				start_info = polymap.value(intPointToQInt64(start_point));
			
			PathCoordInfo second_info;
			second_info.first = NULL;
			IntPoint& second_point = polygon.at((start_index + 1) % num_points);
			if (polymap.contains(intPointToQInt64(second_point)))
				second_info = polymap.value(intPointToQInt64(second_point));
			
			assert(second_info.first);
			PathObject* original = second_info.first->path;
			int original_index = second_info.second->index;
			
			if (start_info.first && start_info.first == second_info.first &&
				((start_info.second->index == second_info.second->index) ||
				 (sequence_increasing && start_info.second->param >= 1))) //&& start_info.second->index + 1 == second_info.second->index
			{
				// Take coordinates directly
				start_param = sequence_increasing ? 0 : 1;
				start_tangent = sequence_increasing ? original->getCoordinate(original_index + 1) : original->getCoordinate(original_index + 2);
				original_end_tangent = sequence_increasing ? original->getCoordinate(original_index + 2) : original->getCoordinate(original_index + 1);
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
				}
			}
			
			// Find better end point approximation and its tangent
			MapCoord end_tangent;
			MapCoord end_coord;
			
			PathCoordInfo end_info;
			end_info.first = NULL;
			IntPoint& end_point = polygon.at(end_index);
			if (polymap.contains(intPointToQInt64(end_point)))
				end_info = polymap.value(intPointToQInt64(end_point));
			
			PathCoordInfo second_last_info;
			second_last_info.first = NULL;
			int second_last_index = end_index - 1;
			if (second_last_index < 0)
				second_last_index = num_points - 1;
			IntPoint& second_last_point = polygon.at(second_last_index);
			if (polymap.contains(intPointToQInt64(second_last_point)))
				second_last_info = polymap.value(intPointToQInt64(second_last_point));
			
			if (end_info.first && end_info.first == second_last_info.first &&
				((end_info.second->index == second_last_info.second->index) ||
				(!sequence_increasing && end_info.second->param >= 1))) //&& end_info.second->index + 1 == second_last_info.second->index
			{
				// Take coordinates directly
				end_coord = sequence_increasing ? original->getCoordinate(original_index + 3) : original->getCoordinate(original_index + 0);
				end_tangent = original_end_tangent;
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
				}
			}
			
			// Rebuild bezier curve
			object->addCoordinate(start_tangent);
			object->addCoordinate(end_tangent);
			object->addCoordinate(convertOriginalCoordinate(end_coord));
			
			qint64 TESTX = end_point.X - end_coord.rawX();
			qint64 TESTY = end_point.Y - end_coord.rawY();
			assert(sqrt(TESTX*TESTX + TESTY*TESTY) < 400);
		}
	}
	else
	{
		// Add a straight line segment
		rebuildCoordinate(end_index, polygon, polymap, object);
	}
}

void BooleanTool::rebuildCoordinate(int index, Polygon& polygon, QHash< qint64, PathCoordInfo >& polymap, PathObject* object, bool start_new_part)
{
	MapCoord coord(0.001 * polygon.at(index).X, 0.001 * polygon.at(index).Y);
	if (polymap.contains(intPointToQInt64(polygon.at(index))))
	{
		PathCoordInfo info = polymap.value(intPointToQInt64(polygon.at(index)));
		MapCoord& original = info.first->path->getCoordinate(info.second->index);
		
		if (original.isDashPoint())
			coord.setDashPoint(true);
	}
	object->addCoordinate(coord, start_new_part);
}

MapCoord BooleanTool::convertOriginalCoordinate(MapCoord in)
{
	in.setHolePoint(false);
	in.setClosePoint(false);
	in.setCurveStart(false);
	return in;
}

bool BooleanTool::check_segment_match(int coord_index, PathObject* original, Polygon& polygon, int start_index, int end_index, bool& out_coords_increasing, bool& out_is_curve)
{
	bool found;
	MapCoord& coord_index_coord = original->getCoordinate(coord_index);
	if (coord_index_coord.isCurveStart())
	{
		MapCoord& coord_index_coord3 = original->getCoordinate(coord_index + 3);
		found = true;
		out_is_curve = true;
		if (coord_index_coord.rawX() == polygon.at(start_index).X && coord_index_coord.rawY() == polygon.at(start_index).Y &&
			coord_index_coord3.rawX() == polygon.at(end_index).X && coord_index_coord3.rawY() == polygon.at(end_index).Y)
			out_coords_increasing = true;
		else if (coord_index_coord.rawX() == polygon.at(end_index).X && coord_index_coord.rawY() == polygon.at(end_index).Y &&
			coord_index_coord3.rawX() == polygon.at(start_index).X && coord_index_coord3.rawY() == polygon.at(start_index).Y)
			out_coords_increasing = false;
		else
			found = false;
	}
	else
	{
		MapCoord& coord_index_coord1 = original->getCoordinate(coord_index + 1);
		found = true;
		out_is_curve = false;
		if (coord_index_coord.rawX() == polygon.at(start_index).X && coord_index_coord.rawY() == polygon.at(start_index).Y &&
			coord_index_coord1.rawX() == polygon.at(end_index).X && coord_index_coord1.rawY() == polygon.at(end_index).Y)
			out_coords_increasing = true;
		else if (coord_index_coord.rawX() == polygon.at(end_index).X && coord_index_coord.rawY() == polygon.at(end_index).Y &&
			coord_index_coord1.rawX() == polygon.at(start_index).X && coord_index_coord1.rawY() == polygon.at(start_index).Y)
			out_coords_increasing = false;
		else
			found = false;
	}
	return found;
}
