/*
 *    Copyright 2014 Thomas Schöps
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


#include "tool_draw_point_gps.h"

#include <QtWidgets>

#include "map.h"
#include "map_editor.h"
#include "object_undo.h"
#include "map_widget.h"
#include "object.h"
#include "renderable.h"
#include "symbol.h"
#include "symbol_point.h"
#include "tool_helpers.h"
#include "util.h"
#include "gps_display.h"
#include "map_editor.h"


DrawPointGPSTool::DrawPointGPSTool(GPSDisplay* gps_display, MapEditorController* editor, QAction* tool_button)
: MapEditorToolBase(QCursor(QPixmap(":/images/cursor-draw-point.png"), 11, 11), DrawPoint, editor, tool_button)
, renderables(new MapRenderables(map()))
, help_label(NULL)
{
	useTouchCursor(false);
	
	preview_object = NULL;
	if (gps_display->hasValidPosition())
		newGPSPosition(gps_display->getLatestGPSCoord(), gps_display->getLatestGPSCoordAccuracy());
	
	connect(gps_display, SIGNAL(mapPositionUpdated(MapCoordF,float)), this, SLOT(newGPSPosition(MapCoordF,float)));
	connect(editor, SIGNAL(activeSymbolChanged(const Symbol*)), this, SLOT(activeSymbolChanged(const Symbol*)));
	connect(map(), SIGNAL(symbolDeleted(int, const Symbol*)), this, SLOT(symbolDeleted(int, const Symbol*)));
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
		editor->showPopupWidget(help_label, "");
	}
}

void DrawPointGPSTool::newGPSPosition(MapCoordF coord, float accuracy)
{
	PointSymbol* point = reinterpret_cast<PointSymbol*>(editor->activeSymbol());
	
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

		x_sum = weight * coord.getX();
		y_sum = weight * coord.getY();
		weights_sum = weight;
	}
	else
	{
		renderables->removeRenderablesOfObject(preview_object, false);
		if (preview_object->getSymbol() != point)
		{
			bool success = preview_object->setSymbol(point, true);
			assert(success);
		}
		
		x_sum += weight * coord.getX();
		y_sum += weight * coord.getY();
		weights_sum += weight;
	}
	
	MapCoordF avg_position(x_sum / weights_sum, y_sum / weights_sum);
	preview_object->setPosition(avg_position);
	if (point->isRotatable())
		preview_object->setRotation(0);
	preview_object->update(true);
	renderables->insertRenderablesOfObject(preview_object);
	updateDirtyRect();
}

void DrawPointGPSTool::clickRelease()
{
	if (! preview_object)
		return;

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
	deactivate();
}

bool DrawPointGPSTool::keyPress(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Tab)
		deactivate();
	else
		return false;
	return true;
}

void DrawPointGPSTool::drawImpl(QPainter* painter, MapWidget* widget)
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

void DrawPointGPSTool::symbolDeleted(int pos, const Symbol* old_symbol)
{
	Q_UNUSED(pos);
	
	if (last_used_symbol == old_symbol)
		deactivate();
}
