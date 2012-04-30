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


#include "tool_draw_rectangle.h"

#include <QtGui>

#include "util.h"
#include "object.h"
#include "map_widget.h"

QCursor* DrawRectangleTool::cursor = NULL;

DrawRectangleTool::DrawRectangleTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget)
 : DrawLineAndAreaTool(editor, tool_button, symbol_widget)
{
	draw_dash_points = true;
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-draw-rectangle.png"), 11, 11);
}

void DrawRectangleTool::init()
{
	updateStatusText();
}

bool DrawRectangleTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() == Qt::LeftButton)
	{
		dragging = false;
		mouse_press_pos = event->pos();
		cur_pos = event->pos();
		cur_pos_map = map_coord;
		
		if (!draw_in_progress)
		{
			click_pos = event->pos();
			click_pos_map = map_coord;
			second_point_set = false;
			third_point_set = false;
			new_corner_needed = false;
			
			startDrawing();
			MapCoord coord = map_coord.toMapCoord();
			coord.setDashPoint(draw_dash_points);
			preview_path->addCoordinate(coord);
			preview_path->addCoordinate(coord);
			updateStatusText();
		}
		else
		{
			updateRectangle();
			
			if (!second_point_set)
			{
				second_point_set = true;
				
				close_vector = click_pos_map - cur_pos_map;
				close_vector.normalize();
				forward_vector = close_vector;
				forward_vector.perpRight();
				preview_path->addCoordinate(map_coord.toMapCoord()); // bring to the correct number of points
				setDirtyRect();
			}
			else
			{
				if (!third_point_set)
				{
					third_point_set = true;
					delete_start_point = false;
				}
				else
					delete_start_point = !delete_start_point;
				
				forward_vector = close_vector;
				close_vector.perpRight();
				if (close_vector.dot(MapCoordF(preview_path->getCoordinate(0) - preview_path->getCoordinate(preview_path->getCoordinateCount() - 3))) < 0)
					close_vector = -close_vector;
			}
			
			new_corner_needed = true;
		}
	}
	else if (event->button() == Qt::RightButton && draw_in_progress)
	{
		if (!third_point_set)
			abortDrawing();
		else
		{
			if (!new_corner_needed)
			{
				// Move preview points to correct position
				cur_pos_map = MapCoordF(preview_path->getCoordinate(0));
				updateRectangle();
				
				// Remove last point which is equal to the first one
				preview_path->deleteCoordinate(preview_path->getCoordinateCount() - 2, false);
			}
			finishDrawing();
		}
	}
	else
		return false;
	return true;
}
bool DrawRectangleTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	bool mouse_down = event->buttons() & Qt::LeftButton;
	
	if (!draw_in_progress)
	{
		setPreviewPointsPosition(map_coord);
		setDirtyRect();
	}
	else
	{
		hidePreviewPoints();
		
		cur_pos = event->pos();
		cur_pos_map = map_coord;
		
		if (mouse_down && !dragging && (event->pos() - mouse_press_pos).manhattanLength() >= QApplication::startDragDistance())
		{
			// Start dragging
			dragging = true;
		}
		
		updatePreview();
	}

	return true;
}
bool DrawRectangleTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if ((event->button() == Qt::LeftButton) && dragging)
	{
		dragging = false;
		return mousePressEvent(event, map_coord, widget);
	}
	else
		return false;
}
bool DrawRectangleTool::mouseDoubleClickEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	if (draw_in_progress)
		finishDrawing();
	return true;
}

bool DrawRectangleTool::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape)
		abortDrawing();
	else if (event->key() == Qt::Key_Backspace)
		undoLastPoint();
	else if (event->key() == Qt::Key_Tab)
		editor->setEditTool();
	else if (event->key() == Qt::Key_Space)
	{
		draw_dash_points = !draw_dash_points;
		updateStatusText();
	}
	else
		return false;
	return true;
}

void DrawRectangleTool::draw(QPainter* painter, MapWidget* widget)
{
	drawPreviewObjects(painter, widget);
	
	if (draw_in_progress && second_point_set)
	{
		painter->setRenderHint(QPainter::Antialiasing);

		/*QPen pen(qRgb(255, 255, 255));
		pen.setWidth(3);
		painter->setPen(pen);
		painter->drawLine(widget->mapToViewport(click_pos_map), widget->mapToViewport(cur_pos_map));*/
		
		QPen pen(active_color);
		pen.setStyle(Qt::DashLine);
		painter->setPen(active_color);
		painter->drawLine(widget->mapToViewport(cur_pos_map) + helper_cross_radius * forward_vector.toQPointF(),
						  widget->mapToViewport(cur_pos_map) - helper_cross_radius * forward_vector.toQPointF());
		painter->drawLine(widget->mapToViewport(cur_pos_map) + helper_cross_radius * close_vector.toQPointF(),
						  widget->mapToViewport(cur_pos_map) - helper_cross_radius * close_vector.toQPointF());
	}
}

void DrawRectangleTool::finishDrawing()
{
	if (delete_start_point)
	{
		preview_path->getPart(0).setClosed(false);
		preview_path->deleteCoordinate(preview_path->getCoordinateCount() - 1, false);
		preview_path->deleteCoordinate(0, false);
		preview_path->getPart(0).setClosed(true, true);
	}
	
	DrawLineAndAreaTool::finishDrawing();
	updateStatusText();
}
void DrawRectangleTool::abortDrawing()
{
	DrawLineAndAreaTool::abortDrawing();
	updateStatusText();
}
void DrawRectangleTool::undoLastPoint()
{
	if (!second_point_set)
	{
		abortDrawing();
		return;
	}
	
	if (preview_path->getPart(0).isClosed())
	{
		preview_path->getPart(0).setClosed(false);
		preview_path->deleteCoordinate(preview_path->getCoordinateCount() - 1, false);
	}
	if (!third_point_set)
	{
		preview_path->deleteCoordinate(preview_path->getCoordinateCount() - 1, false);
		preview_path->deleteCoordinate(preview_path->getCoordinateCount() - 1, false);
		second_point_set = false;
	}
	else
	{
		preview_path->deleteCoordinate(preview_path->getCoordinateCount() - 1, false);
		
		forward_vector = close_vector;
		close_vector.perpRight();
		
		if (preview_path->getCoordinateCount() == 4 && !new_corner_needed)
			third_point_set = false;
	}
	updatePreview();
	
	delete_start_point = !delete_start_point;
}

void DrawRectangleTool::updateRectangle()
{
	if (!second_point_set)
	{
		MapCoord coord = cur_pos_map.toMapCoord();
		coord.setDashPoint(draw_dash_points);
		preview_path->setCoordinate(1, coord);
	}
	else
	{
		int last_coord = preview_path->getPart(0).end_index;
		if (preview_path->getPart(0).isClosed())
		{
			preview_path->getPart(0).setClosed(false);
			preview_path->deleteCoordinate(last_coord, false);
			--last_coord;
		}
		
		float forward_dist = forward_vector.dot(cur_pos_map - MapCoordF(preview_path->getCoordinate(last_coord - 2)));
		MapCoord coord = preview_path->getCoordinate(last_coord - 2) + (forward_dist * forward_vector).toMapCoord();
		coord.setDashPoint(draw_dash_points);
		preview_path->setCoordinate(last_coord - 1, coord);
		
		float close_dist = close_vector.dot(MapCoordF(preview_path->getCoordinate(0) - preview_path->getCoordinate(last_coord - 1)));
		coord = preview_path->getCoordinate(last_coord - 1) + (close_dist * close_vector).toMapCoord();
		coord.setDashPoint(draw_dash_points);
		preview_path->setCoordinate(last_coord, coord);
		
		preview_path->getPart(0).setClosed(true, false);
		assert(preview_path->getPart(0).end_index == last_coord + 1);
	}
	
	updatePreviewPath();
	setDirtyRect();
}
void DrawRectangleTool::updatePreview()
{
	if (new_corner_needed)
	{
		new_corner_needed = false;
		preview_path->addCoordinate(cur_pos_map.toMapCoord());
		updateRectangle();
	}
	
	updateRectangle();
}
void DrawRectangleTool::setDirtyRect()
{
	QRectF rect;
	includePreviewRects(rect);
	
	if (is_helper_tool)
		emit(dirtyRectChanged(rect));
	else
	{
		if (rect.isValid())
			editor->getMap()->setDrawingBoundingBox(rect, (draw_in_progress && second_point_set) ? helper_cross_radius : 0, true); // helper_cross_radius as border is less than ideal but the only way to always ensure visibility of the helper cross at the moment
		else
			editor->getMap()->clearDrawingBoundingBox();
	}
}
void DrawRectangleTool::updateStatusText()
{
	QString text = "";
	
	if (draw_dash_points)
		text += tr("<b>Dash points on.</b> ");
	
	if (!draw_in_progress)
		text += tr("<b>Click</b> to start drawing a rectangle");
	else
		text += tr("<b>Click</b> to set a corner point, <b>Right or double click</b> to finish the rectangle, <b>Backspace</b> to undo, <b>Esc</b> to abort");
	
	text += ", " + tr("<b>Space</b> to toggle dash points");
	
	setStatusBarText(text);
}

#include "tool_draw_rectangle.moc"
