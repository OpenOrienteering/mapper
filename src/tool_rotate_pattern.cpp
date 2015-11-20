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


#include "tool_rotate_pattern.h"

#include <qmath.h>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>

#include "tool_helpers.h"
#include "map_editor.h"
#include "map_widget.h"
#include "object.h"
#include "renderable.h"
#include "util.h"
#include "symbol.h"
#include "symbol_point.h"

RotatePatternTool::RotatePatternTool(MapEditorController* editor, QAction* tool_button)
: MapEditorToolBase(QCursor(QPixmap(":/images/cursor-rotate.png"), 1, 1), Other, editor, tool_button),
  angle_helper(new ConstrainAngleToolHelper())
{
	angle_helper->addDefaultAnglesDeg(0);
	angle_helper->setActive(false);
	connect(angle_helper.data(), SIGNAL(displayChanged()), this, SLOT(updateDirtyRect()));
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
	
	Map::ObjectSelection::const_iterator it_end = editor->getMap()->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = editor->getMap()->selectedObjectsBegin(); it != it_end; ++it)
	{
		Object* object = *it;
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
	MapEditorToolBase::draw(painter, widget);
	
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
	if (editor->getMap()->getNumSelectedObjects() == 0)
		editor->setEditTool();
	else
		updateDirtyRect();
}

void RotatePatternTool::updateStatusText()
{
	if (dragging)
	{
		double rotation = fmod(-1 * (constrained_pos_map - click_pos_map).getAngle() + 3*M_PI/2, 2*M_PI);
		setStatusBarText(trUtf8("<b>Angle:</b> %1°  %2")
			.arg(rotation * 180 / M_PI, 0, 'f', 1)
			.arg(angle_helper->isActive() ? "" : tr("(<u>Ctrl</u> for fixed angles)")));
	}
	else
		setStatusBarText(tr("<b>Drag</b> to set the direction of area fill patterns or point objects"));
}
