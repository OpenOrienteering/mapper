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

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "util.h"
#include "object.h"
#include "map_widget.h"
#include "map_editor.h"
#include "settings.h"

QCursor* DrawRectangleTool::cursor = NULL;

DrawRectangleTool::DrawRectangleTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget)
 : DrawLineAndAreaTool(editor, tool_button, symbol_widget),
   angle_helper(new ConstrainAngleToolHelper())
{
	draw_dash_points = true;
	
	angle_helper->addDefaultAnglesDeg(0);
	angle_helper->setActive(false);
	connect(angle_helper.data(), SIGNAL(displayChanged()), this, SLOT(updateDirtyRect()));
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-draw-rectangle.png"), 11, 11);
}

void DrawRectangleTool::init()
{
	updateStatusText();
}

bool DrawRectangleTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if ((event->button() == Qt::LeftButton) || (draw_in_progress && drawMouseButtonClicked(event)))
	{
		dragging = false;
		mouse_press_pos = event->pos();
		cur_pos = event->pos();
		cur_pos_map = map_coord;
		
		if (!draw_in_progress)
		{
			click_pos = event->pos();
			click_pos_map = map_coord;
			if (angle_helper->isActive())
				angle_helper->setActive(true, click_pos_map);
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
				if (click_pos_map == constrained_pos_map)
					return true;
				second_point_set = true;
				
				preview_path->addCoordinate(map_coord.toMapCoord()); // bring to the correct number of points
				updateDirtyRect();
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
		finishRectangleDrawing();
	}
	else
		return false;
	return true;
}
bool DrawRectangleTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	bool mouse_down = drawMouseButtonHeld(event);
	
	if (!draw_in_progress)
	{
		setPreviewPointsPosition(map_coord);
		updateDirtyRect();
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
	bool result = false;
	if (drawMouseButtonClicked(event) && dragging)
	{
		dragging = false;
		result = mousePressEvent(event, map_coord, widget);
	}
	
	if (event->button() == Qt::RightButton && Settings::getInstance().getSettingCached(Settings::MapEditor_DrawLastPointOnRightClick).toBool())
	{
		finishRectangleDrawing();
		return true;
	}
	return result;
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
	else if (event->key() == Qt::Key_Control)
	{
		angle_helper->setActive(true, click_pos_map);
		if (dragging && draw_in_progress)
			updatePreview();
	}
	else
		return false;
	return true;
}
bool DrawRectangleTool::keyReleaseEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Control)
	{
		angle_helper->setActive(false);
		if (dragging && draw_in_progress)
			updatePreview();
	}
	else
		return false;
	return true;
}

void DrawRectangleTool::draw(QPainter* painter, MapWidget* widget)
{
	const bool use_preview_radius = Settings::getInstance().getSettingCached(Settings::RectangleTool_PreviewLineWidth).toBool();
	
	drawPreviewObjects(painter, widget);
	
	if (draw_in_progress)
	{
		int helper_cross_radius = Settings::getInstance().getSettingCached(Settings::RectangleTool_HelperCrossRadius).toInt();
		painter->setRenderHint(QPainter::Antialiasing);
		
		painter->setPen(second_point_set ? inactive_color : active_color);
		if (preview_point_radius == 0 || !use_preview_radius)
		{
			painter->drawLine(widget->mapToViewport(cur_pos_map) + helper_cross_radius * forward_vector.toQPointF(),
							  widget->mapToViewport(cur_pos_map) - helper_cross_radius * forward_vector.toQPointF());
		}
		else
		{
			painter->drawLine(widget->mapToViewport(cur_pos_map + 0.001f * preview_point_radius * close_vector) + helper_cross_radius * forward_vector.toQPointF(),
							  widget->mapToViewport(cur_pos_map + 0.001f * preview_point_radius * close_vector) - helper_cross_radius * forward_vector.toQPointF());
			painter->drawLine(widget->mapToViewport(cur_pos_map - 0.001f * preview_point_radius * close_vector) + helper_cross_radius * forward_vector.toQPointF(),
							  widget->mapToViewport(cur_pos_map - 0.001f * preview_point_radius * close_vector) - helper_cross_radius * forward_vector.toQPointF());
		}
		
		painter->setPen(active_color);
		if (preview_point_radius == 0 || !use_preview_radius)
		{
			painter->drawLine(widget->mapToViewport(cur_pos_map) + helper_cross_radius * close_vector.toQPointF(),
							  widget->mapToViewport(cur_pos_map) - helper_cross_radius * close_vector.toQPointF());
		}
		else
		{
			painter->drawLine(widget->mapToViewport(cur_pos_map + 0.001f * preview_point_radius * forward_vector) + helper_cross_radius * close_vector.toQPointF(),
							  widget->mapToViewport(cur_pos_map + 0.001f * preview_point_radius * forward_vector) - helper_cross_radius * close_vector.toQPointF());
			painter->drawLine(widget->mapToViewport(cur_pos_map - 0.001f * preview_point_radius * forward_vector) + helper_cross_radius * close_vector.toQPointF(),
							  widget->mapToViewport(cur_pos_map - 0.001f * preview_point_radius * forward_vector) - helper_cross_radius * close_vector.toQPointF());
		}
	}
	else if (draw_in_progress && !second_point_set && angle_helper->isActive())
		angle_helper->draw(painter, widget);
}

void DrawRectangleTool::finishRectangleDrawing()
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
	constrained_pos_map = cur_pos_map;
	if (!second_point_set)
		angle_helper->getConstrainedCursorPosMap(cur_pos_map, constrained_pos_map);
	
	if (!second_point_set)
	{
		close_vector = click_pos_map - constrained_pos_map;
		close_vector.normalize();
		forward_vector = close_vector;
		forward_vector.perpRight();
		
		MapCoord coord = constrained_pos_map.toMapCoord();
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
		
		float forward_dist = forward_vector.dot(constrained_pos_map - MapCoordF(preview_path->getCoordinate(last_coord - 2)));
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
	updateDirtyRect();
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
void DrawRectangleTool::updateDirtyRect()
{
	QRectF rect;
	includePreviewRects(rect);
	
	if (is_helper_tool)
		emit(dirtyRectChanged(rect));
	else
	{
		if (rect.isValid())
		{
			int helper_cross_radius = Settings::getInstance().getSettingCached(Settings::RectangleTool_HelperCrossRadius).toInt();
			int pixel_border = 0;
			if (draw_in_progress)
				pixel_border = helper_cross_radius;	// helper_cross_radius as border is less than ideal but the only way to always ensure visibility of the helper cross at the moment
			else if (draw_in_progress && !second_point_set && angle_helper->isActive())
				pixel_border = qMax(helper_cross_radius, angle_helper->getDisplayRadius());
			editor->getMap()->setDrawingBoundingBox(rect, pixel_border, true);
		}
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
		text += tr("<b>Click or Drag</b> to start drawing a rectangle  (<u>Ctrl</u> for fixed angles)");
	else
		text += tr("<b>Click</b> to set a corner point, <b>Right or double click</b> to finish the rectangle, <b>Backspace</b> to undo, <b>Esc</b> to abort");
	
	text += ", " + tr("<b>Space</b> to toggle dash points");
	
	setStatusBarText(text);
}
