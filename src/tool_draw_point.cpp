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


#include "tool_draw_point.h"

#include <QtGui>

#include "util.h"
#include "renderable.h"
#include "symbol_dock_widget.h"
#include "symbol.h"
#include "object.h"
#include "map_widget.h"
#include "map_undo.h"
#include "symbol_point.h"

QCursor* DrawPointTool::cursor = NULL;

DrawPointTool::DrawPointTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget): MapEditorTool(editor, Other, tool_button), renderables(new RenderableContainer(editor->getMap())), symbol_widget(symbol_widget)
{
	dragging = false;
	preview_object = NULL;
	
	connect(symbol_widget, SIGNAL(selectedSymbolsChanged()), this, SLOT(selectedSymbolsChanged()));
	connect(editor->getMap(), SIGNAL(symbolChanged(int,Symbol*,Symbol*)), this, SLOT(symbolChanged(int,Symbol*,Symbol*)));
	connect(editor->getMap(), SIGNAL(symbolDeleted(int,Symbol*)), this, SLOT(symbolDeleted(int,Symbol*)));
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-draw-point.png"), 11, 11);
}
void DrawPointTool::init()
{
	setStatusBarText(tr("<b>Click</b> to set a point object, <b>Drag</b> to set its rotation if the symbol is rotatable"));
}
DrawPointTool::~DrawPointTool()
{
	if (preview_object)
	{
		renderables->removeRenderablesOfObject(preview_object, false);
		delete preview_object;
	}
}

bool DrawPointTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (!(event->buttons() & Qt::LeftButton))
		return false;
	
	click_pos = event->pos();
	click_pos_map = map_coord;
	cur_pos_map = map_coord;
	dragging = false;
	return true;
}
bool DrawPointTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	PointSymbol* point = reinterpret_cast<PointSymbol*>(symbol_widget->getSingleSelectedSymbol());
	
	if (!(event->buttons() & Qt::LeftButton))
	{
		// Show preview object at this position
		if (!preview_object)
			preview_object = new PointObject(point);
		else
		{
			renderables->removeRenderablesOfObject(preview_object, false);
			if (preview_object->getSymbol() != point)
			{
				bool success = preview_object->setSymbol(point, true);
				assert(success);
			}
		}
		
		preview_object->setPosition(map_coord);
		if (point->isRotatable())
			preview_object->setRotation(0);
		preview_object->update(true);
		renderables->insertRenderablesOfObject(preview_object);
		setDirtyRect(map_coord);
		
		return true;
	}
	if ((event->pos() - click_pos).manhattanLength() < QApplication::startDragDistance())
		return true;
	
	if (point->isRotatable())
	{
		dragging = true;
		editor->setEditingInProgress(true);
		cur_pos = event->pos();
		cur_pos_map = map_coord;
		
		if (preview_object)
		{
			renderables->removeRenderablesOfObject(preview_object, false);
			preview_object->setRotation(calculateRotation(cur_pos, cur_pos_map));
			preview_object->update(true);
			renderables->insertRenderablesOfObject(preview_object);
		}
		
		setDirtyRect(map_coord);
	}
	
	return true;
}
bool DrawPointTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	Map* map = editor->getMap();
	
	PointSymbol* symbol = reinterpret_cast<PointSymbol*>(symbol_widget->getSingleSelectedSymbol());
	PointObject* point = new PointObject(symbol);
	point->setPosition(click_pos_map);
	if (symbol->isRotatable())
		point->setRotation(calculateRotation(event->pos(), map_coord));
	int index = map->addObject(point);
	map->clearObjectSelection(false);
	map->addObjectToSelection(point, true);
	map->clearDrawingBoundingBox();
	
	DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map);
	undo_step->addObject(index);
	map->objectUndoManager().addNewUndoStep(undo_step);
	
	dragging = false;
	editor->setEditingInProgress(false);
	return true;
}
void DrawPointTool::leaveEvent(QEvent* event)
{
	editor->getMap()->clearDrawingBoundingBox();
}
bool DrawPointTool::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Tab)
		editor->setEditTool();
	else
		return false;
	return true;
}

void DrawPointTool::draw(QPainter* painter, MapWidget* widget)
{
	if (preview_object)
	{
		painter->save();
		painter->translate(widget->width() / 2.0 + widget->getMapView()->getDragOffset().x(),
						   widget->height() / 2.0 + widget->getMapView()->getDragOffset().y());
		widget->getMapView()->applyTransform(painter);
		
		renderables->draw(painter, widget->getMapView()->calculateViewedRect(widget->viewportToView(widget->rect())), true, widget->getMapView()->calculateFinalZoomFactor(), true, 0.5f);
		
		painter->restore();
	}
	
	if (dragging && (cur_pos - click_pos).manhattanLength() >= QApplication::startDragDistance())
	{
		painter->setRenderHint(QPainter::Antialiasing);
		
		QPen pen(qRgb(255, 255, 255));
		pen.setWidth(3);
		painter->setPen(pen);
		painter->drawLine(widget->mapToViewport(click_pos_map), widget->mapToViewport(cur_pos_map));
		painter->setPen(active_color);
		painter->drawLine(widget->mapToViewport(click_pos_map), widget->mapToViewport(cur_pos_map));
	}
}

void DrawPointTool::setDirtyRect(MapCoordF mouse_pos)
{
	if (dragging)
	{
		QRectF rect = QRectF(click_pos_map.getX(), click_pos_map.getY(), 0, 0);
		rectInclude(rect, mouse_pos.toQPointF());
		if (preview_object)
			rectInclude(rect, preview_object->getExtent());
		editor->getMap()->setDrawingBoundingBox(rect, 1, true);
	}
	else
	{
		if (preview_object)
			editor->getMap()->setDrawingBoundingBox(preview_object->getExtent(), 0, true);
		else
			editor->getMap()->clearDrawingBoundingBox();
	}
}
float DrawPointTool::calculateRotation(QPoint mouse_pos, MapCoordF mouse_pos_map)
{
	if (dragging && (mouse_pos - click_pos).manhattanLength() >= QApplication::startDragDistance())
		return -atan2(mouse_pos_map.getX() - click_pos_map.getX(), click_pos_map.getY() - mouse_pos_map.getY());
	else
		return 0;
}

void DrawPointTool::selectedSymbolsChanged()
{
	Symbol* single_selected_symbol = symbol_widget->getSingleSelectedSymbol();
	if (single_selected_symbol == NULL || single_selected_symbol->getType() != Symbol::Point || single_selected_symbol->isHidden())
	{
		if (single_selected_symbol && single_selected_symbol->isHidden())
			editor->setEditTool();
		else
			editor->setTool(editor->getDefaultDrawToolForSymbol(single_selected_symbol));
	}
	else
		last_used_symbol = single_selected_symbol;
}
void DrawPointTool::symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol)
{
	// No need to react here
}
void DrawPointTool::symbolDeleted(int pos, Symbol* old_symbol)
{
	if (last_used_symbol == old_symbol)
		editor->setEditTool();
}

#include "tool_draw_point.moc"
