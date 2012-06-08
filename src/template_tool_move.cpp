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


#include "template_tool_move.h"

#include <QMouseEvent>

#include "template.h"

QCursor* TemplateMoveTool::cursor = NULL;

TemplateMoveTool::TemplateMoveTool(Template* templ, MapEditorController* editor, QAction* action) : MapEditorTool(editor, Other, action), templ(templ)
{
	dragging = false;
	
	if (!cursor)
		cursor = new QCursor(Qt::SizeAllCursor);
}

void TemplateMoveTool::init()
{
	setStatusBarText(tr("<b>Drag</b> to move the current template"));
	
	connect(editor->getMap(), SIGNAL(templateDeleted(int,Template*)), this, SLOT(templateDeleted(int,Template*)));
}

bool TemplateMoveTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	dragging = true;
	click_pos_map = map_coord;
	return true;
}
bool TemplateMoveTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (!(event->buttons() & Qt::LeftButton) || !dragging)
		return false;
	
	updateDragging(map_coord);
	return true;
}
bool TemplateMoveTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton || !dragging)
		return false;
	
	updateDragging(map_coord);
	dragging = false;
	return true;
}

void TemplateMoveTool::templateDeleted(int index, Template* temp)
{
	if (templ == temp)
		editor->setTool(NULL);
}

void TemplateMoveTool::updateDragging(MapCoordF mouse_pos_map)
{
	qint64 dx = qRound64(1000 * (mouse_pos_map.getX() - click_pos_map.getX()));
	qint64 dy = qRound64(1000 * (mouse_pos_map.getY() - click_pos_map.getY()));
	click_pos_map = mouse_pos_map;
	
	templ->setTemplateAreaDirty();
	templ->setTemplateX(templ->getTemplateX() + dx);
	templ->setTemplateY(templ->getTemplateY() + dy);
	templ->setTemplateAreaDirty();
	
	for (int i = 0; i < templ->getNumPassPoints(); ++i)
	{
		Template::PassPoint* point = templ->getPassPoint(i);
		
		if (templ->isAdjustmentApplied())
		{
			point->dest_coords_map.moveInt(dx, dy);
			point->calculated_coords_map.moveInt(dx, dy);
		}
		else
			point->src_coords_map.moveInt(dx, dy);
	}
	
	editor->getMap()->setTemplatesDirty();
	editor->getMap()->emitTemplateChanged(templ);
}
