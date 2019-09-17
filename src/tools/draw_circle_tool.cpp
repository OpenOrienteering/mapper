/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2015 Kai Pastor
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


#include "draw_circle_tool.h"

#include <memory>

#include <Qt>
#include <QtGlobal>
#include <QCursor>
#include <QFlags>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QLatin1String>
#include <QPixmap>
#include <QRectF>
#include <QString>

#include "core/map.h"
#include "core/objects/object.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "gui/modifier_key.h"
#include "gui/widgets/key_button_bar.h"
#include "tools/tool.h"
#include "util/util.h"


namespace OpenOrienteering {

DrawCircleTool::DrawCircleTool(MapEditorController* editor, QAction* tool_action, bool is_helper_tool)
 : DrawLineAndAreaTool(editor, DrawCircle, tool_action, is_helper_tool)
{
	// nothing else
}

DrawCircleTool::~DrawCircleTool()
{
	if (key_button_bar)
		editor->deletePopupWidget(key_button_bar);
}

void DrawCircleTool::init()
{
	updateStatusText();
	
	if (editor->isInMobileMode())
	{
		// Create key replacement bar
		key_button_bar = new KeyButtonBar(editor->getMainWidget());
		key_button_bar->addModifierButton(Qt::ControlModifier, tr("From center", "Draw circle starting from center"));
		editor->showPopupWidget(key_button_bar, QString{});
	}
}

const QCursor& DrawCircleTool::getCursor() const
{
	static auto const cursor = scaledToScreen(QCursor{ QPixmap(QString::fromLatin1(":/images/cursor-draw-circle.png")), 11, 11 });
	return cursor;
}

bool DrawCircleTool::mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	if (isDrawingButton(event->button()))
	{
		cur_pos = event->pos();
		cur_pos_map = map_coord;
		
		if (!first_point_set && event->buttons() & Qt::LeftButton)
		{
			click_pos = event->pos();
			circle_start_pos_map = map_coord;
			opposite_pos_map = map_coord;
			dragging = false;
			first_point_set = true;
			start_from_center = event->modifiers() & Qt::ControlModifier;
			
			if (!editingInProgress())
				startDrawing();
		}
		else if (first_point_set && !second_point_set)
		{
			click_pos = event->pos();
			opposite_pos_map = map_coord;
			dragging = false;
			second_point_set = true;
		}
		else
			return false;
		
		hidePreviewPoints();
		return true;
	}
	else if (event->button() == Qt::RightButton && editingInProgress())
	{
		abortDrawing();
		return true;
	}
	return false;
}

bool DrawCircleTool::mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	bool mouse_down = containsDrawingButtons(event->buttons());
	
	if (!mouse_down)
	{
		if (!editingInProgress())
		{
			setPreviewPointsPosition(map_coord);
			setDirtyRect();
		}
		else
		{
			cur_pos = event->pos();
			cur_pos_map = map_coord;
			if (!second_point_set)
				opposite_pos_map = map_coord;
			updateCircle();
		}
	}
	else
	{
		if (!editingInProgress())
			return false;
		
		if ((event->pos() - click_pos).manhattanLength() >= startDragDistance())
		{
			if (!dragging)
			{
				dragging = true;
				updateStatusText();
			}
		}
		
		if (dragging)
		{
			cur_pos = event->pos();
			cur_pos_map = map_coord;
			if (!second_point_set)
				opposite_pos_map = cur_pos_map;
			updateCircle();
		}
	}
	return true;
}

bool DrawCircleTool::mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	if (!isDrawingButton(event->button()))
	{
		if (event->button() == Qt::RightButton)
			abortDrawing();
		return false;
	}
	if (!editingInProgress())
		return false;
	
	updateStatusText();
	
	if (dragging || second_point_set)
	{
		cur_pos = event->pos();
		cur_pos_map = map_coord;
		if (dragging && !second_point_set)
			opposite_pos_map = map_coord;
		updateCircle();
		finishDrawing();
		return true;
	}
	return false;
}

bool DrawCircleTool::keyPressEvent(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_Escape:
		abortDrawing();
		return true;
		
	case Qt::Key_Tab:
		deactivate();
		return true;
		
	}
	return false;
}

void DrawCircleTool::draw(QPainter* painter, MapWidget* widget)
{
	drawPreviewObjects(painter, widget);
}

void DrawCircleTool::finishDrawing()
{
	dragging = false;
	first_point_set = false;
	second_point_set = false;
	updateStatusText();
	
	DrawLineAndAreaTool::finishDrawing();
	// Do not add stuff here as the tool might get deleted in DrawLineAndAreaTool::finishDrawing()!
}
void DrawCircleTool::abortDrawing()
{
	dragging = false;
	first_point_set = false;
	second_point_set = false;
	updateStatusText();
	
	DrawLineAndAreaTool::abortDrawing();
}

void DrawCircleTool::updateCircle()
{
	MapCoordF first_pos_map;
	if (start_from_center)
		first_pos_map = circle_start_pos_map + (circle_start_pos_map - opposite_pos_map);
	else
		first_pos_map = circle_start_pos_map;
	
	float radius = 0.5f * first_pos_map.distanceTo(opposite_pos_map);
	float kappa = BEZIER_KAPPA;
	float m_kappa = 1 - BEZIER_KAPPA;
	
	MapCoordF across = opposite_pos_map - first_pos_map;
	across.setLength(radius);
	MapCoordF right = across.perpRight();
	
	float right_radius = radius;
	if (second_point_set && dragging)
	{
		if (right.length() < 1e-8)
			right_radius = 0;
		else
		{
			MapCoordF to_cursor = cur_pos_map - first_pos_map;
			right_radius = MapCoordF::dotProduct(to_cursor, right) / right.length();
		}
	}
	right.setLength(right_radius);
	
	preview_path->clearCoordinates();
	preview_path->addCoordinate(MapCoord(first_pos_map, MapCoord::CurveStart));
	preview_path->addCoordinate(MapCoord(first_pos_map + kappa * right));
	preview_path->addCoordinate(MapCoord(first_pos_map + right + m_kappa * across));
	preview_path->addCoordinate(MapCoord(first_pos_map + right + across, MapCoord::CurveStart));
	preview_path->addCoordinate(MapCoord(first_pos_map + right + (1 + kappa) * across));
	preview_path->addCoordinate(MapCoord(first_pos_map + kappa * right + 2 * across));
	preview_path->addCoordinate(MapCoord(first_pos_map + 2 * across, MapCoord::CurveStart));
	preview_path->addCoordinate(MapCoord(first_pos_map - kappa * right + 2 * across));
	preview_path->addCoordinate(MapCoord(first_pos_map - right + (1 + kappa) * across));
	preview_path->addCoordinate(MapCoord(first_pos_map - right + across, MapCoord::CurveStart));
	preview_path->addCoordinate(MapCoord(first_pos_map - right + m_kappa * across));
	preview_path->addCoordinate(MapCoord(first_pos_map - kappa * right));
	preview_path->parts().front().setClosed(true, false);
	
	updatePreviewPath();
	setDirtyRect();
}

void DrawCircleTool::setDirtyRect()
{
	QRectF rect;
	includePreviewRects(rect);
	
	if (is_helper_tool)
		emit dirtyRectChanged(rect);
	else
	{
		if (rect.isValid())
			map()->setDrawingBoundingBox(rect, 0, true);
		else
			map()->clearDrawingBoundingBox();
	}
}

void DrawCircleTool::updateStatusText()
{
	QString text;
	if (!first_point_set || (!second_point_set && dragging))
	{
		text = tr("<b>Click</b>: Start a circle or ellipse. ")
		       + tr("<b>Drag</b>: Draw a circle. ")
		       + QLatin1String("| ")
		       + tr("Hold %1 to start drawing from the center.").arg(ModifierKey::control());
	}
	else
	{
		text = tr("<b>Click</b>: Finish the circle. ")
		       + tr("<b>Drag</b>: Draw an ellipse. ")
		       + OpenOrienteering::MapEditorTool::tr("<b>%1</b>: Abort. ").arg(ModifierKey::escape());
	}
	setStatusBarText(text);
}


}  // namespace OpenOrienteering
