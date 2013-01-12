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

#include "tool_edit.h"

#include <limits>

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <qmath.h>

#include "map.h"
#include "map_widget.h"
#include "map_undo.h"
#include "object.h"
#include "settings.h"
#include "symbol_dock_widget.h"
#include "tool_helpers.h"
#include "object_text.h"
#include "symbol_text.h"
#include "util.h"


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
: map(map), start_position(start_pos), prev_drag_x(0), prev_drag_y(0), constraints_calculated(true)
{
}

void ObjectMover::setStartPos(const MapCoordF& start_pos)
{
	this->start_position = start_pos;
}

void ObjectMover::addObject(Object* object)
{
	objects.insert(object);
}

void ObjectMover::addPoint(PathObject* object, int point_index)
{
	QSet< int >* index_set = insertPointObject(object);
	index_set->insert(point_index);

	constraints_calculated = false;
}

void ObjectMover::addLine(PathObject* object, int start_point_index)
{
	QSet< int >* index_set = insertPointObject(object);
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
	qint64 delta_x = qRound64(1000 * (cursor_pos.getX() - start_position.getX())) - prev_drag_x;
	qint64 delta_y = qRound64(1000 * (cursor_pos.getY() - start_position.getY())) - prev_drag_y;
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
	for (QSet< Object* >::iterator it = objects.begin(), end = objects.end(); it != end; ++it)
		(*it)->move(dx, dy);
	
	// Move points
	for (QHash< PathObject*, QSet< int > >::const_iterator it = points.constBegin(), end = points.constEnd(); it != end; ++it)
	{
		PathObject* path = it.key();
		for (QSet< int >::const_iterator pit = it.value().constBegin(), pit_end = it.value().constEnd(); pit != pit_end; ++pit)
		{
			MapCoord coord = path->getCoordinate(*pit);
			coord.setRawX(coord.rawX() + dx);
			coord.setRawY(coord.rawY() + dy);
			path->setCoordinate(*pit, coord);
		}
	}
	
	// Apply handle constraints
	for (size_t i = 0, end = handle_constraints.size(); i < end; ++i)
	{
		OppositeHandleConstraint& constraint = handle_constraints[i];
		if (!move_opposite_handles)
			constraint.object->setCoordinate(constraint.opposite_handle_index, constraint.opposite_handle_original_position);
		else
		{
			MapCoord anchor_point = constraint.object->getCoordinate(constraint.curve_anchor_index);
			MapCoordF to_hover_point = MapCoordF(constraint.object->getCoordinate(constraint.moved_handle_index) - anchor_point);
			to_hover_point.normalize();
			
			MapCoord control = constraint.object->getCoordinate(constraint.opposite_handle_index);
			control.setX(anchor_point.xd() - constraint.opposite_handle_dist * to_hover_point.getX());
			control.setY(anchor_point.yd() - constraint.opposite_handle_dist * to_hover_point.getY());
			constraint.object->setCoordinate(constraint.opposite_handle_index, control);
		}
	}
	
	// Move box text object handles
	for (QHash< TextObject*, int >::const_iterator it = text_handles.constBegin(), end = text_handles.constEnd(); it != end; ++it)
	{
		TextObject* text_object = it.key();
		TextSymbol* text_symbol = text_object->getSymbol()->asText();
		
		QTransform transform;
		transform.rotate(text_object->getRotation() * 180 / M_PI);
		QPointF delta_point = transform.map(QPointF(dx, dy));
		
		int move_point = it.value();
		int x_sign = (move_point <= 1) ? 1 : -1;
		int y_sign = (move_point >= 1 && move_point <= 2) ? 1 : -1;
		
		double new_box_width = qMax(text_symbol->getFontSize() / 2, text_object->getBoxWidth() + 0.001 * x_sign * delta_point.x());
		double new_box_height = qMax(text_symbol->getFontSize() / 2, text_object->getBoxHeight() + 0.001 * y_sign * delta_point.y());
		
		text_object->move(dx / 2, dy / 2);
		text_object->setBox(text_object->getAnchorCoordF().getIntX(), text_object->getAnchorCoordF().getIntY(), new_box_width, new_box_height);
	}
}

QSet< int >* ObjectMover::insertPointObject(PathObject* object)
{
	if (!points.contains(object))
		return &points.insert(object, QSet< int >()).value();
	else
		return &points[object];
}

void ObjectMover::calculateConstraints()
{
	if (constraints_calculated)
		return;
	handle_constraints.clear();
	
	// Remove all objects in the object list from the point list
	for (QSet< Object* >::iterator it = objects.begin(), end = objects.end(); it != end; ++it)
	{
		if ((*it)->getType() == Object::Path)
			points.remove((*it)->asPath());
		else if ((*it)->getType() == Object::Text)
			text_handles.remove((*it)->asText());
	}
	
	// Points
	for (QHash< PathObject*, QSet< int > >::iterator it = points.begin(), end = points.end(); it != end; ++it)
	{
		PathObject* path = it.key();
		QSet< int >& point_set = it.value();
		
		// If end points of closed paths are contained in the move set,
		// change them to the corresponding start points
		// (as these trigger moving the end points automatically and are better to handle:
		//  they are set as curve start points if a curve starts there, in contrast to the end points)
		for (int i = 0; i < path->getNumParts(); ++i)
		{
			PathObject::PathPart& part = path->getPart(i);
			if (part.isClosed() && point_set.contains(part.end_index))
			{
				point_set.remove(part.end_index);
				point_set.insert(part.start_index);
			}
		}
		
		// Expand set of moved points: if curve points are moved, their handles must be moved too.
		// Also find opposite handle constraints.
		for (QSet< int >::iterator pit = point_set.begin(), pit_end = point_set.end(); pit != pit_end; ++pit)
		{
			int index = *pit;
			bool could_be_handle = true;
			PathObject::PathPart& part = path->findPartForIndex(index);
			
			// If a curve starts here, add first handle
			if (path->getCoordinate(index).isCurveStart())
			{
				assert(index < path->getCoordinateCount() - 1);
				point_set.insert(index + 1);
				could_be_handle = false;
			}
			
			// If a curve ends here, add last handle
			int index_minus_3 = path->shiftedCoordIndex(index, -3, part);
			if (index_minus_3 >= 0 &&
				path->getCoordinate(index_minus_3).isCurveStart())
			{
				point_set.insert(path->shiftedCoordIndex(index, -1, part));
				could_be_handle = false;
			}
			
			// If this is a handle and the opposite handle is not moved, add constraint
			if (!could_be_handle)
				continue;
			
			int index_minus_2 = path->shiftedCoordIndex(index, -2, part);
			if (index_minus_2 >= 0 &&
				path->getCoordinate(index_minus_2).isCurveStart())
			{
				int index_plus_2 = path->shiftedCoordIndex(index, 2, part);
				if (index_plus_2 < 0)
					continue; // error
				if (!point_set.contains(index_plus_2))
				{
					int index_plus_1 = path->shiftedCoordIndex(index, 1, part);
					if (index_plus_1 < 0)
						continue; // error
					
					if (path->getCoordinate(index_plus_1).isCurveStart())
					{
						OppositeHandleConstraint constraint;
						constraint.object = path;
						constraint.moved_handle_index = index;
						constraint.curve_anchor_index = index_plus_1;
						constraint.opposite_handle_index = index_plus_2;
						constraint.opposite_handle_original_position = path->getCoordinate(constraint.opposite_handle_index);
						constraint.opposite_handle_dist = constraint.opposite_handle_original_position.lengthTo(path->getCoordinate(constraint.curve_anchor_index));
						handle_constraints.push_back(constraint);
						continue;
					}
				}
			}
			
			int index_minus_1 = path->shiftedCoordIndex(index, -1, part);
			if (index_minus_1 >= 0 &&
				!point_set.contains(index_minus_2) &&
				path->getCoordinate(index_minus_1).isCurveStart())
			{
				int index_minus_4 = path->shiftedCoordIndex(index, -4, part);
				if (index_minus_4 >= 0 &&
					path->getCoordinate(index_minus_4).isCurveStart())
				{
					OppositeHandleConstraint constraint;
					constraint.object = path;
					constraint.moved_handle_index = index;
					constraint.curve_anchor_index = index_minus_1;
					constraint.opposite_handle_index = index_minus_2;
					constraint.opposite_handle_original_position = path->getCoordinate(constraint.opposite_handle_index);
					constraint.opposite_handle_dist = constraint.opposite_handle_original_position.lengthTo(path->getCoordinate(constraint.curve_anchor_index));
					handle_constraints.push_back(constraint);
					continue;
				}
			}
		}
	}
	
	constraints_calculated = true;
}


// ### EditTool ###

// Mac convention for selecting multiple items is the command key (In Qt the command key is ControlModifier),
// for deleting it is the backspace key
#ifdef Q_OS_MAC
const Qt::KeyboardModifiers EditTool::selection_modifier = Qt::ControlModifier;
const Qt::KeyboardModifiers EditTool::control_point_modifier = Qt::ShiftModifier;
const Qt::Key EditTool::selection_key = Qt::Key_Control;
const Qt::Key EditTool::control_point_key = Qt::Key_Shift;
const Qt::Key EditTool::delete_object_key = Qt::Key_Backspace;
#else
const Qt::KeyboardModifiers EditTool::selection_modifier = Qt::ShiftModifier;
const Qt::KeyboardModifiers EditTool::control_point_modifier = Qt::ControlModifier;
const Qt::Key EditTool::selection_key = Qt::Key_Shift;
const Qt::Key EditTool::control_point_key = Qt::Key_Control;
const Qt::Key EditTool::delete_object_key = Qt::Key_Delete;
#endif

EditTool::EditTool(MapEditorController* editor, MapEditorTool::Type type, SymbolWidget* symbol_widget, QAction* tool_button)
 : MapEditorToolBase(QCursor(QPixmap(":/images/cursor-hollow.png"), 1, 1), type, editor, tool_button),
   object_selector(new ObjectSelector(map())),
   symbol_widget(symbol_widget)
{
	connect(symbol_widget, SIGNAL(selectedSymbolsChanged()), this, SLOT(selectedSymbolsChanged()));
}

EditTool::~EditTool()
{
}

void EditTool::selectedSymbolsChanged()
{
	Symbol* symbol = symbol_widget->getSingleSelectedSymbol();
	if (symbol && map()->getNumSelectedObjects() == 0 && !symbol->isHidden() && !symbol->isProtected())
	{
		switchToDefaultDrawTool(symbol);
	}
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
	map()->objectUndoManager().addNewUndoStep(undo_step);
	
	map()->setObjectsDirty();
}

bool EditTool::pointOverRectangle(QPointF point, const QRectF& rect)
{
	int click_tolerance = Settings::getInstance().getSettingCached(Settings::MapEditor_ClickTolerance).toInt();
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
	if (result.getX() < rect.left()) result.setX(rect.left());
	if (result.getY() < rect.top()) result.setY(rect.top());
	if (result.getX() > rect.right()) result.setX(rect.right());
	if (result.getY() > rect.bottom()) result.setY(rect.bottom());
	if (rect.height() > 0 && rect.width() > 0)
	{
		if ((result.getX() - rect.left()) / rect.width() > (result.getY() - rect.top()) / rect.height())
		{
			if ((result.getX() - rect.left()) / rect.width() > (rect.bottom() - result.getY()) / rect.height())
				result.setX(rect.right());
			else
				result.setY(rect.top());
		}
		else
		{
			if ((result.getX() - rect.left()) / rect.width() > (rect.bottom() - result.getY()) / rect.height())
				result.setY(rect.bottom());
			else
				result.setX(rect.left());
		}
	}
	return result;
}

void EditTool::setupAngleHelperFromSelectedObjects()
{
	const int max_num_primary_directions = 5;
	const float angle_window = (2 * M_PI) * 2 / 360.0f;
	// Amount of all path length which has to be covered by an angle
	// to be classified as "primary angle"
	const float path_angle_threshold = 1 / 5.0f;
	
	angle_helper->clearAngles();
	
	std::set< float > primary_directions;
	for (Map::ObjectSelection::const_iterator it = map()->selectedObjectsBegin(), end = map()->selectedObjectsEnd(); it != end; ++it)
	{
		Object* object = *it;
		if (object->getType() == Object::Point)
			primary_directions.insert(fmod_pos(object->asPoint()->getRotation(), M_PI / 2));
		else if (object->getType() == Object::Text)
			primary_directions.insert(fmod_pos(object->asText()->getRotation(), M_PI / 2));
		else if (object->getType() == Object::Path)
		{
			PathObject* path = object->asPath();
			// Maps angles to the path distance covered by them
			QMap< float, float > path_directions;
			
			// Collect segment directions, only looking at the first part
			PathObject::PathPart& part = path->getPart(0);
			float path_length = path->getPathCoordinateVector()[part.path_coord_end_index].clen;
			for (int c = part.start_index; c < part.end_index; ++c)
			{
				if (path->getCoordinate(c).isCurveStart())
					c += 2;
				else
				{
					MapCoordF segment = MapCoordF(path->getCoordinate(c + 1) - path->getCoordinate(c));
					float angle = fmod_pos(-1 * segment.getAngle(), M_PI / 2);
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
		
		if ((int)primary_directions.size() > max_num_primary_directions)
			break;
	}
	
	if ((int)primary_directions.size() > max_num_primary_directions ||
		primary_directions.size() == 0)
	{
		angle_helper->addDefaultAnglesDeg(0);
	}
	else
	{
		// Add base angles
		angle_helper->addAngles(0, M_PI / 2);
		
		// Add object angles
		for (std::set< float >::const_iterator it = primary_directions.begin(), end = primary_directions.end(); it != end; ++it)
			angle_helper->addAngles(*it, M_PI / 2);
	}
}

void EditTool::drawBoundingBox(QPainter* painter, MapWidget* widget, const QRectF& bounding_box, const QRgb& color)
{
	QPen pen(color);
	pen.setStyle(Qt::DashLine);
	painter->setPen(pen);
	painter->setBrush(Qt::NoBrush);
	painter->drawRect(widget->mapToViewport(bounding_box));
}

void EditTool::drawSelectionBox(QPainter* painter, MapWidget* widget, const MapCoordF& corner1, const MapCoordF& corner2)
{
	painter->setPen(active_color);
	painter->setBrush(Qt::NoBrush);
	
	QPoint point1 = widget->mapToViewport(corner1).toPoint();
	QPoint point2 = widget->mapToViewport(corner2).toPoint();
	QPoint top_left = QPoint(qMin(point1.x(), point2.x()), qMin(point1.y(), point2.y()));
	QPoint bottom_right = QPoint(qMax(point1.x(), point2.x()), qMax(point1.y(), point2.y()));
	
	painter->drawRect(QRect(top_left, bottom_right - QPoint(1, 1)));
	painter->setPen(qRgb(255, 255, 255));
	painter->drawRect(QRect(top_left + QPoint(1, 1), bottom_right - QPoint(2, 2)));
}
