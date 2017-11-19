/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014 Kai Pastor
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


#include "pan_tool.h"

#include <Qt>
#include <QCursor>
#include <QPoint>

#include "core/map_view.h"
#include "gui/map/map_widget.h"
#include "tools/tool.h"


PanTool::PanTool(MapEditorController* editor, QAction* tool_action)
 : MapEditorToolBase(QCursor(Qt::OpenHandCursor), Pan, editor, tool_action)
{
	useTouchCursor(false);
}

PanTool::~PanTool() = default;



void PanTool::clickPress()
{
	startDragging();
}

void PanTool::dragStart()
{
	cur_map_widget->setCursor(Qt::ClosedHandCursor);
}

void PanTool::dragMove()
{
	MapView* view = mapWidget()->getMapView();
	view->setPanOffset(cur_pos - click_pos);
}

void PanTool::dragFinish()
{
	MapView* view = mapWidget()->getMapView();
	view->finishPanning(cur_pos - click_pos);
	
	cur_map_widget->setCursor(Qt::OpenHandCursor);
}

void PanTool::dragCanceled()
{
	// finalize current panning
	MapView* view = mapWidget()->getMapView();
	view->finishPanning(view->panOffset());
	
	cur_map_widget->setCursor(Qt::OpenHandCursor);
}

void PanTool::updateStatusText()
{
	setStatusBarText(tr("<b>Drag</b>: Move the map. "));
}

void PanTool::objectSelectionChangedImpl()
{
	// Nothing
}
