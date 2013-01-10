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

#include "map.h"
#include "map_widget.h"
#include "object.h"
#include "renderable.h"
#include "tool_helpers.h"
#include "util.h"

RotateTool::RotateTool(MapEditorController* editor, QAction* tool_button)
 : MapEditorToolBase(QCursor(QPixmap(":/images/cursor-rotate.png"), 1, 1), Other, editor, tool_button),
   angle_helper(new ConstrainAngleToolHelper())
{
	angle_helper->setActive(false);
	rotation_center_set = false;
	rotating = false;
}

RotateTool::~RotateTool()
{
	// Nothing, not inlined
}

void RotateTool::initImpl()
{
	// Set initial rotation center to the bounding box center of the selected objects
	if (map()->getNumSelectedObjects() > 0)
	{
		QRectF rect;
		map()->includeSelectionRect(rect);
		rotation_center = MapCoordF(rect.center());
		rotation_center_set = true;
	}
}

void RotateTool::clickRelease()
{
	rotation_center = cur_pos_map;
	rotation_center_set = true;
	
	updateDirtyRect();
	updateStatusText();
}

void RotateTool::dragStart()
{
	if (!rotating && rotation_center_set)
	{
		// Start rotating
		rotating = true;
		old_rotation = (cur_pos_map - rotation_center).getAngle();
		original_rotation = old_rotation;
		angle_helper->clearAngles();
		angle_helper->addDefaultAnglesDeg(-original_rotation * 180 / M_PI);
		startEditing();
	}
}
void RotateTool::dragMove()
{
	if (rotating)
	{
		angle_helper->getConstrainedCursorPositions(cur_pos_map, constrained_pos_map, constrained_pos, cur_map_widget);
		
		double rotation = (constrained_pos_map - rotation_center).getAngle();
		double delta_rotation = rotation - old_rotation;
		
		Map::ObjectSelection::const_iterator it_end = map()->selectedObjectsEnd();
		for (Map::ObjectSelection::const_iterator it = map()->selectedObjectsBegin(); it != it_end; ++it)
			(*it)->rotateAround(rotation_center, -1.0 * delta_rotation);
		updatePreviewObjects();
		
		old_rotation = rotation;
		updateStatusText();
	}
}
void RotateTool::dragFinish()
{
	if (rotating)
	{
		rotating = false;
		finishEditing();
		updateDirtyRect();
		updateStatusText();
	}
}

void RotateTool::draw(QPainter* painter, MapWidget* widget)
{
	drawSelectionOrPreviewObjects(painter, widget);
	
	if (rotation_center_set)
	{
		painter->setPen(Qt::white);
		painter->setBrush(Qt::NoBrush);
		
		QPoint center = widget->mapToViewport(rotation_center).toPoint();
		painter->drawEllipse(center, 3, 3);
		painter->setPen(Qt::black);
		painter->drawEllipse(center, 4, 4);
	}
	
	if (rotating)
		angle_helper->draw(painter, widget);
}

bool RotateTool::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Control)
	{
		angle_helper->setActive(true, rotation_center);
		if (dragging)
			dragMove();
	}
    return false;
}
bool RotateTool::keyReleaseEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Control && angle_helper->isActive())
	{
		angle_helper->setActive(false);
		if (dragging)
			dragMove();
		return true;
	}
	return false;
}

int RotateTool::updateDirtyRectImpl(QRectF& rect)
{
	if (rotation_center_set)
	{
		rectIncludeSafe(rect, rotation_center.toQPointF());
		return qMax(angle_helper->getDisplayRadius(), 5);
	}
	else
		return rect.isValid() ? 0 : -1;
}

void RotateTool::objectSelectionChangedImpl()
{
	if (map()->getNumSelectedObjects() == 0)
		deactivate();
	else
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
		setStatusBarText(trUtf8("<b>Rotation:</b> %1°  %2")
			.arg(-delta_rotation * 180 / M_PI, 0, 'f', 1)
			.arg(angle_helper->isActive() ? "" : tr("(<u>Ctrl</u> for fixed angles)")));
	}
	else if (!rotation_center_set)
		setStatusBarText(tr("<b>Click</b> to set the rotation center"));
	else
		setStatusBarText(tr("<b>Click</b> to set the rotation center, <b>drag</b> to rotate the selected object(s)"));
}
