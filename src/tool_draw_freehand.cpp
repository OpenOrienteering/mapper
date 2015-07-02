/*
 *    Copyright 2014 Thomas Schöps
 *    Copyright 2014, 2015 Kai Pastor
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


#include "tool_draw_freehand.h"

#include <QKeyEvent>
#include <QMouseEvent>

#include "map.h"
#include "object.h"
#include "util.h"
#include "gui/modifier_key.h"
#include "gui/widgets/key_button_bar.h"
#include "map_editor.h"

QCursor* DrawFreehandTool::cursor = NULL;

DrawFreehandTool::DrawFreehandTool(MapEditorController* editor, QAction* tool_button, bool is_helper_tool)
: DrawLineAndAreaTool(editor, DrawFreehand, tool_button, is_helper_tool)
{
	dragging = false;
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-draw-path.png"), 11, 11);
}

DrawFreehandTool::~DrawFreehandTool()
{
}

void DrawFreehandTool::init()
{
	updateStatusText();
	
	MapEditorTool::init();
}

bool DrawFreehandTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	if (event->button() == Qt::LeftButton && !editingInProgress())
	{
		cur_pos = event->pos();
		cur_pos_map = map_coord;
		
		click_pos = event->pos();
		dragging = false;
		startDrawing();
		updatePath();
		
		hidePreviewPoints();
		return true;
	}
	return false;
}

bool DrawFreehandTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	bool mouse_down = containsDrawingButtons(event->buttons());
	
	if (!mouse_down)
	{
		if (!editingInProgress())
		{
			setPreviewPointsPosition(map_coord);
			setDirtyRect();
		}
	}
	else
	{
		if (!editingInProgress())
			return false;
		
		dragging = true;
		cur_pos = event->pos();
		cur_pos_map = map_coord;
		updatePath();
	}
	return true;
}

bool DrawFreehandTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	if (!editingInProgress())
		return false;
	
	if (!dragging)
	{
		abortDrawing();
		return true;
	}
	else if (dragging && event->button() == Qt::LeftButton)
	{
		cur_pos = event->pos();
		cur_pos_map = map_coord;
		updatePath();
		finishDrawing();
		return true;
	}
	return false;
}

bool DrawFreehandTool::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape)
		abortDrawing();
	else if (event->key() == Qt::Key_Tab)
	{
		deactivate();
		return true;
	}
	return false;
}

void DrawFreehandTool::draw(QPainter* painter, MapWidget* widget)
{
	drawPreviewObjects(painter, widget);
}

void DrawFreehandTool::finishDrawing()
{
	dragging = false;
	updateStatusText();
	
	// Clean up path: remove superfluous points
	if (preview_path->getCoordinateCount() > 2)
	{
		std::vector<bool> point_mask(preview_path->getCoordinateCount(), false);
		point_mask[0] = true;
		point_mask[point_mask.size() - 1] = true;
		checkLineSegment(0, point_mask.size() - 1, point_mask);
		
		for (int i = (int)(point_mask.size() - 2); i > 0; -- i)
		{
			if (! point_mask[i])
				preview_path->deleteCoordinate(i, false);
		}
	}
	
	DrawLineAndAreaTool::finishDrawing();
	// Do not add stuff here as the tool might get deleted in DrawLineAndAreaTool::finishDrawing()!
}
void DrawFreehandTool::abortDrawing()
{
	dragging = false;
	updateStatusText();
	
	DrawLineAndAreaTool::abortDrawing();
}

void DrawFreehandTool::checkLineSegment(int a, int b, std::vector< bool >& point_mask)
{
	if (b <= a + 1)
		return;
	
	const MapCoord& start_coord = preview_path->getRawCoordinateVector()[a];
	const MapCoord& end_coord = preview_path->getRawCoordinateVector()[b];
	
	// Find point between a and b with highest distance from line segment
	float max_distance_sq = -1;
	int best_index = a + 1;
	for (int i = a + 1; i < b; ++ i)
	{
		const MapCoord& coord = preview_path->getRawCoordinateVector()[i];
		
		MapCoordF to_coord = MapCoordF(coord.x() - start_coord.x(), coord.y() - start_coord.y());
		MapCoordF to_next = MapCoordF(end_coord.x() - start_coord.x(), end_coord.y() - start_coord.y());
		MapCoordF tangent = to_next;
		tangent.normalize();
		
		float distance_sq;
		
		float dist_along_line = MapCoordF::dotProduct(to_coord, tangent);
		if (dist_along_line <= 0)
		{
			distance_sq = to_coord.lengthSquared();
		}
		else
		{
			float line_length = MapCoordF(end_coord).distanceTo(MapCoordF(start_coord));
			if (dist_along_line >= line_length)
			{
				distance_sq = MapCoordF(coord).distanceSquaredTo(MapCoordF(end_coord));
			}
			else
			{
				auto right = tangent.perpRight();
				distance_sq = qAbs(MapCoordF::dotProduct(right, to_coord));
				distance_sq = distance_sq*distance_sq;
			}
		}
		
		if (distance_sq > max_distance_sq)
		{
			max_distance_sq = distance_sq;
			best_index = i;
		}
	}
	
	// Make new segment?
	const float split_distance_sq = 0.09f*0.09f;
	
	if (max_distance_sq > split_distance_sq)
	{
		point_mask[best_index] = true;
		checkLineSegment(a, best_index, point_mask);
		checkLineSegment(best_index, b, point_mask);
	}
}

void DrawFreehandTool::updatePath()
{
	float length_threshold_sq = 0.06f*0.06f; // minimum point distance in mm
	
	if (!dragging)
	{
		preview_path->clearCoordinates();
		preview_path->addCoordinate(MapCoord(cur_pos_map));
	}
	else
	{
		if (last_pos_map.distanceSquaredTo(cur_pos_map) < length_threshold_sq)
			return;
		preview_path->addCoordinate(MapCoord(cur_pos_map));
	}
	last_pos_map = cur_pos_map;
	
	updatePreviewPath();
	setDirtyRect();
}

void DrawFreehandTool::setDirtyRect()
{
	QRectF rect;
	includePreviewRects(rect);
	
	if (is_helper_tool)
		emit(dirtyRectChanged(rect));
	else
	{
		if (rect.isValid())
			map()->setDrawingBoundingBox(rect, 0, true);
		else
			map()->clearDrawingBoundingBox();
	}
}

void DrawFreehandTool::updateStatusText()
{
	QString text;
	text = tr("<b>Drag</b>: Draw a path. ") +
			MapEditorTool::tr("<b>%1</b>: Abort. ").arg(ModifierKey::escape());
	setStatusBarText(text);
}
