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


#include "tool_draw_path.h"

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "map.h"
#include "map_undo.h"
#include "map_widget.h"
#include "object.h"
#include "renderable.h"
#include "settings.h"
#include "symbol.h"
#include "symbol_line.h"
#include "symbol_dock_widget.h"
#include "tool_helpers.h"
#include "util.h"
#include "gui/modifier_key.h"

QCursor* DrawPathTool::cursor = NULL;

DrawPathTool::DrawPathTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget, bool allow_closing_paths)
 : DrawLineAndAreaTool(editor, DrawPath, tool_button, symbol_widget), allow_closing_paths(allow_closing_paths),
   finished_path_is_selected(false),
   cur_map_widget(mapWidget()),
   angle_helper(new ConstrainAngleToolHelper()),
   snap_helper(new SnappingToolHelper(map())),
   follow_helper(new FollowPathToolHelper())
{
	angle_helper->setActive(false);
	connect(angle_helper.data(), SIGNAL(displayChanged()), this, SLOT(updateDirtyRect()));
	
	updateSnapHelper();
	connect(snap_helper.data(), SIGNAL(displayChanged()), this, SLOT(updateDirtyRect()));
	
	dragging = false;
	appending = false;
	following = false;
	picking_angle = false;
	picked_angle = false;
	draw_dash_points = false;
	shift_pressed = false;
	ctrl_pressed = false;
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-draw-path.png"), 11, 11);
	
	connect(map(), SIGNAL(objectSelectionChanged()), this, SLOT(objectSelectionChanged()));
}

DrawPathTool::~DrawPathTool()
{
	// Nothing, not inlined
}

void DrawPathTool::init()
{
	updateDashPointDrawing();
	updateStatusText();
}

bool DrawPathTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	cur_map_widget = widget;
	created_point_at_last_mouse_press = false;
	
	if (draw_in_progress &&
		((event->button() == Qt::RightButton) &&
		!Settings::getInstance().getSettingCached(Settings::MapEditor_DrawLastPointOnRightClick).toBool()))
	{
		finishDrawing();
		return true;
	}
	else if (draw_in_progress &&
		((event->button() == Qt::RightButton && event->buttons() & Qt::LeftButton) ||
		 (event->button() == Qt::LeftButton && event->buttons() & Qt::RightButton)))
	{
		if (!previous_point_is_curve_point)
			undoLastPoint();
		if (draw_in_progress)
			finishDrawing();
		return true;
	}
	else if ((event->button() == Qt::LeftButton) || (draw_in_progress && drawMouseButtonClicked(event)))
	{
		dragging = false;
		bool start_appending = false;
		if (shift_pressed)
		{
			SnappingToolHelperSnapInfo snap_info;
			MapCoord snap_coord = snap_helper->snapToObject(map_coord, widget, &snap_info);
			click_pos_map = MapCoordF(snap_coord);
			cur_pos_map = click_pos_map;
			click_pos = widget->mapToViewport(click_pos_map).toPoint();
			
			// Check for following and appending
			if (!is_helper_tool)
			{
				if (!draw_in_progress)
				{
					if (snap_info.type == SnappingToolHelper::ObjectCorners &&
						(snap_info.coord_index == 0 || snap_info.coord_index == snap_info.object->asPath()->getCoordinateCount() - 1) &&
						snap_info.object->getSymbol() == symbol_widget->getSingleSelectedSymbol())
					{
						// Appending to another path
						start_appending = true;
						startAppending(snap_info);
					}
					
					// Setup angle helper
					if (snap_helper->snapToDirection(map_coord, widget, angle_helper.data()))
						picked_angle = true;
				}
				else if (draw_in_progress &&
						 (snap_info.type == SnappingToolHelper::ObjectCorners || snap_info.type == SnappingToolHelper::ObjectPaths) &&
						 snap_info.object->getType() == Object::Path)
				{
					// Start following another path
					startFollowing(snap_info, snap_coord);
					return true;
				}
			}
		}
		else if (!draw_in_progress && ctrl_pressed)
		{
			// Start picking direction of an existing object
			picking_angle = true;
			pickAngle(map_coord, widget);
			return true;
		}
		else
		{
			click_pos = event->pos();
			click_pos_map = map_coord;
			cur_pos_map = map_coord;
		}
		
		if (!draw_in_progress)
		{
			// Start a new path
			startDrawing();
			angle_helper->setCenter(click_pos_map);
			updateSnapHelper();
			
			path_has_preview_point = false;
			previous_point_is_curve_point = false;
			appending = start_appending;
		}
		else
		{
			if (!shift_pressed)
				angle_helper->getConstrainedCursorPosMap(click_pos_map, click_pos_map);
			cur_pos_map = click_pos_map;
		}

		// Set path point
		MapCoord coord = click_pos_map.toMapCoord();
		if (draw_dash_points)
			coord.setDashPoint(true);
		
		if (preview_path->getCoordinateCount() > 0 && picked_angle)
			picked_angle = false;
		if (previous_point_is_curve_point)
		{
			// Do nothing yet, wait until the user drags or releases the mouse button
		}
		else if (path_has_preview_point)
		{
			preview_path->setCoordinate(preview_path->getCoordinateCount() - 1, coord);
			updateAngleHelper();
			created_point_at_last_mouse_press = true;
		}
		else
		{
			if (preview_path->getCoordinateCount() == 0 || !preview_path->getCoordinate(preview_path->getCoordinateCount() - 1).isPositionEqualTo(coord))
			{
				preview_path->addCoordinate(coord);
				updatePreviewPath();
				if (!start_appending)
					updateAngleHelper();
				created_point_at_last_mouse_press = true;
			}
		}
		
		path_has_preview_point = false;
		
		create_segment = true;
		updateDirtyRect();
		updateStatusText();
		return true;
	}
	
	return false;
}

bool DrawPathTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	bool mouse_down = drawMouseButtonHeld(event);
	if (mouse_down)
		left_mouse_down = true;
	cur_pos = event->pos();
	cur_pos_map = map_coord;
	
	if (!mouse_down)
	{
		updateHover();
	}
	else // if (mouse_down)
	{
		if (!draw_in_progress && picking_angle)
		{
			pickAngle(map_coord, widget);
			return true;
		}
		else if (!draw_in_progress)
			return false;
		
		if (following)
		{
			updateFollowing();
			return true;
		}
		
		bool drag_distance_reached = (event->pos() - click_pos).manhattanLength() >= QApplication::startDragDistance();
		if (dragging && !drag_distance_reached)
		{
			if (create_spline_corner)
			{
				create_spline_corner = false;
			}
			else if (path_has_preview_point)
			{
				// Remove preview
				int last = preview_path->getCoordinateCount() - 1;
				if (last >= 3 && preview_path->getCoordinate(last-3).isCurveStart())
				{
					preview_path->deleteCoordinate(last, false);
					preview_path->deleteCoordinate(last-1, false);
					preview_path->deleteCoordinate(last-2, false);
					
					MapCoord coord = preview_path->getCoordinate(last-3);
					coord.setCurveStart(false);
					preview_path->setCoordinate(last-3, coord);
				}
				else if (last >= 1)
				{
					preview_path->deleteCoordinate(last, false);
				}
				
				path_has_preview_point = false;
				dragging = false;
				create_segment = false;
				
				updatePreviewPath();
				updateDirtyRect();
			}
		}
		else if (drag_distance_reached)
		{
			// Giving a direction by dragging
			dragging = true;
			create_spline_corner = false;
			create_segment = true;
			
			if (previous_point_is_curve_point)
				angle_helper->setCenter(click_pos_map);
			
			QPointF constrained_pos;
			angle_helper->getConstrainedCursorPositions(map_coord, constrained_pos_map, constrained_pos, widget);
			
			if (previous_point_is_curve_point)
			{
				hidePreviewPoints();
				float drag_direction = calculateRotation(constrained_pos.toPoint(), constrained_pos_map);
				
				// Add a new node or convert the last node into a corner?
				if ((widget->mapToViewport(previous_pos_map) - click_pos).manhattanLength() >= QApplication::startDragDistance())
					createPreviewCurve(click_pos_map.toMapCoord(), drag_direction);
				else
				{
					create_spline_corner = true;
					// This hides the old direction indicator
					previous_drag_map = previous_pos_map;
				}
			}
			
			updateDirtyRect();
		}
	}
	
	return true;
}

bool DrawPathTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (!drawMouseButtonClicked(event))
		return false;
	left_mouse_down = false;
	if (picking_angle)
	{
		picking_angle = false;
		picked_angle = pickAngle(map_coord, widget);
		return true;
	}
	else if (!draw_in_progress)
		return false;
	if (following)
	{
		finishFollowing();
		if ((event->button() == Qt::RightButton) && Settings::getInstance().getSettingCached(Settings::MapEditor_DrawLastPointOnRightClick).toBool())
			finishDrawing();
		return true;
	}
	if (!create_segment)
		return true;
	
	if (previous_point_is_curve_point && !dragging && !create_spline_corner)
	{
		// The new point has not been added yet
		MapCoord coord;
		if (shift_pressed)
			coord = snap_helper->snapToObject(map_coord, widget);
		else if (angle_helper->isActive())
		{
			QPointF constrained_pos;
			angle_helper->getConstrainedCursorPositions(map_coord, constrained_pos_map, constrained_pos, widget);
			coord = constrained_pos_map.toMapCoord();
		}
		else
			coord = map_coord.toMapCoord();
		if (draw_dash_points)
			coord.setDashPoint(true);
		preview_path->addCoordinate(coord);
		updatePreviewPath();
		updateDirtyRect();
	}
	
	previous_point_is_curve_point = dragging;
	if (previous_point_is_curve_point)
	{
		QPointF constrained_pos;
		angle_helper->getConstrainedCursorPositions(map_coord, constrained_pos_map, constrained_pos, widget);
		
		previous_pos_map = click_pos_map;
		previous_drag_map = constrained_pos_map;
		previous_point_direction = calculateRotation(constrained_pos.toPoint(), constrained_pos_map);
	}
	
	updateAngleHelper();
	
	create_spline_corner = false;
	path_has_preview_point = false;
	dragging = false;
	
	if ((event->button() == Qt::RightButton) && Settings::getInstance().getSettingCached(Settings::MapEditor_DrawLastPointOnRightClick).toBool())
		finishDrawing();
	return true;
}

bool DrawPathTool::mouseDoubleClickEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	if (draw_in_progress)
	{
		if (created_point_at_last_mouse_press)
			undoLastPoint();
		finishDrawing();
	}
	return true;
}

bool DrawPathTool::keyPressEvent(QKeyEvent* event)
{
	bool key_handled = false;
	if (draw_in_progress)
	{
		key_handled = true;
		if (event->key() == Qt::Key_Escape)
			abortDrawing();
		else if (event->key() == Qt::Key_Backspace)
			undoLastPoint();
		else if (event->key() == Qt::Key_Return && allow_closing_paths)
		{
			closeDrawing();
			finishDrawing();
		}
		else
			key_handled = false;
	}
	else if (event->key() == Qt::Key_Backspace && finished_path_is_selected)
	{
		key_handled = removeLastPointFromSelectedPath();
	}
	
	if (event->key() == Qt::Key_Tab)
		deactivate();
	else if (event->key() == Qt::Key_Space)
	{
		draw_dash_points = !draw_dash_points;
		updateStatusText();
	}
	else if (event->key() == Qt::Key_Control)
	{
		ctrl_pressed = true;
		angle_helper->setActive(true);
		if (draw_in_progress && !dragging)
			updateDrawHover();
		picked_angle = false;
		updateStatusText();
	}
	else if (event->key() == Qt::Key_Shift)
	{
		shift_pressed = true;
		if (!dragging)
		{
			updateHover();
			updateDirtyRect();
		}
		updateStatusText();
	}
	else
		return key_handled;
	return true;
}

bool DrawPathTool::keyReleaseEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Control)
	{
		ctrl_pressed = false;
		if (!picked_angle)
			angle_helper->setActive(false);
		if (draw_in_progress && !dragging)
			updateDrawHover();
		updateStatusText();
	}
	else if (event->key() == Qt::Key_Shift)
	{
		shift_pressed = false;
		if (!dragging && !following)
		{
			updateHover();
			updateDirtyRect();
		}
		updateStatusText();
	}
	else
		return false;
	return true;
}

void DrawPathTool::draw(QPainter* painter, MapWidget* widget)
{
	drawPreviewObjects(painter, widget);
	
	if (draw_in_progress)
	{
		painter->setRenderHint(QPainter::Antialiasing);
		if (dragging && (cur_pos - click_pos).manhattanLength() >= QApplication::startDragDistance())
		{
			QPen pen(qRgb(255, 255, 255));
			pen.setWidth(3);
			painter->setPen(pen);
			painter->drawLine(widget->mapToViewport(click_pos_map), widget->mapToViewport(constrained_pos_map));
			painter->setPen(active_color);
			painter->drawLine(widget->mapToViewport(click_pos_map), widget->mapToViewport(constrained_pos_map));
		}
		if (previous_point_is_curve_point)
		{
			QPen pen(qRgb(255, 255, 255));
			pen.setWidth(3);
			painter->setPen(pen);
			painter->drawLine(widget->mapToViewport(previous_pos_map), widget->mapToViewport(previous_drag_map));
			painter->setPen(active_color);
			painter->drawLine(widget->mapToViewport(previous_pos_map), widget->mapToViewport(previous_drag_map));
		}
		
		if (!shift_pressed)
			angle_helper->draw(painter, widget);
	}
	
	if (shift_pressed && !dragging)
		snap_helper->draw(painter, widget);
	else if (!draw_in_progress && (picking_angle || picked_angle))
	{
		if (picking_angle)
			snap_helper->draw(painter, widget);
		angle_helper->draw(painter, widget);
	}
}

void DrawPathTool::updatePreviewPath()
{
	DrawLineAndAreaTool::updatePreviewPath();
	updateStatusText();
}

void DrawPathTool::updateHover()
{
	if (shift_pressed)
		constrained_pos_map = MapCoordF(snap_helper->snapToObject(cur_pos_map, cur_map_widget));
	else
		constrained_pos_map = cur_pos_map;
	
	if (!draw_in_progress)
	{
		// Show preview objects at this position
		setPreviewPointsPosition(constrained_pos_map);
		if (picked_angle)
			angle_helper->setCenter(constrained_pos_map);
		updateDirtyRect();
	}
	else // if (draw_in_progress)
	{
		updateDrawHover();
	}
}

void DrawPathTool::updateDrawHover()
{
	if (!shift_pressed)
		angle_helper->getConstrainedCursorPosMap(cur_pos_map, constrained_pos_map);
	if (!previous_point_is_curve_point && !left_mouse_down && draw_in_progress)
	{
		// Show a line to the cursor position as preview
		hidePreviewPoints();
		
		if (!path_has_preview_point)
		{
			preview_path->addCoordinate(constrained_pos_map.toMapCoord());
			path_has_preview_point = true;
		}
		preview_path->setCoordinate(preview_path->getCoordinateCount() - 1, constrained_pos_map.toMapCoord());
		
		updatePreviewPath();
		updateDirtyRect();	// TODO: Possible optimization: mark only the last segment as dirty
	}
	else if (previous_point_is_curve_point && !left_mouse_down && draw_in_progress)
	{
		setPreviewPointsPosition(constrained_pos_map, 1);
		updateDirtyRect();
	}
}

void DrawPathTool::createPreviewCurve(MapCoord position, float direction)
{
	if (!path_has_preview_point)
	{
		int last = preview_path->getCoordinateCount() - 1;
		(preview_path->getCoordinate(last)).setCurveStart(true);
		
		preview_path->addCoordinate(MapCoord(0, 0));
		preview_path->addCoordinate(MapCoord(0, 0));
		if (draw_dash_points)
			position.setDashPoint(true);
		preview_path->addCoordinate(position);
		
		path_has_preview_point = true;
	}
	
	// Adjust the preview curve
	int last = preview_path->getCoordinateCount() - 1;
	MapCoord previous_point = preview_path->getCoordinate(last - 3);
	MapCoord last_point = preview_path->getCoordinate(last);
	
	double bezier_handle_distance = BEZIER_HANDLE_DISTANCE * previous_point.lengthTo(last_point);
	
	preview_path->setCoordinate(last - 2, MapCoord(previous_point.xd() - bezier_handle_distance * sin(previous_point_direction),
												   previous_point.yd() - bezier_handle_distance * cos(previous_point_direction)));
	preview_path->setCoordinate(last - 1, MapCoord(last_point.xd() + bezier_handle_distance * sin(direction),
												   last_point.yd() + bezier_handle_distance * cos(direction)));
	updatePreviewPath();	
}

void DrawPathTool::undoLastPoint()
{
	if (preview_path->getCoordinateCount() <= (preview_path->getPart(0).isClosed() ? 3 : (path_has_preview_point ? 2 : 1)))
	{
		abortDrawing();
		return;
	}
	
	int last = preview_path->getCoordinateCount() - 1;
	preview_path->deleteCoordinate(last, false);
	
	if (last >= 3 && preview_path->getCoordinate(last - 3).isCurveStart())
	{
		MapCoord first = preview_path->getCoordinate(last - 3);
		MapCoord second = preview_path->getCoordinate(last - 2);
		
		previous_point_is_curve_point = true;
		previous_point_direction = -atan2(second.xd() - first.xd(), first.yd() - second.yd());
		previous_pos_map = MapCoordF(first);
		previous_drag_map = MapCoordF((first.xd() + second.xd()) / 2, (first.yd() + second.yd()) / 2);
		
		preview_path->deleteCoordinate(last - 1, false);
		preview_path->deleteCoordinate(last - 2, false);
		
		first.setCurveStart(false);
		preview_path->setCoordinate(last - 3, first);
	}
	else
	{
		int curve_test = path_has_preview_point ? 5 : 4;	// "possible_previous_curve_start_offset"
		if (path_has_preview_point)
			preview_path->deleteCoordinate(last - 1, false);
		
		if (last >= curve_test && preview_path->getCoordinate(last - curve_test).isCurveStart())
		{
			MapCoord first = preview_path->getCoordinate(last - curve_test + 2);
			MapCoord second = preview_path->getCoordinate(last - curve_test + 3);
			
			previous_point_is_curve_point = true;
			previous_point_direction = -atan2(second.xd() - first.xd(), first.yd() - second.yd());
			previous_pos_map = MapCoordF(second);
			previous_drag_map = MapCoordF(second.xd() + (second.xd() - first.xd()) / 2, second.yd() + (second.yd() - first.yd()) / 2);
		}
		else
			previous_point_is_curve_point = false;
	}
	
	path_has_preview_point = false;
	dragging = false;
	
	updatePreviewPath();
	updateAngleHelper();
	cur_pos_map = click_pos_map;
	updateDirtyRect();
}

bool DrawPathTool::removeLastPointFromSelectedPath()
{
	if (draw_in_progress || map()->getNumSelectedObjects() != 1)
	{
		return false;
	}
	
	Object* object = map()->getFirstSelectedObject();
	if (object->getType() != Object::Path)
	{
		return false;
	}
	
	PathObject* path = object->asPath();
	if (path->getNumParts() != 1)
	{
		return false;
	}
	
	int points_on_path = 0;
	int num_coords = path->getCoordinateCount();
	for (int i = 0; i < num_coords && points_on_path < 3; ++i)
	{
		++points_on_path;
		if (path->getCoordinate(i).isCurveStart())
		{
			i += 2; // Skip the control points.
		}
	}
	
	if (points_on_path < 3)
	{
		// Too few points after deleting the last: delete the whole object.
		map()->deleteSelectedObjects();
		return true;
	}
	
	ReplaceObjectsUndoStep* undo_step = new ReplaceObjectsUndoStep(map());
	Object* undo_duplicate = object->duplicate();
	undo_duplicate->setMap(map());
	undo_step->addObject(object, undo_duplicate);
	map()->objectUndoManager().addNewUndoStep(undo_step);
	updateDirtyRect();
	
	path->getPart(0).setClosed(false);
	path->deleteCoordinate(num_coords - 1, false);
	
	MapCoord& potential_curve_start = path->getCoordinate(qMax(0, num_coords - 4));
	if (num_coords >= 4 && potential_curve_start.isCurveStart())
	{
		path->deleteCoordinate(num_coords - 2, false);
		path->deleteCoordinate(num_coords - 3, false);
		potential_curve_start.setCurveStart(false);
	}
		
	path->update(true);
	map()->setObjectsDirty();
	return true;
}

void DrawPathTool::closeDrawing()
{
	if (preview_path->getCoordinateCount() <= 1)
		return;
	
	if (previous_point_is_curve_point && preview_path->getCoordinate(0).isCurveStart())
	{
		// Finish with a curve
		path_has_preview_point = false;
		
		if (dragging)
			previous_point_direction = -atan2(cur_pos_map.getX() - click_pos_map.getX(), click_pos_map.getY() - cur_pos_map.getY());
		
		MapCoord first = preview_path->getCoordinate(0);
		MapCoord second = preview_path->getCoordinate(1);
		createPreviewCurve(first, -atan2(second.xd() - first.xd(), first.yd() - second.yd()));
		path_has_preview_point = false;
	}
	
	if (preview_path->getNumParts() > 0)
		preview_path->getPart(0).setClosed(true, true);
}

void DrawPathTool::finishDrawing()
{
	// Does the symbols contain only areas? If so, auto-close the path if not done yet
	bool contains_only_areas = !is_helper_tool && (drawing_symbol->getContainedTypes() & ~(Symbol::Area | Symbol::Combined)) == 0 && (drawing_symbol->getContainedTypes() & Symbol::Area);
	if (contains_only_areas && preview_path->getNumParts() > 0)
		preview_path->getPart(0).setClosed(true, true);
	
	// Remove last point if closed and first and last points are equal, or if the last point was just a preview
	if (path_has_preview_point && !dragging)
		preview_path->deleteCoordinate(preview_path->getCoordinateCount() - (preview_path->getPart(0).isClosed() ? 2 : 1), false);
	
	if (preview_path->getCoordinateCount() < (contains_only_areas ? 3 : 2))
	{
		renderables->removeRenderablesOfObject(preview_path, false);
		delete preview_path;
		preview_path = NULL;
	}
	
	dragging = false;
	following = false;
	draw_in_progress = false;
	if (!ctrl_pressed)
		angle_helper->setActive(false);
	updateSnapHelper();
	updateStatusText();
	hidePreviewPoints();
	
	DrawLineAndAreaTool::finishDrawing(appending ? append_to_object : NULL);
	
	finished_path_is_selected = true;
}

void DrawPathTool::abortDrawing()
{
	dragging = false;
	following = false;
	draw_in_progress = false;
	if (!ctrl_pressed)
		angle_helper->setActive(false);
	updateSnapHelper();
	updateStatusText();
	hidePreviewPoints();
	
	DrawLineAndAreaTool::abortDrawing();
}

void DrawPathTool::updateDirtyRect()
{
	QRectF rect;
	
	if (dragging)
	{
		rectIncludeSafe(rect, click_pos_map.toQPointF());
		rectInclude(rect, cur_pos_map.toQPointF());
	}
	if (draw_in_progress && previous_point_is_curve_point)
	{
		rectIncludeSafe(rect, previous_pos_map.toQPointF());
		rectInclude(rect, previous_drag_map.toQPointF());
	}
	if ((draw_in_progress && !dragging) ||
		(!draw_in_progress && !shift_pressed && ctrl_pressed))
	{
		angle_helper->includeDirtyRect(rect);
	}
	if (shift_pressed || (!draw_in_progress && ctrl_pressed))
		snap_helper->includeDirtyRect(rect);
	includePreviewRects(rect);
	
	if (is_helper_tool)
		emit(dirtyRectChanged(rect));
	else
	{
		if (rect.isValid())
			map()->setDrawingBoundingBox(rect, qMax(qMax(dragging ? 1 : 0, angle_helper->getDisplayRadius()), snap_helper->getDisplayRadius()), true);
		else
			map()->clearDrawingBoundingBox();
	}
}

void DrawPathTool::selectedSymbolsChanged()
{
	if (is_helper_tool)
		return;
	DrawLineAndAreaTool::selectedSymbolsChanged();
	
	updateDashPointDrawing();
}

void DrawPathTool::objectSelectionChanged()
{
	finished_path_is_selected = false;
}

void DrawPathTool::updateAngleHelper()
{
	if (picked_angle)
		return;
	if (!preview_path)
	{
		angle_helper->clearAngles();
		angle_helper->addDefaultAnglesDeg(0);
		return;
	}
	updatePreviewPath();
	const MapCoordVector& map_coords = preview_path->getRawCoordinateVector();
	
	bool rectangular_stepping = true;
	double angle;
	if (map_coords.size() >= 2)
	{
		bool ok = false;
		MapCoordF tangent = PathCoord::calculateTangent(map_coords, map_coords.size() - 1, true, ok);
		if (!ok)
			tangent = MapCoordF(1, 0);
		angle = -tangent.getAngle();
	}
	else
	{
		if (previous_point_is_curve_point)
			angle = previous_point_direction;
		else
		{
			angle = 0;
			rectangular_stepping = false;
		}
	}
	
	if (!map_coords.empty())
		angle_helper->setCenter(MapCoordF(map_coords[map_coords.size() - 1]));
	angle_helper->clearAngles();
	if (rectangular_stepping)
		angle_helper->addAnglesDeg(angle * 180 / M_PI, 45);
	else
		angle_helper->addDefaultAnglesDeg(angle * 180 / M_PI);
}

bool DrawPathTool::pickAngle(MapCoordF coord, MapWidget* widget)
{
	MapCoord snap_position;
	bool picked = snap_helper->snapToDirection(coord, widget, angle_helper.data(), &snap_position);
	if (picked)
		angle_helper->setCenter(MapCoordF(snap_position));
	else
	{
		updateAngleHelper();
		angle_helper->setCenter(constrained_pos_map);
	}
	hidePreviewPoints();
	updateDirtyRect();
	return picked;
}

void DrawPathTool::updateSnapHelper()
{
	if (draw_in_progress)
		snap_helper->setFilter(SnappingToolHelper::AllTypes);
	else
	{
		//snap_helper.setFilter((SnappingToolHelper::SnapObjects)(SnappingToolHelper::GridCorners | SnappingToolHelper::ObjectCorners));
		snap_helper->setFilter(SnappingToolHelper::AllTypes);
	}
}

void DrawPathTool::startAppending(SnappingToolHelperSnapInfo& snap_info)
{
	append_to_object = snap_info.object->asPath();
}

void DrawPathTool::startFollowing(SnappingToolHelperSnapInfo& snap_info, const MapCoord& snap_coord)
{
	following = true;
	follow_object = snap_info.object->asPath();
	create_segment = false;
	
	if (snap_info.type == SnappingToolHelper::ObjectCorners)
		follow_helper->startFollowingFromCoord(follow_object, snap_info.coord_index);
	else // if (snap_info.type == SnappingToolHelper::ObjectPaths)
		follow_helper->startFollowingFromPathCoord(follow_object, snap_info.path_coord);
	
	if (path_has_preview_point)
		preview_path->setCoordinate(preview_path->getCoordinateCount() - 1, snap_coord);
	else
		preview_path->addCoordinate(snap_coord);
	path_has_preview_point = false;
	previous_point_is_curve_point = false;
	updatePreviewPath();
	follow_start_index = preview_path->getCoordinateCount() - 1;
}

void DrawPathTool::updateFollowing()
{
	PathCoord path_coord;
	float distance_sq;
	follow_object->calcClosestPointOnPath(cur_pos_map, distance_sq, path_coord, follow_helper->getPartIndex());
	PathObject* temp_object;
	bool success = follow_helper->updateFollowing(path_coord, temp_object);
	
	// Append the temporary object to the preview object at follow_start_index
	for (int i = preview_path->getCoordinateCount() - 1; i >= follow_start_index; --i)
		preview_path->deleteCoordinate(i, false);
	if (success)
	{
		preview_path->appendPath(temp_object);
		preview_path->getCoordinate(preview_path->getCoordinateCount() - 1).setHolePoint(false);
		delete temp_object;
	}
	updatePreviewPath();
	hidePreviewPoints();
	updateDirtyRect();
}

void DrawPathTool::finishFollowing()
{
	following = false;
	
	int last = preview_path->getCoordinateCount() - 1;
	
	if (last >= 3 && preview_path->getCoordinate(last - 3).isCurveStart())
	{
		MapCoord first = preview_path->getCoordinate(last - 1);
		MapCoord second = preview_path->getCoordinate(last);
		
		previous_point_is_curve_point = true;
		previous_point_direction = -atan2(second.xd() - first.xd(), first.yd() - second.yd());
		previous_pos_map = MapCoordF(second);
		previous_drag_map = MapCoordF(second.xd() + (second.xd() - first.xd()) / 2, second.yd() + (second.yd() - first.yd()) / 2);
	}
	else
		previous_point_is_curve_point = false;
	
	updateAngleHelper();
}

float DrawPathTool::calculateRotation(QPoint mouse_pos, MapCoordF mouse_pos_map)
{
	if (dragging && (mouse_pos - click_pos).manhattanLength() >= QApplication::startDragDistance())
		return -atan2(mouse_pos_map.getX() - click_pos_map.getX(), click_pos_map.getY() - mouse_pos_map.getY());
	else
		return 0;
}

void DrawPathTool::updateDashPointDrawing()
{
	Symbol* symbol = symbol_widget->getSingleSelectedSymbol();
	if (symbol && symbol->getType() == Symbol::Line)
	{
		// Auto-activate dash points depending on if the selected symbol has a dash symbol.
		// TODO: instead of just looking if it is a line symbol with dash points,
		// could also check for combined symbols containing lines with dash points
		draw_dash_points = (symbol->asLine()->getDashSymbol() != NULL);
		
		updateStatusText();
	}
	else if (symbol &&
		(symbol->getType() == Symbol::Area ||
		 symbol->getType() == Symbol::Combined))
	{
		draw_dash_points = false;
	}
}

void DrawPathTool::updateStatusText()
{
	QString text;
	static const QString text_more_shift_control_space(MapEditorTool::tr("More: %1, %2, %3").arg(ModifierKey::shift(), ModifierKey::control(), ModifierKey(Qt::Key_Space)));
	static const QString text_more_shift_space(MapEditorTool::tr("More: %1, %2").arg(ModifierKey::shift(), ModifierKey(Qt::Key_Space)));
	static const QString text_more_control_space(MapEditorTool::tr("More: %1, %2").arg(ModifierKey::control(), ModifierKey(Qt::Key_Space)));
	QString text_more(text_more_shift_control_space);
	
	if (draw_in_progress && preview_path && preview_path->getCoordinateCount() >= 2)
	{
		//assert(!preview_path->isDirty());
		float length = map()->getScaleDenominator() * preview_path->getPathCoordinateVector()[preview_path->getPart(0).path_coord_end_index].clen * 0.001f;
		text += tr("<b>Length:</b> %1 m ").arg(QLocale().toString(length, 'f', 1));
		text += "| ";
	}
	
	if (draw_dash_points && !is_helper_tool)
		text += DrawLineAndAreaTool::tr("<b>Dash points on.</b> ") + "| ";
	
	if (!draw_in_progress)
	{
		if (shift_pressed)
		{
			text += DrawLineAndAreaTool::tr("<b>%1+Click</b>: Snap or append to existing objects. ").arg(ModifierKey::shift());
			text_more = text_more_control_space;
		}
		else if (ctrl_pressed)
		{
			text += DrawLineAndAreaTool::tr("<b>%1+Click</b>: Pick direction from existing objects. ").arg(ModifierKey::control());
			text_more = text_more_shift_space;
		}
		else
		{
			text += tr("<b>Click</b>: Start a straight line. <b>Drag</b>: Start a curve. ");
// 			text += DrawLineAndAreaTool::tr(draw_dash_points ? "<b>%1</b> Disable dash points. " : "<b>%1</b>: Enable dash points. ").arg(ModifierKey::space());
		}
	}
	else
	{
		if (shift_pressed)
		{
			text += DrawLineAndAreaTool::tr("<b>%1+Click</b>: Snap to existing objects. ").arg(ModifierKey::shift());
			text += tr("<b>%1+Drag</b>: Follow existing objects. ").arg(ModifierKey::shift());
			text_more = text_more_control_space;
		}
		else if (ctrl_pressed && angle_helper->isActive())
		{
			text += DrawLineAndAreaTool::tr("<b>%1</b>: Fixed angles. ").arg(ModifierKey::control());
			text_more = text_more_shift_space;
		}
		else
		{
			text += tr("<b>Click</b>: Draw a straight line. <b>Drag</b>: Draw a curve. "
			           "<b>Right or double click</b>: Finish the path. "
			           "<b>%1</b>: Close the path. ").arg(ModifierKey::return_key());
			text += DrawLineAndAreaTool::tr("<b>%1</b>: Undo last point. ").arg(ModifierKey::backspace());
			text += MapEditorTool::tr("<b>%1</b>: Abort. ").arg(ModifierKey::escape());
		}
	}
	
	text += "| " + text_more;
	
	setStatusBarText(text);
}
