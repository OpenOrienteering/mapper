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


#include "tool_draw_line_and_area.h"

#include <QPainter>

#include "util.h"
#include "renderable.h"
#include "symbol.h"
#include "symbol_dock_widget.h"
#include "symbol_point.h"
#include "symbol_line.h"
#include "symbol_combined.h"
#include "object.h"
#include "map_editor.h"
#include "map_widget.h"
#include "map_undo.h"

DrawLineAndAreaTool::DrawLineAndAreaTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget)
 : MapEditorTool(editor, Other, tool_button), renderables(new MapRenderables(editor->getMap())), symbol_widget(symbol_widget)
{
	preview_points_shown = false;
	draw_in_progress = false;
	path_combination = NULL;
	preview_path = NULL;
	last_used_symbol = NULL;
	is_helper_tool = symbol_widget == NULL;
	
	if (!is_helper_tool)
	{
		selectedSymbolsChanged();
		connect(symbol_widget, SIGNAL(selectedSymbolsChanged()), this, SLOT(selectedSymbolsChanged()));
		connect(editor->getMap(), SIGNAL(symbolChanged(int,Symbol*,Symbol*)), this, SLOT(symbolChanged(int,Symbol*,Symbol*)));
		connect(editor->getMap(), SIGNAL(symbolDeleted(int,Symbol*)), this, SLOT(symbolDeleted(int,Symbol*)));
	}
}
DrawLineAndAreaTool::~DrawLineAndAreaTool()
{
	deletePreviewObjects();
	delete path_combination;
}

void DrawLineAndAreaTool::leaveEvent(QEvent* event)
{
	if (!draw_in_progress)
		editor->getMap()->clearDrawingBoundingBox();
}

void DrawLineAndAreaTool::selectedSymbolsChanged()
{
	if (is_helper_tool)
		return;
	if (draw_in_progress)
		abortDrawing();
	
	Symbol* symbol = symbol_widget->getSingleSelectedSymbol();
	if (symbol == NULL || ((symbol->getType() & (Symbol::Line | Symbol::Area | Symbol::Combined)) == 0) || symbol->isHidden())
	{
		if (symbol && symbol->isHidden())
			editor->setEditTool();
		else
			editor->setTool(editor->getDefaultDrawToolForSymbol(symbol));
		return;
	}
	
	last_used_symbol = symbol;
	createPreviewPoints();
}
void DrawLineAndAreaTool::symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol)
{
	if (old_symbol == last_used_symbol)
		selectedSymbolsChanged();
}
void DrawLineAndAreaTool::symbolDeleted(int pos, Symbol* old_symbol)
{
	if (old_symbol == last_used_symbol)
		editor->setEditTool();
}

void DrawLineAndAreaTool::createPreviewPoints()
{
	deletePreviewObjects();
	addPreviewPointSymbols(last_used_symbol);
	
	// Create objects for the new symbols
	int size = (int)preview_point_symbols.size();
	preview_points.resize(size);
	for (int i = 0; i < size; ++i)
		preview_points[i] = new PointObject(preview_point_symbols[i]);
}
void DrawLineAndAreaTool::setPreviewPointsPosition(MapCoordF map_coord)
{
	int size = (int)preview_points.size();
	for (int i = 0; i < size; ++i)
	{
		if (preview_points_shown)
			renderables->removeRenderablesOfObject(preview_points[i], false);
		preview_points[i]->setPosition(map_coord);
		preview_points[i]->update(true);
		renderables->insertRenderablesOfObject(preview_points[i]);
	}
	preview_points_shown = true;
}
void DrawLineAndAreaTool::hidePreviewPoints()
{
	if (!preview_points_shown)
		return;
	
	int size = (int)preview_points.size();
	for (int i = 0; i < size; ++i)
		renderables->removeRenderablesOfObject(preview_points[i], false);
	
	preview_points_shown = false;
}

void DrawLineAndAreaTool::includePreviewRects(QRectF& rect)
{
	if (preview_path)
		rectIncludeSafe(rect, preview_path->getExtent());
	
	if (preview_points_shown)
	{
		int size = (int)preview_points.size();
		for (int i = 0; i < size; ++i)
			rectIncludeSafe(rect, preview_points[i]->getExtent());
	}
}
void DrawLineAndAreaTool::drawPreviewObjects(QPainter* painter, MapWidget* widget)
{
	if (preview_path || preview_points_shown)
	{
		painter->save();
		painter->translate(widget->width() / 2.0 + widget->getMapView()->getDragOffset().x(),
						   widget->height() / 2.0 + widget->getMapView()->getDragOffset().y());
		widget->getMapView()->applyTransform(painter);
		
		renderables->draw(painter, widget->getMapView()->calculateViewedRect(widget->viewportToView(widget->rect())), true, widget->getMapView()->calculateFinalZoomFactor(), true, 0.5f);
		
		painter->restore();
	}
}

void DrawLineAndAreaTool::startDrawing()
{
	drawing_symbol = is_helper_tool ? NULL : symbol_widget->getSingleSelectedSymbol();
	if (!path_combination)
		path_combination = new CombinedSymbol();
	path_combination->setNumParts(is_helper_tool ? 2 : 3);
	path_combination->setPart(0, Map::getCoveringWhiteLine());
	path_combination->setPart(1, Map::getCoveringRedLine());
	if (drawing_symbol)
		path_combination->setPart(2, drawing_symbol);
	preview_path = new PathObject(path_combination);
	
	draw_in_progress = true;
	editor->setEditingInProgress(true);
}

void DrawLineAndAreaTool::updatePreviewPath()
{
	renderables->removeRenderablesOfObject(preview_path, false);
	preview_path->update(true);
	renderables->insertRenderablesOfObject(preview_path);
}

void DrawLineAndAreaTool::abortDrawing()
{
	renderables->removeRenderablesOfObject(preview_path, false);
	delete preview_path;
	editor->getMap()->clearDrawingBoundingBox();
	
	preview_path = NULL;
	draw_in_progress = false;
	
	editor->setEditingInProgress(false);
	
	emit(pathAborted());
}

void DrawLineAndAreaTool::finishDrawing()
{
	if (preview_path)
		renderables->removeRenderablesOfObject(preview_path, false);
	
	if (preview_path && !is_helper_tool)
	{
		preview_path->setSymbol(drawing_symbol, true);
		int index = editor->getMap()->addObject(preview_path);
		editor->getMap()->clearObjectSelection(false);
		editor->getMap()->addObjectToSelection(preview_path, true);
		
		DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(editor->getMap());
		undo_step->addObject(index);
		editor->getMap()->objectUndoManager().addNewUndoStep(undo_step);
	}
	editor->getMap()->clearDrawingBoundingBox();
	
	draw_in_progress = false;
	editor->setEditingInProgress(false);
	
	if (is_helper_tool)
	{
		if (preview_path)
		{
			// Ugly HACK to make it possible to delete this tool as response to pathFinished
			PathObject* temp_path = preview_path;
			preview_path = NULL;
			emit(pathFinished(temp_path));
			delete temp_path;
		}
		else
			emit(pathAborted());
	}
	else
		preview_path = NULL;
}

void DrawLineAndAreaTool::deletePreviewObjects()
{
	int size = (int)preview_points.size();
	for (int i = 0; i < size; ++i)
	{
		renderables->removeRenderablesOfObject(preview_points[i], false);
		delete preview_points[i];
	}
	preview_points.clear();
	
	size = (int)preview_point_symbols.size();
	for (int i = 0; i < size; ++i)
	{
		if (!preview_point_symbols_external[i])
			delete preview_point_symbols[i];
	}
	preview_point_symbols.clear();
	preview_point_symbols_external.clear();

	if (preview_path)
	{
		renderables->removeRenderablesOfObject(preview_path, false);
		delete preview_path;
		preview_path = NULL;
	}
	draw_in_progress = false; // FIXME: does not belong here
}
void DrawLineAndAreaTool::addPreviewPointSymbols(Symbol* symbol)
{
	if (symbol->getType() == Symbol::Line)
	{
		LineSymbol* line = reinterpret_cast<LineSymbol*>(symbol);
		
		bool has_main_line = line->getLineWidth() > 0 && line->getColor() != NULL;
		bool has_border_line = line->hasBorder() && line->getBorderLineWidth() > 0 && line->getBorderColor() != NULL;
		if (has_main_line || has_border_line)
		{
			if (has_main_line)
			{
				PointSymbol* preview = new PointSymbol();
				preview->setInnerRadius(line->getLineWidth() / 2);
				preview->setInnerColor(line->getColor());
				preview_point_symbols.push_back(preview);
				preview_point_symbols_external.push_back(false);
			}
			if (has_border_line)
			{
				PointSymbol* preview = new PointSymbol();
				preview->setInnerRadius(line->getLineWidth() / 2 - line->getBorderLineWidth() / 2 + line->getBorderShift());
				preview->setOuterWidth(line->getBorderLineWidth());
				preview->setOuterColor(line->getBorderColor());
				preview_point_symbols.push_back(preview);
				preview_point_symbols_external.push_back(false);
			}
		}
		else if (line->getMidSymbol() && !line->getMidSymbol()->isEmpty())
		{
			preview_point_symbols.push_back(line->getMidSymbol());
			preview_point_symbols_external.push_back(true);
		}
	}
	else if (symbol->getType() == Symbol::Combined)
	{
		CombinedSymbol* combined = reinterpret_cast<CombinedSymbol*>(symbol);
		
		int size = combined->getNumParts();
		for (int i = 0; i < size; ++i)
		{
			if (combined->getPart(i))
				addPreviewPointSymbols(combined->getPart(i));
		}
	}
}
