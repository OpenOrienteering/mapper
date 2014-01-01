/*
 *    Copyright 2012, 2013 Thomas Schöps
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

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "map.h"
#include "map_widget.h"
#include "map_undo.h"
#include "object.h"
#include "renderable.h"
#include "symbol.h"
#include "symbol_dock_widget.h"
#include "symbol_point.h"
#include "tool_helpers.h"
#include "util.h"
#include "gui/modifier_key.h"

DrawPointTool::DrawPointTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget)
 : MapEditorToolBase(QCursor(QPixmap(":/images/cursor-draw-point.png"), 11, 11), DrawPoint, editor, tool_button),
   renderables(new MapRenderables(map())),
   symbol_widget(symbol_widget)
{
	rotating = false;
	preview_object = NULL;
	
	angle_helper->addDefaultAnglesDeg(0);
	angle_helper->setActive(false);
	snap_helper->setFilter(SnappingToolHelper::NoSnapping);
	
	connect(symbol_widget, SIGNAL(selectedSymbolsChanged()), this, SLOT(selectedSymbolsChanged()));
	connect(map(), SIGNAL(symbolDeleted(int,Symbol*)), this, SLOT(symbolDeleted(int,Symbol*)));
}

DrawPointTool::~DrawPointTool()
{
	if (preview_object)
	{
		renderables->removeRenderablesOfObject(preview_object, false);
		delete preview_object;
	}
}

void DrawPointTool::leaveEvent(QEvent* event)
{
	Q_UNUSED(event);
	map()->clearDrawingBoundingBox();
}

void DrawPointTool::mouseMove()
{
	PointSymbol* point = reinterpret_cast<PointSymbol*>(symbol_widget->getSingleSelectedSymbol());
	
	if (!dragging)
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
		
		preview_object->setPosition(constrained_pos_map);
		if (point->isRotatable())
			preview_object->setRotation(0);
		preview_object->update(true);
		renderables->insertRenderablesOfObject(preview_object);
		updateDirtyRect();
		
		return;
	}
}

void DrawPointTool::clickPress()
{
	rotating = false;
}

void DrawPointTool::clickRelease()
{
#if defined(ANDROID)
	mouseMove();
#endif
	PointObject* point = preview_object->duplicate()->asPoint();
	
	int index = map()->addObject(point);
	map()->clearObjectSelection(false);
	map()->addObjectToSelection(point, true);
	map()->setObjectsDirty();
	map()->clearDrawingBoundingBox();
	renderables->removeRenderablesOfObject(preview_object, false);
	
	DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map());
	undo_step->addObject(index);
	map()->objectUndoManager().addNewUndoStep(undo_step);
	
	setEditingInProgress(false);
	updateStatusText();
}

void DrawPointTool::dragStart()
{
	setEditingInProgress(true);
}

void DrawPointTool::dragMove()
{
	updateStatusText();
	
	bool new_rotating = preview_object && preview_object->getSymbol()->asPoint()->isRotatable();
	if (new_rotating && !rotating && active_modifiers & Qt::ControlModifier)
		angle_helper->setActive(true, preview_object->getCoordF());
	rotating = new_rotating;
	
	if (rotating)
	{
		renderables->removeRenderablesOfObject(preview_object, false);
		preview_object->setRotation(calculateRotation(constrained_pos, constrained_pos_map));
		preview_object->update(true);
		renderables->insertRenderablesOfObject(preview_object);
	}
	
	updateDirtyRect();
}

void DrawPointTool::dragFinish()
{
	setEditingInProgress(false);
	clickRelease();
}

bool DrawPointTool::keyPress(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Tab)
		deactivate();
	else if (event->key() == Qt::Key_Control)
	{
		if (dragging && rotating)
		{
			angle_helper->setActive(true, preview_object->getCoordF());
			calcConstrainedPositions(cur_map_widget);
			dragMove();
		}
	}
	else if (event->key() == Qt::Key_Shift)
	{
		snap_helper->setFilter(SnappingToolHelper::AllTypes);
		calcConstrainedPositions(cur_map_widget);
		if (dragging)
			dragMove();
		else
			mouseMove();
	}
	else
		return false;
	return true;
}

bool DrawPointTool::keyRelease(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Control && angle_helper->isActive())
	{
		angle_helper->setActive(false);
		calcConstrainedPositions(cur_map_widget);
		if (dragging)
			dragMove();
	}
	else if (event->key() == Qt::Key_Shift)
	{
		snap_helper->setFilter(SnappingToolHelper::NoSnapping);
		calcConstrainedPositions(cur_map_widget);
		if (dragging)
			dragMove();
		else
			mouseMove();
	}
	else
		return false;
	return true;
}

void DrawPointTool::drawImpl(QPainter* painter, MapWidget* widget)
{
	if (preview_object)
	{
		painter->save();
		painter->translate(widget->width() / 2.0 + widget->getMapView()->getDragOffset().x(),
						   widget->height() / 2.0 + widget->getMapView()->getDragOffset().y());
		widget->getMapView()->applyTransform(painter);
		
		renderables->draw(painter, widget->getMapView()->calculateViewedRect(widget->viewportToView(widget->rect())), true, widget->getMapView()->calculateFinalZoomFactor(), true, true, 0.5f);
		
		painter->restore();
	}
	
	if (dragging && rotating)
	{
		painter->setRenderHint(QPainter::Antialiasing);
		
		QPen pen(qRgb(255, 255, 255));
		pen.setWidth(3);
		painter->setPen(pen);
		painter->drawLine(widget->mapToViewport(preview_object->getCoordF()), widget->mapToViewport(constrained_pos_map));
		painter->setPen(active_color);
		painter->drawLine(widget->mapToViewport(preview_object->getCoordF()), widget->mapToViewport(constrained_pos_map));
		
		angle_helper->draw(painter, widget);
	}
	
	if (active_modifiers & Qt::ShiftModifier)
		snap_helper->draw(painter, widget);
}

int DrawPointTool::updateDirtyRectImpl(QRectF& rect)
{
	if (dragging)
	{
		rectIncludeSafe(rect, click_pos_map.toQPointF());
		rectInclude(rect, constrained_pos_map.toQPointF());
		if (preview_object)
			rectInclude(rect, preview_object->getExtent());
		return qMax(1, angle_helper->getDisplayRadius());
	}
	else
	{
		if (preview_object)
		{
			rectIncludeSafe(rect, preview_object->getExtent());
			return 0;
		}
		else
			return -1;
	}
}

float DrawPointTool::calculateRotation(QPointF mouse_pos, MapCoordF mouse_pos_map)
{
	QPoint preview_object_pos = cur_map_widget->mapToViewport(preview_object->getCoordF()).toPoint();
	if (dragging && (mouse_pos - preview_object_pos).manhattanLength() >= QApplication::startDragDistance())
		return -atan2(mouse_pos_map.getX() - preview_object->getCoordF().getX(), preview_object->getCoordF().getY() - mouse_pos_map.getY());
	else
		return 0;
}

void DrawPointTool::updateStatusText()
{
	if (dragging && rotating)
	{
		static const double pi_x_2 = M_PI * 2.0;
		static const double to_deg = 180.0 / M_PI;
		double angle = fmod(calculateRotation(constrained_pos, constrained_pos_map) + pi_x_2, pi_x_2) * to_deg;
		setStatusBarText( trUtf8("<b>Angle:</b> %1° ").arg(QLocale().toString(angle, 'f', 1)) + "| " +
		                  tr("<b>%1</b>: Fixed angles. ").arg(ModifierKey::control()) );
	}
	else
	{
		setStatusBarText( tr("<b>Click</b>: Create a point object. <b>Drag</b>: Create an object and set its orientation (if rotatable). "));
	}
}

void DrawPointTool::objectSelectionChangedImpl()
{
	// NOP
}

void DrawPointTool::selectedSymbolsChanged()
{
	Symbol* single_selected_symbol = symbol_widget->getSingleSelectedSymbol();
	if (single_selected_symbol == NULL || single_selected_symbol->getType() != Symbol::Point || single_selected_symbol->isHidden())
	{
		if (single_selected_symbol && single_selected_symbol->isHidden())
			deactivate();
		else
			switchToDefaultDrawTool(single_selected_symbol);
	}
	else
		last_used_symbol = single_selected_symbol;
}

void DrawPointTool::symbolDeleted(int pos, Symbol* old_symbol)
{
	Q_UNUSED(pos);
	
	if (last_used_symbol == old_symbol)
		deactivate();
}
