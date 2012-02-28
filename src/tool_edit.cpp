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


#include "tool_edit.h"

#include <algorithm>

#include <QtGui>

#include "util.h"
#include "symbol.h"
#include "object.h"
#include "map_widget.h"
#include "map_undo.h"
#include "symbol_dock_widget.h"
#include "draw_text.h"
#include "symbol_text.h"

QCursor* EditTool::cursor = NULL;

// Mac convention for selecting multiple items is the command key (In Qt the command key is ControlModifier)
#ifdef Q_WS_MAC
const Qt::KeyboardModifiers EditTool::selection_modifier = Qt::ControlModifier;
const Qt::KeyboardModifiers EditTool::control_point_modifier = Qt::ShiftModifier;
const Qt::Key EditTool::control_point_key = Qt::Key_Shift;
#else
const Qt::KeyboardModifiers EditTool::selection_modifier = Qt::ShiftModifier;
const Qt::KeyboardModifiers EditTool::control_point_modifier = Qt::ControlModifier;
const Qt::Key EditTool::control_point_key = Qt::Key_Control;
#endif

EditTool::EditTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget) : MapEditorTool(editor, Edit, tool_button), renderables(editor->getMap()), symbol_widget(symbol_widget)
{
	dragging = false;
	hover_point = -2;
	text_editor = NULL;
	
	control_pressed = false;
	space_pressed = false;

	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-hollow.png"), 1, 1);
	loadPointHandles();
}
void EditTool::init()
{
	connect(editor->getMap(), SIGNAL(selectedObjectsChanged()), this, SLOT(selectedObjectsChanged()));
	connect(symbol_widget, SIGNAL(selectedSymbolsChanged()), this, SLOT(selectedSymbolsChanged()));
	selectedObjectsChanged();
}
EditTool::~EditTool()
{
	if (text_editor)
		delete text_editor;
	deleteOldSelectionRenderables(old_renderables, false);
}

bool EditTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (!(event->buttons() & Qt::LeftButton))
		return false;
	
	dragging = false;
	box_selection = false;
	no_more_effect_on_click = false;
	click_pos = event->pos();
	click_pos_map = map_coord;
	cur_pos = event->pos();
	cur_pos_map = map_coord;
	
	updateHoverPoint(widget->mapToViewport(map_coord), widget);
	
	bool single_object_selected = editor->getMap()->getNumSelectedObjects() == 1;
	Object* single_selected_object = NULL;
	if (single_object_selected)
		single_selected_object = *editor->getMap()->selectedObjectsBegin();
	
	if (hover_point >= 0)
	{
		opposite_curve_handle_index = -1;
		
		if (single_object_selected && single_selected_object->getType() == Object::Path)
		{
			PathObject* path = reinterpret_cast<PathObject*>(single_selected_object);
			
			// Check if clicked on a bezier curve handle
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
			else if (event->modifiers() & Qt::ControlModifier)
			{
				// Clicked on a regular path point while holding Ctrl -> delete the point
				if (path->calcNumRegularPoints() <= 2 || (!(path->getSymbol()->getContainedTypes() & Symbol::Line) && path->getCoordinateCount() <= 3))
				{
					// Delete the object
					deleteSelectedObjects();
					no_more_effect_on_click = true;
					return true;
				}
				else
				{
					ReplaceObjectsUndoStep* undo_step = new ReplaceObjectsUndoStep(editor->getMap());	// TODO: use optimized undo step
					Object* undo_duplicate = path->duplicate();
					undo_duplicate->setMap(editor->getMap());
					undo_step->addObject(path, undo_duplicate);
					editor->getMap()->objectUndoManager().addNewUndoStep(undo_step);
					
					path->deleteCoordinate(hover_point, true);
					path->update(true);
					updateHoverPoint(widget->mapToViewport(map_coord), widget);
					updateDirtyRect();
					no_more_effect_on_click = true;
					return true;
				}
			}
		}
	}
	else if (hover_point == -2)
	{
		if (text_editor)
			return text_editor->mousePressEvent(event, map_coord, widget);
		
		if (hoveringOverSingleText(map_coord))
		{
			TextObject* text_object = reinterpret_cast<TextObject*>(single_selected_object);
			
			startEditing();
			
			// Don't show the original text while editing
			editor->getMap()->removeRenderablesOfObject(single_selected_object, true);
			
			// Make sure that the TextObjectEditorHelper remembers the correct standard cursor
			widget->setCursor(*getCursor());
			
			old_text = text_object->getText();
			old_horz_alignment = (int)text_object->getHorizontalAlignment();
			old_vert_alignment = (int)text_object->getVerticalAlignment();
			text_editor = new TextObjectEditorHelper(text_object, editor);
			connect(text_editor, SIGNAL(selectionChanged(bool)), this, SLOT(textSelectionChanged(bool)));
			
			// Select clicked position
			int pos = text_object->calcTextPositionAt(map_coord, false);
			text_editor->setSelection(pos, pos);
			
			updatePreviewObjects();
		}
	}
	
	if (single_object_selected && single_selected_object->getType() == Object::Path && hover_point < 0 && event->modifiers() & control_point_modifier)
	{
		// Add new point to path
		PathObject* path = reinterpret_cast<PathObject*>(single_selected_object);
		
		float distance_sq;
		PathCoord path_coord;
		path->calcClosestPointOnPath(map_coord, distance_sq, path_coord);
		
		float click_tolerance_map_sq = widget->getMapView()->pixelToLength(click_tolerance);
		click_tolerance_map_sq = click_tolerance_map_sq * click_tolerance_map_sq;
		
		if (distance_sq <= click_tolerance_map_sq)
		{
			startEditing();
			dragging = true;	// necessary to prevent second call to startEditing()
			hover_point = path->subdivide(path_coord.index, path_coord.param);
			if (space_pressed)
			{
				MapCoord point = path->getCoordinate(hover_point);
				point.setDashPoint(true);
				path->setCoordinate(hover_point, point);
			}
			opposite_curve_handle_index = -1;
			updatePreviewObjects();
		}
	}
	
	return true;
}
bool EditTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	bool mouse_down = event->buttons() & Qt::LeftButton;
	
	if (!mouse_down)
	{
		if (text_editor)
			return text_editor->mouseMoveEvent(event, map_coord, widget);
		
		updateHoverPoint(widget->mapToViewport(map_coord), widget);
		
		// For texts, decide whether to show the beam cursor
		if (hoveringOverSingleText(map_coord))
			widget->setCursor(QCursor(Qt::IBeamCursor));
		else
			widget->setCursor(*getCursor());
	}
	else // if (mouse_down)
	{
		if (no_more_effect_on_click)
			return true;
		if (text_editor && hover_point == -2)
			return text_editor->mouseMoveEvent(event, map_coord, widget);
		
		if (!dragging && (event->pos() - click_pos).manhattanLength() >= QApplication::startDragDistance())
		{
			// Start dragging
			if (hover_point >= -1)
				startEditing();
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
	
	if (no_more_effect_on_click)
	{
		no_more_effect_on_click = false;
		return true;	
	}
	if (text_editor && hover_point == -2)
	{
		if (text_editor->mouseReleaseEvent(event, map_coord, widget))
			return true;
		else
			finishEditing();
	}

	if (dragging)
	{
		// Dragging finished
		if (hover_point >= -1)
		{
			updateDragging(event->pos(), widget);
			finishEditing();
		}
		else if (box_selection)
		{
			// Do box selection
			bool selection_changed = false;
			
			std::vector<Object*> objects;
			map->findObjectsAtBox(click_pos_map, cur_pos_map, objects);
			
			if (!(event->modifiers() & selection_modifier))
			{
				if (map->getNumSelectedObjects() > 0)
					selection_changed = true;
				map->clearObjectSelection(false);
			}
			
			int size = objects.size();
			for (int i = 0; i < size; ++i)
			{
				if (!(event->modifiers() & selection_modifier))
					map->addObjectToSelection(objects[i], i == size - 1);
				else
					map->toggleObjectSelection(objects[i], i == size - 1);
				selection_changed = true;
			}
			
			box_selection = false;
			if (selection_changed)
				updateStatusText();
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
		if (!(event->modifiers() & selection_modifier) || map->getNumSelectedObjects() == 0)
		{
			if (objects.empty())
			{
				// Clicked on empty space, deselect everything
				last_results.clear();
				map->clearObjectSelection(true);
			}
			else if (!last_results.empty() && selectionInfosEqual(objects, last_results))
			{
				// If this result is the same as last time, select next object
				next_object_to_select = next_object_to_select % last_results_ordered.size();
				
				map->clearObjectSelection(false);
				map->addObjectToSelection(last_results_ordered[next_object_to_select].second, true);
				
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
			}
			else
			{
				// Toggle selection of highest priority object
				last_results = objects;
				std::sort(objects.begin(), objects.end(), sortObjects);
				last_results_ordered = objects;
				
				map->toggleObjectSelection(objects.begin()->second, true);
			}
		}
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

bool EditTool::keyPressEvent(QKeyEvent* event)
{
	if (text_editor)
		return text_editor->keyPressEvent(event);
	
	int num_selected_objects = editor->getMap()->getNumSelectedObjects();
	
	// Delete objects if the delete key is pressed, or backspace key on Mac OSX, since that is the convention for Mac apps
	#ifdef Q_WS_MAC
		Qt::Key delete_key = Qt::Key_Backspace;
	#else
		Qt::Key delete_key = Qt::Key_Delete;
	#endif

	if (num_selected_objects > 0 && event->key() == delete_key)
		deleteSelectedObjects();
	else if (event->key() == Qt::Key_Tab)
	{
		MapEditorTool* draw_tool = editor->getDefaultDrawToolForSymbol(editor->getSymbolWidget()->getSingleSelectedSymbol());
		if (draw_tool)
			editor->setTool(draw_tool);
	}
	else if (event->key() == Qt::Key_Space)
		space_pressed = true;
	else if (event->key() == control_point_key)
	{
		control_pressed = true;
		updateStatusText();
	}
	else
		return false;
	
	return true;
}
bool EditTool::keyReleaseEvent(QKeyEvent* event)
{
	if (text_editor)
		return text_editor->keyReleaseEvent(event);
	
	if (event->key() == Qt::Key_Space)
		space_pressed = false;
	else if (event->key() == control_point_key)
	{
		control_pressed = false;
		updateStatusText();
	}
	
	return false;
}
void EditTool::focusOutEvent(QFocusEvent* event)
{
	// Deactivate all modifiers - not always correct, but should be wrong only in very unusual cases and better than leaving the modifiers on forever
	control_pressed = false;
	space_pressed = false;
	updateStatusText();
}

void EditTool::draw(QPainter* painter, MapWidget* widget)
{
	int num_selected_objects = editor->getMap()->getNumSelectedObjects();
	if (num_selected_objects > 0)
	{
		editor->getMap()->drawSelection(painter, true, widget, renderables.isEmpty() ? NULL : &renderables);
		
		if (!text_editor)
		{
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
				drawPointHandles(hover_point, painter, selection, widget);
			}
		}
	}
	
	// Text editor
	if (text_editor)
	{
		painter->save();
		widget->applyMapTransform(painter);
		text_editor->draw(painter, widget);
		painter->restore();
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

void EditTool::selectedObjectsChanged()
{
	updateStatusText();
	updateDirtyRect();
	if (calculateBoxTextHandles(box_text_handles))
		updateDirtyRect();
}
void EditTool::selectedSymbolsChanged()
{
	Symbol* symbol = symbol_widget->getSingleSelectedSymbol();
	if (symbol && editor->getMap()->getNumSelectedObjects() == 0)
	{
		MapEditorTool* draw_tool = editor->getDefaultDrawToolForSymbol(symbol);
		editor->setTool(draw_tool);
	}
}
void EditTool::textSelectionChanged(bool text_change)
{
	updatePreviewObjects();
}

void EditTool::updateStatusText()
{
	QString str = tr("<b>Click</b> to select an object, <b>Drag</b> for box selection, <b>Shift</b> to toggle selection");
	if (editor->getMap()->getNumSelectedObjects() > 0)
	{
		str += tr(", <b>Del</b> to delete");
		
		if (editor->getMap()->getNumSelectedObjects() == 1)
		{
			Object* single_selected_object = *editor->getMap()->selectedObjectsBegin();
			if (single_selected_object->getType() == Object::Path)
			{
				if (control_pressed)
					str = tr("<b>Ctrl+Click</b> on point to delete it, on path to add a new point, with <b>Space</b> to make it a dash point");
				else
					str += tr("; Try <u>Ctrl</u>");
			}
		}
	}
	setStatusBarText(str);
}

void EditTool::updatePreviewObjects()
{
	updateSelectionEditPreview(renderables);
	updateDirtyRect();
}
void EditTool::updateDirtyRect()
{
	selection_extent = QRectF();
	editor->getMap()->includeSelectionRect(selection_extent);
	
	QRectF rect = selection_extent;
	bool single_object_selected = editor->getMap()->getNumSelectedObjects() == 1;
	
	// For selected paths, include the control points
	if (single_object_selected)
	{
		Object* object = *editor->getMap()->selectedObjectsBegin();
		includeControlPointRect(rect, object, box_text_handles);
	}
	
	// Text selection
	if (text_editor)
		text_editor->includeDirtyRect(rect);
	
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
	int new_hover_point = findHoverPoint(point, (editor->getMap()->getNumSelectedObjects() == 1) ? *editor->getMap()->selectedObjectsBegin() : NULL, true, box_text_handles, &selection_extent, widget);
	if (text_editor)
		new_hover_point = -2;
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
	
	Object::Type first_selected_object_type = (*map->selectedObjectsBegin())->getType();
	if (hover_point >= 0 && first_selected_object_type == Object::Text && (reinterpret_cast<TextObject*>(*map->selectedObjectsBegin())->hasSingleAnchor() == false))
	{
		// Dragging a box text object handle
		TextObject* text_object = reinterpret_cast<TextObject*>(*map->selectedObjectsBegin());
		TextSymbol* text_symbol = reinterpret_cast<TextSymbol*>(text_object->getSymbol());
		
		QTransform transform;
		transform.rotate(text_object->getRotation() * 180 / M_PI);
		QPointF delta_point = transform.map(QPointF(delta_x, delta_y));
		
		int x_sign = (hover_point <= 1) ? 1 : -1;
		int y_sign = (hover_point >= 1 && hover_point <= 2) ? 1 : -1;
		
		double new_box_width = qMax(text_symbol->getFontSize() / 2, text_object->getBoxWidth() + 0.001 * x_sign * delta_point.x());
		double new_box_height = qMax(text_symbol->getFontSize() / 2, text_object->getBoxHeight() + 0.001 * y_sign * delta_point.y());
		
		text_object->move(delta_x / 2, delta_y / 2);
		text_object->setBox(text_object->getAnchorPosition(), new_box_width, new_box_height);
		if (calculateBoxTextHandles(box_text_handles))
			updateDirtyRect();
	}
	else if (hover_point == -1 || (map->getNumSelectedObjects() == 1 &&
		(first_selected_object_type == Object::Point || first_selected_object_type == Object::Text)))
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

bool EditTool::hoveringOverSingleText(MapCoordF cursor_pos_map)
{
	if (hover_point != -2)
		return false;
	
	Map* map = editor->getMap();
	Object* single_selected_object = (map->getNumSelectedObjects() == 1) ? *map->selectedObjectsBegin() : NULL;
	if (single_selected_object && single_selected_object->getType() == Object::Text)
	{
		TextObject* text_object = reinterpret_cast<TextObject*>(single_selected_object);
		return text_object->calcTextPositionAt(cursor_pos_map, true) >= 0;
	}
	return false;
}

void EditTool::startEditing()
{
	startEditingSelection(old_renderables, &undo_duplicates);
	
	// Save original extent to be able to mark it as dirty later
	original_selection_extent = selection_extent;
}
void EditTool::finishEditing()
{
	Map* map = editor->getMap();
	bool create_undo_step = true;
	bool delete_objects = false;
	
	if (text_editor)
	{
		delete text_editor;
		text_editor = NULL;
		
		TextObject* text_object = reinterpret_cast<TextObject*>(*editor->getMap()->selectedObjectsBegin());
		if (text_object->getText().isEmpty())
		{
			text_object->setText(old_text);
			text_object->setHorizontalAlignment((TextObject::HorizontalAlignment)old_horz_alignment);
			text_object->setVerticalAlignment((TextObject::VerticalAlignment)old_vert_alignment);
			create_undo_step = false;
			delete_objects = true;
		}
		else if (text_object->getText() == old_text && (int)text_object->getHorizontalAlignment() == old_horz_alignment && (int)text_object->getVerticalAlignment() == old_vert_alignment)
			create_undo_step = false;
	}
	
	finishEditingSelection(renderables, old_renderables, create_undo_step, &undo_duplicates, delete_objects);
	
	map->setObjectAreaDirty(original_selection_extent);
	updateDirtyRect();
	map->setObjectsDirty();
	
	if (delete_objects)
		deleteSelectedObjects();
}
void EditTool::deleteSelectedObjects()
{
	AddObjectsUndoStep* undo_step = new AddObjectsUndoStep(editor->getMap());
	MapLayer* layer = editor->getMap()->getCurrentLayer();
	
	Map::ObjectSelection::const_iterator it_end = editor->getMap()->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = editor->getMap()->selectedObjectsBegin(); it != it_end; ++it)
	{
		int index = layer->findObjectIndex(*it);
		undo_step->addObject(index, *it);
	}
	for (Map::ObjectSelection::const_iterator it = editor->getMap()->selectedObjectsBegin(); it != it_end; ++it)
		editor->getMap()->deleteObject(*it, true);
	editor->getMap()->clearObjectSelection(true);
	updateStatusText();
	editor->getMap()->objectUndoManager().addNewUndoStep(undo_step);
}

#include "tool_edit.moc"
