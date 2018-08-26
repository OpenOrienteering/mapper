/*
 *    Copyright 2012-2014 Thomas Schöps
 *    Copyright 2013-2016 Kai Pastor
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


#include "draw_point_tool.h"

#include <cmath>

#include <Qt>
#include <QtGlobal>
#include <QtMath>
#include <QCursor>
#include <QFlags>
#include <QKeyEvent>
#include <QLatin1Char>
#include <QLatin1String>
#include <QLocale>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QPointer>
#include <QRgb>
#include <QString>
#include <QStringList>

#include "core/map.h"
#include "core/map_coord.h"
#include "core/map_view.h"
#include "core/objects/object.h"
#include "core/renderables/renderable.h"
#include "core/symbols/symbol.h"
#include "core/symbols/point_symbol.h"
#include "gui/modifier_key.h"
#include "gui/widgets/key_button_bar.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "tools/tool.h"
#include "tools/tool_base.h"
#include "tools/tool_helpers.h"
#include "undo/object_undo.h"
#include "util/util.h"


namespace OpenOrienteering {

DrawPointTool::DrawPointTool(MapEditorController* editor, QAction* tool_action)
: MapEditorToolBase(QCursor(QPixmap(QString::fromLatin1(":/images/cursor-draw-point.png")), 11, 11), DrawPoint, editor, tool_action)
, renderables(new MapRenderables(map()))
{
	// all done in initImpl()
}

DrawPointTool::~DrawPointTool()
{
	if (preview_object)
		renderables->removeRenderablesOfObject(preview_object.get(), false);
}

void DrawPointTool::initImpl()
{
	if (editor->isInMobileMode())
	{
		// Create key replacement bar
		key_button_bar = new KeyButtonBar(editor->getMainWidget());
		key_button_bar->addModifierButton(Qt::ShiftModifier, tr("Snap", "Snap to existing objects"));
		key_button_bar->addModifierButton(Qt::ControlModifier, tr("Angle", "Using constrained angles"));
		key_button_bar->addKeyButton(Qt::Key_Escape, tr("Reset", "Reset rotation"));
		editor->showPopupWidget(key_button_bar, QString{});
	}
	
	if (!preview_object)
	{
		if (editor->activeSymbol()->getType() == Symbol::Point)
			preview_object.reset(new PointObject(editor->activeSymbol()->asPoint()));
		else
			preview_object.reset(new PointObject(Map::getUndefinedPoint()));
	}
	
	angle_helper->addDefaultAnglesDeg(0);
	angle_helper->setActive(false);
	
	snap_helper->setFilter(SnappingToolHelper::NoSnapping);
	
	connect(editor, &MapEditorController::activeSymbolChanged, this, &DrawPointTool::activeSymbolChanged);
	connect(map(), &Map::symbolDeleted, this, &DrawPointTool::symbolDeleted);
}

void DrawPointTool::activeSymbolChanged(const Symbol* symbol)
{
	if (!symbol)
		switchToDefaultDrawTool(nullptr);
	else if (symbol->isHidden())
		deactivate();
	else if (symbol->getType() != Symbol::Point)
		switchToDefaultDrawTool(symbol);
	else if (preview_object)
	{
		renderables->removeRenderablesOfObject(preview_object.get(), false);
		preview_object->setRotation(0);
		preview_object->setSymbol(symbol, true);
		// Let update happen on later event.
	}
}

void DrawPointTool::symbolDeleted(int /*unused*/, const Symbol* symbol)
{
	if (preview_object && preview_object->getSymbol() == symbol)
		deactivate();
}

void DrawPointTool::updatePreviewObject(const MapCoordF& pos)
{
	renderables->removeRenderablesOfObject(preview_object.get(), false);
	preview_object->setPosition(pos);
	preview_object->update();
	renderables->insertRenderablesOfObject(preview_object.get());
	updateDirtyRect();
}

void DrawPointTool::createObject()
{
	PointObject* point = preview_object->duplicate()->asPoint();
	
	int index = map()->addObject(point);
	map()->clearObjectSelection(false);
	map()->addObjectToSelection(point, true);
	map()->setObjectsDirty();
	
	auto undo_step = new DeleteObjectsUndoStep(map());
	undo_step->addObject(index);
	map()->push(undo_step);
}

void DrawPointTool::leaveEvent(QEvent* /*unused*/)
{
	map()->clearDrawingBoundingBox();
}

void DrawPointTool::mouseMove()
{
	Q_ASSERT(!editingInProgress());
	updatePreviewObject(constrained_pos_map);
}

void DrawPointTool::clickPress()
{
	updatePreviewObject(constrained_pos_map);
}

void DrawPointTool::clickRelease()
{
	createObject();
	
	map()->clearDrawingBoundingBox();
	renderables->removeRenderablesOfObject(preview_object.get(), false);
	updateStatusText();
}

void DrawPointTool::dragStart()
{
	setEditingInProgress(true);
}

void DrawPointTool::dragMove()
{
	if (preview_object->getSymbol()->asPoint()->isRotatable())
	{
		bool enable_angle_helper = active_modifiers & Qt::ControlModifier;
		if (angle_helper->isActive() != enable_angle_helper)
		{
			angle_helper->setActive(enable_angle_helper, preview_object->getCoordF());
			updateConstrainedPositions();
		}
		
		renderables->removeRenderablesOfObject(preview_object.get(), false);
		preview_object->setRotation(calculateRotation(constrained_pos, constrained_pos_map));
		preview_object->update();
		renderables->insertRenderablesOfObject(preview_object.get());
		updateDirtyRect();
		
		updateStatusText();
	}
}

void DrawPointTool::dragFinish()
{
	setEditingInProgress(false);
	clickRelease();
	angle_helper->setActive(false);
}

bool DrawPointTool::keyPress(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_Tab:
		deactivate();
		return true;
	
	case Qt::Key_Escape:
	case Qt::Key_0:
		if (!editingInProgress())
		{
			preview_object->setRotation(0);
			updateStatusText();
			if (!editor->isInMobileMode())
				mouseMove();
			return true;
		}
		break;
		
	case Qt::Key_Control:
		if (isDragging())
			dragMove();
		return false; // not consuming Ctrl
		
	case Qt::Key_Shift:
		snap_helper->setFilter(SnappingToolHelper::AllTypes);
		reapplyConstraintHelpers();
		return false; // not consuming Shift
		
	}
	return false;
}

bool DrawPointTool::keyRelease(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_Control:
		if (isDragging())
			dragMove();
		return false; // not consuming Ctrl
		
	case Qt::Key_Shift:
		snap_helper->setFilter(SnappingToolHelper::NoSnapping);
		reapplyConstraintHelpers();
		return false; // not consuming Shift
		
	}
	
	return false;
}

void DrawPointTool::drawImpl(QPainter* painter, MapWidget* widget)
{
	Q_ASSERT(preview_object);
	
	const MapView* map_view = widget->getMapView();
	painter->save();
	painter->translate(widget->width() / 2.0 + map_view->panOffset().x(),
	                   widget->height() / 2.0 + map_view->panOffset().y());
	painter->setWorldTransform(map_view->worldTransform(), true);
	
	RenderConfig config = { *map(), map_view->calculateViewedRect(widget->viewportToView(widget->rect())), map_view->calculateFinalZoomFactor(), RenderConfig::Tool, 0.5 };
	renderables->draw(painter, config);
	
	painter->restore();
	
	if (preview_object->getSymbol()->asPoint()->isRotatable())
	{
		if (isDragging())
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
		{
			snap_helper->draw(painter, widget);
		}
	}
}

int DrawPointTool::updateDirtyRectImpl(QRectF& rect)
{
	Q_ASSERT(preview_object);
	
	int result = 0;
	rectIncludeSafe(rect, preview_object->getExtent());
	if (isDragging() && preview_object->getSymbol()->asPoint()->isRotatable())
	{
		rectInclude(rect, constrained_click_pos_map);
		rectInclude(rect, constrained_pos_map);
		result = qMax(3, angle_helper->getDisplayRadius());
	}
	
	return result;
}

double DrawPointTool::calculateRotation(const QPointF& mouse_pos, const MapCoordF& mouse_pos_map) const
{
	double result = 0.0;
	if (isDragging())
	{
		QPoint preview_object_pos = cur_map_widget->mapToViewport(preview_object->getCoordF()).toPoint();
		if ((mouse_pos - preview_object_pos).manhattanLength() >= startDragDistance())
			result = -atan2(mouse_pos_map.x() - preview_object->getCoordF().x(), preview_object->getCoordF().y() - mouse_pos_map.y());
	}
	return result;
}

void DrawPointTool::updateStatusText()
{
	if (isDragging())
	{
		auto angle = qRadiansToDegrees(preview_object->getRotation());
		setStatusBarText( trUtf8("<b>Angle:</b> %1° ").arg(QLocale().toString(angle, 'f', 1)) + QLatin1String("| ") +
		                  tr("<b>%1</b>: Fixed angles. ").arg(ModifierKey::control()) );
	}
	else if (static_cast<const PointSymbol*>(preview_object->getSymbol())->isRotatable())
	{
		auto parts = QStringList {
		    tr("<b>Click</b>: Create a point object."),
		    tr("<b>Drag</b>: Create an object and set its orientation."),
		};
		auto angle = qRadiansToDegrees(preview_object->getRotation());
		if (!qIsNull(angle))
		{
			parts.push_front(trUtf8("<b>Angle:</b> %1° ").arg(QLocale().toString(angle, 'f', 1)) + QLatin1Char('|'));
			parts.push_back(tr("<b>%1, 0</b>: Reset rotation.").arg(ModifierKey::escape()));
		}
		setStatusBarText(parts.join(QLatin1Char(' ')));
	}
	else
	{
		// Same as click_text above.
		setStatusBarText(tr("<b>Click</b>: Create a point object."));
	}
}

void DrawPointTool::objectSelectionChangedImpl()
{
	// nothing
}


}  // namespace OpenOrienteering
