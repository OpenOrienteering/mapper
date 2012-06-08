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


#include "tool_rotate.h"

#include <qmath.h>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>

#include "map_widget.h"
#include "object.h"
#include "renderable.h"
#include "util.h"

QCursor* RotateTool::cursor = NULL;

RotateTool::RotateTool(MapEditorController* editor, QAction* tool_button)
: MapEditorTool(editor, Other, tool_button),
  old_renderables(new MapRenderables(editor->getMap())), 
  renderables(new MapRenderables(editor->getMap()))
{
	rotation_center_set = false;
	rotating = false;
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-rotate.png"), 1, 1);
}

void RotateTool::init()
{
	updateDirtyRect();
    updateStatusText();
}

RotateTool::~RotateTool()
{
	deleteOldSelectionRenderables(*old_renderables, false);
}

bool RotateTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (!(event->buttons() & Qt::LeftButton))
		return false;
	
	click_pos = event->pos();
	return true;
}

bool RotateTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (!(event->buttons() & Qt::LeftButton))
		return false;
	
	if (rotating)
		updateDragging(map_coord);
	else if ( !rotating && rotation_center_set &&
	          (event->pos() - click_pos).manhattanLength() >= QApplication::startDragDistance() )
	{
		// Start rotating
		rotating = true;
		old_rotation = (map_coord - rotation_center).getAngle();
		original_rotation = old_rotation;
		startEditingSelection(*old_renderables, &undo_duplicates);
	}
	return true;
}

bool RotateTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	if (!rotating)
	{
		rotation_center = map_coord;
		rotation_center_set = true;
	}
	else
	{
		rotating = false;
		updateDragging(map_coord);
		finishEditingSelection(*renderables, *old_renderables, true, &undo_duplicates);
		editor->getMap()->setObjectsDirty();
		editor->getMap()->emitSelectionEdited();
	}
	
	updateDirtyRect();
	updateStatusText();
	return true;
}

void RotateTool::draw(QPainter* painter, MapWidget* widget)
{
	editor->getMap()->drawSelection(painter, true, widget, renderables->isEmpty() ? NULL : renderables.data());
	
	if (rotation_center_set)
	{
		painter->setPen(Qt::white);
		painter->setBrush(Qt::NoBrush);
		
		QPoint center = widget->mapToViewport(rotation_center).toPoint();
		painter->drawEllipse(center, 3, 3);
		painter->setPen(Qt::black);
		painter->drawEllipse(center, 4, 4);
	}
}

void RotateTool::updateDirtyRect()
{
	QRectF rect;
	editor->getMap()->includeSelectionRect(rect);
	
	if (rotation_center_set)
	{
		rectIncludeSafe(rect, rotation_center.toQPointF());
		editor->getMap()->setDrawingBoundingBox(rect, 5, true);
	}
	else if (rect.isValid())
		editor->getMap()->setDrawingBoundingBox(rect, 0, true);
	else
		editor->getMap()->clearDrawingBoundingBox();
}

void RotateTool::updateDragging(const MapCoordF cursor_pos_map)
{
	if (rotating)
	{
		double rotation = (cursor_pos_map - rotation_center).getAngle();
		double delta_rotation = rotation - old_rotation;
		
		Map::ObjectSelection::const_iterator it_end = editor->getMap()->selectedObjectsEnd();
		for (Map::ObjectSelection::const_iterator it = editor->getMap()->selectedObjectsBegin(); it != it_end; ++it)
			(*it)->rotateAround(rotation_center, -1.0 * delta_rotation);
		updatePreviewObjects();
		
		old_rotation = rotation;
		updateStatusText();
	}
}

void RotateTool::updatePreviewObjects()
{
	updateSelectionEditPreview(*renderables);
	updateDirtyRect();
}

void RotateTool::updateStatusText()
{
	if (rotating)
	{
		double delta_rotation = old_rotation - original_rotation;
		if (delta_rotation < -M_PI)
			delta_rotation = delta_rotation + 2*M_PI;
		else if (delta_rotation > M_PI)
			delta_rotation = delta_rotation - 2*M_PI;
		setStatusBarText(tr("<b>Rotation:</b> %1").arg(-delta_rotation * 180 / M_PI, 0, 'f', 1));
	}
	else if (!rotation_center_set)
		setStatusBarText(tr("<b>Click</b> to set the rotation center"));
	else
		setStatusBarText(tr("<b>Click</b> to set the rotation center, <b>drag</b> to rotate the selected object(s)"));
}
