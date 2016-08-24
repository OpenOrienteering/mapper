/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2016 Kai Pastor
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
#include <QKeyEvent>
#include <QPainter>

#include "map.h"
#include "map_widget.h"
#include "object.h"
#include "renderable.h"
#include "tool_helpers.h"
#include "util.h"
#include "gui/modifier_key.h"

RotateTool::RotateTool(MapEditorController* editor, QAction* tool_button)
: MapEditorToolBase { QCursor { QString::fromLatin1(":/images/cursor-rotate.png"), 1, 1 }, Other, editor, tool_button }
, angle_helper      { new ConstrainAngleToolHelper() }
{
	angle_helper->setActive(false);
	rotation_center_set = false;
	rotating = false;
}

RotateTool::~RotateTool()
{
	// nothing, not inlined
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
		old_rotation = (cur_pos_map - rotation_center).angle();
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
		
		double rotation = (constrained_pos_map - rotation_center).angle();
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

bool RotateTool::keyPress(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Control)
	{
		angle_helper->setActive(true, rotation_center);
		if (isDragging())
			dragMove();
	}
	return false;
}

bool RotateTool::keyRelease(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Control && angle_helper->isActive())
	{
		angle_helper->setActive(false);
		if (isDragging())
			dragMove();
		return true;
	}
	return false;
}


void RotateTool::drawImpl(QPainter* painter, MapWidget* widget)
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


int RotateTool::updateDirtyRectImpl(QRectF& rect)
{
	if (rotation_center_set)
	{
		rectIncludeSafe(rect, rotation_center);
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
		static const double pi_x_2 = M_PI * 2.0;
		static const double to_deg = 180.0 / M_PI;
		double delta_rotation = old_rotation - original_rotation;
		if (delta_rotation < -M_PI)
			delta_rotation = delta_rotation + pi_x_2;
		else if (delta_rotation > M_PI)
			delta_rotation = delta_rotation - pi_x_2;
		setStatusBarText( trUtf8("<b>Rotation:</b> %1° ").arg(QLocale().toString(-delta_rotation * to_deg, 'f', 1)) + QLatin1String("| ") +
		                  tr("<b>%1</b>: Fixed angles. ").arg(ModifierKey::control()) );
	}
	else if (!rotation_center_set)
	{
		setStatusBarText( tr("<b>Click</b>: Set the center of rotation. ") );
	}
	else
	{
		setStatusBarText( tr("<b>Click</b>: Set the center of rotation. ") +
		                  tr("<b>Drag</b>: Rotate the selected objects. ") );
	}
}
