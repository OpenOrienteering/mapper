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


#include "draw_rectangle_tool.h"

#include <limits>
#include <memory>

#include <Qt>
#include <QtGlobal>
#include <QtMath>
#include <QCursor>
#include <QFlags>
#include <QKeyEvent>
#include <QLatin1String>
#include <QLine>
#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QToolButton>
#include <QVariant>

#include "settings.h"
#include "core/map.h"
#include "core/map_view.h"
#include "core/objects/object.h"
#include "core/symbols/symbol.h"
#include "gui/modifier_key.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "gui/widgets/key_button_bar.h"
#include "tools/tool.h"
#include "tools/tool_helpers.h"
#include "util/util.h"


namespace OpenOrienteering {

DrawRectangleTool::DrawRectangleTool(MapEditorController* editor, QAction* tool_action, bool is_helper_tool)
: DrawLineAndAreaTool(editor, DrawRectangle, tool_action, is_helper_tool)
, angle_helper(new ConstrainAngleToolHelper())
, snap_helper(new SnappingToolHelper(this))
{
	cur_map_widget = mapWidget();
	
	angle_helper->addDefaultAnglesDeg(0);
	angle_helper->setActive(false);
	connect(angle_helper.data(), &ConstrainAngleToolHelper::displayChanged, this, &DrawRectangleTool::updateDirtyRect);
	
	snap_helper->setFilter(SnappingToolHelper::AllTypes);
	connect(snap_helper.data(), &SnappingToolHelper::displayChanged, this, &DrawRectangleTool::updateDirtyRect);
}

DrawRectangleTool::~DrawRectangleTool()
{
	if (key_button_bar)
		editor->deletePopupWidget(key_button_bar);
}

void DrawRectangleTool::init()
{
	updateStatusText();
	
	if (editor->isInMobileMode())
	{
		// Create key replacement bar
		key_button_bar = new KeyButtonBar(editor->getMainWidget());
		key_button_bar->addKeyButton(Qt::Key_Return, tr("Finish"));
		key_button_bar->addModifierButton(Qt::ShiftModifier, tr("Snap", "Snap to existing objects"));
		key_button_bar->addModifierButton(Qt::ControlModifier, tr("Line snap", "Snap to previous lines"));
		dash_points_button = key_button_bar->addKeyButton(Qt::Key_Space, tr("Dash", "Drawing dash points"));
		dash_points_button->setCheckable(true);
		dash_points_button->setChecked(draw_dash_points);
		key_button_bar->addKeyButton(Qt::Key_Backspace, tr("Undo"));
		key_button_bar->addKeyButton(Qt::Key_Escape, tr("Abort"));
		editor->showPopupWidget(key_button_bar, QString{});
	}
	
	MapEditorTool::init();
}

const QCursor& DrawRectangleTool::getCursor() const
{
	static auto const cursor = scaledToScreen(QCursor{ QPixmap(QString::fromLatin1(":/images/cursor-draw-rectangle.png")), 11, 11 });
	return cursor;
}

bool DrawRectangleTool::mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	// Adjust flags to have possibly more recent state
	auto modifiers = event->modifiers();
	ctrl_pressed = modifiers & Qt::ControlModifier;
	shift_pressed = modifiers & Qt::ShiftModifier;
	cur_map_widget = widget;
	if (isDrawingButton(event->button()))
	{
		dragging = false;
		click_pos = event->pos();
		click_pos_map = map_coord;
		cur_pos = event->pos();
		cur_pos_map = click_pos_map;
		if (shift_pressed)
			cur_pos_map = MapCoordF(snap_helper->snapToObject(cur_pos_map, widget));
		constrained_pos_map = cur_pos_map;
		
		if (!editingInProgress())
		{
			if (ctrl_pressed)
			{
				// Pick direction
				pickDirection(cur_pos_map, widget);
			}
			else
			{
				// Start drawing
				if (angle_helper->isActive())
					angle_helper->setCenter(click_pos_map);
				startDrawing();
				MapCoord coord = MapCoord(cur_pos_map);
				coord.setDashPoint(draw_dash_points);
				preview_path->addCoordinate(coord);
				preview_path->addCoordinate(coord);
				angles.push_back(0);
				updateStatusText();
			}
		}
		else
		{
			if (angles.size() >= 2 && drawingParallelTo(angles[angles.size() - 2]))
			{
				// Drawing parallel to last section, just move the last point
				undoLastPoint();
			}
			
			// Add new point
			auto cur_point_index = angles.size();
			if (!preview_path->getCoordinate(cur_point_index).isPositionEqualTo(preview_path->getCoordinate(cur_point_index - 1)))
			{
				MapCoord coord = MapCoord(cur_pos_map);
				coord.setDashPoint(draw_dash_points);
				preview_path->addCoordinate(coord);
				if (angles.size() == 1)
				{
					// Bring to correct number of points: line becomes a rectangle
					preview_path->addCoordinate(coord);
				}
				angles.push_back(0);
				
				// preview_path was modified. Non-const getCoordinate is fine.
				angle_helper->setActive(true, MapCoordF(preview_path->getCoordinate(cur_point_index)));
				angle_helper->clearAngles();
				angle_helper->addAngles(angles[0], M_PI/4);
			
				if (event->button() != Qt::RightButton || !drawOnRightClickEnabled())
				{
					updateHover(false);
					updateHover(false); // Call it again, really.
				}
			}
		}
	}
	else if (event->button() == Qt::RightButton && editingInProgress())
	{
		// preview_path is going to be modified. Non-const getCoordinate is fine.
		constrained_pos_map = MapCoordF(preview_path->getCoordinate(angles.size() - 1));
		undoLastPoint();
		if (editingInProgress()) // despite undoLastPoint()
			finishDrawing();
		no_more_effect_on_click = true;
	}
	else
	{
		return false;
	}
	
	return true;
}

bool DrawRectangleTool::mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	cur_pos = event->pos();
	cur_pos_map = map_coord;
	constrained_pos_map = cur_pos_map;
	updateHover(containsDrawingButtons(event->buttons()));
	return true;
}

void DrawRectangleTool::updateHover(bool mouse_down)
{
	if (shift_pressed)
		constrained_pos_map = MapCoordF(snap_helper->snapToObject(cur_pos_map, cur_map_widget));
	else
		constrained_pos_map = cur_pos_map;
	
	if (!editingInProgress())
	{
		setPreviewPointsPosition(constrained_pos_map);
		updateDirtyRect();
		
		if (mouse_down && ctrl_pressed)
			pickDirection(constrained_pos_map, cur_map_widget);
		else if (!mouse_down)
			angle_helper->setCenter(constrained_pos_map);
	}
	else
	{
		hidePreviewPoints();
		if (mouse_down && !dragging && (cur_pos - click_pos).manhattanLength() >= startDragDistance())
		{
			// Start dragging
			dragging = true;
		}
		if (!mouse_down || dragging)
			updateRectangle();
	}
}

bool DrawRectangleTool::mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	cur_pos = event->pos();
	cur_pos_map = map_coord;
	if (shift_pressed)
		cur_pos_map = MapCoordF(snap_helper->snapToObject(cur_pos_map, widget));
	constrained_pos_map = cur_pos_map;
	
	if (no_more_effect_on_click)
	{
		no_more_effect_on_click = false;
		return true;
	}
	
	if (ctrl_pressed && event->button() == Qt::LeftButton && !editingInProgress())
	{
		pickDirection(cur_pos_map, widget);
		return true;
	}
	
	bool result = false;
	if (editingInProgress())
	{
		if (isDrawingButton(event->button()) && dragging)
		{
			dragging = false;
			result = mousePressEvent(event, cur_pos_map, widget);
		}
		
		if (event->button() == Qt::RightButton && drawOnRightClickEnabled())
		{
			if (!dragging)
			{
				// preview_path is going to be modified. Non-const getCoordinate is fine.
				constrained_pos_map = MapCoordF(preview_path->getCoordinate(angles.size() - 1));
				undoLastPoint();
			}
			if (editingInProgress()) // despite undoLastPoint()
				finishDrawing();
			return true;
		}
	}
	return result;
}

bool DrawRectangleTool::mouseDoubleClickEvent(QMouseEvent* event, const MapCoordF& /*map_coord*/, MapWidget* /*widget*/)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	if (editingInProgress())
	{
		// Finish drawing by double click.
		// As the double click is also reported as two single clicks first
		// (in total: press - release - press - double - release),
		// need to undo two steps in general.
		if (angles.size() > 2 && !ctrl_pressed)
		{
			undoLastPoint();
			updateHover(false);
		}
		
		if (angles.size() <= 1)
		{
			abortDrawing();
		}
		else
		{
			// preview_path is going to be modified. Non-const getCoordinate is fine.
			constrained_pos_map = MapCoordF(preview_path->getCoordinate(angles.size() - 1));
			undoLastPoint();
			finishDrawing();
		}
		no_more_effect_on_click = true;
	}
	return true;
}

bool DrawRectangleTool::keyPressEvent(QKeyEvent* event)
{
	switch(event->key())
	{
	case Qt::Key_Return:
		if (editingInProgress())
		{
			if (angles.size() <= 1)
			{
				abortDrawing();
			}
			else
			{
				// preview_path is going to be modified. Non-const getCoordinate is fine.
				constrained_pos_map = MapCoordF(preview_path->getCoordinate(angles.size() - 1));
				undoLastPoint();
				finishDrawing();
			}
			return true;
		}
		break;
		
	case Qt::Key_Escape:
		if (editingInProgress())
		{
			abortDrawing();
			return true;
		}
		break;
		
	case Qt::Key_Backspace:
		if (editingInProgress())
		{
			undoLastPoint();
			updateHover(false);
			return true;
		}
		break;
		
	case Qt::Key_Tab:
		deactivate();
		return true;
		
	case Qt::Key_Space:
		draw_dash_points = !draw_dash_points;
		if (dash_points_button)
			dash_points_button->setChecked(draw_dash_points);
		updateStatusText();
		return true;
		
	case Qt::Key_Control:
		ctrl_pressed = true;
		if (editingInProgress() && angles.size() == 1)
		{
			angle_helper->clearAngles();
			angle_helper->addDefaultAnglesDeg(0);
			angle_helper->setActive(true, MapCoordF(preview_path->getCoordinate(0)));
			if (dragging)
				updateRectangle();
		}
		else if (editingInProgress() && angles.size() > 2)
		{
			// Try to snap to existing lines
			updateRectangle();
		}
		updateStatusText();
		return false; // not consuming Ctrl
		
	case Qt::Key_Shift:
		shift_pressed = true;
		updateHover(false);
		updateStatusText();
		return false; // not consuming Shift
		
	}
	return false;
}

bool DrawRectangleTool::keyReleaseEvent(QKeyEvent* event)
{
	switch(event->key())
	{
	case Qt::Key_Control:
		ctrl_pressed = false;
		if (!picked_direction && (!editingInProgress() || angles.size() == 1))
		{
			angle_helper->setActive(false);
			if (dragging && editingInProgress())
				updateRectangle();
		}
		else if (editingInProgress() && angles.size() > 2)
		{
			updateRectangle();
		}
		if (picked_direction)
			picked_direction = false;
		updateStatusText();
		return false; // not consuming Ctrl
	
	case Qt::Key_Shift:
		shift_pressed = false;
		updateHover(false);
		updateStatusText();
		return false; // not consuming Shift
		
	}
	return false;
}

void DrawRectangleTool::draw(QPainter* painter, MapWidget* widget)
{
	drawPreviewObjects(painter, widget);
	
	if (editingInProgress())
	{
		// Snap line
		if (snapped_to_line)
		{
			// Simple hack to make line extend over the whole screen in most cases
			const auto scale_factor = 100;
			MapCoord a = snapped_to_line_b + (snapped_to_line_a - snapped_to_line_b) * scale_factor;
			MapCoord b = snapped_to_line_b - (snapped_to_line_a - snapped_to_line_b) * scale_factor;
			
			painter->setPen(selection_color);
			painter->drawLine(widget->mapToViewport(a), widget->mapToViewport(b));
		}
		
		// Helper cross
		bool use_preview_radius = Settings::getInstance().getSettingCached(Settings::RectangleTool_PreviewLineWidth).toBool();
		const double preview_radius_min_pixels = 2.5;
		if (widget->getMapView()->lengthToPixel(preview_point_radius) < preview_radius_min_pixels)
			use_preview_radius = false;
		
		auto helper_cross_radius = Settings::getInstance().getRectangleToolHelperCrossRadiusPx();
		painter->setRenderHint(QPainter::Antialiasing);
		
		auto perp_vector = forward_vector.perpRight();
		
		painter->setPen((angles.size() > 1) ? inactive_color : active_color);
		if (preview_point_radius == 0 || !use_preview_radius)
		{
			painter->drawLine(widget->mapToViewport(cur_pos_map) + helper_cross_radius * forward_vector,
							  widget->mapToViewport(cur_pos_map) - helper_cross_radius * forward_vector);
		}
		else
		{
			painter->drawLine(widget->mapToViewport(cur_pos_map + preview_point_radius * perp_vector / 1000) + helper_cross_radius * forward_vector,
							  widget->mapToViewport(cur_pos_map + preview_point_radius * perp_vector / 1000) - helper_cross_radius * forward_vector);
			painter->drawLine(widget->mapToViewport(cur_pos_map - preview_point_radius * perp_vector / 1000) + helper_cross_radius * forward_vector,
							  widget->mapToViewport(cur_pos_map - preview_point_radius * perp_vector / 1000) - helper_cross_radius * forward_vector);
		}
		
		painter->setPen(active_color);
		if (preview_point_radius == 0 || !use_preview_radius)
		{
			painter->drawLine(widget->mapToViewport(cur_pos_map) + helper_cross_radius * perp_vector,
							  widget->mapToViewport(cur_pos_map) - helper_cross_radius * perp_vector);
		}
		else
		{
			painter->drawLine(widget->mapToViewport(cur_pos_map + preview_point_radius * forward_vector / 1000) + helper_cross_radius * perp_vector,
							  widget->mapToViewport(cur_pos_map + preview_point_radius * forward_vector / 1000) - helper_cross_radius * perp_vector);
			painter->drawLine(widget->mapToViewport(cur_pos_map - preview_point_radius * forward_vector / 1000) + helper_cross_radius * perp_vector,
							  widget->mapToViewport(cur_pos_map - preview_point_radius * forward_vector / 1000) - helper_cross_radius * perp_vector);
		}
	}
	
	angle_helper->draw(painter, widget);
	
	if (shift_pressed)
		snap_helper->draw(painter, widget);
}

void DrawRectangleTool::finishDrawing()
{
	snapped_to_line = false;
	if (angles.size() == 1 && preview_path &&
		!(preview_path->getSymbol()->getContainedTypes() & Symbol::Line))
	{
		// Do not draw a line object with an area-only symbol
		return;
	}
	
	if (angles.size() > 1 && drawingParallelTo(angles[0]))
	{
		// Delete first point
		deleteClosePoint();
		preview_path->deleteCoordinate(0, false);
		preview_path->parts().front().setClosed(true, true);
	}
	
	angle_helper->setActive(false);
	angles.clear();
	setEditingInProgress(false);
	updateStatusText();
	DrawLineAndAreaTool::finishDrawing();
	// HACK: do not add stuff here as the tool might get deleted on call to DrawLineAndAreaTool::finishDrawing()!
}

void DrawRectangleTool::abortDrawing()
{
	snapped_to_line = false;
	angle_helper->setActive(false);
	angles.clear();
	setEditingInProgress(false);
	updateStatusText();
	DrawLineAndAreaTool::abortDrawing();
	// HACK: do not add stuff here as the tool might get deleted on call to DrawLineAndAreaTool::finishDrawing()!
}

void DrawRectangleTool::undoLastPoint()
{
	if (angles.size() == 1)
	{
		abortDrawing();
		return;
	}
	
	deleteClosePoint();
	preview_path->deleteCoordinate(preview_path->getCoordinateCount() - 1, false);
	if (angles.size() == 2)
		preview_path->deleteCoordinate(preview_path->getCoordinateCount() - 1, false);
	
	angles.pop_back();
	forward_vector = MapCoordF(1, 0);
	forward_vector.rotate(-angles[angles.size() - 1]);
	
	// preview_path was modified. Non-const getCoordinate is fine.
	angle_helper->setCenter(MapCoordF(preview_path->getCoordinate(angles.size() - 1)));
	
	updateRectangle();
}

void DrawRectangleTool::pickDirection(const MapCoordF& coord, MapWidget* widget)
{
	MapCoord snap_position;
	snap_helper->snapToDirection(coord, widget, angle_helper.data(), &snap_position);
	angle_helper->setActive(true, MapCoordF(snap_position));
	updateDirtyRect();
	picked_direction = true;
}

bool DrawRectangleTool::drawingParallelTo(double angle) const
{
	const double epsilon = 0.01;
	double cur_angle = angles[angles.size() - 1];
	return qAbs(fmod_pos(cur_angle, M_PI) - fmod_pos(angle, M_PI)) < epsilon;
}

MapCoordF DrawRectangleTool::calculateClosingVector() const
{
	auto cur_point_index = angles.size();
	auto close_vector = MapCoordF(1, 0);
	close_vector.rotate(-angles[0]);
	if (drawingParallelTo(angles[0]))
		close_vector = close_vector.perpRight();
	if (MapCoordF::dotProduct(close_vector, MapCoordF(preview_path->getCoordinate(0) - preview_path->getCoordinate(cur_point_index - 1))) < 0)
		close_vector = -close_vector;
	return close_vector;
}

void DrawRectangleTool::deleteClosePoint()
{
	if (preview_path->parts().front().isClosed())
	{
		auto cur_point_index = angles.size();
		preview_path->parts().front().setClosed(false);
		while (preview_path->getCoordinateCount() > cur_point_index + 2)
			preview_path->deleteCoordinate(preview_path->getCoordinateCount() - 1, false);
	}
}

void DrawRectangleTool::updateRectangle()
{
	double angle = angle_helper->getConstrainedCursorPosMap(constrained_pos_map, constrained_pos_map);
	
	if (angles.size() == 1)
	{
		// Update vectors and angles
		// preview_path is going to be modified. Non-const getCoordinate is fine.
		forward_vector = constrained_pos_map - MapCoordF(preview_path->getCoordinate(0));
		forward_vector.normalize();
		
		angles.back() = -forward_vector.angle();
		
		// Update rectangle
		MapCoord coord = MapCoord(constrained_pos_map);
		coord.setDashPoint(draw_dash_points);
		preview_path->setCoordinate(1, coord);
	}
	else
	{
		// Update vectors and angles
		forward_vector = MapCoordF::fromPolar(1.0, -angle);
		
		angles.back() = angle;
		
		// Update rectangle
		auto cur_point_index = angles.size();
		deleteClosePoint();
		
		// preview_path was be modified. Non-const getCoordinate is fine.
		auto forward_dist = MapCoordF::dotProduct(forward_vector, constrained_pos_map - MapCoordF(preview_path->getCoordinate(cur_point_index - 1)));
		MapCoord coord = preview_path->getCoordinate(cur_point_index - 1) + MapCoord(forward_dist * forward_vector);
		
		snapped_to_line = false;
		auto best_snap_distance_sq = std::numeric_limits<qreal>::max();
		if (ctrl_pressed && angles.size() > 2)
		{
			// Try to snap to existing lines
			MapCoord original_coord = coord;
			for (int i = 0; i < int(angles.size()) - 1; ++i)
			{
				MapCoordF direction(100, 0);
				direction.rotate(-angles[i]);
				
				int num_steps;
				double angle_step, angle_offset = 0;
				if (i == 0 || qAbs(qAbs(fmod_pos(angles[i], M_PI) - fmod_pos(angles[i-1], M_PI)) - (M_PI / 2)) < 0.1)
				{
					num_steps = 2;
					angle_step = M_PI/2;
				}
				else
				{
					num_steps = 4;
					angle_step = M_PI/4;
				}
				
				for (int k = 0; k < num_steps; ++k)
				{
					if (qAbs(fmod_pos(angle, M_PI) - fmod_pos(angles[i] - (angle_offset + k * angle_step), M_PI)) < 0.1)
						continue;
					
					MapCoordF rotated_direction = direction;
					rotated_direction.rotate(angle_offset + k * angle_step);
					
					QLineF a(QPointF(preview_path->getCoordinate(cur_point_index - 1)), MapCoordF(preview_path->getCoordinate(cur_point_index - 1)) + forward_vector);
					QLineF b(QPointF(preview_path->getCoordinate(i)), MapCoordF(preview_path->getCoordinate(i)) + rotated_direction);
					QPointF intersection_point;
					QLineF::IntersectType intersection_type = a.intersect(b, &intersection_point);
					if (intersection_type == QLineF::NoIntersection)
						continue;
					
					auto snap_distance_sq = original_coord.distanceSquaredTo(MapCoord(intersection_point));
					if (snap_distance_sq > best_snap_distance_sq)
						continue;
					
					best_snap_distance_sq = snap_distance_sq;
					coord = MapCoord(intersection_point);
					snapped_to_line_a = coord;
					snapped_to_line_b = coord + MapCoord(rotated_direction);
					snapped_to_line = true;
				}
			}
		}
		
		coord.setDashPoint(draw_dash_points);
		preview_path->setCoordinate(cur_point_index, coord);
		
		auto close_vector = calculateClosingVector();
		auto close_dist = MapCoordF::dotProduct(close_vector, MapCoordF(preview_path->getCoordinate(0) - preview_path->getCoordinate(cur_point_index)));
		coord = preview_path->getCoordinate(cur_point_index) + MapCoord(close_dist * close_vector);
		coord.setDashPoint(draw_dash_points);
		preview_path->setCoordinate(cur_point_index + 1, coord);
		
		preview_path->parts().front().setClosed(true, true);
	}
	
	updatePreviewPath();
	updateDirtyRect();
}

void DrawRectangleTool::updateDirtyRect()
{
	QRectF rect;
	includePreviewRects(rect);
	
	if (shift_pressed)
		snap_helper->includeDirtyRect(rect);
	if (is_helper_tool)
		emit dirtyRectChanged(rect);
	else
	{
		if (angle_helper->isActive())
			angle_helper->includeDirtyRect(rect);
		if (rect.isValid())
		{
			int pixel_border = 0;
			if (editingInProgress())
				// helper_cross_radius as border is less than ideal but the only way to always ensure visibility of the helper cross at the moment
				pixel_border = qCeil(Settings::getInstance().getRectangleToolHelperCrossRadiusPx());
			if (angle_helper->isActive())
				pixel_border = qMax(pixel_border, angle_helper->getDisplayRadius());
			map()->setDrawingBoundingBox(rect, pixel_border, true);
		}
		else
			map()->clearDrawingBoundingBox();
	}
}

void DrawRectangleTool::updateStatusText()
{
	QString text;
	static const QString text_more_shift_control_space(::OpenOrienteering::MapEditorTool::tr("More: %1, %2, %3").arg(ModifierKey::shift(), ModifierKey::control(), ModifierKey::space()));
	static const QString text_more_shift_space(::OpenOrienteering::MapEditorTool::tr("More: %1, %2").arg(ModifierKey::shift(), ModifierKey::space()));
	static const QString text_more_control_space(::OpenOrienteering::MapEditorTool::tr("More: %1, %2").arg(ModifierKey::control(), ModifierKey::space()));
	QString text_more(text_more_shift_control_space);
	
	if (draw_dash_points)
		text += ::OpenOrienteering::DrawLineAndAreaTool::tr("<b>Dash points on.</b> ") + QLatin1String("| ");
	
	if (!editingInProgress())
	{
		if (ctrl_pressed)
		{
			text += ::OpenOrienteering::DrawLineAndAreaTool::tr("<b>%1+Click</b>: Pick direction from existing objects. ").arg(ModifierKey::control());
			text_more = text_more_shift_space;
		}
		else if (shift_pressed)
		{
			text += ::OpenOrienteering::DrawLineAndAreaTool::tr("<b>%1+Click</b>: Snap to existing objects. ").arg(ModifierKey::shift());
			text_more = text_more_control_space;
		}
		else
		{
			text += tr("<b>Click or Drag</b>: Start drawing a rectangle. ");	
		}
	}
	else
	{
		if (ctrl_pressed)
		{
			if (angles.size() == 1)
			{
				text += ::OpenOrienteering::DrawLineAndAreaTool::tr("<b>%1</b>: Fixed angles. ").arg(ModifierKey::control());
			}
			else
			{
				text += tr("<b>%1</b>: Snap to previous lines. ").arg(ModifierKey::control());
			}
			text_more = text_more_shift_space;
		}
		else if (shift_pressed)
		{
			text += ::OpenOrienteering::DrawLineAndAreaTool::tr("<b>%1+Click</b>: Snap to existing objects. ").arg(ModifierKey::shift());
			text_more = text_more_control_space;
		}
		else
		{
			text += tr("<b>Click</b>: Set a corner point. <b>Right or double click</b>: Finish the rectangle. ");
			text += ::OpenOrienteering::DrawLineAndAreaTool::tr("<b>%1</b>: Undo last point. ").arg(ModifierKey::backspace());
			text += ::OpenOrienteering::MapEditorTool::tr("<b>%1</b>: Abort. ").arg(ModifierKey::escape());
		}
	}
	
	text += QLatin1String("| ") + text_more;
	
	setStatusBarText(text);
}


}  // namespace OpenOrienteering
