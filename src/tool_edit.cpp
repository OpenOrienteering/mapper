/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2015 Kai Pastor
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

#include "tool_edit.h"

#include <limits>

#include <qmath.h>
#include <QPainter>

#include "map.h"
#include "map_widget.h"
#include "object_undo.h"
#include "object.h"
#include "settings.h"
#include "tool_helpers.h"
#include "object_text.h"
#include "symbol_text.h"
#include "util.h"

class SymbolWidget;


// ### ObjectSelector ###

ObjectSelector::ObjectSelector(Map* map)
 : map(map)
{
}

bool ObjectSelector::selectAt(MapCoordF position, double tolerance, bool toggle)
{
	bool selection_changed;
	
	bool single_object_selected = map->getNumSelectedObjects() == 1;
	Object* single_selected_object = NULL;
	if (single_object_selected)
		single_selected_object = *map->selectedObjectsBegin();
	
	// Clicked - get objects below cursor
	SelectionInfoVector objects;
	map->findObjectsAt(position, 0.001f * tolerance, false, false, false, false, objects);
	if (objects.empty())
		map->findObjectsAt(position, 0.001f * 1.5f * tolerance, false, true, false, false, objects);
	
	// Selection logic, trying to select the most relevant object(s)
	if (!toggle || map->getNumSelectedObjects() == 0)
	{
		if (objects.empty())
		{
			// Clicked on empty space, deselect everything
			selection_changed = map->getNumSelectedObjects() > 0;
			last_results.clear();
			map->clearObjectSelection(true);
		}
		else if (!last_results.empty() && selectionInfosEqual(objects, last_results))
		{
			// If this result is the same as last time, select next object
			next_object_to_select = next_object_to_select % last_results_ordered.size();
			
			map->clearObjectSelection(false);
			map->addObjectToSelection(last_results_ordered[next_object_to_select].second, true);
			selection_changed = true;
			
			++next_object_to_select;
		}
		else
		{
			// Results different - select object with highest priority, if it is not the same as before
			last_results = objects;
			std::sort(objects.begin(), objects.end(), sortObjects);
			last_results_ordered = objects;
			next_object_to_select = 1;
			
			map->clearObjectSelection(false);
			if (single_selected_object == objects.begin()->second)
			{
				next_object_to_select = next_object_to_select % last_results_ordered.size();
				map->addObjectToSelection(objects[next_object_to_select].second, true);
				++next_object_to_select;
			}
			else
				map->addObjectToSelection(objects.begin()->second, true);
			
			selection_changed = true;
		}
	}
	else
	{
		// Shift held and something is already selected
		if (objects.empty())
		{
			// do nothing
			selection_changed = false;
		}
		else if (!last_results.empty() && selectionInfosEqual(objects, last_results))
		{
			// Toggle last selected object - must work the same way, regardless if other objects are selected or not
			next_object_to_select = next_object_to_select % last_results_ordered.size();
			
			if (map->toggleObjectSelection(last_results_ordered[next_object_to_select].second, true) == false)
				++next_object_to_select;	// only advance if object has been deselected
			selection_changed = true;
		}
		else
		{
			// Toggle selection of highest priority object
			last_results = objects;
			std::sort(objects.begin(), objects.end(), sortObjects);
			last_results_ordered = objects;
			
			map->toggleObjectSelection(objects.begin()->second, true);
			selection_changed = true;
		}
	}
	
	return selection_changed;
}

bool ObjectSelector::selectBox(MapCoordF corner1, MapCoordF corner2, bool toggle)
{
	bool selection_changed = false;
	
	std::vector<Object*> objects;
	map->findObjectsAtBox(corner1, corner2, false, false, objects);
	
	if (!toggle)
	{
		if (map->getNumSelectedObjects() > 0)
			selection_changed = true;
		map->clearObjectSelection(false);
	}
	
	int size = objects.size();
	for (int i = 0; i < size; ++i)
	{
		if (toggle)
			map->toggleObjectSelection(objects[i], i == size - 1);
		else
			map->addObjectToSelection(objects[i], i == size - 1);
			
		selection_changed = true;
	}
	
	return selection_changed;
}

bool ObjectSelector::sortObjects(const std::pair< int, Object* >& a, const std::pair< int, Object* >& b)
{
	if (a.first != b.first)
		return a.first < b.first;
	
	float a_area = a.second->getExtent().width() * a.second->getExtent().height();
	float b_area = b.second->getExtent().width() * b.second->getExtent().height();
	
	return a_area < b_area;
}

bool ObjectSelector::selectionInfosEqual(const SelectionInfoVector& a, const SelectionInfoVector& b)
{
	return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}


// ### ObjectMover ###

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

void ObjectMover::addTextHandle(TextObject* text, int handle)
{
	text_handles.insert(text, handle);
}

void ObjectMover::move(const MapCoordF& cursor_pos, bool move_opposite_handles, qint64* out_dx, qint64* out_dy)
{
	qint64 delta_x = qRound64(1000 * (cursor_pos.x() - start_position.x())) - prev_drag_x;
	qint64 delta_y = qRound64(1000 * (cursor_pos.y() - start_position.y())) - prev_drag_y;
	if (out_dx)
		*out_dx = delta_x;
	if (out_dy)
		*out_dy = delta_y;
	
	move(delta_x, delta_y, move_opposite_handles);
	
	prev_drag_x += delta_x;
	prev_drag_y += delta_y;
}

void ObjectMover::move(qint64 dx, qint64 dy, bool move_opposite_handles)
{
	calculateConstraints();
	
	// Move objects
	for (auto object : objects)
		object->move(dx, dy);
	
	// Move points
	for (auto it = points.constBegin(), end = points.constEnd(); it != end; ++it)
	{
		PathObject* path = it.key();
		for (auto pit = it.value().constBegin(), pit_end = it.value().constEnd(); pit != pit_end; ++pit)
		{
			MapCoord coord = path->getCoordinate(*pit);
			coord.setRawX(coord.rawX() + dx);
			coord.setRawY(coord.rawY() + dy);
			path->setCoordinate(*pit, coord);
		}
	}
	
	// Apply handle constraints
	for (auto& constraint : handle_constraints)
	{
		if (!move_opposite_handles)
		{
			constraint.object->setCoordinate(constraint.opposite_handle_index, constraint.opposite_handle_original_position);
		}
		else
		{
			MapCoord anchor_point = constraint.object->getCoordinate(constraint.curve_anchor_index);
			MapCoordF to_hover_point = MapCoordF(constraint.object->getCoordinate(constraint.moved_handle_index) - anchor_point);
			to_hover_point.normalize();
			
			MapCoord control = constraint.object->getCoordinate(constraint.opposite_handle_index);
			control.setX(anchor_point.xd() - constraint.opposite_handle_dist * to_hover_point.x());
			control.setY(anchor_point.yd() - constraint.opposite_handle_dist * to_hover_point.y());
			constraint.object->setCoordinate(constraint.opposite_handle_index, control);
		}
	}
	
	// Move box text object handles
	for (auto it = text_handles.constBegin(), end = text_handles.constEnd(); it != end; ++it)
	{
		TextObject* text_object = it.key();
		const TextSymbol* text_symbol = text_object->getSymbol()->asText();
		
		QTransform transform;
		transform.rotate(text_object->getRotation() * 180 / M_PI);
		QPointF delta_point = transform.map(QPointF(dx, dy));
		
		int move_point = it.value();
		int x_sign = (move_point <= 1) ? 1 : -1;
		int y_sign = (move_point >= 1 && move_point <= 2) ? 1 : -1;
		
		double new_box_width = qMax(text_symbol->getFontSize() / 2, text_object->getBoxWidth() + 0.001 * x_sign * delta_point.x());
		double new_box_height = qMax(text_symbol->getFontSize() / 2, text_object->getBoxHeight() + 0.001 * y_sign * delta_point.y());
		
		auto anchor = MapCoord { text_object->getAnchorCoordF() };
		text_object->move(dx / 2, dy / 2);
		text_object->setBox(anchor.rawX(), anchor.rawY(), new_box_width, new_box_height);
	}
}

ObjectMover::CoordIndexSet* ObjectMover::insertPointObject(PathObject* object)
{
	if (!points.contains(object))
		return &points.insert(object, CoordIndexSet()).value();
	else
		return &points[object];
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
			points.remove(object->asPath());
			break;
			
		case Object::Text:
			text_handles.remove(object->asText());
			break;
			
		default:
			; // nothing
		}
	}
	
	// Points
	for (QHash< PathObject*, CoordIndexSet>::iterator it = points.begin(), end = points.end(); it != end; ++it)
	{
		PathObject* path = it.key();
		auto& point_set = it.value();
		
		// If end points of closed paths are contained in the move set,
		// change them to the corresponding start points
		// (as these trigger moving the end points automatically and are better to handle:
		//  they are set as curve start points if a curve starts there, in contrast to the end points)
		for (const auto& part : path->parts())
		{
			if (part.isClosed() && point_set.contains(part.last_index))
			{
				point_set.remove(part.last_index);
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
				if (index != part.first_index &&
				    path->getCoordinate(part.prevCoordIndex(index)).isCurveStart())
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
					constraint.object = path;
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
					constraint.object = path;
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


// ### EditTool ###

#ifdef Q_OS_MAC
const Qt::Key EditTool::delete_object_key = Qt::Key_Backspace;
#else
const Qt::Key EditTool::delete_object_key = Qt::Key_Delete;
#endif

EditTool::EditTool(MapEditorController* editor, MapEditorTool::Type type, QAction* tool_button)
 : MapEditorToolBase { QCursor(QPixmap(":/images/cursor-hollow.png"), 1, 1), type, editor, tool_button }
 , object_selector { new ObjectSelector(map()) }
{
	; // nothing
}

EditTool::~EditTool()
{
	; // nothing
}

void EditTool::deleteSelectedObjects()
{
	map()->deleteSelectedObjects();
	updateStatusText();
}

void EditTool::createReplaceUndoStep(Object* object)
{
	ReplaceObjectsUndoStep* undo_step = new ReplaceObjectsUndoStep(map());
	Object* undo_duplicate = object->duplicate();
	undo_duplicate->setMap(map());
	undo_step->addObject(object, undo_duplicate);
	map()->push(undo_step);
	
	map()->setObjectsDirty();
}

bool EditTool::pointOverRectangle(QPointF point, const QRectF& rect) const
{
	cur_map_widget->getMapView();
	float click_tolerance = clickTolerance();
	if (point.x() < rect.left() - click_tolerance) return false;
	if (point.y() < rect.top() - click_tolerance) return false;
	if (point.x() > rect.right() + click_tolerance) return false;
	if (point.y() > rect.bottom() + click_tolerance) return false;
	if (point.x() > rect.left() + click_tolerance &&
		point.y() > rect.top() + click_tolerance &&
		point.x() < rect.right() - click_tolerance &&
		point.y() < rect.bottom() - click_tolerance) return false;
	return true;
}

MapCoordF EditTool::closestPointOnRect(MapCoordF point, const QRectF& rect)
{
	MapCoordF result = point;
	if (result.x() < rect.left()) result.setX(rect.left());
	if (result.y() < rect.top()) result.setY(rect.top());
	if (result.x() > rect.right()) result.setX(rect.right());
	if (result.y() > rect.bottom()) result.setY(rect.bottom());
	if (rect.height() > 0 && rect.width() > 0)
	{
		if ((result.x() - rect.left()) / rect.width() > (result.y() - rect.top()) / rect.height())
		{
			if ((result.x() - rect.left()) / rect.width() > (rect.bottom() - result.y()) / rect.height())
				result.setX(rect.right());
			else
				result.setY(rect.top());
		}
		else
		{
			if ((result.x() - rect.left()) / rect.width() > (rect.bottom() - result.y()) / rect.height())
				result.setY(rect.bottom());
			else
				result.setX(rect.left());
		}
	}
	return result;
}

void EditTool::setupAngleHelperFromSelectedObjects()
{
	const std::size_t max_num_primary_directions = 5;
	const float angle_window = (2 * M_PI) * 2 / 360.0f;
	// Amount of all path length which has to be covered by an angle
	// to be classified as "primary angle"
	const float path_angle_threshold = 1 / 5.0f;
	
	angle_helper->clearAngles();
	
	std::set< float > primary_directions;
	for (const auto object : map()->selectedObjects())
	{
		if (object->getType() == Object::Point)
		{
			primary_directions.insert(fmod_pos(object->asPoint()->getRotation(), M_PI / 2));
		}
		else if (object->getType() == Object::Text)
		{
			primary_directions.insert(fmod_pos(object->asText()->getRotation(), M_PI / 2));
		}
		else if (object->getType() == Object::Path)
		{
			auto path = object->asPath();
			// Maps angles to the path distance covered by them
			QMap< float, float > path_directions;
			
			// Collect segment directions, only looking at the first part
			auto& part = path->parts().front();
			float path_length = part.path_coords.back().clen;
			for (auto c = part.first_index; c < part.last_index; c = part.nextCoordIndex(c))
			{
				if (!path->getCoordinate(c).isCurveStart())
				{
					MapCoordF segment = MapCoordF(path->getCoordinate(c + 1) - path->getCoordinate(c));
					float angle = fmod_pos(-1 * segment.angle(), M_PI / 2);
					float length = segment.length();
					
					QMap< float, float >::iterator angle_it = path_directions.find(angle);
					if (angle_it != path_directions.end())
						angle_it.value() += length;
					else
						path_directions.insert(angle, length);
				}
			}
			
			// Determine primary directions by moving a window over the collected angles
			// and determining maxima.
			// The iterators are the next angle which crosses the respective window border.
			float angle_start = -1 * angle_window;
			QMap< float, float >::const_iterator start_it = path_directions.constBegin();
			float angle_end = 0;
			QMap< float, float >::const_iterator end_it = path_directions.constBegin();
			
			bool length_increasing = true;
			float cur_length = 0;
			while (start_it != path_directions.constEnd())
			{
				float start_dist = start_it.key() - angle_start;
				float end_dist = (end_it == path_directions.constEnd()) ?
					std::numeric_limits<float>::max() :
					(end_it.key() - angle_end);
				if (start_dist > end_dist)
				{
					// A new angle enters the window (at the end)
					cur_length += end_it.value();
					length_increasing = true;
					++end_it;
					
					angle_start += end_dist;
					angle_end += end_dist;
				}
				else // if (start_dist <= end_dist)
				{
					// An angle leaves the window (at the start)
					// Check if we had a significant maximum.
					if (length_increasing &&
						cur_length / path_length >= path_angle_threshold)
					{
						// Find the average angle and inset it
						float angle = 0;
						float total_weight = 0;
						for (QMap< float, float >::const_iterator angle_it = start_it; angle_it != end_it; ++angle_it)
						{
							angle += angle_it.key() * angle_it.value();
							total_weight += angle_it.value();
						}
						primary_directions.insert(angle / total_weight);
					}
					
					cur_length -= start_it.value();
					length_increasing = false;
					++start_it;
					
					angle_start += start_dist;
					angle_end += start_dist;
				}
			}
		}
		
		if (primary_directions.size() > max_num_primary_directions)
			break;
	}
	
	if (primary_directions.size() > max_num_primary_directions ||
		primary_directions.size() == 0)
	{
		angle_helper->addDefaultAnglesDeg(0);
	}
	else
	{
		// Add base angles
		angle_helper->addAngles(0, M_PI / 2);
		
		// Add object angles
		for (auto angle : primary_directions)
			angle_helper->addAngles(angle, M_PI / 2);
	}
}

void EditTool::drawBoundingBox(QPainter* painter, MapWidget* widget, const QRectF& bounding_box, const QRgb& color)
{
	QPen pen(color);
	pen.setStyle(Qt::DashLine);
	if (scaleFactor() > 1)
		pen.setWidth(scaleFactor());
	painter->setPen(pen);
	painter->setBrush(Qt::NoBrush);
	painter->drawRect(widget->mapToViewport(bounding_box));
}

void EditTool::drawBoundingPath(QPainter *painter, MapWidget *widget, const std::vector<QPointF>& bounding_path, const QRgb &color)
{
	Q_ASSERT(bounding_path.size() > 0);
	
	QPen pen(color);
	pen.setStyle(Qt::DashLine);
	if (scaleFactor() > 1)
		pen.setWidth(scaleFactor());
	painter->setPen(pen);
	painter->setBrush(Qt::NoBrush);
	
	QPainterPath painter_path;
	painter_path.moveTo(widget->mapToViewport(bounding_path[0]));
	for (std::size_t i = 1; i < bounding_path.size(); ++i)
		painter_path.lineTo(widget->mapToViewport(bounding_path[i]));
	painter_path.closeSubpath();
	painter->drawPath(painter_path);
}
