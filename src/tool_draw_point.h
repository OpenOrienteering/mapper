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

#include <QScopedPointer>

#include "tool.h"
#include "tool_helpers.h"

class MapWidget;
class PointObject;
class Symbol;
class SymbolWidget;
class ConstrainAngleToolHelper;

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
    virtual bool keyReleaseEvent(QKeyEvent* event);
	
	virtual void draw(QPainter* painter, MapWidget* widget);
	
	static QCursor* cursor;
	
protected slots:
	void selectedSymbolsChanged();
	void symbolDeleted(int pos, Symbol* old_symbol);
	void updateDirtyRect();
	
protected:
	float calculateRotation(QPointF mouse_pos, MapCoordF mouse_pos_map);
	void updateDragging();
	void updateStatusText();
	
	QPoint click_pos;
	MapCoordF click_pos_map;
	QPointF cur_pos;
	MapCoordF cur_pos_map;
	QPointF constrained_pos;
	MapCoordF constrained_pos_map;
	bool dragging;
	QScopedPointer<ConstrainAngleToolHelper> angle_helper;
	
	Symbol* last_used_symbol;
	PointObject* preview_object;
	QScopedPointer<MapRenderables> renderables;
	SymbolWidget* symbol_widget;
	MapWidget* cur_map_widget;
};

#endif
