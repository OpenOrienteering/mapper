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


#ifndef _OPENORIENTEERING_TOOL_CUT_H_
#define _OPENORIENTEERING_TOOL_CUT_H_

#include "map_editor.h"
#include "object.h"

/// Tool to cut objects into smaller pieces
class CutTool : public MapEditorTool
{
Q_OBJECT
public:
	CutTool(MapEditorController* editor, QAction* tool_button);
	virtual ~CutTool();
	
    virtual void init();
    virtual QCursor* getCursor() {return cursor;}
    
    virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
    virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
    virtual void draw(QPainter* painter, MapWidget* widget);
	
	static QCursor* cursor;
	
public slots:
	void selectedObjectsChanged();
	
protected:
	void updateStatusText();
	void updatePreviewObjects();
	void updateDirtyRect();
	void updateDragging(MapCoordF cursor_pos_map);
	void updateHoverPoint(QPointF cursor_pos_screen, MapWidget* widget);
	
	// Mouse handling
	QPoint click_pos;
	MapCoordF click_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	bool dragging;
	
	int hover_point;
	Object* hover_object;
	
	RenderableContainer renderables;
};

#endif
