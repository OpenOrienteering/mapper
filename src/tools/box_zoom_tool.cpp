/*
 *    Copyright 2025 Andreas Bertheussen
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

#include "box_zoom_tool.h"

#include <Qt>
#include <QCursor>
#include <QPoint>
#include <QPainter>

#include "core/map.h"
#include "core/map_view.h"
#include "gui/map/map_widget.h"
#include "tools/tool.h"
#include "util/util.h"

namespace OpenOrienteering {

BoxZoomTool::BoxZoomTool(MapEditorController* editor, QAction* tool_action)
 : MapEditorToolBase( scaledToScreen(QCursor{ QPixmap(QString::fromLatin1(":/images/cursor-zoom.png")), 12, 12 }), BoxZoom, editor, tool_action)
{
	useTouchCursor(false);
}

BoxZoomTool::~BoxZoomTool() = default;

void BoxZoomTool::clickPress()
{
	startDragging();
}

void BoxZoomTool::dragStart()
{
	setEditingInProgress(true);	// suppress pie context menu
	selection_start_point = click_pos_map;
	updateDirtyRect();
}

void BoxZoomTool::dragMove()
{
	selection_end_point = cur_pos_map;

	// to create a valid rectangle, separate the coordinates into top left and bottom right points
	selection_top_left = MapCoordF(qMin(selection_start_point.x(),selection_end_point.x()), qMin(selection_start_point.y(),selection_end_point.y()));
	selection_bottom_right = MapCoordF(qMax(selection_start_point.x(),selection_end_point.x()), qMax(selection_start_point.y(),selection_end_point.y()));

	selection_rectangle = QRectF(selection_top_left, selection_bottom_right);
	updateDirtyRect();
}

void BoxZoomTool::dragFinish()
{
	if (selection_rectangle.isValid())
		mapWidget()->adjustViewToRect(selection_rectangle, MapWidget::ContinuousZoom);

	// invalidate selection rectangle since it has been used
	selection_rectangle = QRectF(0,0,0,0);
	updateDirtyRect();

	setEditingInProgress(false);
}

void BoxZoomTool::dragCanceled()
{
	selection_rectangle = QRectF(0,0,0,0);
	updateDirtyRect();

	setEditingInProgress(false);
}

void BoxZoomTool::updateStatusText()
{
	setStatusBarText(tr("<b>Drag</b>: Select area to zoom in on. "));
}

void BoxZoomTool::objectSelectionChangedImpl()
{

}

void BoxZoomTool::drawImpl(QPainter* painter, MapWidget* widget)
{
	drawSelectionOrPreviewObjects(painter, widget, false);

	if (selection_rectangle.isValid())
		drawSelectionBox(painter, widget, selection_top_left, selection_bottom_right);
}

int BoxZoomTool::updateDirtyRectImpl(QRectF& rect)
{
	rectIncludeSafe(rect, selection_rectangle);
	return 5;
}

}  // namespace OpenOrienteering
