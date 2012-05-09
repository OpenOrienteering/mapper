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

class DrawPathTool;

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
	void pathFinished(PathObject* split_path);
	
protected:
	void updateStatusText();
	void updatePreviewObjects();
	void deletePreviewPath();
	void updateDirtyRect(const QRectF* path_rect = NULL);
	void updateDragging(MapCoordF cursor_pos_map, MapWidget* widget);
	void updateHoverPoint(QPointF cursor_pos_screen, MapWidget* widget);
	bool findEditPoint(PathCoord& out_edit_point, PathObject*& out_edit_object, MapCoordF cursor_pos_map, int with_type, int without_type, MapWidget* widget);
	
	void startCuttingArea(const PathCoord& coord, MapWidget* widget);
	
	// Mouse handling
	QPoint click_pos;
	MapCoordF click_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	bool dragging;
	
	int hover_point;
	Object* hover_object;
	PathObject* edit_object;
	
	// For removing segments from lines
	int drag_part_index;
	float drag_start_len;
	float drag_end_len;
	bool drag_forward;		// true if [drag_start_len; drag_end_len] is the drag range, else [drag_end_len; drag_start_len]
	bool dragging_on_line;
	
	// For cutting areas
	bool cutting_area;
	DrawPathTool* path_tool;
	MapWidget* edit_widget;
	
	// Preview objects for dragging
	PathObject* preview_path;
	RenderableContainer renderables;
};

#endif
