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


#include "tool_draw_rectangle.h"

#include <limits>

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "util.h"
#include "object.h"
#include "map_widget.h"
#include "settings.h"
#include "tool_helpers.h"
#include "symbol_dock_widget.h"
#include "gui/modifier_key.h"

QCursor* DrawRectangleTool::cursor = NULL;

DrawRectangleTool::DrawRectangleTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget)
 : DrawLineAndAreaTool(editor, DrawRectangle, tool_button, symbol_widget),
   angle_helper(new ConstrainAngleToolHelper()),
   snap_helper(new SnappingToolHelper(map()))
{
	cur_map_widget = mapWidget();
	draw_dash_points = true;
	shift_pressed = false;
	ctrl_pressed = false;
	picked_direction = false;
	snapped_to_line = false;
	no_more_effect_on_click = false;
	
	angle_helper->addDefaultAnglesDeg(0);
	angle_helper->setActive(false);
	connect(angle_helper.data(), SIGNAL(displayChanged()), this, SLOT(updateDirtyRect()));
	
	snap_helper->setFilter(SnappingToolHelper::AllTypes);
	connect(snap_helper.data(), SIGNAL(displayChanged()), this, SLOT(updateDirtyRect()));
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-draw-rectangle.png"), 11, 11);
}

DrawRectangleTool::~DrawRectangleTool()
{
}

void DrawRectangleTool::init()
{
	updateStatusText();
}

bool DrawRectangleTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	ctrl_pressed = event->modifiers() & Qt::ControlModifier;
	shift_pressed = event->modifiers() & Qt::ShiftModifier;
	cur_map_widget = widget;
	if (event->button() == Qt::LeftButton || (draw_in_progress && drawMouseButtonClicked(event)))
	{
		dragging = false;
		click_pos = event->pos();
		click_pos_map = map_coord;
		cur_pos = event->pos();
		cur_pos_map = click_pos_map;
		if (shift_pressed)
			cur_pos_map = MapCoordF(snap_helper->snapToObject(cur_pos_map, widget));
		constrained_pos_map = cur_pos_map;
		
		if (!draw_in_progress)
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
				MapCoord coord = cur_pos_map.toMapCoord();
				coord.setDashPoint(draw_dash_points);
				preview_path->addCoordinate(coord);
				preview_path->addCoordinate(coord);
				angles.push_back(0);
				updateStatusText();
			}
		}
		else
		{
			// Add a point to the path
			updateRectangle();
			
			if (angles.size() >= 2 && drawingParallelTo(angles[angles.size() - 2]))
			{
				// Drawing parallel to last section, just move the last point
				undoLastPoint();
			}
			
			// Add new point
			int cur_point_index = angles.size();
			if (preview_path->getCoordinate(cur_point_index).isPositionEqualTo(preview_path->getCoordinate(cur_point_index - 1)))
				return true;
			
			MapCoord coord = cur_pos_map.toMapCoord();
			coord.setDashPoint(draw_dash_points);
			preview_path->addCoordinate(coord);
			if (angles.size() == 1)
			{
				// Bring to correct number of points: line becomes a rectangle
				preview_path->addCoordinate(coord);
			}
			angles.push_back(0);
			++cur_point_index;
			
			angle_helper->setActive(true, MapCoordF(preview_path->getCoordinate(cur_point_index - 1)));
			angle_helper->clearAngles();
			angle_helper->addAngles(angles[0], M_PI/4);
		}
	}
	else if (event->button() == Qt::RightButton && draw_in_progress)
	{
		constrained_pos_map = MapCoordF(preview_path->getCoordinate(angles.size() - 1));
		undoLastPoint();
		if (draw_in_progress)
			finishDrawing();
	}
	else
		return false;
	return true;
}

bool DrawRectangleTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	cur_pos = event->pos();
	cur_pos_map = map_coord;
	constrained_pos_map = cur_pos_map;
	updateHover(drawMouseButtonHeld(event));
	return true;
}

void DrawRectangleTool::updateHover(bool mouse_down)
{
	if (shift_pressed)
		constrained_pos_map = MapCoordF(snap_helper->snapToObject(cur_pos_map, cur_map_widget));
	else
		constrained_pos_map = cur_pos_map;
	
	if (!draw_in_progress)
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
		if (mouse_down && !dragging && (cur_pos - click_pos).manhattanLength() >= QApplication::startDragDistance())
		{
			// Start dragging
			dragging = true;
		}
		updateRectangle();
	}
}

bool DrawRectangleTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
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
	
	if (ctrl_pressed && event->button() == Qt::LeftButton && !draw_in_progress)
	{
		pickDirection(cur_pos_map, widget);
		return true;
	}
	
	bool result = false;
	if (draw_in_progress)
	{
		if (drawMouseButtonClicked(event) && dragging)
		{
			dragging = false;
			result = mousePressEvent(event, cur_pos_map, widget);
		}
		
		if (event->button() == Qt::RightButton && Settings::getInstance().getSettingCached(Settings::MapEditor_DrawLastPointOnRightClick).toBool())
		{
			if (!dragging)
			{
				cur_pos_map = MapCoordF(preview_path->getCoordinate(angles.size() - 1));
				undoLastPoint();
			}
			finishDrawing();
			return true;
		}
	}
	return result;
}

bool DrawRectangleTool::mouseDoubleClickEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	if (draw_in_progress)
	{
		// Finish drawing by double click.
		// As the double click is also reported as two single clicks first
		// (in total: press - release - press - double - release),
		// need to undo two steps in general.
		if (angles.size() <= 1)
			abortDrawing();
		else
		{
			if (angles.size() > 2 && !ctrl_pressed)
				undoLastPoint();
			cur_pos_map = MapCoordF(preview_path->getCoordinate(angles.size() - 1));
			undoLastPoint();
			finishDrawing();
		}
		no_more_effect_on_click = true;
	}
	return true;
}

bool DrawRectangleTool::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape)
		abortDrawing();
	else if (event->key() == Qt::Key_Backspace)
		undoLastPoint();
	else if (event->key() == Qt::Key_Tab)
		deactivate();
	else if (event->key() == Qt::Key_Space)
	{
		draw_dash_points = !draw_dash_points;
		updateStatusText();
	}
	else if (event->key() == Qt::Key_Control)
	{
		ctrl_pressed = true;
		if (draw_in_progress && angles.size() == 1)
		{
			angle_helper->clearAngles();
			angle_helper->addDefaultAnglesDeg(0);
			angle_helper->setActive(true, MapCoordF(preview_path->getCoordinate(0)));
			if (dragging)
				updateRectangle();
		}
		else if (draw_in_progress && angles.size() > 2)
		{
			// Try to snap to existing lines
			updateRectangle();
		}
		updateStatusText();
	}
	else if (event->key() == Qt::Key_Shift)
	{
		shift_pressed = true;
		updateHover(false);
		updateStatusText();
	}
	else
		return false;
	return true;
}

bool DrawRectangleTool::keyReleaseEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Control)
	{
		ctrl_pressed = false;
		if (!picked_direction && (!draw_in_progress || (draw_in_progress && angles.size() == 1)))
		{
			angle_helper->setActive(false);
			if (dragging && draw_in_progress)
				updateRectangle();
		}
		else if (draw_in_progress && angles.size() > 2)
		{
			updateRectangle();
		}
		if (picked_direction)
			picked_direction = false;
		updateStatusText();
	}
	else if (event->key() == Qt::Key_Shift)
	{
		shift_pressed = false;
		updateHover(false);
		updateStatusText();
	}
	else
		return false;
	return true;
}

void DrawRectangleTool::draw(QPainter* painter, MapWidget* widget)
{
	drawPreviewObjects(painter, widget);
	
	if (draw_in_progress)
	{
		// Snap line
		if (snapped_to_line)
		{
			// Simple hack to make line extend over the whole screen in most cases
			const float scale_factor = 100;
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
		
		int helper_cross_radius = Settings::getInstance().getSettingCached(Settings::RectangleTool_HelperCrossRadius).toInt();
		painter->setRenderHint(QPainter::Antialiasing);
		
		MapCoordF perp_vector = forward_vector;
		perp_vector.perpRight();
		
		painter->setPen((angles.size() > 1) ? inactive_color : active_color);
		if (preview_point_radius == 0 || !use_preview_radius)
		{
			painter->drawLine(widget->mapToViewport(cur_pos_map) + helper_cross_radius * forward_vector.toQPointF(),
							  widget->mapToViewport(cur_pos_map) - helper_cross_radius * forward_vector.toQPointF());
		}
		else
		{
			painter->drawLine(widget->mapToViewport(cur_pos_map + 0.001f * preview_point_radius * perp_vector) + helper_cross_radius * forward_vector.toQPointF(),
							  widget->mapToViewport(cur_pos_map + 0.001f * preview_point_radius * perp_vector) - helper_cross_radius * forward_vector.toQPointF());
			painter->drawLine(widget->mapToViewport(cur_pos_map - 0.001f * preview_point_radius * perp_vector) + helper_cross_radius * forward_vector.toQPointF(),
							  widget->mapToViewport(cur_pos_map - 0.001f * preview_point_radius * perp_vector) - helper_cross_radius * forward_vector.toQPointF());
		}
		
		painter->setPen(active_color);
		if (preview_point_radius == 0 || !use_preview_radius)
		{
			painter->drawLine(widget->mapToViewport(cur_pos_map) + helper_cross_radius * perp_vector.toQPointF(),
							  widget->mapToViewport(cur_pos_map) - helper_cross_radius * perp_vector.toQPointF());
		}
		else
		{
			painter->drawLine(widget->mapToViewport(cur_pos_map + 0.001f * preview_point_radius * forward_vector) + helper_cross_radius * perp_vector.toQPointF(),
							  widget->mapToViewport(cur_pos_map + 0.001f * preview_point_radius * forward_vector) - helper_cross_radius * perp_vector.toQPointF());
			painter->drawLine(widget->mapToViewport(cur_pos_map - 0.001f * preview_point_radius * forward_vector) + helper_cross_radius * perp_vector.toQPointF(),
							  widget->mapToViewport(cur_pos_map - 0.001f * preview_point_radius * forward_vector) - helper_cross_radius * perp_vector.toQPointF());
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
		preview_path->getPart(0).setClosed(true, true);
	}
	
	angle_helper->setActive(false);
	angles.clear();
	draw_in_progress = false;
	updateStatusText();
	DrawLineAndAreaTool::finishDrawing();
	// HACK: do not add stuff here as the tool might get deleted on call to DrawLineAndAreaTool::finishDrawing()!
}

void DrawRectangleTool::abortDrawing()
{
	snapped_to_line = false;
	angle_helper->setActive(false);
	angles.clear();
	draw_in_progress = false;
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
	updateCloseVector();
	
	angle_helper->setCenter(MapCoordF(preview_path->getCoordinate(angles.size() - 1)));
		
	updateRectangle();
}

void DrawRectangleTool::pickDirection(MapCoordF coord, MapWidget* widget)
{
	MapCoord snap_position;
	snap_helper->snapToDirection(coord, widget, angle_helper.data(), &snap_position);
	angle_helper->setActive(true, MapCoordF(snap_position));
	updateDirtyRect();
	picked_direction = true;
}

bool DrawRectangleTool::drawingParallelTo(double angle)
{
	const double epsilon = 0.01;
	double cur_angle = angles[angles.size() - 1];
	return qAbs(fmod_pos(cur_angle, M_PI) - fmod_pos(angle, M_PI)) < epsilon;
}

void DrawRectangleTool::updateCloseVector()
{
	int cur_point_index = angles.size();
	close_vector = MapCoordF(1, 0);
	close_vector.rotate(-angles[0]);
	if (drawingParallelTo(angles[0]))
		close_vector.perpRight();
	if (close_vector.dot(MapCoordF(preview_path->getCoordinate(0) - preview_path->getCoordinate(cur_point_index - 1))) < 0)
		close_vector = -close_vector;
}

void DrawRectangleTool::deleteClosePoint()
{
	int cur_point_index = angles.size();
	if (preview_path->getPart(0).isClosed())
	{
		preview_path->getPart(0).setClosed(false);
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
		forward_vector = constrained_pos_map - MapCoordF(preview_path->getCoordinate(0));
		forward_vector.normalize();
		close_vector = -forward_vector;
		
		angles[angles.size() - 1] = -forward_vector.getAngle();
		
		// Update rectangle
		MapCoord coord = constrained_pos_map.toMapCoord();
		coord.setDashPoint(draw_dash_points);
		preview_path->setCoordinate(1, coord);
	}
	else
	{
		// Update vectors and angles
		forward_vector = MapCoordF(1, 0);
		forward_vector.rotate(-angle);
		updateCloseVector();
		
		angles[angles.size() - 1] = angle;
		
		// Update rectangle
		int cur_point_index = angles.size();
		deleteClosePoint();
		
		float forward_dist = forward_vector.dot(constrained_pos_map - MapCoordF(preview_path->getCoordinate(cur_point_index - 1)));
		MapCoord coord = preview_path->getCoordinate(cur_point_index - 1) + (forward_dist * forward_vector).toMapCoord();
		
		snapped_to_line = false;
		float best_snap_distance_sq = std::numeric_limits<float>::max();
		if (ctrl_pressed && angles.size() > 2)
		{
			// Try to snap to existing lines
			MapCoord original_coord = coord;
			for (int i = 0; i < (int)angles.size() - 1; ++i)
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
					
					QLineF a(preview_path->getCoordinate(cur_point_index - 1).toQPointF(), (MapCoordF(preview_path->getCoordinate(cur_point_index - 1)) + forward_vector).toQPointF());
					QLineF b(preview_path->getCoordinate(i).toQPointF(), (MapCoordF(preview_path->getCoordinate(i)) + rotated_direction).toQPointF());
					QPointF intersection_point;
					QLineF::IntersectType intersection_type = a.intersect(b, &intersection_point);
					if (intersection_type == QLineF::NoIntersection)
						continue;
					
					float snap_distance_sq = original_coord.lengthSquaredTo(MapCoord(intersection_point));
					if (snap_distance_sq > best_snap_distance_sq)
						continue;
					
					best_snap_distance_sq = snap_distance_sq;
					coord = MapCoord(intersection_point);
					snapped_to_line_a = coord;
					snapped_to_line_b = coord + rotated_direction.toMapCoord();
					snapped_to_line = true;
				}
			}
		}
		
		coord.setDashPoint(draw_dash_points);
		preview_path->setCoordinate(cur_point_index, coord);
		
		float close_dist = close_vector.dot(MapCoordF(preview_path->getCoordinate(0) - preview_path->getCoordinate(cur_point_index)));
		coord = preview_path->getCoordinate(cur_point_index) + (close_dist * close_vector).toMapCoord();
		coord.setDashPoint(draw_dash_points);
		preview_path->setCoordinate(cur_point_index + 1, coord);
		
		preview_path->getPart(0).setClosed(true, false);
		assert(preview_path->getPart(0).end_index == cur_point_index + 2);
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
		emit(dirtyRectChanged(rect));
	else
	{
		if (angle_helper->isActive())
			angle_helper->includeDirtyRect(rect);
		if (rect.isValid())
		{
			int helper_cross_radius = Settings::getInstance().getSettingCached(Settings::RectangleTool_HelperCrossRadius).toInt();
			int pixel_border = 0;
			if (draw_in_progress)
				pixel_border = helper_cross_radius;	// helper_cross_radius as border is less than ideal but the only way to always ensure visibility of the helper cross at the moment
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
	static const QString text_more_shift_control_space(MapEditorTool::tr("More: %1, %2, %3").arg(ModifierKey::shift(), ModifierKey::control(), ModifierKey(Qt::Key_Space)));
	static const QString text_more_shift_space(MapEditorTool::tr("More: %1, %2").arg(ModifierKey::shift(), ModifierKey(Qt::Key_Space)));
	static const QString text_more_control_space(MapEditorTool::tr("More: %1, %2").arg(ModifierKey::control(), ModifierKey(Qt::Key_Space)));
	QString text_more(text_more_shift_control_space);
	
	if (draw_dash_points)
		text += DrawLineAndAreaTool::tr("<b>Dash points on.</b> ") + "| ";
	
	if (!draw_in_progress)
	{
		if (ctrl_pressed)
		{
			text += DrawLineAndAreaTool::tr("<b>%1+Click</b>: Pick direction from existing objects. ").arg(ModifierKey::control());
			text_more = text_more_shift_space;
		}
		else if (shift_pressed)
		{
			text += DrawLineAndAreaTool::tr("<b>%1+Click</b>: Snap to existing objects. ").arg(ModifierKey::shift());
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
				text += DrawLineAndAreaTool::tr("<b>%1</b>: Fixed angles. ").arg(ModifierKey::control());
			}
			else
			{
				text += tr("<b>%1</b>: Snap to previous lines. ").arg(ModifierKey::control());
			}
			text_more = text_more_shift_space;
		}
		else if (shift_pressed)
		{
			text += DrawLineAndAreaTool::tr("<b>%1+Click</b>: Snap to existing objects. ").arg(ModifierKey::shift());
			text_more = text_more_control_space;
		}
		else
		{
			text += tr("<b>Click</b>: Set a corner point. <b>Right or double click</b>: Finish the rectangle. ");
			text += DrawLineAndAreaTool::tr("<b>%1</b>: Undo last point. ").arg(ModifierKey::backspace());
			text += MapEditorTool::tr("<b>%1</b>: Abort. ").arg(ModifierKey::escape());
		}
	}
	
	text += "| " + text_more;
	
	setStatusBarText(text);
}
