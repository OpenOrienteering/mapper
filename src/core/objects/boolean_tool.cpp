/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014, 2015 Kai Pastor
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


#include "boolean_tool.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include <QtGlobal>
#include <QDebug>
#include <QScopedPointer>

#include "core/map.h"
#include "core/map_coord.h"
#include "core/map_part.h"
#include "core/path_coord.h"
#include "core/virtual_path.h"
#include "core/objects/object.h"
#include "core/symbols/symbol.h"
#include "undo/object_undo.h"
#include "undo/undo.h"
#include "util/backports.h"
#include "util/util.h"


//### Local helper functions

/*
 * NB: qHash must be in the same namespace as the QHash key.
 * This is a C++ language issue, not a Qt issue.
 * Cf. https://bugreports.qt-project.org/browse/QTBUG-34912
 */
namespace ClipperLib {

/**
 * Implements qHash for ClipperLib::IntPoint.
 * 
 * This is needed to use ClipperLib::IntPoint as QHash key.
 */
uint qHash(const IntPoint& point, uint seed)
{
	quint64 tmp = (quint64(point.X) & 0xffffffff) | (quint64(point.Y) << 32);
	return ::qHash(tmp, seed); // must use :: namespace to prevent endless recursion
}


}  // namespace ClipperLib



namespace OpenOrienteering {

/**
 * Removes flags from the coordinate to be able to use it in the reconstruction.
 */
static MapCoord resetCoordinate(MapCoord in)
{
	in.setFlags(0);
	return in;
}

/**
 * Compares a MapCoord and ClipperLib::IntPoint.
 */
bool operator==(const MapCoord& lhs, const ClipperLib::IntPoint& rhs)
{
	return lhs.nativeX() == rhs.X && lhs.nativeY() == rhs.Y;
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
		qWarning("The first selected object must be a path.");
		return false; // in release build
	}

	// Filter selected objects into in_objects
	PathObjects in_objects;
	in_objects.reserve(map->getNumSelectedObjects());
	for (Object* object : map->selectedObjects())
	{
		if (object->getType() == Object::Path)
		{
			PathObject* const path = object->asPath();
			if (op != MergeHoles || (object->getSymbol()->getContainedTypes() & Symbol::Area && path->parts().size() > 1 ))
			{
				in_objects.push_back(path);
			}
		}
	}
	
	// Perform the core operation
	QScopedPointer<CombinedUndoStep> undo_step(new CombinedUndoStep(map));
	PathObjects out_objects;
	if (!executeForObjects(primary_object->asPath(), in_objects, out_objects, *undo_step))
	{
		Q_ASSERT(out_objects.size() == 0);
		return false; // in release build
	}
	
	map->push(undo_step.take());
	map->setObjectsDirty();
	map->emitSelectionChanged();
	map->emitSelectionEdited();
	return true;
}

bool BooleanTool::executePerSymbol()
{
	PathObjects backlog;
	backlog.reserve(map->getNumSelectedObjects());
	
	// Filter area objects into initial backlog
	for (Object* object : map->selectedObjects())
	{
		if (object->getSymbol()->getContainedTypes() & Symbol::Area)
			backlog.push_back(object->asPath());
	}
	
	QScopedPointer<CombinedUndoStep> undo_step(new CombinedUndoStep(map));
	PathObjects new_backlog;
	new_backlog.reserve(backlog.size()/2);
	PathObjects in_objects;
	in_objects.reserve(backlog.size()/2);
	PathObjects out_objects;
	while (!backlog.empty())
	{
		PathObject* const primary_object = backlog.front();
		const Symbol* const symbol = primary_object->getSymbol();
		
		// Filter objects by symbol into in_objects or new_backlog, respectively
		new_backlog.clear();
		in_objects.clear();
		for (PathObject* object : backlog)
		{
			if (object->getSymbol() == symbol)
			{
				if (op != MergeHoles || (object->getSymbol()->getContainedTypes() & Symbol::Area && object->parts().size() > 1 ))
				{
					in_objects.push_back(object);
				}
			}
			else
			{
				new_backlog.push_back(object);
			}
		}
		backlog.swap(new_backlog);
		
		// Short cut for single object of given symbol
		if (in_objects.size() == 1)
			continue;
		
		// Perform the core operation
		out_objects.clear();
		executeForObjects(primary_object, in_objects, out_objects, *undo_step);
	}
	
	bool const have_changes = undo_step->getNumSubSteps() > 0;
	if (have_changes)
	{
		map->push(undo_step.take());
		map->setObjectsDirty();
		map->emitSelectionChanged();
		map->emitSelectionEdited();
	}
	return have_changes;
}

bool BooleanTool::executeForObjects(PathObject* subject, PathObjects& in_objects, PathObjects& out_objects, CombinedUndoStep& undo_step)
{
	if (!executeForObjects(subject, in_objects, out_objects))
	{
		Q_ASSERT(out_objects.size() == 0);
		return false; // in release build
	}
	
	// Add original objects to undo step, and remove them from map.
	QScopedPointer<AddObjectsUndoStep> add_step(new AddObjectsUndoStep(map));
	for (PathObject* object : in_objects)
	{
		if (op != Difference || object == subject)
		{
			add_step->addObject(object, object);
		}
	}
	// Keep as separate loop to get the correct index in the previous loop
	for (PathObject* object : in_objects)
	{
		if (op != Difference || object == subject)
		{
			map->removeObjectFromSelection(object, false);
			map->getCurrentPart()->deleteObject(object, true);
			object->setMap(map); // necessary so objects are saved correctly
		}
	}
	
	// Add resulting objects to map, and create delete step for them
	QScopedPointer<DeleteObjectsUndoStep> delete_step(new DeleteObjectsUndoStep(map));
	MapPart* part = map->getCurrentPart();
	for (PathObject* object : out_objects)
	{
		map->addObject(object);
		map->addObjectToSelection(object, false);
	}
	// Keep as separate loop to get the correct index in the previous loop
	for (PathObject* object : out_objects)
	{
		delete_step->addObject(part->findObjectIndex(object));
	}
	
	undo_step.push(add_step.take());
	undo_step.push(delete_step.take());
	return true;
}

bool BooleanTool::executeForObjects(PathObject* subject, PathObjects& in_objects, PathObjects& out_objects)
{
	// Convert the objects to Clipper polygons and
	// create a hash map, mapping point positions to the PathCoords.
	// These paths are to be regarded as closed.
	PolyMap polymap;
	
	ClipperLib::Paths subject_polygons;
	pathObjectToPolygons(subject, subject_polygons, polymap);
	
	ClipperLib::Paths clip_polygons;
	for (PathObject* object : in_objects)
	{
		if (object != subject)
		{
			pathObjectToPolygons(object, clip_polygons, polymap);
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
	default:            qWarning("Undefined operation");
	                    return false;
	}

	ClipperLib::PolyTree solution;
	bool success = clipper.Execute(clip_type, solution, fill_type, fill_type);
	if (success)
	{
		// Try to convert the solution polygons to objects again
		polyTreeToPathObjects(solution, out_objects, subject, polymap);
	}
	
	return success;
}

void BooleanTool::polyTreeToPathObjects(const ClipperLib::PolyTree& tree, PathObjects& out_objects, const PathObject* proto, const PolyMap& polymap)
{
	for (int i = 0, count = tree.ChildCount(); i < count; ++i)
		outerPolyNodeToPathObjects(*tree.Childs[i], out_objects, proto, polymap);
}

void BooleanTool::outerPolyNodeToPathObjects(const ClipperLib::PolyNode& node, PathObjects& out_objects, const PathObject* proto, const PolyMap& polymap)
{
	auto object = std::unique_ptr<PathObject>{ proto->duplicate() };
	object->clearCoordinates();
	
	try
	{
		polygonToPathPart(node.Contour, polymap, object.get());
		for (int i = 0, i_count = node.ChildCount(); i < i_count; ++i)
		{
			polygonToPathPart(node.Childs[i]->Contour, polymap, object.get());
			
			// Add outer polygons contained by (nested within) holes ...
			for (int j = 0, j_count = node.Childs[i]->ChildCount(); j < j_count; ++j)
				outerPolyNodeToPathObjects(*node.Childs[i]->Childs[j], out_objects, proto, polymap);
		}
		
		out_objects.push_back(object.release());
	}
	catch (std::range_error&)
	{
		// Do nothing
	}
}



void BooleanTool::executeForLine(const PathObject* area, const PathObject* line, BooleanTool::PathObjects& out_objects)
{
	if (op != BooleanTool::Intersection && op != BooleanTool::Difference)
	{
		Q_ASSERT_X(false, "BooleanTool::executeForLine", "Only intersection and difference are supported.");
		return; // no-op in release build
	}
	if (line->parts().size() != 1)
	{
		Q_ASSERT_X(false, "BooleanTool::executeForLine", "Only single-part lines are supported.");
		return; // no-op in release build
	}
	
	// Calculate all intersection points of line with the area path
	PathObject::Intersections intersections;
	line->calcAllIntersectionsWith(area, intersections);
	intersections.normalize();
	
	PathObject* first_segment = nullptr;
	PathObject* last_segment = nullptr;
	
	const auto& part        = line->parts().front();
	const auto& path_coords = part.path_coords;
	
	// Only one segment?
	if (intersections.empty())
	{
		auto middle_length = part.length();
		auto middle = SplitPathCoord::at(path_coords, middle_length);
		if (area->isPointInsideArea(middle.pos) == (op == BooleanTool::Intersection))
			out_objects.push_back(line->duplicate());
		return;
	}
	
	// First segment
	auto middle_length = intersections[0].length / 2;
	auto middle = SplitPathCoord::at(path_coords, middle_length);
	if (area->isPointInsideArea(middle.pos) == (op == BooleanTool::Intersection))
	{
		PathObject* segment = line->duplicate();
		segment->changePathBounds(0, 0.0, intersections[0].length);
		first_segment = segment;
	}
	
	// Middle segments
	for (std::size_t i = 0; i < intersections.size() - 1; ++i)
	{
		middle_length = (intersections[i].length + intersections[i+1].length) / 2;
		auto middle = SplitPathCoord::at(path_coords, middle_length);
		if (area->isPointInsideArea(middle.pos) == (op == BooleanTool::Intersection))
		{
			PathObject* segment = line->duplicate();
			segment->changePathBounds(0, intersections[i].length, intersections[i+1].length);
			out_objects.push_back(segment);
		}
	}
	
	// Last segment
	middle_length = (part.length() + intersections.back().length) / 2;
	middle = SplitPathCoord::at(path_coords, middle_length);
	if (area->isPointInsideArea(middle.pos) == (op == BooleanTool::Intersection))
	{
		PathObject* segment = line->duplicate();
		segment->changePathBounds(0, intersections.back().length, part.length());
		last_segment = segment;
	}
	
	// If the line was closed at the beginning, merge the first and last segments if they were kept both.
	if (part.isClosed() && first_segment && last_segment)
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

void BooleanTool::pathObjectToPolygons(
        const PathObject* object,
        ClipperLib::Paths& polygons,
        PolyMap& polymap)
{
	object->update();
	auto coords = object->getRawCoordinateVector();
	
	polygons.reserve(polygons.size() + object->parts().size());
	
	for (const auto& part : object->parts())
	{
		const PathCoordVector& path_coords = part.path_coords;
		auto path_coords_end = path_coords.size();
		if (part.isClosed())
			--path_coords_end;
		
		ClipperLib::Path polygon;
		polygon.reserve(path_coords_end);
		for (auto i = 0u; i < path_coords_end; ++i)
		{
			auto& path_coord = path_coords[i];
			if (path_coord.param == 0.0)
			{
				auto point = coords[path_coord.index];
				polygon.push_back(ClipperLib::IntPoint(point.nativeX(), point.nativeY()));
			}
			else
			{
				auto point = MapCoord { path_coord.pos };
				polygon.push_back(ClipperLib::IntPoint(point.nativeX(), point.nativeY()));
			}
			polymap.insertMulti(polygon.back(), std::make_pair(&part, &path_coord));
		}
		
		bool orientation = Orientation(polygon);
		if ( (&part == &object->parts().front()) != orientation )
		{
			std::reverse(polygon.begin(), polygon.end());
		}
		
		// Push_back shall move the polygon.
		static_assert(std::is_nothrow_move_constructible<ClipperLib::Path>::value, "ClipperLib::Path must be nothrow move constructible");
		polygons.push_back(polygon);
	}
}

void BooleanTool::polygonToPathPart(const ClipperLib::Path& polygon, const PolyMap& polymap, PathObject* object)
{
	auto num_points = polygon.size();
	if (num_points < 3)
		return;
	
	// Index of first used point in polygon
	auto part_start_index = 0u;
	auto cur_info = PathCoordInfo{ nullptr, nullptr };
	
	// Check if we can find either an unknown intersection point
	// or a path coord with parameter 0.
	// This gives a starting point to search for curves to rebuild
	// (because we cannot start in the middle of a curve)
	for (; part_start_index < num_points; ++part_start_index)
	{
		auto current_point = polygon.at(part_start_index);
		if (!polymap.contains(current_point))
			break;
		
		if (polymap.value(current_point).second->param == 0.0)
		{
			cur_info = polymap.value(current_point);
			break;
		}
	}
	
	if (part_start_index == num_points)
	{
		// Did not find a valid starting point. Return the part as a polygon.
		for (auto i = 0u; i < num_points; ++i)
			object->addCoordinate(MapCoord::fromNative64(polygon.at(i).X, polygon.at(i).Y), i == 0);
		object->parts().back().setClosed(true, true);
		return;
	}
	
	// Add the first point to the object
	rebuildCoordinate(part_start_index, polygon, polymap, object, true);
	
	
	// Index of first segment point in polygon
	auto segment_start_index = part_start_index;
	bool have_sequence = false;
	bool sequence_increasing = false;
	bool stop_before = false;
	
	// Advance along the boundary and rebuild the curve for every sequence
	// of path coord pointers with the same path and index.
	auto i = part_start_index;
	do
	{
		++i;
		if (i >= num_points)
			i = 0;
		
		PathCoordInfo new_info{ nullptr, nullptr };
		auto new_point = polygon.at(i);
		if (polymap.contains(new_point))
			new_info = polymap.value(new_point);
		
		if (cur_info.first && cur_info.first == new_info.first)
		{
			// Same original part
			auto cur_coord_index = cur_info.second->index;
			const auto cur_coord = cur_info.first->path->getCoordinate(cur_coord_index);
			
			auto new_coord_index = new_info.second->index;
			const auto new_coord = new_info.first->path->getCoordinate(new_coord_index);
			
			auto cur_coord_index_adjusted = cur_coord_index;
			if (cur_coord_index_adjusted == new_info.first->first_index)
				cur_coord_index_adjusted = new_info.first->last_index;
			
			auto new_coord_index_adjusted = new_coord_index;
			if (new_coord_index_adjusted == new_info.first->first_index)
				new_coord_index_adjusted = new_info.first->last_index;
			
			if (cur_coord_index == new_coord_index)
			{
				// Somewhere on a curve
				bool param_increasing = new_info.second->param > cur_info.second->param;
				if (!have_sequence)
				{
					have_sequence = true;
					sequence_increasing = param_increasing;
				}
				else if (have_sequence && sequence_increasing != param_increasing)
				{
					stop_before = true;
				}
			}
			else if (new_info.second->param == 0.0 &&
			         ( (cur_coord.isCurveStart() && new_coord_index_adjusted == cur_coord_index + 3) ||
					   (!cur_coord.isCurveStart() && new_coord_index_adjusted == cur_coord_index + 1) ) )
			{
				// Original curve is from cur_coord_index to new_coord_index_adjusted.
				if (!have_sequence)
				{
					have_sequence = true;
					sequence_increasing = true;
				}
				else
				{
					stop_before = !sequence_increasing;
				}
			}
			else if (cur_info.second->param == 0.0 &&
			         ( (new_coord.isCurveStart() && new_coord_index + 3 == cur_coord_index_adjusted) ||
					   (!new_coord.isCurveStart() && new_coord_index + 1 == cur_coord_index_adjusted) ) )
			{
				// Original curve is from new_coord_index to cur_coord_index_adjusted.
				if (!have_sequence)
				{
					have_sequence = true;
					sequence_increasing = false;
				}
				else
				{
					stop_before = sequence_increasing;
				}
			}
			else if ((segment_start_index + 1) % num_points != i)
			{
				// Not immediately after segment_start_index
				stop_before = true;
			}
		}
		
		if (i == part_start_index ||
		    stop_before ||
		    (new_info.second && new_info.second->param == 0.0) ||
			(cur_info.first && (cur_info.first != new_info.first || cur_info.second->index != new_info.second->index) && i != (segment_start_index + 1) % num_points) ||
			!new_info.first)
		{
			if (stop_before)
			{
				if (i == 0)
					i = num_points - 1;
				else
					--i;
			}
			
			if (have_sequence)
				// A sequence of at least two points belonging to the same curve
				rebuildSegment(segment_start_index, i, sequence_increasing, polygon, polymap, object);
			else
				// A single straight edge
				rebuildCoordinate(i, polygon, polymap, object);
			
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
		
		cur_info = new_info;
	}
	while  (i != part_start_index);
	
	object->parts().back().connectEnds();
}

void BooleanTool::rebuildSegment(
        ClipperLib::Path::size_type start_index,
        ClipperLib::Path::size_type end_index,
        bool sequence_increasing,
        const ClipperLib::Path& polygon,
        const PolyMap& polymap,
        PathObject* object)
{
	auto num_points = polygon.size();
	
	object->getCoordinateRef(object->getCoordinateCount() - 1).setCurveStart(true);
	
	if ((start_index + 1) % num_points == end_index)
	{
		// This could happen for a straight line or a very flat curve - take coords directly from original
		rebuildTwoIndexSegment(start_index, end_index, sequence_increasing, polygon, polymap, object);
		return;
	}

	// Get polygon point coordinates
	const auto& start_point       = polygon.at(start_index);
	const auto& second_point      = polygon.at((start_index + 1) % num_points);
	const auto& second_last_point = polygon.at((end_index ? end_index : num_points) - 1);
	const auto& end_point         = polygon.at(end_index);
	
	// Try to find the middle coordinates in the same part
	bool found = false;
	PathCoordInfo second_info{ nullptr, nullptr };
	PathCoordInfo second_last_info{ nullptr, nullptr };
	for (auto second_it = polymap.find(second_point); second_it != polymap.end(); ++second_it)
	{
		for (auto second_last_it = polymap.find(second_last_point);
		     second_last_it != polymap.end() && second_last_it.key() == second_last_point;
		     ++second_last_it)
		{
			if (second_it->first == second_last_it->first &&
			    second_it->second->index == second_last_it->second->index)
			{
				// Same part
				found = true;
				second_info = *second_it;
				second_last_info = *second_last_it;
				break;
			}
		}
		if (found)
			break;
	}
	
	if (!found)
	{
		// Need unambiguous path part information to find the original object with high probability
		qDebug() << "BooleanTool::rebuildSegment: cannot identify original object!";
		rebuildSegmentFromPathOnly(start_point, second_point, second_last_point, end_point, object);
		return;
	}
	
	const PathPart* original_path = second_info.first;
	
	// Try to find the outer coordinates in the same part
	PathCoordInfo start_info{ nullptr, nullptr };
	for (auto start_it = polymap.find(start_point);
	     start_it != polymap.end() && start_it.key() == start_point;
	     ++start_it)
	{
		if (start_it->first == original_path)
		{
			start_info = *start_it;
			break;
		}
	}
	Q_ASSERT(!start_info.first || start_info.first == second_info.first);
	
	PathCoordInfo end_info{ nullptr, nullptr };
	for (auto end_it = polymap.find(end_point);
	     end_it != polymap.end() && end_it.key() == end_point;
	     ++end_it)
	{
		if (end_it->first == original_path)
		{
			end_info = *end_it;
			break;
		}
	}
	Q_ASSERT(!end_info.first || end_info.first == second_info.first);
	
	const PathObject* original = original_path->path;
	auto edge_start = second_info.second->index;
	if (edge_start == second_info.first->last_index)
		edge_start = second_info.first->first_index;
	
	// Find out start tangent
	auto start_param = 0.0;
	MapCoord start_coord = MapCoord::fromNative64(start_point.X, start_point.Y);
	MapCoord start_tangent;
	MapCoord end_tangent;
	MapCoord end_coord;
	
	double start_error_sq, end_error_sq;
	// Maximum difference in mm from reconstructed start and end coords to the
	// intersection points returned by Clipper
	const double error_bound = 0.4;
	
	if (sequence_increasing)
	{
		if ( second_info.second->param == 0.0 ||
		     ( start_info.first &&
		       start_info.second->param == 0.0 &&
		       ( start_info.second->index == edge_start ||
		         (start_info.second->index == start_info.first->last_index && start_info.first->first_index == edge_start) ) ) )
		{
			// Take coordinates directly
			start_tangent = original->getCoordinate(edge_start + 1);
			end_tangent = original->getCoordinate(edge_start + 2);
			
			start_error_sq = start_coord.distanceSquaredTo(original->getCoordinate(edge_start + 0));
			if (start_error_sq > error_bound)
				qDebug() << "BooleanTool::rebuildSegment: start error too high in increasing direct case: " << sqrt(start_error_sq);
		}
		else
		{
			// Approximate coords
			const PathCoord* prev_coord = second_info.second - 1;
			
			auto dx = second_point.X - start_point.X;
			auto dy = second_point.Y - start_point.Y;
			auto point_dist = 0.001 * sqrt(dx*dx + dy*dy);
			
			auto delta_start_param = (second_info.second->param - prev_coord->param) * point_dist / qMax(1e-7f, (second_info.second->clen - prev_coord->clen));
			start_param = qBound(0.0, second_info.second->param - delta_start_param, 1.0);
			
			MapCoordF unused, o2, o3, o4;
			PathCoord::splitBezierCurve(MapCoordF(original->getCoordinate(edge_start + 0)), MapCoordF(original->getCoordinate(edge_start + 1)),
			                            MapCoordF(original->getCoordinate(edge_start + 2)), MapCoordF(original->getCoordinate(edge_start + 3)),
			                            start_param,
			                            unused, unused, o2, o3, o4);
			start_tangent = MapCoord(o3);
			end_tangent = MapCoord(o4);
			
			start_error_sq = start_coord.distanceSquaredTo(MapCoord(o2));
			if (start_error_sq > error_bound)
				qDebug() << "BooleanTool::rebuildSegment: start error too high in increasing general case: " << sqrt(start_error_sq);
		}
		
		// Find better end point approximation and its tangent
		if ( second_last_info.second->param == 0.0 ||
		     (end_info.first &&
		      end_info.second->param == 0.0 &&
		      ( end_info.second->index == edge_start+3 ||
		        (end_info.second->index == end_info.first->first_index && end_info.first->last_index == edge_start+3) ) ) )
		{
			// Take coordinates directly
			end_coord = original->getCoordinate(edge_start + 3);
			
			auto test_x = end_point.X - end_coord.nativeX();
			auto test_y = end_point.Y - end_coord.nativeY();
			end_error_sq = 0.001 * sqrt(test_x*test_x + test_y*test_y);
			if (end_error_sq > error_bound)
				qDebug() << "BooleanTool::rebuildSegment: end error too high in increasing direct case: " << sqrt(end_error_sq);
		}
		else
		{
			// Approximate coords
			const PathCoord* next_coord = second_last_info.second + 1;
			auto next_coord_param = next_coord->param;
			if (next_coord_param == 0.0)
				next_coord_param = 1.0;
			
			auto dx = end_point.X - second_last_point.X;
			auto dy = end_point.Y - second_last_point.Y;
			auto point_dist = 0.001 * sqrt(dx*dx + dy*dy);
			
			auto delta_end_param = (next_coord_param - second_last_info.second->param) * point_dist / qMax(1e-7f, (next_coord->clen - second_last_info.second->clen));
			auto end_param = (second_last_info.second->param + delta_end_param - start_param) / (1.0 - start_param);
			
			MapCoordF o0, o1, o2, unused;
			PathCoord::splitBezierCurve(MapCoordF(start_coord), MapCoordF(start_tangent),
			                            MapCoordF(end_tangent), MapCoordF(original->getCoordinate(edge_start + 3)),
			                            end_param,
			                            o0, o1, o2, unused, unused);
			start_tangent = MapCoord(o0);
			end_tangent = MapCoord(o1);
			end_coord = MapCoord(o2);
			
			auto test_x = end_point.X - end_coord.nativeX();
			auto test_y = end_point.Y - end_coord.nativeY();
			end_error_sq = 0.001 * sqrt(test_x*test_x + test_y*test_y);
			if (end_error_sq > error_bound)
				qDebug() << "BooleanTool::rebuildSegment: end error too high in increasing general case: " << sqrt(end_error_sq);
		}
	}
	else // if (!sequence_increasing)
	{
		if ( second_info.second->param == 0.0 ||
		     ( start_info.first &&
		       start_info.second->param == 0.0 &&
		       ( start_info.second->index == edge_start+3 ||
			     (start_info.second->index == start_info.first->first_index && start_info.first->last_index == edge_start+3) ) ) )
		{
			// Take coordinates directly
			start_tangent = original->getCoordinate(edge_start + 2);
			end_tangent = original->getCoordinate(edge_start + 1);
			
			start_error_sq = start_coord.distanceSquaredTo(original->getCoordinate(edge_start + 3));
			if (start_error_sq > error_bound)
				qDebug() << "BooleanTool::rebuildSegment: start error too high in decreasing direct case: " << sqrt(start_error_sq);
		}
		else
		{
			// Approximate coords
			const PathCoord* next_coord = second_info.second + 1;
			auto next_coord_param = next_coord->param;
			if (next_coord_param == 0.0)
				next_coord_param = 1.0;
			
			auto dx = second_point.X - start_point.X;
			auto dy = second_point.Y - start_point.Y;
			auto point_dist = 0.001 * sqrt(dx*dx + dy*dy);
			
			auto delta_start_param = (next_coord_param - second_info.second->param) * point_dist / qMax(1e-7f, (next_coord->clen - second_info.second->clen));
			start_param = qBound(0.0, 1.0 - second_info.second->param + delta_start_param, 1.0);
			
			MapCoordF unused, o2, o3, o4;
			PathCoord::splitBezierCurve(MapCoordF(original->getCoordinate(edge_start + 3)), MapCoordF(original->getCoordinate(edge_start + 2)),
			                            MapCoordF(original->getCoordinate(edge_start + 1)), MapCoordF(original->getCoordinate(edge_start + 0)),
			                            start_param,
			                            unused, unused, o2, o3, o4);
			start_tangent = MapCoord(o3);
			end_tangent = MapCoord(o4);
			
			start_error_sq = start_coord.distanceSquaredTo(MapCoord(o2));
			if (start_error_sq > error_bound)
				qDebug() << "BooleanTool::rebuildSegment: start error too high in decreasing general case: " << sqrt(start_error_sq);
		}
		
		// Find better end point approximation and its tangent
		if ( second_last_info.second->param == 0.0 ||
		     ( end_info.first &&
		       end_info.second->param == 0.0 &&
		       ( end_info.second->index == edge_start ||
		         (end_info.second->index == end_info.first->last_index && end_info.first->first_index == edge_start) ) ) )
		{
			// Take coordinates directly
			end_coord = original->getCoordinate(edge_start + 0);
			
			auto test_x = end_point.X - end_coord.nativeX();
			auto test_y = end_point.Y - end_coord.nativeY();
			end_error_sq = 0.001 * sqrt(test_x*test_x + test_y*test_y);
			if (end_error_sq > error_bound)
				qDebug() << "BooleanTool::rebuildSegment: end error too high in decreasing direct case: " << sqrt(end_error_sq);
		}
		else
		{
			// Approximate coords
			const PathCoord* prev_coord = second_last_info.second - 1;
			
			auto dx = end_point.X - second_last_point.X;
			auto dy = end_point.Y - second_last_point.Y;
			auto point_dist = 0.001 * sqrt(dx*dx + dy*dy);
			
			auto delta_end_param = (second_last_info.second->param - prev_coord->param) * point_dist / qMax(1e-7f, (second_last_info.second->clen - prev_coord->clen));
			auto end_param = (1.0 - second_last_info.second->param + delta_end_param) / (1 - start_param);
			
			MapCoordF o0, o1, o2, unused;
			PathCoord::splitBezierCurve(MapCoordF(start_coord), MapCoordF(start_tangent),
			                            MapCoordF(end_tangent), MapCoordF(original->getCoordinate(edge_start + 0)),
			                            end_param,
			                            o0, o1, o2, unused, unused);
			start_tangent = MapCoord(o0);
			end_tangent = MapCoord(o1);
			end_coord = MapCoord(o2);
			
			auto test_x = end_point.X - end_coord.nativeX();
			auto test_y = end_point.Y - end_coord.nativeY();
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

void BooleanTool::rebuildSegmentFromPathOnly(
        const ClipperLib::IntPoint& start_point,
        const ClipperLib::IntPoint& second_point,
        const ClipperLib::IntPoint& second_last_point,
        const ClipperLib::IntPoint& end_point,
        PathObject* object)
{
	MapCoord start_point_c = MapCoord::fromNative64(start_point.X, start_point.Y);
	MapCoord second_point_c = MapCoord::fromNative64(second_point.X, second_point.Y);
	MapCoord second_last_point_c = MapCoord::fromNative64(second_last_point.X, second_last_point.Y);
	MapCoord end_point_c = MapCoord::fromNative64(end_point.X, end_point.Y);
	
	MapCoordF polygon_start_tangent = MapCoordF(second_point_c - start_point_c);
	polygon_start_tangent.normalize();
	MapCoordF polygon_end_tangent = MapCoordF(second_last_point_c - end_point_c);
	polygon_end_tangent.normalize();
	
	double tangent_length = BEZIER_HANDLE_DISTANCE * start_point_c.distanceTo(end_point_c);
	object->addCoordinate(MapCoord(MapCoordF(start_point_c) + tangent_length * polygon_start_tangent));
	object->addCoordinate(MapCoord((MapCoordF(end_point_c) + tangent_length * polygon_end_tangent)));
	object->addCoordinate(end_point_c);
}

void BooleanTool::rebuildTwoIndexSegment(
        ClipperLib::Path::size_type start_index,
        ClipperLib::Path::size_type end_index,
        bool sequence_increasing,
        const ClipperLib::Path& polygon,
        const PolyMap& polymap,
        PathObject* object)
{
	Q_UNUSED(sequence_increasing); // only used in Q_ASSERT.
	
	PathCoordInfo start_info = polymap.value(polygon.at(start_index));
	PathCoordInfo end_info = polymap.value(polygon.at(end_index));
	const PathObject* original = end_info.first->path;
	
	bool coords_increasing;
	bool is_curve;
	int coord_index;
	if (start_info.second->index == end_info.second->index)
	{
		coord_index = end_info.second->index;
		bool found = checkSegmentMatch(original, coord_index, polygon, start_index, end_index, coords_increasing, is_curve);
		if (!found)
		{
			object->getCoordinateRef(object->getCoordinateCount() - 1).setCurveStart(false);
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
				object->getCoordinateRef(object->getCoordinateCount() - 1).setCurveStart(false);
				rebuildCoordinate(end_index, polygon, polymap, object);
				return;
			}
		}
	}
	
	if (!is_curve)
		object->getCoordinateRef(object->getCoordinateCount() - 1).setCurveStart(false);
	
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

void BooleanTool::rebuildCoordinate(
        ClipperLib::Path::size_type index,
        const ClipperLib::Path& polygon,
        const PolyMap& polymap,
        PathObject* object,
        bool start_new_part)
{
	auto coord = MapCoord::fromNative64(polygon.at(index).X, polygon.at(index).Y);
	if (polymap.contains(polygon.at(index)))
	{
		PathCoordInfo info = polymap.value(polygon.at(index));
		const auto original = info.first->path->getCoordinate(info.second->index);
		
		if (original.isDashPoint())
			coord.setDashPoint(true);
	}
	object->addCoordinate(coord, start_new_part);
}

bool BooleanTool::checkSegmentMatch(
        const PathObject* original,
        int coord_index,
        const ClipperLib::Path& polygon,
        ClipperLib::Path::size_type start_index,
        ClipperLib::Path::size_type end_index,
        bool& out_coords_increasing,
        bool& out_is_curve )
{
	
	const MapCoord first = original->getCoordinate(coord_index);
	out_is_curve = first.isCurveStart();
	
	auto other_index = (coord_index + (out_is_curve ? 3 : 1)) % original->getCoordinateCount();
	const MapCoord other = original->getCoordinate(other_index);
	
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


}  // namespace OpenOrienteering
