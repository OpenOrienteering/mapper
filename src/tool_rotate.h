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


#ifndef _OPENORIENTEERING_TOOL_ROTATE_H_
#define _OPENORIENTEERING_TOOL_ROTATE_H_

#include "map_editor.h"
#include "object.h"

/// Tool to rotate objects
class RotateTool : public MapEditorTool
{
Q_OBJECT
public:
	RotateTool(MapEditorController* editor, QAction* tool_button);
	virtual ~RotateTool();
	
    virtual void init();
    virtual QCursor* getCursor() {return cursor;}
    
    virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
    virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
    virtual void draw(QPainter* painter, MapWidget* widget);
	
	static QCursor* cursor;
	
protected:
	void updateStatusText();
	void updatePreviewObjects();
	void updateDirtyRect();
	void updateDragging(MapCoordF cursor_pos_map);
	
	// Mouse handling
	QPoint click_pos;
	MapCoordF click_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	bool dragging;
	
	bool rotation_center_set;
	MapCoordF rotation_center;
	bool rotating;
	double old_rotation;
	
	std::vector<Object*> undo_duplicates;
	RenderableVector old_renderables;
	RenderableContainer renderables;
};

#endif
