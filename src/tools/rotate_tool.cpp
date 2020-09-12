/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2017 Kai Pastor
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


#include "rotate_tool.h"

#include <memory>

#include <Qt>
#include <QtMath>
#include <QKeyEvent>
#include <QPainter>
#include <QCursor>
#include <QLatin1String>
#include <QLocale>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QString>

#include "core/map.h"
#include "core/map_view.h"
#include "core/objects/object.h"
#include "gui/modifier_key.h"
#include "gui/map/map_widget.h"
#include "gui/util_gui.h"
#include "tools/tool.h"
#include "tools/tool_helpers.h"
#include "util/util.h"


namespace OpenOrienteering {

RotateTool::RotateTool(MapEditorController* editor, QAction* tool_action)
: MapEditorToolBase { QCursor { QString::fromLatin1(":/images/cursor-rotate.png"), 1, 1 }, Other, editor, tool_action }
{
	// nothing else
}

RotateTool::~RotateTool() = default;



void RotateTool::initImpl()
{
	// Set initial rotation center to the bounding box center of the selected objects
	if (map()->getNumSelectedObjects() > 0)
	{
		QRectF rect;
		map()->includeSelectionRect(rect);
		rotation_center = rect.center();
	}
	else
	{
		rotation_center = MapCoordF(cur_map_widget->getMapView()->center());
	}
	angle_helper->setCenter(rotation_center);
}


void RotateTool::mouseMove()
{
	updateStatusText();
}

void RotateTool::clickRelease()
{
	rotation_center = cur_pos_map;
	angle_helper->setCenter(rotation_center);
	updateDirtyRect();
	updateStatusText();
}


void RotateTool::dragStart()
{
	original_rotation = (click_pos_map - rotation_center).angle();
	startEditing(map()->selectedObjects());
}

void RotateTool::dragMove()
{
	current_rotation = (constrained_pos_map - rotation_center).angle() - original_rotation;
	cur_map_widget->update();
	updateStatusText();
}

void RotateTool::dragFinish()
{
	if (qAbs(current_rotation) > 0.0001)
	{
		for (auto object : editedObjects())
			object->rotateAround(rotation_center, -current_rotation);
	}
	original_rotation = current_rotation = 0;
	finishEditing();
	updateDirtyRect();
	updateStatusText();
}

bool RotateTool::keyPress(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_Control:
		angle_helper->clearAngles();
		angle_helper->addDefaultAnglesDeg(-qRadiansToDegrees(original_rotation));
		angle_helper->setActive(true, rotation_center);
		reapplyConstraintHelpers();
		break;
		
	default:
		; // nothing
	}
	return false;
}

bool RotateTool::keyRelease(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_Control:
		angle_helper->setActive(false);
		reapplyConstraintHelpers();
		return false; // not consuming Ctrl
		
	}
	return false;
}


void RotateTool::drawImpl(QPainter* painter, MapWidget* widget)
{
	const auto center = widget->mapToViewport(rotation_center);
	
	painter->save();
	if (isDragging())
	{
		painter->translate(center);
		painter->rotate(qRadiansToDegrees(current_rotation));
		painter->translate(-center);
	}
	map()->drawSelection(painter, true, widget);
	painter->restore();
	
	Util::Marker::drawCenterMarker(painter, center);
}


int RotateTool::updateDirtyRectImpl(QRectF& rect)
{
	rectIncludeSafe(rect, rotation_center);
	return 5;
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
	QString text;
	if (isDragging())
	{
		auto display_rotation = qRadiansToDegrees(current_rotation);
		if (display_rotation <= -180)
			display_rotation += 360;
		else if (display_rotation > 180)
			display_rotation -= 360;
		else if (display_rotation > -0.05 && display_rotation < 0.0)
			display_rotation = +0.0;
		text = tr("<b>Rotation:</b> %1° ").arg(QLocale().toString(display_rotation, 'f', 1));
		if (!angle_helper->isActive())
			text += QLatin1String("| ");
	}
	else
	{
		text = tr("<b>Click</b>: Set the center of rotation. ") +
		       tr("<b>Drag</b>: Rotate the selected objects. ");
	}
	
	if (!angle_helper->isActive())
	{
		text += tr("<b>%1</b>: Fixed angles. ").arg(ModifierKey::control());
	}
	
	setStatusBarText(text);
}


}  // namespace OpenOrienteering
