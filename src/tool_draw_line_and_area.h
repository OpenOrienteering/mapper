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


#ifndef _OPENORIENTEERING_DRAW_LINE_AND_AREA_H_
#define _OPENORIENTEERING_DRAW_LINE_AND_AREA_H_

#include "tool.h"

#include <QScopedPointer>

class CombinedSymbol;
class PointObject;
class PathObject;
class MapRenderables;
class Symbol;
class PointSymbol;
class SymbolWidget;

/// Base class for drawing tools for line and area symbols.
/// Provides some common functionality like for example displaying the preview objects.
class DrawLineAndAreaTool : public MapEditorTool
{
Q_OBJECT
public:
	DrawLineAndAreaTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget);
	virtual ~DrawLineAndAreaTool();
	
	virtual void leaveEvent(QEvent* event);
	
signals:
	void dirtyRectChanged(const QRectF& rect);
	void pathAborted();
	void pathFinished(PathObject* path);
	
protected slots:
	void selectedSymbolsChanged();
	void symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol);
	void symbolDeleted(int pos, Symbol* old_symbol);
	
protected:
	void createPreviewPoints();
	void addPreviewPointSymbols(Symbol* symbol);
	void setPreviewPointsPosition(MapCoordF map_coord, int points_index = 0);
	void hidePreviewPoints();
	
	void startDrawing();
	void updatePreviewPath();
	virtual void abortDrawing();
	virtual void finishDrawing(PathObject* append_to_object = NULL);
	void deletePreviewObjects();
	
	void includePreviewRects(QRectF& rect);
	void drawPreviewObjects(QPainter* painter, MapWidget* widget);
	
	
	std::vector<PointSymbol*> preview_point_symbols;
	std::vector<bool> preview_point_symbols_external;
	std::vector<PointObject*> preview_points[2];
	int preview_point_radius;
	bool preview_points_shown;
	
	CombinedSymbol* path_combination;
	Symbol* drawing_symbol;
	PathObject* preview_path;
	bool draw_in_progress;
	
	Symbol* last_used_symbol;
	QScopedPointer<MapRenderables> renderables;
	SymbolWidget* symbol_widget;
	
	bool is_helper_tool;
};

#endif
