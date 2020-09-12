/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2019 Kai Pastor
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


#include "rotate_pattern_tool.h"

#include <cmath>
#include <memory>

#include <Qt>
#include <QtMath>
#include <QCursor>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLatin1String>
#include <QLocale>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QString>

#include "core/map.h"
#include "core/map_coord.h"
#include "gui/map/map_widget.h"
#include "core/objects/object.h"
#include "core/symbols/symbol.h"
#include "gui/modifier_key.h"
#include "tools/tool.h"
#include "tools/tool_helpers.h"
#include "util/util.h"


namespace OpenOrienteering {

RotatePatternTool::RotatePatternTool(MapEditorController* editor, QAction* tool_action)
: MapEditorToolBase(QCursor(QPixmap(QString::fromLatin1(":/images/cursor-rotate.png")), 1, 1), Other, editor, tool_action)
{
	// Nothing else here, but there is initImpl().
}


RotatePatternTool::~RotatePatternTool() = default;



void RotatePatternTool::initImpl()
{
	angle_helper->addDefaultAnglesDeg(0);
}



// This function contains translations. Keep it close to the top of the file so
// that line numbers remain stable here when changing other parts of the file.
void RotatePatternTool::updateStatusText()
{
	QString text;
	if (isDragging())
	{
		constexpr auto pi_x_1_5 = M_PI * 1.5;
		constexpr auto pi_x_2 = M_PI * 2.0;
		constexpr auto to_deg = 180.0 / M_PI;
		const auto rotation = std::fmod(-(constrained_pos_map - click_pos_map).angle() + pi_x_1_5, pi_x_2) * to_deg;
		text = tr("<b>Angle:</b> %1° ").arg(QLocale().toString(rotation, 'f', 1));
	}
	else
	{
		text = tr("<b>Drag</b>: Set the direction of area fill patterns or point objects. ");
	}
	if (!active_modifiers.testFlag(Qt::ControlModifier))
	{
		if (isDragging())
			text +=  QLatin1String("| ");
		text += tr("<b>%1</b>: Fixed angles. ").arg(ModifierKey::control());
	}
	setStatusBarText(text);
}



bool RotatePatternTool::keyPressEvent(QKeyEvent* event)
{
	switch(event->key())
	{
	case Qt::Key_Control:
		active_modifiers |= Qt::ControlModifier;
		
		auto active = QGuiApplication::mouseButtons().testFlag(Qt::LeftButton);
		angle_helper->setActive(active, click_pos_map);
		if (isDragging())
		{
			updateConstrainedPositions();
			dragMove();
		}
		else if (active)
		{
			updateDirtyRect(); // for active angle helper
		}
		updateStatusText();
		return false; // not consuming Ctrl
		
	}
	return false;
}


bool RotatePatternTool::keyReleaseEvent(QKeyEvent* event)
{
	switch(event->key())
	{
	case Qt::Key_Control:
		active_modifiers &= ~Qt::ControlModifier;
		
		if (angle_helper->isActive())
		{
			if (!isDragging())
			{
				updateDirtyRect();
			}
			angle_helper->setActive(false);
			if (isDragging())
			{
				updateConstrainedPositions();
				dragMove();
			}
		}
		updateStatusText();
		return false; // not consuming Ctrl
		
	}
	return false;
}



void RotatePatternTool::clickPress()
{
	angle_helper->setActive(active_modifiers.testFlag(Qt::ControlModifier), click_pos_map);
	updateDirtyRect(); // for active angle helper
	updateStatusText();
}


void RotatePatternTool::clickRelease()
{
	updateDirtyRect(); // for active angle helper
	angle_helper->setActive(false);
	updateStatusText();
}


void RotatePatternTool::dragStart()
{
	startEditing(map()->selectedObjects());
	updateStatusText();
}


void RotatePatternTool::dragMove()
{
	const auto rotation = -M_PI / 2 - (constrained_pos_map - click_pos_map).angle();
	
	for (auto object : editedObjects())
	{
		if (object->getSymbol()->isRotatable())
		{
			object->setRotation(rotation);
		}
		else if (object->getType() == Object::Path)
		{
			object->asPath()->setPatternOrigin(MapCoord(click_pos_map));
			object->asPath()->setPatternRotation(rotation);
		}
	}
	
	updatePreviewObjects();
	updateStatusText();
}


void RotatePatternTool::dragFinish()
{
	finishEditing();
	angle_helper->setActive(false);
	updateStatusText();
}



void RotatePatternTool::drawImpl(QPainter* painter, MapWidget* widget)
{
	drawSelectionOrPreviewObjects(painter, widget);
	
	if (isDragging())
	{
		painter->setPen(MapEditorTool::active_color);
		painter->setBrush(Qt::NoBrush);
		painter->drawLine(widget->mapToViewport(click_pos_map), widget->mapToViewport(constrained_pos_map));
	}
}



int RotatePatternTool::updateDirtyRectImpl(QRectF& rect)
{
	if (isDragging())
	{
		rectIncludeSafe(rect, click_pos_map);
		rectIncludeSafe(rect, constrained_pos_map);
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


}  // namespace OpenOrienteering
