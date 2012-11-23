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


#include "tool_pan.h"

#include "map_editor.h"
#include "map_widget.h"

PanTool::PanTool(MapEditorController* editor, QAction* tool_button)
 : MapEditorToolBase(QCursor(Qt::SizeAllCursor), Other, editor, tool_button)
{
}

void PanTool::dragStart()
{
}
void PanTool::dragMove()
{
	MapView* view = editor->getMainWidget()->getMapView();
	view->setDragOffset(cur_pos - click_pos);
}
void PanTool::dragFinish()
{
	MapView* view = editor->getMainWidget()->getMapView();
	view->completeDragging(cur_pos - click_pos);
}

void PanTool::updateStatusText()
{
	setStatusBarText(tr("<b>Drag</b> to move the map"));
}
void PanTool::objectSelectionChangedImpl()
{
}
