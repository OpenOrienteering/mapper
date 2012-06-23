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


#include "tool_scale.h"

#include <qmath.h>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>

#include "map_widget.h"
#include "object.h"
#include "renderable.h"
#include "util.h"

QCursor* ScaleTool::cursor = NULL;

ScaleTool::ScaleTool(MapEditorController* editor, QAction* tool_button)
: MapEditorTool(editor, Other, tool_button),
  old_renderables(new MapRenderables(editor->getMap())), 
  renderables(new MapRenderables(editor->getMap()))
{
	scaling_center_set = false;
	scaling = false;
	scaling_factor = 1;
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-scale.png"), 1, 1);
}

void ScaleTool::init()
{
	// Set initial scaling center to the center of the bounding box of the selected objects
	Map* map = editor->getMap();
	if (map->getNumSelectedObjects() > 0)
	{
		QRectF rect;
		map->includeSelectionRect(rect);
		scaling_center = MapCoordF(rect.center());
		scaling_center_set = true;
	}
	
	updateDirtyRect();
	updateStatusText();
}

ScaleTool::~ScaleTool()
{
	deleteOldSelectionRenderables(*old_renderables, false);
}

bool ScaleTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (!(event->buttons() & Qt::LeftButton))
		return false;
	
	click_pos = event->pos();
	return true;
}

bool ScaleTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (!(event->buttons() & Qt::LeftButton))
		return false;
	
	if (scaling)
		updateDragging(map_coord);
	else if ( !scaling && scaling_center_set &&
	          (event->pos() - click_pos).manhattanLength() >= QApplication::startDragDistance() )
	{
		// Start scaling
		scaling = true;
		original_scale = (map_coord - scaling_center).length();
		startEditingSelection(*old_renderables, &undo_duplicates);
	}
	return true;
}

bool ScaleTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	if (!scaling)
	{
		scaling_center = map_coord;
		scaling_center_set = true;
	}
	else
	{
		scaling = false;
		updateDragging(map_coord);
		finishEditingSelection(*renderables, *old_renderables, true, &undo_duplicates);
		editor->getMap()->setObjectsDirty();
		editor->getMap()->emitSelectionEdited();
	}
	
	updateDirtyRect();
	updateStatusText();
	return true;
}

void ScaleTool::draw(QPainter* painter, MapWidget* widget)
{
	editor->getMap()->drawSelection(painter, true, widget, renderables->isEmpty() ? NULL : renderables.data());
	
	if (scaling_center_set)
	{
		painter->setPen(Qt::white);
		painter->setBrush(Qt::NoBrush);
		
		QPoint center = widget->mapToViewport(scaling_center).toPoint();
		painter->drawEllipse(center, 3, 3);
		painter->setPen(Qt::black);
		painter->drawEllipse(center, 4, 4);
	}
}

void ScaleTool::updateDirtyRect()
{
	QRectF rect;
	editor->getMap()->includeSelectionRect(rect);
	
	if (scaling_center_set)
	{
		rectIncludeSafe(rect, scaling_center.toQPointF());
		editor->getMap()->setDrawingBoundingBox(rect, 5, true);
	}
	else if (rect.isValid())
		editor->getMap()->setDrawingBoundingBox(rect, 0, true);
	else
		editor->getMap()->clearDrawingBoundingBox();
}

void ScaleTool::updateDragging(const MapCoordF cursor_pos_map)
{
	if (scaling)
	{
		double scaling = (cursor_pos_map - scaling_center).length();
		scaling_factor = scaling / qMax(1e-7, original_scale);
		
		resetEditedObjects(&undo_duplicates);
		Map::ObjectSelection::const_iterator it_end = editor->getMap()->selectedObjectsEnd();
		for (Map::ObjectSelection::const_iterator it = editor->getMap()->selectedObjectsBegin(); it != it_end; ++it)
		{
			(*it)->move(-scaling_center.getIntX(), -scaling_center.getIntY());
			(*it)->scale(scaling_factor);
			(*it)->move(scaling_center.getIntX(), scaling_center.getIntY());
		}
		updatePreviewObjects();
		updateStatusText();
	}
}

void ScaleTool::updatePreviewObjects()
{
	updateSelectionEditPreview(*renderables);
	updateDirtyRect();
}

void ScaleTool::updateStatusText()
{
	if (scaling)
		setStatusBarText(tr("<b>Scaling:</b> %1%").arg(scaling_factor * 100, 0, 'f', 1));
	else if (!scaling_center_set)
		setStatusBarText(tr("<b>Click</b> to set the scaling center"));
	else
		setStatusBarText(tr("<b>Click</b> to set the scaling center, <b>drag</b> to scale the selected object(s)"));
}
