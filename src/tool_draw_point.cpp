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
#include "map_editor.h"
#include "map_widget.h"
#include "map_undo.h"
#include "symbol_point.h"
#include "tool_helpers.h"

QCursor* DrawPointTool::cursor = NULL;

DrawPointTool::DrawPointTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget)
 : MapEditorTool(editor, Other, tool_button),
   angle_helper(new ConstrainAngleToolHelper()),
   renderables(new MapRenderables(editor->getMap())),
   symbol_widget(symbol_widget),
   cur_map_widget(editor->getMainWidget())
{
	dragging = false;
	preview_object = NULL;
	
	angle_helper->addDefaultAnglesDeg(0);
	angle_helper->setActive(false);
	connect(angle_helper.data(), SIGNAL(displayChanged()), this, SLOT(updateDirtyRect()));
	
	connect(symbol_widget, SIGNAL(selectedSymbolsChanged()), this, SLOT(selectedSymbolsChanged()));
	connect(editor->getMap(), SIGNAL(symbolDeleted(int,Symbol*)), this, SLOT(symbolDeleted(int,Symbol*)));
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-draw-point.png"), 11, 11);
}
void DrawPointTool::init()
{
	updateStatusText();
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
	cur_map_widget = widget;
	
	click_pos = event->pos();
	click_pos_map = map_coord;
	cur_pos_map = map_coord;
	if (angle_helper->isActive())
		angle_helper->setActive(true, click_pos_map);
	dragging = false;
	return true;
}
bool DrawPointTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	PointSymbol* point = reinterpret_cast<PointSymbol*>(symbol_widget->getSingleSelectedSymbol());
	cur_pos = event->pos();
	cur_pos_map = map_coord;
	
	if (!(event->buttons() & Qt::LeftButton) && !dragging)
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
		updateDirtyRect();
		
		return true;
	}
	if ((event->pos() - click_pos).manhattanLength() < QApplication::startDragDistance())
		return true;
	
	if (point->isRotatable())
	{
		if (!dragging)
		{
			dragging = true;
			editor->setEditingInProgress(true);
		}
		
		updateDragging();
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
		point->setRotation(calculateRotation(constrained_pos, constrained_pos_map));
	int index = map->addObject(point);
	map->clearObjectSelection(false);
	map->addObjectToSelection(point, true);
	map->clearDrawingBoundingBox();
	renderables->removeRenderablesOfObject(preview_object, false);
	
	DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map);
	undo_step->addObject(index);
	map->objectUndoManager().addNewUndoStep(undo_step);
	
	dragging = false;
	editor->setEditingInProgress(false);
	updateStatusText();
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
	else if (event->key() == Qt::Key_Control)
	{
		angle_helper->setActive(true, click_pos_map);
		if (dragging)
			updateDragging();
	}
	else
		return false;
	return true;
}
bool DrawPointTool::keyReleaseEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Control && angle_helper->isActive())
	{
		angle_helper->setActive(false);
		if (dragging)
			updateDragging();
	}
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
		painter->drawLine(widget->mapToViewport(click_pos_map), widget->mapToViewport(constrained_pos_map));
		painter->setPen(active_color);
		painter->drawLine(widget->mapToViewport(click_pos_map), widget->mapToViewport(constrained_pos_map));
		
		angle_helper->draw(painter, widget);
	}
}

void DrawPointTool::updateDirtyRect()
{
	if (dragging)
	{
		QRectF rect = QRectF(click_pos_map.getX(), click_pos_map.getY(), 0, 0);
		rectInclude(rect, constrained_pos_map.toQPointF());
		if (preview_object)
			rectInclude(rect, preview_object->getExtent());
		angle_helper->includeDirtyRect(rect);
		editor->getMap()->setDrawingBoundingBox(rect, qMax(1, angle_helper->getDisplayRadius()), true);
	}
	else
	{
		if (preview_object)
			editor->getMap()->setDrawingBoundingBox(preview_object->getExtent(), 0, true);
		else
			editor->getMap()->clearDrawingBoundingBox();
	}
}
float DrawPointTool::calculateRotation(QPointF mouse_pos, MapCoordF mouse_pos_map)
{
	if (dragging && (mouse_pos - click_pos).manhattanLength() >= QApplication::startDragDistance())
		return -atan2(mouse_pos_map.getX() - click_pos_map.getX(), click_pos_map.getY() - mouse_pos_map.getY());
	else
		return 0;
}

void DrawPointTool::updateDragging()
{
	assert(dragging);
	angle_helper->getConstrainedCursorPositions(cur_pos_map, constrained_pos_map, constrained_pos, cur_map_widget);
	updateStatusText();
	
	if (preview_object)
	{
		renderables->removeRenderablesOfObject(preview_object, false);
		preview_object->setRotation(calculateRotation(constrained_pos, constrained_pos_map));
		preview_object->update(true);
		renderables->insertRenderablesOfObject(preview_object);
	}
	
	updateDirtyRect();
}

void DrawPointTool::updateStatusText()
{
	if (dragging)
	{
		setStatusBarText(trUtf8("<b>Angle:</b> %1°  %2")
			.arg(fmod(calculateRotation(constrained_pos, constrained_pos_map) + 2*M_PI, 2*M_PI) * 180 / M_PI, 0, 'f', 1)
			.arg(angle_helper->isActive() ? "" : tr("(<u>Ctrl</u> for fixed angles)")));
	}
	else
		setStatusBarText(tr("<b>Click</b> to set a point object, <b>Drag</b> to set its rotation if the symbol is rotatable"));
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

void DrawPointTool::symbolDeleted(int pos, Symbol* old_symbol)
{
	if (last_used_symbol == old_symbol)
		editor->setEditTool();
}
