/*
 *    Copyright 2014 Thomas Schöps
 *    Copyright 2014-2017 Kai Pastor
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


#include "draw_freehand_tool.h"

#include <Qt>
#include <QCursor>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPixmap>
#include <QRectF>
#include <QString>

#include "core/map.h"
#include "core/map_view.h"
#include "core/objects/object.h"
#include "gui/modifier_key.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "tools/tool.h"


namespace OpenOrienteering {

DrawFreehandTool::DrawFreehandTool(MapEditorController* editor, QAction* tool_action, bool is_helper_tool)
: DrawLineAndAreaTool(editor, DrawFreehand, tool_action, is_helper_tool)
{
	// nothing else
}


DrawFreehandTool::~DrawFreehandTool() = default;


void DrawFreehandTool::init()
{
	updateStatusText();
	
	MapEditorTool::init();
}


const QCursor& DrawFreehandTool::getCursor() const
{
	static auto const cursor = scaledToScreen(QCursor{ QPixmap(QString::fromLatin1(":/images/cursor-draw-path.png")), 11, 11 });
	return cursor;
}



bool DrawFreehandTool::mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	if (event->button() == Qt::LeftButton && !editingInProgress())
	{
		last_pos = cur_pos = event->pos();
		cur_pos_map = map_coord;
		
		startDrawing();
		preview_path->addCoordinate(MapCoord(cur_pos_map));
		hidePreviewPoints();
		return true;
	}
	
	return false;
}


bool DrawFreehandTool::mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	if (editingInProgress())
	{
		cur_pos = event->pos();
		cur_pos_map = map_coord;
		updatePath();
		return true;
	}
	
	setPreviewPointsPosition(map_coord);
	setDirtyRect();
	return false;
}


bool DrawFreehandTool::mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	if (event->button() == Qt::LeftButton && editingInProgress())
	{	
		if (preview_path->getCoordinateCount() < 2)
		{
			abortDrawing();
			return true;
		}
		
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
	switch (event->key())
	{
	case Qt::Key_Escape:
		abortDrawing();
		return true;
		
	case Qt::Key_Tab:
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
	updateStatusText();
	
	// Clean up path: remove superfluous points
	if (preview_path->getCoordinateCount() > 2)
	{
		// Use 999 instead of 1000, and save the rounding.
		split_distance_sq = editor->getMainWidget()->getMapView()->pixelToLength(1) / 999;
		split_distance_sq *= split_distance_sq;
		
		point_mask.assign(preview_path->getCoordinateCount(), false);
		point_mask.front() = true;
		point_mask.back() = true;
		checkLineSegment(0, point_mask.size() - 1);
		
		for (auto i = point_mask.size() - 1; i != 0; )
		{
			--i;
			if (!point_mask[i])
				preview_path->deleteCoordinate(i, false);
		}
	}
	
	DrawLineAndAreaTool::finishDrawing();
	// Do not add stuff here as the tool might get deleted in DrawLineAndAreaTool::finishDrawing()!
}


void DrawFreehandTool::abortDrawing()
{
	updateStatusText();
	
	DrawLineAndAreaTool::abortDrawing();
}



void DrawFreehandTool::checkLineSegment(std::size_t first, std::size_t last)
{
	if (last <= first + 1)
		return;
	
	// 'best_index' indicates the point on the path between first and last
	// with the highest distance from the direct line between first and last.
	auto best_index = first;
	
	// This block ensures the stack is cleaned up before going into recursion.
	{
		auto max_distance_sq = qreal(0);
		const auto start_coord = MapCoordF(preview_path->getRawCoordinateVector()[first]);
		const auto end_coord = MapCoordF(preview_path->getRawCoordinateVector()[last]);
		auto tangent = end_coord - start_coord;
		tangent.normalize();
		
		for (auto i = first + 1; i < last; ++i)
		{
			const auto coord = MapCoordF(preview_path->getRawCoordinateVector()[i]);
			const auto to_coord = coord - start_coord;
			
			qreal distance_sq;
			
			auto dist_along_line = MapCoordF::dotProduct(to_coord, tangent);
			if (dist_along_line <= 0)
			{
				distance_sq = to_coord.lengthSquared();
			}
			else if (dist_along_line >= end_coord.distanceTo(start_coord))
			{
				distance_sq = coord.distanceSquaredTo(end_coord);
			}
			else
			{
				auto right = tangent.perpRight();
				distance_sq = qAbs(MapCoordF::dotProduct(right, to_coord));
				distance_sq = distance_sq*distance_sq;
			}
			
			if (distance_sq > max_distance_sq)
			{
				max_distance_sq = distance_sq;
				best_index = i;
			}
		}
		
		// Make new segment?
		if (max_distance_sq < split_distance_sq)
			return;
	}		
	
	point_mask[best_index] = true;
	checkLineSegment(first, best_index);
	checkLineSegment(best_index, last);
}



void DrawFreehandTool::updatePath()
{
	if ((last_pos - cur_pos).manhattanLength() <= 2)
		return;
	
	preview_path->addCoordinate(MapCoord(cur_pos_map));
	last_pos = cur_pos;
	
	updatePreviewPath();
	setDirtyRect();
}



void DrawFreehandTool::setDirtyRect()
{
	QRectF rect;
	includePreviewRects(rect);
	
	if (is_helper_tool)
	{
		emit dirtyRectChanged(rect);
	}
	else if (rect.isValid())
	{
		map()->setDrawingBoundingBox(rect, 0, true);
	}
	else
	{
		map()->clearDrawingBoundingBox();
	}
}



void DrawFreehandTool::updateStatusText()
{
	QString text;
	text = tr("<b>Drag</b>: Draw a path. ") +
			OpenOrienteering::MapEditorTool::tr("<b>%1</b>: Abort. ").arg(ModifierKey::escape());
	setStatusBarText(text);
}


}  // namespace OpenOrienteering
