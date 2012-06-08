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
#include "map_widget.h"

QCursor* DrawPathTool::cursor = NULL;

DrawPathTool::DrawPathTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget, bool allow_closing_paths)
 : DrawLineAndAreaTool(editor, tool_button, symbol_widget), allow_closing_paths(allow_closing_paths)
{
	dragging = false;
	space_pressed = false;
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-draw-path.png"), 11, 11);
}
void DrawPathTool::init()
{
	updateStatusText();
}

bool DrawPathTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if ((event->buttons() & Qt::RightButton) && draw_in_progress)
	{
		finishDrawing();
		return true;
	}
	else if (event->buttons() & Qt::LeftButton)
	{
		click_pos = event->pos();
		click_pos_map = map_coord;
		cur_pos_map = map_coord;
		dragging = false;
		
		if (!draw_in_progress)
		{
			// Start a new path
			startDrawing();
			
			path_has_preview_point = false;
			previous_point_is_curve_point = false;
			
			updateStatusText();
		}

		// Set path point
		MapCoord coord = map_coord.toMapCoord();
		if (space_pressed)
			coord.setDashPoint(true);
		
		if (previous_point_is_curve_point)
			; // Do nothing yet, wait until the user drags or releases the mouse button
		else if (path_has_preview_point)
			preview_path->setCoordinate(preview_path->getCoordinateCount() - 1, coord);
		else
		{
			if (preview_path->getCoordinateCount() == 0 || !preview_path->getCoordinate(preview_path->getCoordinateCount() - 1).isPositionEqualTo(coord))
			{
				preview_path->addCoordinate(coord);
				updatePreviewPath();
			}
		}
		
		path_has_preview_point = false;
		
		create_segment = true;
		setDirtyRect(map_coord);
		return true;
	}
	
	return false;
}
bool DrawPathTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	bool mouse_down = event->buttons() & Qt::LeftButton;
	
	if (!mouse_down)
	{
		if (!draw_in_progress)
		{
			// Show preview objects at this position
			setPreviewPointsPosition(map_coord);
			setDirtyRect(map_coord);
		}
		else // if (draw_in_progress)
		{
			if (!previous_point_is_curve_point)
			{
				// Show a line to the cursor position as preview
				hidePreviewPoints();
				
				if (!path_has_preview_point)
				{
					preview_path->addCoordinate(map_coord.toMapCoord());
					path_has_preview_point = true;
				}
				preview_path->setCoordinate(preview_path->getCoordinateCount() - 1, map_coord.toMapCoord());
				
				updatePreviewPath();
				setDirtyRect(map_coord);	// TODO: Possible optimization: mark only the last segment as dirty
			}
		}
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
				setDirtyRect(map_coord);
			}
			
			return true;
		}
		
		// Giving a direction by dragging
		dragging = true;
		create_segment = true;
		cur_pos = event->pos();
		cur_pos_map = map_coord;
		
		if (previous_point_is_curve_point)
		{
			hidePreviewPoints();
			
			float drag_direction = calculateRotation(event->pos(), map_coord);
			createPreviewCurve(click_pos_map.toMapCoord(), drag_direction);
		}
		
		setDirtyRect(map_coord);
	}
	
	return true;
}
bool DrawPathTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	if (!draw_in_progress)
		return false;
	if (!create_segment)
		return true;
	
	if (previous_point_is_curve_point && !dragging)
	{
		// The new point has not been added yet
		MapCoord coord = map_coord.toMapCoord();
		if (space_pressed)
			coord.setDashPoint(true);
		preview_path->addCoordinate(coord);
		updatePreviewPath();
		setDirtyRect(map_coord);
	}
	
	previous_point_is_curve_point = dragging;
	if (previous_point_is_curve_point)
	{
		previous_pos_map = click_pos_map;
		previous_drag_map = map_coord;
		previous_point_direction = calculateRotation(event->pos(), map_coord);
	}
	
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
	else
		return key_handled;
	
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
			painter->drawLine(widget->mapToViewport(click_pos_map), widget->mapToViewport(cur_pos_map));
			painter->setPen(active_color);
			painter->drawLine(widget->mapToViewport(click_pos_map), widget->mapToViewport(cur_pos_map));
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
	setDirtyRect(click_pos_map);
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
	updateStatusText();
	
	DrawLineAndAreaTool::finishDrawing();
}
void DrawPathTool::abortDrawing()
{
	dragging = false;
	draw_in_progress = false;
	updateStatusText();
	
	DrawLineAndAreaTool::abortDrawing();
}

void DrawPathTool::setDirtyRect(MapCoordF mouse_pos)
{
	QRectF rect;
	
	if (dragging)
	{
		rectIncludeSafe(rect, click_pos_map.toQPointF());
		rectInclude(rect, mouse_pos.toQPointF());
	}
	if (draw_in_progress && previous_point_is_curve_point)
	{
		rectIncludeSafe(rect, previous_pos_map.toQPointF());
		rectInclude(rect, previous_drag_map.toQPointF());
	}
	includePreviewRects(rect);
	
	if (is_helper_tool)
		emit(dirtyRectChanged(rect));
	else
	{
		if (rect.isValid())
			editor->getMap()->setDrawingBoundingBox(rect, dragging ? 1 : 0, true);
		else
			editor->getMap()->clearDrawingBoundingBox();
	}
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
		text += tr("<b>Click</b> to start a polygonal segment, <b>Drag</b> to start a curve");
	else
	{
		text += tr("<b>Click</b> to draw a polygonal segment, <b>Drag</b> to draw a curve, <b>Right or double click</b> to finish the path, "
					"<b>Return</b> to close the path, <b>Backspace</b> to undo, <b>Esc</b> to abort. Try <b>Space</b>");
	}
	
	setStatusBarText(text);
}
