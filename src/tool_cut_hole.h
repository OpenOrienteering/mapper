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


#ifndef _OPENORIENTEERING_TOOL_CUT_HOLE_H_
#define _OPENORIENTEERING_TOOL_CUT_HOLE_H_

#include "map_editor.h"
#include "object.h"

class DrawPathTool;

/// Tool to cut holes into area objects
class CutHoleTool : public MapEditorTool
{
Q_OBJECT
public:
	CutHoleTool(MapEditorController* editor, QAction* tool_button);
	virtual ~CutHoleTool();
	
    virtual void init();
    virtual QCursor* getCursor() {return cursor;}
    
    virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
    virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseDoubleClickEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual void leaveEvent(QEvent* event);
	
	virtual bool keyPressEvent(QKeyEvent* event);
	virtual bool keyReleaseEvent(QKeyEvent* event);
	virtual void focusOutEvent(QFocusEvent* event);
	
    virtual void draw(QPainter* painter, MapWidget* widget);
	
	static QCursor* cursor;
	
public slots:
	void objectSelectionChanged();
	void pathDirtyRectChanged(const QRectF& rect);
	void pathAborted();
	void pathFinished(PathObject* hole_path);
	
protected:
	void updateStatusText();
	void updateDirtyRect(const QRectF* path_rect = NULL);
	void updateDragging(MapCoordF cursor_pos_map, MapWidget* widget);
	
	// Mouse handling
	/*QPoint click_pos;
	MapCoordF click_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	bool dragging;*/
	
	DrawPathTool* path_tool;	// TODO: make it possible to use other draw tools, too (circles or ellipses)
	MapWidget* edit_widget;
	
	// Preview objects for dragging
	//PathObject* preview_path;
	//RenderableContainer renderables;
};

#endif
