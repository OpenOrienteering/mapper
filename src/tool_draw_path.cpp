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


#include "tool_draw_path.h"

#include <QtGui>

#include "util.h"
#include "renderable.h"
#include "symbol.h"
#include "object.h"
#include "map_editor.h"
#include "map_widget.h"

QCursor* DrawPathTool::cursor = NULL;

DrawPathTool::DrawPathTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget, bool allow_closing_paths)
 : DrawLineAndAreaTool(editor, tool_button, symbol_widget), allow_closing_paths(allow_closing_paths),
   angle_helper(new ConstrainAngleToolHelper()),
   snap_helper(editor->getMap())
{
	cur_map_widget = editor->getMainWidget();
	
	angle_helper->setActive(false);
	connect(angle_helper.data(), SIGNAL(displayChanged()), this, SLOT(updateDirtyRect()));
	
	updateSnapHelper();
	connect(&snap_helper, SIGNAL(displayChanged()), this, SLOT(updateDirtyRect()));
	
	dragging = false;
	space_pressed = false;
	shift_pressed = false;
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-draw-path.png"), 11, 11);
}
void DrawPathTool::init()
{
	updateStatusText();
}

bool DrawPathTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	cur_map_widget = widget;
	
	if ((event->buttons() & Qt::RightButton) && draw_in_progress)
	{
		finishDrawing();
		return true;
	}
	else if (event->buttons() & Qt::LeftButton)
	{
		if (shift_pressed)
		{
			click_pos_map = MapCoordF(snap_helper.snapToObject(map_coord, widget));
			cur_pos_map = click_pos_map;
			click_pos = widget->mapToViewport(click_pos_map).toPoint();
		}
		else
		{
			click_pos = event->pos();
			click_pos_map = map_coord;
			cur_pos_map = map_coord;
		}
		dragging = false;
		
		if (!draw_in_progress)
		{
			// Start a new path
			startDrawing();
			angle_helper->setCenter(click_pos_map);
			updateSnapHelper();
			
			path_has_preview_point = false;
			previous_point_is_curve_point = false;
			
			updateStatusText();
		}
		else
		{
			if (!shift_pressed)
				angle_helper->getConstrainedCursorPosMap(click_pos_map, click_pos_map);
			cur_pos_map = click_pos_map;
		}

		// Set path point
		MapCoord coord = click_pos_map.toMapCoord();
		if (space_pressed)
			coord.setDashPoint(true);
		
		if (previous_point_is_curve_point)
		{
			// Do nothing yet, wait until the user drags or releases the mouse button
			angle_helper->setCenter(click_pos_map);
		}
		else if (path_has_preview_point)
		{
			preview_path->setCoordinate(preview_path->getCoordinateCount() - 1, coord);
			updateAngleHelper();
		}
		else
		{
			if (preview_path->getCoordinateCount() == 0 || !preview_path->getCoordinate(preview_path->getCoordinateCount() - 1).isPositionEqualTo(coord))
			{
				preview_path->addCoordinate(coord);
				updatePreviewPath();
				updateAngleHelper();
			}
		}
		
		path_has_preview_point = false;
		
		create_segment = true;
		updateDirtyRect();
		return true;
	}
	
	return false;
}
bool DrawPathTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	bool mouse_down = event->buttons() & Qt::LeftButton;
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
		if (!draw_in_progress)
			return false;
		
		if ((event->pos() - click_pos).manhattanLength() < QApplication::startDragDistance())
		{
			if (path_has_preview_point)
			{
				// Remove preview
				int last = preview_path->getCoordinateCount() - 1;
				preview_path->deleteCoordinate(last, false);
				preview_path->deleteCoordinate(last-1, false);
				preview_path->deleteCoordinate(last-2, false);
				
				MapCoord coord = preview_path->getCoordinate(last-3);
				coord.setCurveStart(false);
				preview_path->setCoordinate(last-3, coord);
				
				path_has_preview_point = false;
				dragging = false;
				create_segment = false;
				
				updatePreviewPath();
				updateDirtyRect();
			}
			
			return true;
		}
		
		// Giving a direction by dragging
		dragging = true;
		create_segment = true;
		
		QPointF constrained_pos;
		angle_helper->getConstrainedCursorPositions(map_coord, constrained_pos_map, constrained_pos, widget);
		
		if (previous_point_is_curve_point)
		{
			hidePreviewPoints();
			float drag_direction = calculateRotation(constrained_pos.toPoint(), constrained_pos_map);
			createPreviewCurve(click_pos_map.toMapCoord(), drag_direction);
		}
		
		updateDirtyRect();
	}
	
	return true;
}
bool DrawPathTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	left_mouse_down = false;
	if (!draw_in_progress)
		return false;
	if (!create_segment)
		return true;
	
	if (previous_point_is_curve_point && !dragging)
	{
		// The new point has not been added yet
		MapCoord coord;
		if (shift_pressed)
			coord = snap_helper.snapToObject(map_coord, widget);
		else
			coord = map_coord.toMapCoord();
		if (space_pressed)
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
	
	path_has_preview_point = false;
	dragging = false;
	return true;
}
bool DrawPathTool::mouseDoubleClickEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
    if (draw_in_progress)
		finishDrawing();
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
	if (event->key() == Qt::Key_Tab)
		editor->setEditTool();
	else if (event->key() == Qt::Key_Space)
	{
		space_pressed = !space_pressed;
		updateStatusText();
	}
	else if (event->key() == Qt::Key_Control)
	{
		angle_helper->setActive(true);
		if (draw_in_progress && !dragging)
			updateDrawHover();
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
		angle_helper->setActive(false);
		if (draw_in_progress && !dragging)
			updateDrawHover();
		updateStatusText();
	}
	else if (event->key() == Qt::Key_Shift)
	{
		shift_pressed = false;
		if (!dragging)
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
		snap_helper.draw(painter, widget);
}

void DrawPathTool::updateHover()
{
	if (shift_pressed)
		constrained_pos_map = MapCoordF(snap_helper.snapToObject(cur_pos_map, cur_map_widget));
	else
		constrained_pos_map = cur_pos_map;
	
	if (!draw_in_progress)
	{
		// Show preview objects at this position
		setPreviewPointsPosition(constrained_pos_map);
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
}

void DrawPathTool::createPreviewCurve(MapCoord position, float direction)
{
	if (!path_has_preview_point)
	{
		int last = preview_path->getCoordinateCount() - 1;
		(preview_path->getCoordinate(last)).setCurveStart(true);
		
		preview_path->addCoordinate(MapCoord(0, 0));
		preview_path->addCoordinate(MapCoord(0, 0));
		if (space_pressed)
			position.setDashPoint(true);
		preview_path->addCoordinate(position);
		
		path_has_preview_point = true;
	}
	
	// Adjust the preview curve
	int last = preview_path->getCoordinateCount() - 1;
	MapCoord previous_point = preview_path->getCoordinate(last - 3);
	MapCoord last_point = preview_path->getCoordinate(last);
	
	double bezier_handle_distance = 0.4 * previous_point.lengthTo(last_point);
	
	preview_path->setCoordinate(last - 2, MapCoord(previous_point.xd() - bezier_handle_distance * sin(previous_point_direction),
												   previous_point.yd() - bezier_handle_distance * cos(previous_point_direction)));
	preview_path->setCoordinate(last - 1, MapCoord(last_point.xd() + bezier_handle_distance * sin(direction),
												   last_point.yd() + bezier_handle_distance * cos(direction)));
	updatePreviewPath();	
}
void DrawPathTool::undoLastPoint()
{
	if (preview_path->getCoordinateCount() <= (preview_path->getPart(0).isClosed() ? 2 : 1))
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
		previous_point_is_curve_point = false;
		
		if (path_has_preview_point)
			preview_path->deleteCoordinate(last - 1, false);
	}
	
	path_has_preview_point = false;
	dragging = false;
	
	updatePreviewPath();
	updateAngleHelper();
	cur_pos_map = click_pos_map;
	updateDirtyRect();
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
	draw_in_progress = false;
	updateSnapHelper();
	updateStatusText();
	
	DrawLineAndAreaTool::finishDrawing();
}
void DrawPathTool::abortDrawing()
{
	dragging = false;
	draw_in_progress = false;
	updateSnapHelper();
	updateStatusText();
	
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
	if (draw_in_progress && !dragging)
		angle_helper->includeDirtyRect(rect);
	if (shift_pressed)
		snap_helper.includeDirtyRect(rect);
	includePreviewRects(rect);
	
	if (is_helper_tool)
		emit(dirtyRectChanged(rect));
	else
	{
		if (rect.isValid())
			editor->getMap()->setDrawingBoundingBox(rect, qMax(qMax(dragging ? 1 : 0, angle_helper->getDisplayRadius()), snap_helper.getDisplayRadius()), true);
		else
			editor->getMap()->clearDrawingBoundingBox();
	}
}

void DrawPathTool::updateAngleHelper()
{
	if (!preview_path)
		return;
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

void DrawPathTool::updateSnapHelper()
{
	if (draw_in_progress)
		snap_helper.setFilter(SnappingToolHelper::AllTypes);
	else
		snap_helper.setFilter((SnappingToolHelper::SnapObjects)(SnappingToolHelper::GridCorners | SnappingToolHelper::ObjectCorners));
}

float DrawPathTool::calculateRotation(QPoint mouse_pos, MapCoordF mouse_pos_map)
{
	if (dragging && (mouse_pos - click_pos).manhattanLength() >= QApplication::startDragDistance())
		return -atan2(mouse_pos_map.getX() - click_pos_map.getX(), click_pos_map.getY() - mouse_pos_map.getY());
	else
		return 0;
}

void DrawPathTool::updateStatusText()
{
	if (is_helper_tool)
		return;
	
	QString text = "";
	if (space_pressed)
		text += tr("<b>Dash points on.</b> ");
	
	if (!draw_in_progress)
		text += tr("<b>Click</b> to start a polygonal segment, <b>Drag</b> to start a curve. Hold <u>Shift</u> to snap to existing objects.");
	else
	{
		if (shift_pressed)
			text += tr("<u>Shift</u>: snap to existing objects");
		else if (angle_helper->isActive())
			text += tr("<u>Ctrl</u>: fixed angles");
		else
			text += tr("<b>Click</b> to draw a polygonal segment, <b>Drag</b> to draw a curve, <b>Right or double click</b> to finish the path, "
					   "<b>Return</b> to close the path, <b>Backspace</b> to undo, <b>Esc</b> to abort. Try <b>Space</b>, <u>Shift</u>, <u>Ctrl</u>");
	}
	
	setStatusBarText(text);
}
