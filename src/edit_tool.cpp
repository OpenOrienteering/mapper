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


#include "edit_tool.h"

#include <algorithm>

#include <QtGui>

#include "util.h"
#include "symbol.h"
#include "object.h"
#include "map_widget.h"
#include "symbol_dock_widget.h"

QCursor* EditTool::cursor = NULL;
QImage* EditTool::point_handles = NULL;

EditTool::EditTool(MapEditorController* editor, QAction* tool_button): MapEditorTool(editor, Edit, tool_button), renderables(editor->getMap())
{
	dragging = false;
	hover_point = -2;

	if (!cursor)
		cursor = new QCursor(QPixmap("images/cursor-hollow.png"), 1, 1);
	if (!point_handles)
		point_handles = new QImage("images/point-handles.png");
}
void EditTool::init()
{
	updateDirtyRect();
	updateStatusText();
}
EditTool::~EditTool()
{
	deleteOldRenderables();
}

bool EditTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (!(event->buttons() & Qt::LeftButton))
		return false;
	
	updateHoverPoint(widget->mapToViewport(map_coord), widget);
	
	if (hover_point >= 0)
	{
		opposite_curve_handle_index = -1;
		
		bool single_object_selected = editor->getMap()->getNumSelectedObjects() == 1;
		Object* single_selected_object = NULL;
		if (single_object_selected)
			single_selected_object = *editor->getMap()->selectedObjectsBegin();
		
		if (single_object_selected && single_selected_object->getType() == Object::Path)
		{
			PathObject* path = reinterpret_cast<PathObject*>(single_selected_object);
			
			int num_coords = path->getCoordinateCount();
			if (hover_point >= 1 && path->getCoordinate(hover_point - 1).isCurveStart() &&
				((hover_point >= 4 && path->getCoordinate(hover_point - 4).isCurveStart()) ||
				 (hover_point <= 3 && path->isPathClosed() && path->getCoordinate((hover_point - 4 + 2*num_coords) % num_coords).isCurveStart())))
			{
				opposite_curve_handle_index = (hover_point - 2 + num_coords) % num_coords;
				curve_anchor_index = (hover_point - 1 + num_coords) % num_coords;
			}
			else if (hover_point >= 2 && path->getCoordinate(hover_point - 2).isCurveStart() &&
				((hover_point < num_coords - 1 && path->getCoordinate(hover_point + 1).isCurveStart()) ||
				 (hover_point == num_coords - 1 && path->isPathClosed() && path->getCoordinate(0).isCurveStart())))
			{
				opposite_curve_handle_index = (hover_point + 2) % num_coords;
				curve_anchor_index = (hover_point + 1) % num_coords;
			}
		
			if (opposite_curve_handle_index >= 0)
				opposite_curve_handle_dist = path->getCoordinate(opposite_curve_handle_index).lengthTo(path->getCoordinate(curve_anchor_index));
		}
	}
	
	click_pos = event->pos();
	click_pos_map = map_coord;
	cur_pos = event->pos();
	cur_pos_map = map_coord;
	dragging = false;
	box_selection = false;
	return true;
}
bool EditTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	bool mouse_down = event->buttons() & Qt::LeftButton;
	
	if (!mouse_down)
	{
		updateHoverPoint(widget->mapToViewport(map_coord), widget);
	}
	else // if (mouse_down)
	{
		if (!dragging && (event->pos() - click_pos).manhattanLength() >= 1)
		{
			// Start dragging
			if (hover_point >= -1)
			{
				// Temporarily take the edited objects out of the map so their map renderables are not updated
				Map::ObjectSelection::const_iterator it_end = editor->getMap()->selectedObjectsEnd();
				for (Map::ObjectSelection::const_iterator it = editor->getMap()->selectedObjectsBegin(); it != it_end; ++it)
				{
					(*it)->setMap(NULL);
					
					// Cache old renderables until the object is inserted into the map again
					old_renderables.reserve(old_renderables.size() + (*it)->getNumRenderables());
					RenderableVector::const_iterator rit_end = (*it)->endRenderables();
					for (RenderableVector::const_iterator rit = (*it)->beginRenderables(); rit != rit_end; ++rit)
						old_renderables.push_back(*rit);
					(*it)->takeRenderables();
				}
				
				// Save original extent to be able to mark it as dirty later
				original_selection_extent = selection_extent;
			}
			else if (hover_point == -2)
				box_selection = true;
			
			dragging = true;
		}
		
		if (dragging)
		{
			if (hover_point >= -1)
			{
				updateDragging(event->pos(), widget);
				updatePreviewObjects();
			}
			else if (box_selection)
			{
				cur_pos = event->pos();
				cur_pos_map = map_coord;
				updateDirtyRect();
			}
		}
	}
	
	// NOTE: This must be after the rest of the processing
	cur_pos = event->pos();
	cur_pos_map = map_coord;
	
	return true;
}
bool EditTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	Map* map = editor->getMap();
	bool selection_changed = false;
	
	if (dragging)
	{
		// Dragging finished
		if (hover_point >= -1)
		{
			updateDragging(event->pos(), widget);
			
			Map::ObjectSelection::const_iterator it_end = editor->getMap()->selectedObjectsEnd();
			for (Map::ObjectSelection::const_iterator it = editor->getMap()->selectedObjectsBegin(); it != it_end; ++it)
			{
				(*it)->setMap(map);
				(*it)->update(true);
			}
			renderables.clear();
			deleteOldRenderables();
			
			map->setObjectAreaDirty(original_selection_extent);
			updateDirtyRect();
			map->setObjectsDirty();
		}
		else if (box_selection)
		{
			// Do box selection
			std::vector<Object*> objects;
			map->findObjectsAtBox(click_pos_map, cur_pos_map, objects);
			
			if (!(event->modifiers() & Qt::ShiftModifier))
			{
				if (map->getNumSelectedObjects() > 0)
					selection_changed = true;
				map->clearObjectSelection(false);
			}
			
			int size = objects.size();
			for (int i = 0; i < size; ++i)
			{
				if (!(event->modifiers() & Qt::ShiftModifier))
					map->addObjectToSelection(objects[i], i == size - 1);
				else
					map->toggleObjectSelection(objects[i], i == size - 1);
				selection_changed = true;
			}
			
			box_selection = false;
			if (!selection_changed)
				updateDirtyRect();
		}
		
		dragging = false;
	}
	else
	{
		bool single_object_selected = map->getNumSelectedObjects() == 1;
		Object* single_selected_object = NULL;
		if (single_object_selected)
			single_selected_object = *map->selectedObjectsBegin();
		
		// Clicked - get objects below cursor
		SelectionInfoVector objects;
		map->findObjectsAt(map_coord, 0.001f *widget->getMapView()->pixelToLength(MapEditorTool::click_tolerance), false, objects);
		if (objects.empty())
			map->findObjectsAt(map_coord, 0.001f * widget->getMapView()->pixelToLength(1.5f * MapEditorTool::click_tolerance), true, objects);
		
		// Selection logic, trying to select the most relevant object(s)
		if (!(event->modifiers() & Qt::ShiftModifier) || map->getNumSelectedObjects() == 0)
		{
			if (objects.empty())
			{
				// Clicked on empty space, deselect everything
				last_results.clear();
				map->clearObjectSelection(true);
				selection_changed = true;
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
	}
	
	if (selection_changed)
	{
		updateStatusText();
		updateDirtyRect();
	}
	
	return true;
}
bool EditTool::sortObjects(const std::pair<int, Object*>& a, const std::pair<int, Object*>& b)
{
	if (a.first != b.first)
		return a.first < b.first;
	
	float a_area = a.second->getExtent().width() * a.second->getExtent().height();
	float b_area = b.second->getExtent().width() * b.second->getExtent().height();
	
	return a_area < b_area;
}
bool EditTool::selectionInfosEqual(const SelectionInfoVector& a, const SelectionInfoVector& b)
{
	return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}
void EditTool::deleteOldRenderables()
{
	int size = old_renderables.size();
	for (int i = 0; i < size; ++i)
		delete old_renderables[i];
	old_renderables.clear();
}

int EditTool::findHoverPoint(QPointF cursor, Object* object, MapWidget* widget)
{
	const float click_tolerance_squared = click_tolerance * click_tolerance;
	
	// Check object
	if (object)
	{
		if (object->getType() == Object::Point)
		{
			PointObject* point = reinterpret_cast<PointObject*>(object);
			if (distanceSquared(widget->mapToViewport(point->getPosition()), cursor) <= click_tolerance_squared)
				return 0;
		}
		else if (object->getType() == Object::Path)
		{
			PathObject* path = reinterpret_cast<PathObject*>(object);
			int size = path->getCoordinateCount();
			
			for (int i = 0; i < size; ++i)
			{
				if (distanceSquared(widget->mapToViewport(path->getCoordinate(i)), cursor) <= click_tolerance_squared)
					return i;
			}
		}
	}
	
	// Check bounding box
	QRectF selection_extent_viewport = widget->mapToViewport(selection_extent);
	if (cursor.x() < selection_extent_viewport.left() - click_tolerance) return -2;
	if (cursor.y() < selection_extent_viewport.top() - click_tolerance) return -2;
	if (cursor.x() > selection_extent_viewport.right() + click_tolerance) return -2;
	if (cursor.y() > selection_extent_viewport.bottom() + click_tolerance) return -2;
	if (cursor.x() > selection_extent_viewport.left() + click_tolerance &&
		cursor.y() > selection_extent_viewport.top() + click_tolerance &&
		cursor.x() < selection_extent_viewport.right() - click_tolerance &&
		cursor.y() < selection_extent_viewport.bottom() - click_tolerance) return -2;
	return -1;
}
void EditTool::drawPointHandles(QPainter* painter, Object* object, MapWidget* widget)
{
	if (object->getType() == Object::Point)
	{
		PointObject* point = reinterpret_cast<PointObject*>(object);
		drawPointHandle(painter, widget->mapToViewport(point->getPosition()), NormalHandle, hover_point == 0);
	}
	else if (object->getType() == Object::Path)
	{
		PathObject* path = reinterpret_cast<PathObject*>(object);
		int size = path->getCoordinateCount();
		
		bool have_curve = path->isPathClosed() && size >= 3 && path->getCoordinate(size - 3).isCurveStart();
		painter->setBrush(Qt::NoBrush);
		
		for (int i = 0; i < size; ++i)
		{
			MapCoord coord = path->getCoordinate(i);
			QPointF point = widget->mapToViewport(coord);
			
			if (have_curve)
			{
				int curve_index = (i == 0) ? (size - 1) : (i - 1);
				QPointF curve_handle = widget->mapToViewport(path->getCoordinate(curve_index));
				drawCurveHandleLine(painter, point, curve_handle, hover_point == i);
				drawPointHandle(painter, curve_handle, CurveHandle, hover_point == i || hover_point == curve_index);
				have_curve = false;
			}
			
			/*if ((i == 0 && !path->isPathClosed()) || (i >= 1 && path->getCoordinate(i-1).isHolePoint()))
				drawPointHandle(painter, point, StartHandle, widget);
			else if ((i == size - 1 && !path->isPathClosed()) || coord.isHolePoint())
				drawPointHandle(painter, point, EndHandle, widget);
			else*/
				drawPointHandle(painter, point, coord.isDashPoint() ? DashHandle : NormalHandle, hover_point == i);
			
			if (coord.isCurveStart())
			{
				QPointF curve_handle = widget->mapToViewport(path->getCoordinate(i+1));
				drawCurveHandleLine(painter, point, curve_handle, hover_point == i);
				drawPointHandle(painter, curve_handle, CurveHandle, hover_point == i || hover_point == i + 1);
				i += 2;
				have_curve = true;
			}
		}
	}
	else
		assert(false);
}
void EditTool::drawPointHandle(QPainter* painter, QPointF point, EditTool::PointHandleType type, bool active)
{
	painter->drawImage(qRound(point.x()) - 5, qRound(point.y()) - 5, *point_handles, (int)type * 11, active ? 11 : 0, 11, 11);
}
void EditTool::drawCurveHandleLine(QPainter* painter, QPointF point, QPointF curve_handle, bool active)
{
	const float handle_radius = 3;
	painter->setPen(active ? active_color : inactive_color);
	
	QPointF to_handle = curve_handle - point;
	float to_handle_len = to_handle.x()*to_handle.x() + to_handle.y()*to_handle.y();
	if (to_handle_len > 0.00001f)
	{
		to_handle_len = sqrt(to_handle_len);
		QPointF change = to_handle * (handle_radius / to_handle_len);
		
		point += change;
		curve_handle -= change;
	}
	
	painter->drawLine(point, curve_handle);
}

bool EditTool::keyPressEvent(QKeyEvent* event)
{
	int num_selected_objects = editor->getMap()->getNumSelectedObjects();
	
	if (num_selected_objects > 0 && event->key() == Qt::Key_Delete)
	{
		Map::ObjectSelection::const_iterator it_end = editor->getMap()->selectedObjectsEnd();
		for (Map::ObjectSelection::const_iterator it = editor->getMap()->selectedObjectsBegin(); it != it_end; ++it)
			editor->getMap()->deleteObject(*it, false);
		editor->getMap()->clearObjectSelection(true);
		updateStatusText();
	}
	else if (event->key() == Qt::Key_Tab)
	{
		MapEditorTool* draw_tool = editor->getDefaultDrawToolForSymbol(editor->getSymbolWidget()->getSingleSelectedSymbol());
		if (draw_tool)
			editor->setTool(draw_tool);
	}
	else
		return false;
	
	return true;
}
bool EditTool::keyReleaseEvent(QKeyEvent* event)
{
	// Nothing (yet?)
	return false;
}

void EditTool::draw(QPainter* painter, MapWidget* widget)
{
	int num_selected_objects = editor->getMap()->getNumSelectedObjects();
	if (num_selected_objects > 0)
	{
		editor->getMap()->drawSelection(painter, true, widget, renderables.isEmpty() ? NULL : &renderables);
		if (selection_extent.isValid())
		{
			QPen pen((hover_point == -1) ? active_color : selection_color);
			pen.setStyle(Qt::DashLine);
			painter->setPen(pen);
			painter->setBrush(Qt::NoBrush);
			painter->drawRect(widget->mapToViewport(selection_extent));
		}
		
		if (num_selected_objects == 1)
		{
			Object* selection = *editor->getMap()->selectedObjectsBegin();
			drawPointHandles(painter, selection, widget);
		}
	}
	
	// Box selection
	if (dragging && box_selection)
	{
		painter->setPen(active_color);
		painter->setBrush(Qt::NoBrush);
		
		QPoint point1 = widget->mapToViewport(click_pos_map).toPoint();
		QPoint point2 = widget->mapToViewport(cur_pos_map).toPoint();
		QPoint top_left = QPoint(qMin(point1.x(), point2.x()), qMin(point1.y(), point2.y()));
		QPoint bottom_right = QPoint(qMax(point1.x(), point2.x()), qMax(point1.y(), point2.y()));
		
		painter->drawRect(QRect(top_left, bottom_right - QPoint(1, 1)));
		painter->setPen(qRgb(255, 255, 255));
		painter->drawRect(QRect(top_left + QPoint(1, 1), bottom_right - QPoint(2, 2)));
	}
}

void EditTool::updateStatusText()
{
	QString str = tr("<b>Click</b> to select an object, <b>Drag</b> for box selection, <b>Shift</b> to toggle selection");
	if (editor->getMap()->getNumSelectedObjects() > 0)
		str += tr(", <b>Del</b> to delete");
	setStatusBarText(str);
}

void EditTool::updatePreviewObjects()
{
	Map::ObjectSelection::const_iterator it_end = editor->getMap()->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = editor->getMap()->selectedObjectsBegin(); it != it_end; ++it)
	{
		renderables.removeRenderablesOfObject(*it, false);
		(*it)->update(true);
		renderables.insertRenderablesOfObject(*it);
	}
	
	updateDirtyRect();
}

void EditTool::updateDirtyRect()
{
	selection_extent = QRectF();
	editor->getMap()->includeSelectionRect(selection_extent);
	
	QRectF rect = selection_extent;
	bool single_object_selected = editor->getMap()->getNumSelectedObjects() == 1;
	
	// For selected paths, include the control points
	Object* object = *editor->getMap()->selectedObjectsBegin();
	if (single_object_selected && object->getType() == Object::Path)
	{
		PathObject* path = reinterpret_cast<PathObject*>(object);
		int size = path->getCoordinateCount();
		for (int i = 0; i < size; ++i)
			rectInclude(rect, path->getCoordinate(i).toQPointF());
	}
	
	// Box selection
	if (dragging && box_selection)
	{
		rectIncludeSafe(rect, click_pos_map.toQPointF());
		rectIncludeSafe(rect, cur_pos_map.toQPointF());
	}
	
	if (rect.isValid())
		editor->getMap()->setDrawingBoundingBox(rect, single_object_selected ? 6 : 1, true);
	else
		editor->getMap()->clearDrawingBoundingBox();
}

void EditTool::updateHoverPoint(QPointF point, MapWidget* widget)
{
	int new_hover_point = findHoverPoint(point, (editor->getMap()->getNumSelectedObjects() == 1) ? *editor->getMap()->selectedObjectsBegin() : NULL, widget);
	if (new_hover_point != hover_point)
	{
		updateDirtyRect();
		hover_point = new_hover_point;
	}
}
void EditTool::updateDragging(QPoint cursor_pos, MapWidget* widget)
{
	Map* map = editor->getMap();
	
	qint64 prev_drag_x = widget->getMapView()->pixelToLength(cur_pos.x() - click_pos.x());
	qint64 prev_drag_y = widget->getMapView()->pixelToLength(cur_pos.y() - click_pos.y());
	
	qint64 delta_x = widget->getMapView()->pixelToLength(cursor_pos.x() - click_pos.x()) - prev_drag_x;
	qint64 delta_y = widget->getMapView()->pixelToLength(cursor_pos.y() - click_pos.y()) - prev_drag_y;
	
	if (hover_point == -1 || (map->getNumSelectedObjects() == 1 && (*map->selectedObjectsBegin())->getType() == Object::Point))
	{
		Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
		for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
			(*it)->move(delta_x, delta_y);
	}
	else
	{
		assert(map->getNumSelectedObjects() == 1);
		Object* object = *map->selectedObjectsBegin();
		assert(object->getType() == Object::Path);
		PathObject* path = reinterpret_cast<PathObject*>(object);
		assert(hover_point >= 0 && hover_point < path->getCoordinateCount());
		
		MapCoord coord = path->getCoordinate(hover_point);
		coord.setRawX(coord.rawX() + delta_x);
		coord.setRawY(coord.rawY() + delta_y);
		path->setCoordinate(hover_point, coord);
		
		// Move curve handles with the point they extend from
		if (coord.isCurveStart())
		{
			assert(hover_point < path->getCoordinateCount() - 1);
			MapCoord control = path->getCoordinate(hover_point + 1);
			control.setRawX(control.rawX() + delta_x);
			control.setRawY(control.rawY() + delta_y);
			path->setCoordinate(hover_point + 1, control);
		}
		if ((hover_point >= 3 && path->getCoordinate(hover_point - 3).isCurveStart()) ||
			(hover_point == 0 && path->isPathClosed() && path->getCoordinate(path->getCoordinateCount() - 3).isCurveStart()))
		{
			int index = (hover_point == 0) ? (path->getCoordinateCount() - 1) : (hover_point - 1);
			MapCoord control = path->getCoordinate(index);
			control.setRawX(control.rawX() + delta_x);
			control.setRawY(control.rawY() + delta_y);
			path->setCoordinate(index, control);
		}
		
		// Move opposite curve handles
		if (opposite_curve_handle_index >= 0)
		{
			MapCoord anchor_point = path->getCoordinate(curve_anchor_index);
			MapCoordF to_hover_point = MapCoordF(coord.xd() - anchor_point.xd(), coord.yd() - anchor_point.yd());
			to_hover_point.normalize();
			
			MapCoord control = path->getCoordinate(opposite_curve_handle_index);
			control.setX(anchor_point.xd() - opposite_curve_handle_dist * to_hover_point.getX());
			control.setY(anchor_point.yd() - opposite_curve_handle_dist * to_hover_point.getY());
			path->setCoordinate(opposite_curve_handle_index, control);
		}
	}
}

#include "edit_tool.moc"
