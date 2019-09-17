/*
 *    Copyright 2014 Thomas Schöps
 *    Copyright 2014, 2015 Kai Pastor
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


#include "draw_point_gps_tool.h"

#include <Qt>
#include <QtGlobal>
#include <QCursor>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QString>

#include "core/map.h"
#include "core/map_coord.h"
#include "core/map_view.h"
#include "core/objects/object.h"
#include "core/renderables/renderable.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "sensors/gps_display.h"
#include "tools/tool.h"
#include "undo/object_undo.h"
#include "util/util.h"


namespace OpenOrienteering {

DrawPointGPSTool::DrawPointGPSTool(GPSDisplay* gps_display, MapEditorController* editor, QAction* tool_action)
: MapEditorToolBase(QCursor(QPixmap(QString::fromLatin1(":/images/cursor-draw-point.png")), 11, 11), DrawPoint, editor, tool_action)
, renderables(new MapRenderables(map()))
{
	useTouchCursor(false);
	
	preview_object = nullptr;
	if (gps_display->hasValidPosition())
		newGPSPosition(gps_display->getLatestGPSCoord(), gps_display->getLatestGPSCoordAccuracy());
	
	connect(gps_display, &GPSDisplay::mapPositionUpdated, this, &DrawPointGPSTool::newGPSPosition);
	connect(editor, &MapEditorController::activeSymbolChanged, this, &DrawPointGPSTool::activeSymbolChanged);
	connect(map(), &Map::symbolDeleted, this, &DrawPointGPSTool::symbolDeleted);
}

DrawPointGPSTool::~DrawPointGPSTool()
{
	if (preview_object)
	{
		renderables->removeRenderablesOfObject(preview_object, false);
		delete preview_object;
	}
	if (help_label)
		editor->deletePopupWidget(help_label);
}

void DrawPointGPSTool::initImpl()
{
	if (editor->isInMobileMode())
	{
		help_label = new QLabel(tr("Touch the map to finish averaging"));
		editor->showPopupWidget(help_label, QString{});
	}
}

void DrawPointGPSTool::newGPSPosition(const MapCoordF& coord, float accuracy)
{
	auto point = reinterpret_cast<PointSymbol*>(editor->activeSymbol());
	
	// Calculate weight from accuracy. This is arbitrarily chosen.
	float weight;
	if (accuracy < 0)
		weight = 1; // accuracy unknown
	else
		weight = 1.0f / qMax(0.5f, accuracy);
	
	if (! preview_object)
	{
		// This is the first received position.
		preview_object = new PointObject(point);

		x_sum = weight * coord.x();
		y_sum = weight * coord.y();
		weights_sum = weight;
	}
	else
	{
		renderables->removeRenderablesOfObject(preview_object, false);
		if (preview_object->getSymbol() != point)
		{
			bool success = preview_object->setSymbol(point, true);
			Q_ASSERT(success);
			Q_UNUSED(success);
		}
		
		x_sum += weight * coord.x();
		y_sum += weight * coord.y();
		weights_sum += weight;
	}
	
	MapCoordF avg_position(x_sum / weights_sum, y_sum / weights_sum);
	preview_object->setPosition(avg_position);
	preview_object->setRotation(0);
	preview_object->update();
	renderables->insertRenderablesOfObject(preview_object);
	updateDirtyRect();
}

void DrawPointGPSTool::clickRelease()
{
	if (! preview_object)
		return;

	auto point = preview_object->duplicate()->asPoint();
	
	int index = map()->addObject(point);
	map()->clearObjectSelection(false);
	map()->addObjectToSelection(point, true);
	map()->setObjectsDirty();
	map()->clearDrawingBoundingBox();
	renderables->removeRenderablesOfObject(preview_object, false);
	
	auto undo_step = new DeleteObjectsUndoStep(map());
	undo_step->addObject(index);
	map()->push(undo_step);
	
	setEditingInProgress(false);
	deactivate();
}

bool DrawPointGPSTool::keyPress(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_Tab:
		deactivate();
		return true;
		
	}
	return false;
}

void DrawPointGPSTool::drawImpl(QPainter* painter, MapWidget* widget)
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
}

int DrawPointGPSTool::updateDirtyRectImpl(QRectF& rect)
{
	if (preview_object)
	{
		rectIncludeSafe(rect, preview_object->getExtent());
		return 0;
	}
	else
		return -1;
}

void DrawPointGPSTool::updateStatusText()
{
	setStatusBarText( tr("<b>Click</b>: Finish setting the object. "));
}

void DrawPointGPSTool::objectSelectionChangedImpl()
{
	// NOP
}

void DrawPointGPSTool::activeSymbolChanged(const Symbol* symbol)
{
	if (!symbol || symbol->getType() != Symbol::Point || symbol->isHidden())
	{
		if (symbol && symbol->isHidden())
			deactivate();
		else
			switchToDefaultDrawTool(symbol);
	}
	else
		last_used_symbol = symbol;
}

void DrawPointGPSTool::symbolDeleted(int pos, const Symbol* old_symbol)
{
	Q_UNUSED(pos);
	
	if (last_used_symbol == old_symbol)
		deactivate();
}


}  // namespace OpenOrienteering
