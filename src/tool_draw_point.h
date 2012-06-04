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


#ifndef _OPENORIENTEERING_DRAW_POINT_H_
#define _OPENORIENTEERING_DRAW_POINT_H_

#include "map_editor.h"

#include <QScopedPointer>

class PointObject;

/// Tool to draw point objects
class DrawPointTool : public MapEditorTool
{
Q_OBJECT
public:
	DrawPointTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget);
    virtual ~DrawPointTool();
	
    virtual void init();
    virtual QCursor* getCursor() {return cursor;}
    
    virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
    virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
    virtual void leaveEvent(QEvent* event);
    virtual bool keyPressEvent(QKeyEvent* event);
	
    virtual void draw(QPainter* painter, MapWidget* widget);
	
	static QCursor* cursor;
	
protected slots:
	void selectedSymbolsChanged();
	void symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol);
	void symbolDeleted(int pos, Symbol* old_symbol);
	
protected:
	void setDirtyRect(MapCoordF mouse_pos);
	float calculateRotation(QPoint mouse_pos, MapCoordF mouse_pos_map);
	
	QPoint click_pos;
	MapCoordF click_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	bool dragging;
	
	Symbol* last_used_symbol;
	PointObject* preview_object;
	QScopedPointer<MapRenderables> renderables;
	SymbolWidget* symbol_widget;
};

#endif
