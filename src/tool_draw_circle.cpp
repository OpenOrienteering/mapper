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


#include "tool_draw_circle.h"

#include <QtGui>

#include "util.h"
#include "object.h"

QCursor* DrawCircleTool::cursor = NULL;

DrawCircleTool::DrawCircleTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget)
 : DrawLineAndAreaTool(editor, tool_button, symbol_widget)
{
	dragging = false;
	first_point_set = false;
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-draw-circle.png"), 11, 11);
}

void DrawCircleTool::init()
{
	updateStatusText();
}

bool DrawCircleTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->buttons() & (Qt::LeftButton | Qt::RightButton))
	{
		hidePreviewPoints();
		
		cur_pos = event->pos();
		cur_pos_map = map_coord;
		
		if (!first_point_set && event->buttons() & Qt::LeftButton)
		{
			click_pos = event->pos();
			click_pos_map = map_coord;
			dragging = false;
			first_point_set = true;
			
			if (!draw_in_progress)
				startDrawing();
		}
		else if (first_point_set)
		{
			updateCircle();
			finishDrawing();
		}
		else
			return false;
		return true;
	}
	return false;
}
bool DrawCircleTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	bool mouse_down = event->buttons() & Qt::LeftButton;
	
	if (!mouse_down)
	{
		if (!draw_in_progress)
		{
			setPreviewPointsPosition(map_coord);
			setDirtyRect();
		}
		else
		{
			cur_pos = event->pos();
			cur_pos_map = map_coord;
			updateCircle();
		}
	}
	else
	{
		if (!draw_in_progress)
			return false;
		
		if ((event->pos() - click_pos).manhattanLength() >= QApplication::startDragDistance())
			dragging = true;
		
		if (dragging)
		{
			cur_pos = event->pos();
			cur_pos_map = map_coord;
			updateCircle();
		}
	}
	return true;
}
bool DrawCircleTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	if (!draw_in_progress)
		return false;
	
	if (dragging)
	{
		cur_pos = event->pos();
		cur_pos_map = map_coord;
		updateCircle();
		finishDrawing();
		return true;
	}
	return false;
}

bool DrawCircleTool::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape)
		abortDrawing();
	else if (event->key() == Qt::Key_Tab)
	{
		editor->setEditTool();
		return true;
	}
	return false;
}

void DrawCircleTool::draw(QPainter* painter, MapWidget* widget)
{
	drawPreviewObjects(painter, widget);
}

void DrawCircleTool::finishDrawing()
{
	dragging = false;
	first_point_set = false;
	updateStatusText();
	
	DrawLineAndAreaTool::finishDrawing();
}
void DrawCircleTool::abortDrawing()
{
	dragging = false;
	first_point_set = false;
	updateStatusText();
	
	DrawLineAndAreaTool::abortDrawing();
}

void DrawCircleTool::updateCircle()
{
	float radius = 0.5f * click_pos_map.lengthTo(cur_pos_map);
	float kappa = BEZIER_KAPPA;
	float m_kappa = 1 - BEZIER_KAPPA;
	
	MapCoordF across = cur_pos_map - click_pos_map;
	across.setLength(radius);
	MapCoordF right = across;
	right.perpRight();
	right.setLength(radius);
	
	preview_path->clearCoordinates();
	preview_path->addCoordinate(click_pos_map.toCurveStartMapCoord());
	preview_path->addCoordinate((click_pos_map + kappa * right).toMapCoord());
	preview_path->addCoordinate((click_pos_map + right + m_kappa * across).toMapCoord());
	preview_path->addCoordinate((click_pos_map + right + across).toCurveStartMapCoord());
	preview_path->addCoordinate((click_pos_map + right + (1 + kappa) * across).toMapCoord());
	preview_path->addCoordinate((click_pos_map + kappa * right + 2 * across).toMapCoord());
	preview_path->addCoordinate((click_pos_map + 2 * across).toCurveStartMapCoord());
	preview_path->addCoordinate((click_pos_map - kappa * right + 2 * across).toMapCoord());
	preview_path->addCoordinate((click_pos_map - right + (1 + kappa) * across).toMapCoord());
	preview_path->addCoordinate((click_pos_map - right + across).toCurveStartMapCoord());
	preview_path->addCoordinate((click_pos_map - right + m_kappa * across).toMapCoord());
	preview_path->addCoordinate((click_pos_map - kappa * right).toMapCoord());
	preview_path->getPart(0).setClosed(true, false);
	
	updatePreviewPath();
	setDirtyRect();
}
void DrawCircleTool::setDirtyRect()
{
	QRectF rect;
	includePreviewRects(rect);
	
	if (is_helper_tool)
		emit(dirtyRectChanged(rect));
	else
	{
		if (rect.isValid())
			editor->getMap()->setDrawingBoundingBox(rect, 0, true);
		else
			editor->getMap()->clearDrawingBoundingBox();
	}
}
void DrawCircleTool::updateStatusText()
{
	setStatusBarText(tr("<b>Click</b> or <b>Drag</b> to draw a circle, <b>Esc</b> to abort"));
}
