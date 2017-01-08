/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2015 Kai Pastor
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


#include "draw_line_and_area_tool.h"

#include <QPainter>

#include "core/map.h"
#include "gui/map/map_editor.h"
#include "undo/object_undo.h"
#include "gui/map/map_widget.h"
#include "core/objects/object.h"
#include "core/renderables/renderable.h"
#include "core/symbols/symbol.h"
#include "core/symbols/combined_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "util/util.h"

DrawLineAndAreaTool::DrawLineAndAreaTool(MapEditorController* editor, Type type, QAction* tool_action, bool is_helper_tool)
: MapEditorTool(editor, type, tool_action)
, is_helper_tool(is_helper_tool)
, drawing_symbol(NULL)
, preview_point_radius(0)
, preview_points_shown(false)
, path_combination(Map::getCoveringCombinedLine()->duplicate()->asCombined())
, preview_path(NULL)
, renderables(new MapRenderables(map()))
{
	// Helper tools don't draw the active symbol.
	if (!is_helper_tool)
	{
		drawing_symbol = editor->activeSymbol();
		if (drawing_symbol)
			createPreviewPoints();
		
	}
	
	connect(editor, SIGNAL(activeSymbolChanged(const Symbol*)), this, SLOT(setDrawingSymbol(const Symbol*)));
}

DrawLineAndAreaTool::~DrawLineAndAreaTool()
{
	deletePreviewObjects();
}

void DrawLineAndAreaTool::leaveEvent(QEvent* event)
{
	Q_UNUSED(event);
	
	if (!editingInProgress())
		map()->clearDrawingBoundingBox();
}

void DrawLineAndAreaTool::setDrawingSymbol(const Symbol* symbol)
{
	// Avoid using deleted symbol
	if (map()->findSymbolIndex(drawing_symbol) == -1)
		symbol = NULL;
	
	// End current editing
	if (editingInProgress())
	{
		if (symbol)
			finishDrawing();
		else
			abortDrawing();
	}
	
	// Handle new symbol
	if (!is_helper_tool)
		drawing_symbol = symbol;
	
	if (!symbol)
		deactivate();
	else if (symbol->isHidden())
		deactivate();
	else if ((symbol->getType() & (Symbol::Line | Symbol::Area | Symbol::Combined)) == 0)
		switchToDefaultDrawTool(symbol);
	else
		createPreviewPoints();
}

void DrawLineAndAreaTool::createPreviewPoints()
{
	deletePreviewObjects();
	
	preview_point_radius = 0;
	addPreviewPointSymbols(drawing_symbol);
	
	// Create objects for the new symbols
	std::size_t size = preview_point_symbols.size();
	for (std::size_t p = 0; p < 2; ++p)
	{
		preview_points[p].resize(size);
		for (std::size_t i = 0; i < size; ++i)
			preview_points[p][i] = new PointObject(preview_point_symbols[i]);
	}
}

void DrawLineAndAreaTool::setPreviewPointsPosition(MapCoordF map_coord, int index)
{
	std::size_t size = preview_points[index].size();
	for (std::size_t i = 0; i < size; ++i)
	{
		if (preview_points_shown)
			renderables->removeRenderablesOfObject(preview_points[index][i], false);
		preview_points[index][i]->setPosition(map_coord);
		preview_points[index][i]->update();
		renderables->insertRenderablesOfObject(preview_points[index][i]);
	}
	preview_points_shown = true;
}

void DrawLineAndAreaTool::hidePreviewPoints()
{
	if (preview_points_shown)
	{
		for (std::size_t p = 0; p < 2; ++p)
		{
			std::size_t size = preview_points[p].size();
			for (std::size_t i = 0; i < size; ++i)
				renderables->removeRenderablesOfObject(preview_points[p][i], false);
		}
		
		preview_points_shown = false;
	}
}

void DrawLineAndAreaTool::includePreviewRects(QRectF& rect)
{
	if (preview_path)
		rectIncludeSafe(rect, preview_path->getExtent());
	
	if (preview_points_shown)
	{
		for (std::size_t p = 0; p < 2; ++p)
		{
			std::size_t size = preview_points[p].size();
			for (std::size_t i = 0; i < size; ++i)
				rectIncludeSafe(rect, preview_points[p][i]->getExtent());
		}
	}
}

void DrawLineAndAreaTool::drawPreviewObjects(QPainter* painter, MapWidget* widget)
{
	if (preview_path || preview_points_shown)
	{
		const MapView* map_view = widget->getMapView();
		painter->save();
		painter->translate(widget->width() / 2.0 + map_view->panOffset().x(),
						   widget->height() / 2.0 + map_view->panOffset().y());
		painter->setWorldTransform(map_view->worldTransform(), true);
		
		RenderConfig config = { *map(), map_view->calculateViewedRect(widget->viewportToView(widget->rect())), map_view->calculateFinalZoomFactor(), RenderConfig::Tool, 0.5 };
		renderables->draw(painter, config);
		
		painter->restore();
	}
}

void DrawLineAndAreaTool::startDrawing()
{
	auto num_symbol_parts = Map::getCoveringCombinedLine()->getNumParts();
	if (drawing_symbol)
	{
		path_combination->setNumParts(num_symbol_parts + 1);
		path_combination->setPart(num_symbol_parts, drawing_symbol, false);
	}
	else
	{
		path_combination->setNumParts(num_symbol_parts);
	}
	
	preview_path = new PathObject(path_combination.data());
	
	setEditingInProgress(true);
}

void DrawLineAndAreaTool::updatePreviewPath()
{
	renderables->removeRenderablesOfObject(preview_path, false);
	preview_path->update();
	renderables->insertRenderablesOfObject(preview_path);
}

void DrawLineAndAreaTool::abortDrawing()
{
	renderables->removeRenderablesOfObject(preview_path, false);
	delete preview_path;
	map()->clearDrawingBoundingBox();
	
	preview_path = NULL;
	
	setEditingInProgress(false);
	
	emit pathAborted();
}

void DrawLineAndAreaTool::finishDrawing()
{
    finishDrawing(NULL);
}

void DrawLineAndAreaTool::finishDrawing(PathObject* append_to_object)
{
	if (preview_path)
		renderables->removeRenderablesOfObject(preview_path, false);
	
	if (preview_path && !is_helper_tool)
	{
		Q_ASSERT(drawing_symbol);
		preview_path->setSymbol(drawing_symbol, true);

		bool can_be_appended = false;
		if (append_to_object)
			can_be_appended = append_to_object->canBeConnected(preview_path, 0.01*0.01);
		
		if (can_be_appended)
		{
			Object* undo_duplicate = append_to_object->duplicate();
			append_to_object->connectIfClose(preview_path, 0.01*0.01);
			delete preview_path;
			
			map()->clearObjectSelection(false);
			map()->addObjectToSelection(append_to_object, true);
			
			MapPart* cur_part = map()->getCurrentPart();
			ReplaceObjectsUndoStep* undo_step = new ReplaceObjectsUndoStep(map());
			undo_step->addObject(cur_part->findObjectIndex(append_to_object), undo_duplicate);
			map()->push(undo_step);
		}
		else
		{
			int index = map()->addObject(preview_path);
			map()->clearObjectSelection(false);
			map()->addObjectToSelection(preview_path, true);
			
			DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map());
			undo_step->addObject(index);
			map()->push(undo_step);
		}
	}
	map()->clearDrawingBoundingBox();
	map()->setObjectsDirty();
	
	setEditingInProgress(false);
	
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
	for (int p = 0; p < 2; ++p)
	{
		int size = (int)preview_points[p].size();
		for (int i = 0; i < size; ++i)
		{
			renderables->removeRenderablesOfObject(preview_points[p][i], false);
			delete preview_points[p][i];
		}
		preview_points[p].clear();
	}
	
	int size = (int)preview_point_symbols.size();
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
	//drawing_in_progress = false; // FIXME: does not belong here
}

void DrawLineAndAreaTool::addPreviewPointSymbols(const Symbol* symbol)
{
	if (!symbol)
	{
		// Don't abort in release build
		Q_ASSERT(symbol); 
	}
	else if (symbol->getType() == Symbol::Line)
	{
		const LineSymbol* line = reinterpret_cast<const LineSymbol*>(symbol);
		
		bool has_main_line = line->getLineWidth() > 0 && line->getColor() != NULL;
		bool has_border_line = line->hasBorder() && (line->getBorder().isVisible() || line->getRightBorder().isVisible());
		if (has_main_line || has_border_line)
		{
			if (has_main_line)
			{
				PointSymbol* preview = new PointSymbol();
				preview->setInnerRadius(line->getLineWidth() / 2);
				preview->setInnerColor(line->getColor());
				preview_point_symbols.push_back(preview);
				preview_point_symbols_external.push_back(false);
				if (preview->getInnerColor() != NULL)
					preview_point_radius = qMax(preview_point_radius, preview->getInnerRadius());
			}
			if (has_border_line)
			{
				addPreviewPointSymbolsForBorder(line, &line->getBorder());
				if (line->areBordersDifferent())
					addPreviewPointSymbolsForBorder(line, &line->getRightBorder());
			}
		}
		else if (line->getMidSymbol() && !line->getMidSymbol()->isEmpty())
		{
			preview_point_symbols.push_back(line->getMidSymbol());
			preview_point_symbols_external.push_back(true);
			if (line->getMidSymbol()->getOuterColor() != NULL)
				preview_point_radius = qMax(preview_point_radius, line->getMidSymbol()->getInnerRadius() + line->getMidSymbol()->getOuterWidth());
			else if (line->getMidSymbol()->getInnerColor() != NULL)
				preview_point_radius = qMax(preview_point_radius, line->getMidSymbol()->getInnerRadius());
		}
	}
	else if (symbol->getType() == Symbol::Combined)
	{
		const CombinedSymbol* combined = reinterpret_cast<const CombinedSymbol*>(symbol);
		
		int size = combined->getNumParts();
		for (int i = 0; i < size; ++i)
		{
			if (combined->getPart(i))
				addPreviewPointSymbols(combined->getPart(i));
		}
	}
}

void DrawLineAndAreaTool::addPreviewPointSymbolsForBorder(const LineSymbol* line, const LineSymbolBorder* border)
{
	if (!border->isVisible())
		return;
	
	PointSymbol* preview = new PointSymbol();
	preview->setInnerRadius(line->getLineWidth() / 2 - border->width / 2 + border->shift);
	preview->setOuterWidth(border->width);
	preview->setOuterColor(border->color);
	
	preview_point_symbols.push_back(preview);
	preview_point_symbols_external.push_back(false);
	
	if (preview->getOuterColor() != NULL)
		preview_point_radius = qMax(preview_point_radius, preview->getInnerRadius() + preview->getOuterWidth());
}

