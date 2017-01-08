/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#ifndef _OPENORIENTEERING_TEMPLATE_TOOL_MOVE_H_
#define _OPENORIENTEERING_TEMPLATE_TOOL_MOVE_H_

#include "tools/tool.h"

class Template;

/** Tool to move a template by hand. */
class TemplateMoveTool : public MapEditorTool
{
Q_OBJECT
public:
	TemplateMoveTool(Template* templ, MapEditorController* editor, QAction* toolAction = NULL);
	
	virtual void init();
	virtual const QCursor& getCursor() const;
	
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
public slots:
	void templateDeleted(int index, const Template* temp);
	
private:
	void updateDragging(MapCoordF mouse_pos_map);
	
	Template* templ;
	bool dragging;
	MapCoordF click_pos_map;
};

#endif
