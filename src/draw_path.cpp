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


#include "draw_path.h"

#include <QtGui>

#include "util.h"
#include "symbol_dock_widget.h"
#include "symbol.h"
#include "object.h"
#include "map_widget.h"
#include "symbol_point.h"
#include "symbol_line.h"
#include "symbol_combined.h"
#include "map_undo.h"

QCursor* DrawPathTool::cursor = NULL;

DrawPathTool::DrawPathTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget): MapEditorTool(editor, Other, tool_button), renderables(editor->getMap()), symbol_widget(symbol_widget)
{
	dragging = false;
	draw_in_progress = false;
	preview_points_shown = false;
	path_combination = NULL;
	preview_path = NULL;
	
	space_pressed = false;
	
	selectedSymbolsChanged();
	connect(symbol_widget, SIGNAL(selectedSymbolsChanged()), this, SLOT(selectedSymbolsChanged()));
	connect(editor->getMap(), SIGNAL(symbolChanged(int,Symbol*,Symbol*)), this, SLOT(symbolChanged(int,Symbol*,Symbol*)));
	connect(editor->getMap(), SIGNAL(symbolDeleted(int,Symbol*)), this, SLOT(symbolDeleted(int,Symbol*)));
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-draw-path.png"), 11, 11);
}
void DrawPathTool::init()
{
	updateStatusText();
}
DrawPathTool::~DrawPathTool()
{
	deleteObjects();
	delete path_combination;
}

bool DrawPathTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if ((event->buttons() & Qt::RightButton) && draw_in_progress)
	{
		finishDrawing();
		return true;
	}
	else if (event->buttons() & Qt::LeftButton)
	{
		click_pos = event->pos();
		click_pos_map = map_coord;
		cur_pos_map = map_coord;
		dragging = false;
		
		if (!draw_in_progress)
		{
			// Start a new path
			drawing_symbol = symbol_widget->getSingleSelectedSymbol();
			if (!path_combination)
				path_combination = new CombinedSymbol();
			path_combination->setNumParts(3);
			path_combination->setPart(0, drawing_symbol);
			path_combination->setPart(1, Map::getCoveringWhiteLine());
			path_combination->setPart(2, Map::getCoveringRedLine());
			preview_path = new PathObject(path_combination);
			
			path_has_preview_point = false;
			previous_point_is_curve_point = false;
			
			draw_in_progress = true;
			updateStatusText();
			
			editor->setEditingInProgress(true);
		}

		// Set path point
		MapCoord coord = map_coord.toMapCoord();
		if (space_pressed)
			coord.setDashPoint(true);
		
		if (previous_point_is_curve_point)
			; // Do nothing yet, wait until the user drags or releases the mouse button
		else if (path_has_preview_point)
			preview_path->setCoordinate(preview_path->getCoordinateCount() - 1, coord);
		else
		{
			if (preview_path->getCoordinateCount() == 0 || !preview_path->getCoordinate(preview_path->getCoordinateCount() - 1).isPositionEqualTo(coord))
			{
				preview_path->addCoordinate(coord);
				updatePreviewPath();
			}
		}
		
		path_has_preview_point = false;
		
		create_segment = true;
		setDirtyRect(map_coord);
		return true;
	}
	
	return false;
}
bool DrawPathTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	bool mouse_down = event->buttons() & Qt::LeftButton;
	
	if (!mouse_down)
	{
		if (!draw_in_progress)
		{
			// Show preview objects at this position
			int size = (int)preview_points.size();
			for (int i = 0; i < size; ++i)
			{
				if (preview_points_shown)
					renderables.removeRenderablesOfObject(preview_points[i], false);
				preview_points[i]->setPosition(map_coord.toMapCoord());
				preview_points[i]->update(true);
				renderables.insertRenderablesOfObject(preview_points[i]);
			}
			preview_points_shown = true;
			
			setDirtyRect(map_coord);
		}
		else // if (draw_in_progress)
		{
			if (!previous_point_is_curve_point)
			{
				// Show a line to the cursor position as preview
				hidePreviewPoints();
				
				if (!path_has_preview_point)
				{
					preview_path->addCoordinate(map_coord.toMapCoord());
					path_has_preview_point = true;
				}
				preview_path->setCoordinate(preview_path->getCoordinateCount() - 1, map_coord.toMapCoord());
				
				updatePreviewPath();
				setDirtyRect(map_coord);	// TODO: Possible optimization: mark only the last segment as dirty
			}
		}
	}
	else // if (mouse_down)
	{
		if (!draw_in_progress)
			return false;
		
		if ((event->pos() - click_pos).manhattanLength() < QApplication::startDragDistance())
		{
			if (path_has_preview_point)
			{
				// Remove preview
				int last = preview_path->getCoordinateCount() - 1;
				preview_path->deleteCoordinate(last);
				preview_path->deleteCoordinate(last-1);
				preview_path->deleteCoordinate(last-2);
				
				MapCoord coord = preview_path->getCoordinate(last-3);
				coord.setCurveStart(false);
				preview_path->setCoordinate(last-3, coord);
				
				path_has_preview_point = false;
				dragging = false;
				create_segment = false;
				
				updatePreviewPath();
				setDirtyRect(map_coord);
			}
			
			return true;
		}
		
		// Giving a direction by dragging
		dragging = true;
		create_segment = true;
		cur_pos = event->pos();
		cur_pos_map = map_coord;
		
		if (previous_point_is_curve_point)
		{
			hidePreviewPoints();
			
			float drag_direction = calculateRotation(event->pos(), map_coord);
			createPreviewCurve(click_pos_map.toMapCoord(), drag_direction);
		}
		
		setDirtyRect(map_coord);
	}
	
	return true;
}
bool DrawPathTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	if (!draw_in_progress)
		return false;
	if (!create_segment)
		return true;
	
	if (previous_point_is_curve_point && !dragging)
	{
		// The new point has not been added yet
		MapCoord coord = map_coord.toMapCoord();
		if (space_pressed)
			coord.setDashPoint(true);
		preview_path->addCoordinate(coord);
		updatePreviewPath();
		setDirtyRect(map_coord);
	}
	
	previous_point_is_curve_point = dragging;
	if (previous_point_is_curve_point)
	{
		previous_pos_map = click_pos_map;
		previous_drag_map = map_coord;
		previous_point_direction = calculateRotation(event->pos(), map_coord);
	}
	
	path_has_preview_point = false;
	dragging = false;
	return true;
}
bool DrawPathTool::mouseDoubleClickEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
    if (draw_in_progress)
		finishDrawing();
	return true;
}
void DrawPathTool::leaveEvent(QEvent* event)
{
	if (!draw_in_progress)
		editor->getMap()->clearDrawingBoundingBox();
}

bool DrawPathTool::keyPressEvent(QKeyEvent* event)
{
	if (draw_in_progress)
	{
		if (event->key() == Qt::Key_Escape)
			abortDrawing();
		else if (event->key() == Qt::Key_Backspace)
			undoLastPoint();
		else if (event->key() == Qt::Key_Return)
		{
			closeDrawing();
			finishDrawing();
		}
	}
	if (event->key() == Qt::Key_Tab)
		editor->setEditTool();
	else if (event->key() == Qt::Key_Space)
	{
		space_pressed = !space_pressed;
		updateStatusText();
	}
	else
		return false;
	
	return true;
}
bool DrawPathTool::keyReleaseEvent(QKeyEvent* event)
{
	return false;
}
void DrawPathTool::focusOutEvent(QFocusEvent* event)
{
	// Deactivate all modifiers - not always correct, but should be wrong only in very unusual cases and better than leaving the modifiers on forever
	//space_pressed = false;
	//updateStatusText();
}

void DrawPathTool::draw(QPainter* painter, MapWidget* widget)
{
	if (preview_path || preview_points_shown)
	{
		painter->save();
		painter->translate(widget->width() / 2.0 + widget->getMapView()->getDragOffset().x(),
						   widget->height() / 2.0 + widget->getMapView()->getDragOffset().y());
		widget->getMapView()->applyTransform(painter);
		
		renderables.draw(painter, widget->getMapView()->calculateViewedRect(widget->viewportToView(widget->rect())), true, widget->getMapView()->calculateFinalZoomFactor(), 0.5f);
		
		painter->restore();
	}
	
	if (draw_in_progress)
	{
		painter->setRenderHint(QPainter::Antialiasing);
		if (dragging && (cur_pos - click_pos).manhattanLength() >= QApplication::startDragDistance())
		{
			QPen pen(qRgb(255, 255, 255));
			pen.setWidth(3);
			painter->setPen(pen);
			painter->drawLine(widget->mapToViewport(click_pos_map), widget->mapToViewport(cur_pos_map));
			painter->setPen(active_color);
			painter->drawLine(widget->mapToViewport(click_pos_map), widget->mapToViewport(cur_pos_map));
		}
		if (previous_point_is_curve_point)
		{
			QPen pen(qRgb(255, 255, 255));
			pen.setWidth(3);
			painter->setPen(pen);
			painter->drawLine(widget->mapToViewport(previous_pos_map), widget->mapToViewport(previous_drag_map));
			painter->setPen(active_color);
			painter->drawLine(widget->mapToViewport(previous_pos_map), widget->mapToViewport(previous_drag_map));
		}
	}
}

void DrawPathTool::createPreviewCurve(MapCoord position, float direction)
{
	if (!path_has_preview_point)
	{
		int last = preview_path->getCoordinateCount() - 1;
		MapCoord coord = preview_path->getCoordinate(last);
		coord.setCurveStart(true);
		preview_path->setCoordinate(last, coord);
		
		preview_path->addCoordinate(MapCoord(0, 0));
		preview_path->addCoordinate(MapCoord(0, 0));
		if (space_pressed)
			position.setDashPoint(true);
		preview_path->addCoordinate(position);
		
		path_has_preview_point = true;
	}
	
	// Adjust the preview curve
	int last = preview_path->getCoordinateCount() - 1;
	MapCoord previous_point = preview_path->getCoordinate(last - 3);
	MapCoord last_point = preview_path->getCoordinate(last);
	
	double bezier_handle_distance = 0.4 * previous_point.lengthTo(last_point);
	
	preview_path->setCoordinate(last - 2, MapCoord(previous_point.xd() - bezier_handle_distance * sin(previous_point_direction),
												   previous_point.yd() - bezier_handle_distance * cos(previous_point_direction)));
	preview_path->setCoordinate(last - 1, MapCoord(last_point.xd() + bezier_handle_distance * sin(direction),
												   last_point.yd() + bezier_handle_distance * cos(direction)));
	updatePreviewPath();	
}
void DrawPathTool::undoLastPoint()
{
	if (preview_path->getCoordinateCount() <= 1)
	{
		abortDrawing();
		return;
	}
	
	int last = preview_path->getCoordinateCount() - 1;
	preview_path->deleteCoordinate(last);
	
	if (last >= 3 && preview_path->getCoordinate(last - 3).isCurveStart())
	{
		MapCoord first = preview_path->getCoordinate(last - 3);
		MapCoord second = preview_path->getCoordinate(last - 2);
		
		previous_point_is_curve_point = true;
		previous_point_direction = -atan2(second.xd() - first.xd(), first.yd() - second.yd());
		previous_pos_map = MapCoordF(first);
		previous_drag_map = MapCoordF((first.xd() + second.xd()) / 2, (first.yd() + second.yd()) / 2);
		
		preview_path->deleteCoordinate(last - 1);
		preview_path->deleteCoordinate(last - 2);
		
		first.setCurveStart(false);
		preview_path->setCoordinate(last - 3, first);
	}
	else
	{
		previous_point_is_curve_point = false;
		
		if (path_has_preview_point)
			preview_path->deleteCoordinate(last - 1);
	}
	
	path_has_preview_point = false;
	dragging = false;
	
	updatePreviewPath();
	setDirtyRect(click_pos_map);
}
void DrawPathTool::closeDrawing()
{
	if (preview_path->getCoordinateCount() <= 1)
		return;
	
	if (previous_point_is_curve_point && preview_path->getCoordinate(0).isCurveStart())
	{
		// Finish with a curve
		path_has_preview_point = false;
		
		if (dragging)
			previous_point_direction = -atan2(cur_pos_map.getX() - click_pos_map.getX(), click_pos_map.getY() - cur_pos_map.getY());
		
		MapCoord first = preview_path->getCoordinate(0);
		MapCoord second = preview_path->getCoordinate(1);
		createPreviewCurve(first, -atan2(second.xd() - first.xd(), first.yd() - second.yd()));
		path_has_preview_point = false;
	}
	
	preview_path->setPathClosed(true);
}
void DrawPathTool::finishDrawing()
{
	// Does the symbols contain only areas? If so, auto-close the path if not done yet
	bool contains_only_areas = (drawing_symbol->getContainedTypes() & ~(Symbol::Area | Symbol::Combined)) == 0;
	if (!preview_path->isPathClosed() && contains_only_areas)
		preview_path->setPathClosed(true);
	
	// Remove last point if closed and first and last points are equal, or if the last point was just a preview
	if ((preview_path->isPathClosed() && preview_path->getCoordinate(0).isPositionEqualTo(preview_path->getCoordinate(preview_path->getCoordinateCount() - 1))) ||
		(path_has_preview_point && !dragging))
		preview_path->deleteCoordinate(preview_path->getCoordinateCount() - 1);
	
	renderables.removeRenderablesOfObject(preview_path, false);
	
	if (preview_path->getCoordinateCount() < (contains_only_areas ? 3 : 2))
		delete preview_path;
	else
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
	
	preview_path = NULL;
	dragging = false;
	draw_in_progress = false;
	
	updateStatusText();
	editor->setEditingInProgress(false);
}
void DrawPathTool::abortDrawing()
{
	renderables.removeRenderablesOfObject(preview_path, false);
	delete preview_path;
	editor->getMap()->clearDrawingBoundingBox();
	
	preview_path = NULL;
	dragging = false;
	draw_in_progress = false;
	
	updateStatusText();
	editor->setEditingInProgress(false);
}

void DrawPathTool::setDirtyRect(MapCoordF mouse_pos)
{
	QRectF rect;
	
	if (dragging)
	{
		rectIncludeSafe(rect, click_pos_map.toQPointF());
		rectInclude(rect, mouse_pos.toQPointF());
	}
	if (draw_in_progress && previous_point_is_curve_point)
	{
		rectIncludeSafe(rect, previous_pos_map.toQPointF());
		rectInclude(rect, previous_drag_map.toQPointF());
	}
	includePreviewRects(rect);
	
	if (rect.isValid())
		editor->getMap()->setDrawingBoundingBox(rect, dragging ? 1 : 0, true);
	else
		editor->getMap()->clearDrawingBoundingBox();
}
void DrawPathTool::includePreviewRects(QRectF& rect)
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
float DrawPathTool::calculateRotation(QPoint mouse_pos, MapCoordF mouse_pos_map)
{
	if (dragging && (mouse_pos - click_pos).manhattanLength() >= QApplication::startDragDistance())
		return -atan2(mouse_pos_map.getX() - click_pos_map.getX(), click_pos_map.getY() - mouse_pos_map.getY());
	else
		return 0;
}
void DrawPathTool::updateStatusText()
{
	QString text = "";
	if (space_pressed)
		text += tr("<b>Dash points on.</b> ");
	
	if (!draw_in_progress)
		text += tr("<b>Click</b> to start a polygonal segment, <b>Drag</b> to start a curve");
	else
	{
		text += tr("<b>Click</b> to draw a polygonal segment, <b>Drag</b> to draw a curve, <b>Right or double click</b> to finish the path, "
					"<b>Return</b> to close the path, <b>Backspace</b> to undo, <b>Esc</b> to abort. Try <b>Space</b>");
	}
	
	setStatusBarText(text);
}

void DrawPathTool::updatePreviewPath()
{
	renderables.removeRenderablesOfObject(preview_path, false);
	preview_path->update(true);
	renderables.insertRenderablesOfObject(preview_path);
}

void DrawPathTool::hidePreviewPoints()
{
	if (!preview_points_shown)
		return;
	
	int size = (int)preview_points.size();
	for (int i = 0; i < size; ++i)
		renderables.removeRenderablesOfObject(preview_points[i], false);
	
	preview_points_shown = false;
}
void DrawPathTool::deleteObjects()
{
	int size = (int)preview_points.size();
	for (int i = 0; i < size; ++i)
	{
		renderables.removeRenderablesOfObject(preview_points[i], false);
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
		renderables.removeRenderablesOfObject(preview_path, false);
		delete preview_path;
		preview_path = NULL;
	}
	draw_in_progress = false;
}
void DrawPathTool::addPreviewSymbols(Symbol* symbol)
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
			addPreviewSymbols(combined->getPart(i));
	}
}

void DrawPathTool::selectedSymbolsChanged()
{
	if (draw_in_progress)
		abortDrawing();
	
	Symbol* symbol = symbol_widget->getSingleSelectedSymbol();
	if (symbol == NULL || ((symbol->getType() & (Symbol::Line | Symbol::Area | Symbol::Combined)) == 0))
	{
		MapEditorTool* draw_tool = editor->getDefaultDrawToolForSymbol(symbol);
		editor->setTool(draw_tool);	
		return;
	}

	deleteObjects();
	addPreviewSymbols(symbol);
	
	// Create objects for the symbols
	int size = (int)preview_point_symbols.size();
	preview_points.resize(size);
	for (int i = 0; i < size; ++i)
		preview_points[i] = new PointObject(preview_point_symbols[i]);
	
	last_used_symbol = symbol;
}
void DrawPathTool::symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol)
{
	if (old_symbol == last_used_symbol)
		selectedSymbolsChanged();
}
void DrawPathTool::symbolDeleted(int pos, Symbol* old_symbol)
{
	if (old_symbol == last_used_symbol)
		editor->setEditTool();
}

#include "draw_path.moc"
