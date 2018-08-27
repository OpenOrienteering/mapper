/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2015, 2017 Kai Pastor
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

#include <Qt>
#include <QCursor>
#include <QFlags>
#include <QMouseEvent>

#include "core/map.h"
#include "templates/template.h"
#include "util/transformation.h"


namespace OpenOrienteering {

TemplateMoveTool::TemplateMoveTool(Template* templ, MapEditorController* editor, QAction* tool_action)
: MapEditorTool(editor, Other, tool_action), templ(templ)
{
	// nothing else
}

TemplateMoveTool::~TemplateMoveTool() = default;



void TemplateMoveTool::init()
{
	setStatusBarText(tr("<b>Drag</b> to move the current template"));
	
	connect(map(), &Map::templateDeleted, this, &TemplateMoveTool::templateDeleted);
	
	MapEditorTool::init();
}

const QCursor& TemplateMoveTool::getCursor() const
{
	static auto const cursor = QCursor(Qt::SizeAllCursor);
	return cursor;
}

bool TemplateMoveTool::mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	if (event->button() != Qt::LeftButton)
		return false;
	
	dragging = true;
	click_pos_map = map_coord;
	return true;
}
bool TemplateMoveTool::mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	if (!(event->buttons() & Qt::LeftButton) || !dragging)
		return false;
	
	updateDragging(map_coord);
	return true;
}
bool TemplateMoveTool::mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	if (event->button() != Qt::LeftButton || !dragging)
		return false;
	
	updateDragging(map_coord);
	dragging = false;
	return true;
}

void TemplateMoveTool::templateDeleted(int index, const Template* temp)
{
	Q_UNUSED(index);
	if (templ == temp)
		deactivate();
}

void TemplateMoveTool::updateDragging(const MapCoordF& mouse_pos_map)
{
	auto move = MapCoord { mouse_pos_map - click_pos_map };
	click_pos_map = mouse_pos_map;
	
	templ->setTemplateAreaDirty();
	templ->setTemplatePosition(templ->templatePosition() + move);
	templ->setTemplateAreaDirty();
	
	for (int i = 0; i < templ->getNumPassPoints(); ++i)
	{
		PassPoint* point = templ->getPassPoint(i);
		auto move_f = MapCoordF { move };
		
		if (templ->isAdjustmentApplied())
		{
			point->dest_coords += move_f;
			point->calculated_coords += move_f;
		}
		else
		{
			point->src_coords += move_f;
		}
	}
	
	map()->setTemplatesDirty();
	map()->emitTemplateChanged(templ);
}


}  // namespace OpenOrienteering
