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


#ifndef _OPENORIENTEERING_DRAW_PATH_H_
#define _OPENORIENTEERING_DRAW_PATH_H_

#include "map_editor.h"

class CombinedSymbol;
class PointObject;
class PathObject;
class Symbol;

/// Tool to draw path objects
class DrawPathTool : public MapEditorTool
{
Q_OBJECT
public:
	DrawPathTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget);
	virtual ~DrawPathTool();
	
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
	
protected slots:
	void selectedSymbolsChanged();
	void symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol);
	void symbolDeleted(int pos, Symbol* old_symbol);
	
protected:
	void createPreviewCurve(MapCoord position, float direction);
	void closeDrawing();
	void finishDrawing();
	void abortDrawing();
	void undoLastPoint();
	
	void setDirtyRect(MapCoordF mouse_pos);
	void includePreviewRects(QRectF& rect);
	float calculateRotation(QPoint mouse_pos, MapCoordF mouse_pos_map);
	void updateStatusText();
	
	void updatePreviewPath();
	
	void hidePreviewPoints();
	void deleteObjects();
	void addPreviewSymbols(Symbol* symbol);
	
	QPoint click_pos;
	MapCoordF click_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	MapCoordF previous_pos_map;
	MapCoordF previous_drag_map;
	bool dragging;
	
	bool draw_in_progress;
	bool path_has_preview_point;
	bool previous_point_is_curve_point;
	float previous_point_direction;
	bool create_segment;
	
	bool space_pressed;
	
	std::vector<PointSymbol*> preview_point_symbols;
	std::vector<bool> preview_point_symbols_external;
	std::vector<PointObject*> preview_points;
	bool preview_points_shown;
	
	CombinedSymbol* path_combination;
	Symbol* drawing_symbol;
	PathObject* preview_path;
	
	Symbol* last_used_symbol;
	RenderableContainer renderables;
	SymbolWidget* symbol_widget;
};

#endif
