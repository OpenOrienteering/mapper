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


#include "tool_rotate_pattern.h"

#include <qmath.h>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>

#include "map.h"
#include "map_widget.h"
#include "object.h"
#include "renderable.h"
#include "symbol.h"
#include "symbol_point.h"
#include "tool_helpers.h"
#include "util.h"
#include "gui/modifier_key.h"


RotatePatternTool::RotatePatternTool(MapEditorController* editor, QAction* tool_button)
: MapEditorToolBase(QCursor(QPixmap(":/images/cursor-rotate.png"), 1, 1), Other, editor, tool_button),
  angle_helper(new ConstrainAngleToolHelper())
{
	angle_helper->addDefaultAnglesDeg(0);
	angle_helper->setActive(false);
	connect(angle_helper.data(), SIGNAL(displayChanged()), this, SLOT(updateDirtyRect()));
}

RotatePatternTool::~RotatePatternTool()
{
	// nothing, not inlined!
}

void RotatePatternTool::dragStart()
{
	startEditing();
	if (angle_helper->isActive())
		angle_helper->setActive(true, click_pos_map);
}
void RotatePatternTool::dragMove()
{
	angle_helper->getConstrainedCursorPositions(cur_pos_map, constrained_pos_map, constrained_pos, cur_map_widget);
	double rotation = -M_PI / 2 - (constrained_pos_map - click_pos_map).getAngle();
	
	Map::ObjectSelection::const_iterator it_end = map()->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map()->selectedObjectsBegin(); it != it_end; ++it)
	{
		Object* object = *it;
		// TODO: Refactor, provided unified interface for rotation in Object
		if (object->getType() == Object::Point)
		{
			if (object->getSymbol()->asPoint()->isRotatable())
				object->asPoint()->setRotation(rotation);
		}
		else if (object->getType() == Object::Path)
		{
			object->asPath()->setPatternOrigin(click_pos_map.toMapCoord());
			object->asPath()->setPatternRotation(rotation);
		}
	}
	
	updatePreviewObjects();
	updateStatusText();
}
void RotatePatternTool::dragFinish()
{
	finishEditing();
	updateDirtyRect();
	updateStatusText();
}

void RotatePatternTool::draw(QPainter* painter, MapWidget* widget)
{
	drawSelectionOrPreviewObjects(painter, widget);
	
	if (dragging)
	{
		painter->setPen(MapEditorTool::active_color);
		painter->setBrush(Qt::NoBrush);
		
		painter->drawLine(widget->mapToViewport(click_pos_map), widget->mapToViewport(constrained_pos_map));
		
		angle_helper->draw(painter, widget);
	}
}

bool RotatePatternTool::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Control)
	{
		angle_helper->setActive(true, click_pos_map);
		if (dragging)
			dragMove();
	}
	return false;
}
bool RotatePatternTool::keyReleaseEvent(QKeyEvent* event)
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

int RotatePatternTool::updateDirtyRectImpl(QRectF& rect)
{
	if (dragging)
	{
		rectIncludeSafe(rect, click_pos_map.toQPointF());
		rectIncludeSafe(rect, cur_pos_map.toQPointF());
		angle_helper->includeDirtyRect(rect);
	}
	
	return rect.isValid() ? angle_helper->getDisplayRadius() : -1;
}

void RotatePatternTool::objectSelectionChangedImpl()
{
	if (map()->getNumSelectedObjects() == 0)
		deactivate();
	else
		updateDirtyRect();
}

void RotatePatternTool::updateStatusText()
{
	if (dragging)
	{
		static const double pi_x_1_5 = M_PI * 1.5;
		static const double pi_x_2 = M_PI * 2.0;
		static const double to_deg = 180.0 / M_PI;
		double rotation = fmod(-(constrained_pos_map - click_pos_map).getAngle() + pi_x_1_5, pi_x_2) * to_deg;
		setStatusBarText( trUtf8("<b>Angle:</b> %1° ").arg(QLocale().toString(rotation, 'f', 1)) + "| " +
		                  tr("<b>%1</b>: Fixed angles. ").arg(ModifierKey::control()) );
	}
	else
	{
		setStatusBarText(tr("<b>Drag</b>: Set the direction of area fill patterns or point objects. "));
	}
}
