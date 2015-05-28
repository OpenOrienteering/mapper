/*
 *    Copyright 2012-2014 Thomas Schöps
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


#include "tool_draw_point.h"

#include <QKeyEvent>
#include <QPainter>

#include "map.h"
#include "map_editor.h"
#include "object_undo.h"
#include "map_widget.h"
#include "object.h"
#include "renderable.h"
#include "settings.h"
#include "symbol.h"
#include "symbol_point.h"
#include "tool_helpers.h"
#include "util.h"
#include "gui/modifier_key.h"
#include "gui/widgets/key_button_bar.h"
#include "map_editor.h"

DrawPointTool::DrawPointTool(MapEditorController* editor, QAction* tool_button)
: MapEditorToolBase(QCursor(QPixmap(":/images/cursor-draw-point.png"), 11, 11), DrawPoint, editor, tool_button)
, renderables(new MapRenderables(map()))
, key_button_bar(NULL)
{
	rotating = false;
	preview_object = NULL;
	
	angle_helper->addDefaultAnglesDeg(0);
	angle_helper->setActive(false);
	snap_helper->setFilter(SnappingToolHelper::NoSnapping);
	
	connect(editor, SIGNAL(activeSymbolChanged(const Symbol*)), this, SLOT(activeSymbolChanged(const Symbol*)));
	connect(map(), SIGNAL(symbolDeleted(int, const Symbol*)), this, SLOT(symbolDeleted(int, const Symbol*)));
}

DrawPointTool::~DrawPointTool()
{
	if (preview_object)
	{
		renderables->removeRenderablesOfObject(preview_object, false);
		delete preview_object;
	}
	
	if (key_button_bar)
		editor->deletePopupWidget(key_button_bar);
}

void DrawPointTool::initImpl()
{
	if (editor->isInMobileMode())
	{
		// Create key replacement bar
		key_button_bar = new KeyButtonBar(this, editor->getMainWidget());
		key_button_bar->addModifierKey(Qt::Key_Shift, Qt::ShiftModifier, tr("Snap", "Snap to existing objects"));
		key_button_bar->addModifierKey(Qt::Key_Control, Qt::ControlModifier, tr("Angle", "Using constrained angles"));
		editor->showPopupWidget(key_button_bar, "");
	}
	
	if (!preview_object)
	{
		if (editor->activeSymbol()->getType() == Symbol::Point)
			preview_object = new PointObject(editor->activeSymbol()->asPoint());
		else
			preview_object = new PointObject(Map::getUndefinedPoint());
	}
}

void DrawPointTool::leaveEvent(QEvent* event)
{
	Q_UNUSED(event);
	map()->clearDrawingBoundingBox();
}

void DrawPointTool::mouseMove()
{
	PointSymbol* point = reinterpret_cast<PointSymbol*>(editor->activeSymbol());
	
	if (!isDragging())
	{
		// Show preview object at this position
		renderables->removeRenderablesOfObject(preview_object, false);
		if (preview_object->getSymbol() != point)
		{
			bool success = preview_object->setSymbol(point, true);
			Q_ASSERT(success);
			Q_UNUSED(success);
		}
		
		preview_object->setPosition(constrained_pos_map);
		if (point->isRotatable())
			preview_object->setRotation(0);
		preview_object->update();
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
	PointObject* point = preview_object->duplicate()->asPoint();
	
	int index = map()->addObject(point);
	map()->clearObjectSelection(false);
	map()->addObjectToSelection(point, true);
	map()->setObjectsDirty();
	map()->clearDrawingBoundingBox();
	renderables->removeRenderablesOfObject(preview_object, false);
	
	DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map());
	undo_step->addObject(index);
	map()->push(undo_step);
	
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
		preview_object->update();
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
		if (isDragging() && rotating)
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
		if (isDragging())
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
		if (isDragging())
			dragMove();
	}
	else if (event->key() == Qt::Key_Shift)
	{
		snap_helper->setFilter(SnappingToolHelper::NoSnapping);
		calcConstrainedPositions(cur_map_widget);
		if (isDragging())
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
		const MapView* map_view = widget->getMapView();
		painter->save();
		painter->translate(widget->width() / 2.0 + map_view->panOffset().x(),
						   widget->height() / 2.0 + map_view->panOffset().y());
		painter->setWorldTransform(map_view->worldTransform(), true);
		
		RenderConfig config = { *map(), map_view->calculateViewedRect(widget->viewportToView(widget->rect())), map_view->calculateFinalZoomFactor(), RenderConfig::Tool, 0.5 };
		renderables->draw(painter, config);
		
		painter->restore();
	}
	
	if (isDragging() && rotating)
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
	if (isDragging())
	{
		rectIncludeSafe(rect, click_pos_map);
		rectInclude(rect, constrained_pos_map);
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
	if (isDragging() && (mouse_pos - preview_object_pos).manhattanLength() >= Settings::getInstance().getStartDragDistancePx())
		return -atan2(mouse_pos_map.getX() - preview_object->getCoordF().getX(), preview_object->getCoordF().getY() - mouse_pos_map.getY());
	else
		return 0;
}

void DrawPointTool::updateStatusText()
{
	if (isDragging() && rotating)
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

void DrawPointTool::activeSymbolChanged(const Symbol* symbol)
{
	if (symbol == NULL || symbol->getType() != Symbol::Point || symbol->isHidden())
	{
		if (symbol && symbol->isHidden())
			deactivate();
		else
			switchToDefaultDrawTool(symbol);
	}
	else
		last_used_symbol = symbol;
}

void DrawPointTool::symbolDeleted(int pos, const Symbol* old_symbol)
{
	Q_UNUSED(pos);
	
	if (last_used_symbol == old_symbol)
		deactivate();
}
