/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2017 Kai Pastor
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

#include "object_mover.h"

#include <QtMath>
#include <QPointF>
#include <QTransform>

#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "core/symbols/symbol.h"
#include "core/symbols/text_symbol.h"


namespace OpenOrienteering {

ObjectMover::ObjectMover(Map* map, const MapCoordF& start_pos)
 : start_position(start_pos), prev_drag_x(0), prev_drag_y(0), constraints_calculated(true)
{
	Q_UNUSED(map);
}



void ObjectMover::setStartPos(const MapCoordF& start_pos)
{
	this->start_position = start_pos;
}


void ObjectMover::addObject(Object* object)
{
	objects.insert(object);
}


void ObjectMover::addPoint(PathObject* object, MapCoordVector::size_type point_index)
{
	Q_ASSERT(point_index < object->getCoordinateCount());
	
	auto index_set = insertPointObject(object);
	index_set->insert(point_index);

	constraints_calculated = false;
}


void ObjectMover::addLine(PathObject* object, MapCoordVector::size_type start_point_index)
{
	Q_ASSERT(start_point_index < object->getCoordinateCount());
	
	auto index_set = insertPointObject(object);
	index_set->insert(start_point_index);
	index_set->insert(start_point_index + 1);
	if (object->getCoordinate(start_point_index).isCurveStart())
	{
		index_set->insert(start_point_index + 2);
		index_set->insert(start_point_index + 3);
	}
	
	constraints_calculated = false;
}


void ObjectMover::addTextHandle(TextObject* text, MapCoordVector::size_type handle)
{
	text_handles.insert({text, handle});
}


void ObjectMover::move(const MapCoordF& cursor_pos, bool move_opposite_handles, qint32* out_dx, qint32* out_dy)
{
	auto delta_x = qRound(1000 * (cursor_pos.x() - start_position.x())) - prev_drag_x;
	auto delta_y = qRound(1000 * (cursor_pos.y() - start_position.y())) - prev_drag_y;
	if (out_dx)
		*out_dx = delta_x;
	if (out_dy)
		*out_dy = delta_y;
	
	move(delta_x, delta_y, move_opposite_handles);
	
	prev_drag_x += delta_x;
	prev_drag_y += delta_y;
}


void ObjectMover::move(qint32 dx, qint32 dy, bool move_opposite_handles)
{
	calculateConstraints();
	
	// Move objects
	for (auto object : objects)
		object->move(dx, dy);
	
	// Move points
	for (auto& item : points)
	{
		PathObject* path = item.first;
		for (auto index : item.second)
		{
			auto coord = path->getCoordinate(index);
			coord.setNativeX(coord.nativeX() + dx);
			coord.setNativeY(coord.nativeY() + dy);
			path->setCoordinate(index, coord);
		}
	}
	
	// Apply handle constraints
	if (move_opposite_handles)
	{
		for (auto& constraint : handle_constraints)
		{
			MapCoord anchor_point = constraint.object->getCoordinate(constraint.curve_anchor_index);
			MapCoordF to_hover_point = MapCoordF(constraint.object->getCoordinate(constraint.moved_handle_index) - anchor_point);
			to_hover_point.normalize();
			
			MapCoord control = constraint.object->getCoordinate(constraint.opposite_handle_index);
			control.setX(anchor_point.x() - constraint.opposite_handle_dist * to_hover_point.x());
			control.setY(anchor_point.y() - constraint.opposite_handle_dist * to_hover_point.y());
			constraint.object->setCoordinate(constraint.opposite_handle_index, control);
		}
	}
	
	// Move box text object handles
	for (auto& handle : text_handles)
	{
		TextObject* text_object = handle.first;
		const TextSymbol* text_symbol = text_object->getSymbol()->asText();
		
		QTransform transform;
		transform.rotate(qRadiansToDegrees(text_object->getRotation()));
		QPointF delta_point = transform.map(QPointF(dx, dy));
		
		const auto move_point = handle.second;
		int x_sign = (move_point <= 1) ? 1 : -1;
		int y_sign = (move_point >= 1 && move_point <= 2) ? 1 : -1;
		
		double new_box_width = qMax(text_symbol->getFontSize() / 2, text_object->getBoxWidth() + 0.001 * x_sign * delta_point.x());
		double new_box_height = qMax(text_symbol->getFontSize() / 2, text_object->getBoxHeight() + 0.001 * y_sign * delta_point.y());
		
		auto anchor = MapCoord { text_object->getAnchorCoordF() };
		text_object->move(dx / 2, dy / 2);
		text_object->setBox(anchor.nativeX(), anchor.nativeY(), new_box_width, new_box_height);
	}
}


ObjectMover::CoordIndexSet* ObjectMover::insertPointObject(PathObject* object)
{
	return &points.insert({object, CoordIndexSet()}).first->second;
}


void ObjectMover::calculateConstraints()
{
	if (constraints_calculated)
		return;
	
	handle_constraints.clear();
	
	// Remove all objects in the object list from the point list
	for (auto object : objects)
	{
		switch (object->getType())
		{
		case Object::Path:
			points.erase(object->asPath());
			break;
			
		case Object::Text:
			text_handles.erase(object->asText());
			break;
			
		default:
			; // nothing
		}
	}
	
	// Points
	for (auto& item : points)
	{
		const PathObject* path = item.first;
		auto& point_set = item.second;
		
		// If end points of closed paths are contained in the move set,
		// change them to the corresponding start points
		// (as these trigger moving the end points automatically and are better to handle:
		//  they are set as curve start points if a curve starts there, in contrast to the end points)
		for (const auto& part : path->parts())
		{
			if (part.isClosed() && point_set.find(part.last_index) != point_set.end())
			{
				point_set.erase(part.last_index);
				point_set.insert(part.first_index);
			}
		}
		
		// Expand set of moved points:
		// If curve points are moved, their handles must be moved, too.
		std::vector<MapCoordVector::size_type> handles;
		for (auto index : point_set)
		{
			if (path->isCurveHandle(index))
			{
				handles.push_back(index);
			}
			else
			{
				// If a curve starts here, add first handle
				if (path->getCoordinate(index).isCurveStart())
				{
					Q_ASSERT(index + 1 < path->getCoordinateCount());
					handles.push_back(index + 1);
				}
				
				// If a curve ends here, add last handle
				auto& part = *path->findPartForIndex(index);
				if (index == part.first_index && part.isClosed())
				{
					index = part.last_index;
				}
				if (index > part.first_index && path->getCoordinate(part.prevCoordIndex(index)).isCurveStart())
				{
					handles.push_back(index - 1);
				}
			}
		}
		
		// Add the handles to the list of points.
		// Determine opposite handle constraints.
		for (auto index : handles)
		{
			Q_ASSERT(path->isCurveHandle(index));
			auto& part = *path->findPartForIndex(index);
			auto end_index = part.last_index;
			
			point_set.insert(index);
			
			if (index == part.prevCoordIndex(index) + 1)
			{
				// First handle of a curve
				auto curve_anchor_index = index - 1;
				if (part.isClosed() && curve_anchor_index == part.first_index)
					curve_anchor_index = end_index;
				
				if (curve_anchor_index != part.first_index &&
				    path->getCoordinate(part.prevCoordIndex(curve_anchor_index)).isCurveStart())
				{
					OppositeHandleConstraint constraint;
					constraint.object = const_cast<PathObject*>(path);  // = item.first
					constraint.moved_handle_index = index;
					constraint.curve_anchor_index = index - 1;
					constraint.opposite_handle_index = curve_anchor_index - 1;
					constraint.opposite_handle_original_position = path->getCoordinate(constraint.opposite_handle_index);
					constraint.opposite_handle_dist = constraint.opposite_handle_original_position.distanceTo(path->getCoordinate(constraint.curve_anchor_index));
					handle_constraints.push_back(constraint);
				}
			}
			else
			{
				// Second handle of a curve
				auto curve_anchor_index = index + 1;
				if (part.isClosed() && curve_anchor_index == end_index)
					curve_anchor_index = part.first_index;
				
				if (curve_anchor_index != end_index &&
				    path->getCoordinate(curve_anchor_index).isCurveStart())
				{
					OppositeHandleConstraint constraint;
					constraint.object = const_cast<PathObject*>(path);  // = item.first
					constraint.moved_handle_index = index;
					constraint.curve_anchor_index = curve_anchor_index;
					constraint.opposite_handle_index = curve_anchor_index + 1;
					constraint.opposite_handle_original_position = path->getCoordinate(constraint.opposite_handle_index);
					constraint.opposite_handle_dist = constraint.opposite_handle_original_position.distanceTo(path->getCoordinate(constraint.curve_anchor_index));
					handle_constraints.push_back(constraint);
				}
			}
		}
	}
	
	constraints_calculated = true;
}


}  // namespace OpenOrienteering
