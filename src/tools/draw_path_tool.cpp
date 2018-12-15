/*
 *    Copyright 2012-2014 Thomas Schöps
 *    Copyright 2013-2017 Kai Pastor
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


#include "draw_path_tool.h"

#include <cmath>
#include <memory>
#include <vector>

#include <Qt>
#include <QtMath>
#include <QColor>
#include <QCursor>
#include <QFlags>
#include <QLatin1String>
#include <QKeyEvent>
#include <QLineF>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QRgb>
#include <QSizeF>
#include <QString>
#include <QToolButton>
#include <QVarLengthArray>

#include "core/map.h"
#include "core/path_coord.h"
#include "core/virtual_coord_vector.h"
#include "core/virtual_path.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/symbol.h"
#include "core/objects/object.h"
#include "core/renderables/renderable.h"
#include "gui/modifier_key.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "gui/widgets/key_button_bar.h"
#include "tools/tool.h"
#include "tools/tool_helpers.h"
#include "util/util.h"
#include "undo/object_undo.h"


namespace OpenOrienteering {

DrawPathTool::DrawPathTool(MapEditorController* editor, QAction* tool_action, bool is_helper_tool, bool allow_closing_paths)
: DrawLineAndAreaTool(editor, DrawPath, tool_action, is_helper_tool)
, cur_map_widget(mapWidget())
, angle_helper(new ConstrainAngleToolHelper())
, azimuth_helper(new AzimuthInfoHelper(cur_map_widget, active_color))
, snap_helper(new SnappingToolHelper(this))
, follow_helper(new FollowPathToolHelper())
, allow_closing_paths(allow_closing_paths)
{
	angle_helper->setActive(false);
	connect(angle_helper.get(), &ConstrainAngleToolHelper::displayChanged, this, &DrawPathTool::updateDirtyRect);
	
	updateSnapHelper();
	connect(snap_helper.get(), &SnappingToolHelper::displayChanged, this, &DrawPathTool::updateDirtyRect);
	
	connect(map(), &Map::objectSelectionChanged, this, &DrawPathTool::objectSelectionChanged);
}

DrawPathTool::~DrawPathTool()
{
	if (key_button_bar)
		editor->deletePopupWidget(key_button_bar);
}

void DrawPathTool::init()
{
	updateDashPointDrawing();
	updateStatusText();
	
	if (editor->isInMobileMode())
	{
		// Create key replacement bar
		key_button_bar = new KeyButtonBar(editor->getMainWidget());
		key_button_bar->addKeyButton(Qt::Key_Return, Qt::ControlModifier, tr("Finish"));
		key_button_bar->addKeyButton(Qt::Key_Return, tr("Close"));
		key_button_bar->addModifierButton(Qt::ShiftModifier, tr("Snap", "Snap to existing objects"));
		key_button_bar->addModifierButton(Qt::ControlModifier, tr("Angle", "Using constrained angles"));
		azimuth_button = key_button_bar->addKeyButton(Qt::Key_Space, Qt::ControlModifier, tr("Info", "Show segment azimuth and length"));
		azimuth_button->setCheckable(true);
		azimuth_button->setChecked(azimuth_helper->isActive());
		dash_points_button = key_button_bar->addKeyButton(Qt::Key_Space, tr("Dash", "Drawing dash points"));
		dash_points_button->setCheckable(true);
		dash_points_button->setChecked(draw_dash_points);
		key_button_bar->addKeyButton(Qt::Key_Backspace, tr("Undo"));
		key_button_bar->addKeyButton(Qt::Key_Escape, tr("Abort"));
		editor->showPopupWidget(key_button_bar, QString{});
	}
	
	MapEditorTool::init();
}

const QCursor& DrawPathTool::getCursor() const
{
	static auto const cursor = scaledToScreen(QCursor{ QPixmap(QString::fromLatin1(":/images/cursor-draw-path.png")), 11, 11 });
	return cursor;
}

bool DrawPathTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	cur_map_widget = widget;
	created_point_at_last_mouse_press = false;
	
	if (editingInProgress() &&
		((event->button() == Qt::RightButton) &&
		!drawOnRightClickEnabled()))
	{
		finishDrawing();
		return true;
	}
	else if (editingInProgress() &&
		((event->button() == Qt::RightButton && event->buttons() & Qt::LeftButton) ||
		 (event->button() == Qt::LeftButton && event->buttons() & Qt::RightButton)))
	{
		if (!previous_point_is_curve_point)
			undoLastPoint();
		if (editingInProgress())
			finishDrawing();
		return true;
	}
	else if (isDrawingButton(event->button()))
	{
		dragging = false;
		bool start_appending = false;
		if (shift_pressed)
		{
			SnappingToolHelperSnapInfo snap_info;
			MapCoord snap_coord = snap_helper->snapToObject(map_coord, widget, &snap_info);
			click_pos_map = MapCoordF(snap_coord);
			cur_pos_map = click_pos_map;
			click_pos = widget->mapToViewport(click_pos_map).toPoint();
			
			// Check for following and appending
			if (!is_helper_tool)
			{
				if (!editingInProgress())
				{
					if (snap_info.type == SnappingToolHelper::ObjectCorners &&
						(snap_info.coord_index == 0 || snap_info.coord_index == snap_info.object->asPath()->getCoordinateCount() - 1) &&
						snap_info.object->getSymbol() == editor->activeSymbol())
					{
						// Appending to another path
						start_appending = true;
						startAppending(snap_info);
					}
					
					// Setup angle helper
					if (snap_helper->snapToDirection(map_coord, widget, angle_helper.get()))
						picked_angle = true;
				}
				else if (editingInProgress() &&
						 (snap_info.type == SnappingToolHelper::ObjectCorners || snap_info.type == SnappingToolHelper::ObjectPaths) &&
						 snap_info.object->getType() == Object::Path)
				{
					// Start following another path
					picked_angle = false;
					startFollowing(snap_info, snap_coord);
					return true;
				}
			}
		}
		else if (!editingInProgress() && ctrl_pressed)
		{
			// Start picking direction of an existing object
			picking_angle = true;
			pickAngle(map_coord, widget);
			return true;
		}
		else
		{
			click_pos = event->pos();
			click_pos_map = map_coord;
			cur_pos_map = map_coord;
		}
		
		if (!editingInProgress())
		{
			// Start a new path
			startDrawing();
			angle_helper->setCenter(click_pos_map);
			updateSnapHelper();
			
			path_has_preview_point = false;
			previous_point_is_curve_point = false;
			appending = start_appending;
		}
		else
		{
			if (!shift_pressed)
			{
				if (previous_point_is_curve_point
				    && (cur_pos - widget->mapToViewport(previous_drag_map)).manhattanLength() < startDragDistance())
				{
					click_pos_map = previous_drag_map;
				}
				else
				{
					angle_helper->getConstrainedCursorPosMap(click_pos_map, click_pos_map);
				}
			}
			cur_pos_map = click_pos_map;
		}

		// Set path point
		auto coord = MapCoord { click_pos_map };
		if (draw_dash_points)
			coord.setDashPoint(true);
		
		if (preview_path->getCoordinateCount() > 0 && picked_angle)
			picked_angle = false;
		if (previous_point_is_curve_point)
		{
			// Do nothing yet, wait until the user drags or releases the mouse button
		}
		else if (path_has_preview_point)
		{
			preview_path->setCoordinate(preview_path->getCoordinateCount() - 1, coord);
			updateAngleHelper();
			created_point_at_last_mouse_press = true;
		}
		else
		{
			if (preview_path->getCoordinateCount() == 0 || !preview_path->getCoordinate(preview_path->getCoordinateCount() - 1).isPositionEqualTo(coord))
			{
				preview_path->addCoordinate(coord);
				updatePreviewPath();
				if (!start_appending)
					updateAngleHelper();
				created_point_at_last_mouse_press = true;
			}
		}
		
		path_has_preview_point = false;
		
		create_segment = true;
		updateDirtyRect();
		updateStatusText();
		return true;
	}
	
	return false;
}

bool DrawPathTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	cur_pos = event->pos();
	cur_pos_map = map_coord;
	
	if (!containsDrawingButtons(event->buttons()))
	{
		updateHover();
	}
	else if (!editingInProgress())
	{
		left_mouse_down = true;
		if (picking_angle)
			pickAngle(map_coord, widget);
		else
			return false;
	}
	else if (following)
	{
		updateFollowing();
	}
	else
	{
		if ((event->pos() - click_pos).manhattanLength() >= startDragDistance())
		{
			// Giving a direction by dragging
			dragging = true;
			create_spline_corner = false;
			create_segment = true;
			
			if (previous_point_is_curve_point)
				angle_helper->setCenter(click_pos_map);
			
			QPointF constrained_pos;
			angle_helper->getConstrainedCursorPositions(map_coord, constrained_pos_map, constrained_pos, widget);
			
			if (previous_point_is_curve_point)
			{
				hidePreviewPoints();
				auto drag_direction = calculateRotation(constrained_pos.toPoint(), constrained_pos_map);
				
				// Add a new node or convert the last node into a corner?
				if ((widget->mapToViewport(previous_pos_map) - click_pos).manhattanLength() >= startDragDistance())
				{
					createPreviewCurve(MapCoord(click_pos_map), drag_direction);
				}
				else
				{
					create_spline_corner = true;
					// This hides the old direction indicator
					previous_drag_map = previous_pos_map;
				}
			}
			
			updateDirtyRect();
		}
	}
	
	return true;
}

bool DrawPathTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (!isDrawingButton(event->button()))
		return false;

	left_mouse_down = false;
	
	if (picking_angle)
	{
		picking_angle = false;
		picked_angle = pickAngle(map_coord, widget);
		return true;
	}
	else if (!editingInProgress())
	{
		return false;
	}
	
	if (following)
	{
		finishFollowing();
		if ((event->button() == Qt::RightButton) && drawOnRightClickEnabled())
			finishDrawing();
		return true;
	}
	
	if (!create_segment)
		return true;
	
	if (previous_point_is_curve_point && !dragging && !create_spline_corner)
	{
		// The new point has not been added yet
		MapCoord coord;
		if (shift_pressed)
		{
			coord = snap_helper->snapToObject(map_coord, widget);
		}
		if (previous_point_is_curve_point
		    && (cur_pos - widget->mapToViewport(click_pos_map)).manhattanLength() < startDragDistance())
		{
			coord = MapCoord{click_pos_map};
		}
		else if (angle_helper->isActive())
		{
			QPointF constrained_pos;
			angle_helper->getConstrainedCursorPositions(map_coord, constrained_pos_map, constrained_pos, widget);
			coord = MapCoord(constrained_pos_map);
		}
		else
		{
			coord = MapCoord(map_coord);
		}
		
		if (draw_dash_points)
			coord.setDashPoint(true);
		preview_path->addCoordinate(coord);
		updatePreviewPath();
	}
	
	previous_point_is_curve_point = dragging;
	if (previous_point_is_curve_point)
	{
		QPointF constrained_pos;
		angle_helper->getConstrainedCursorPositions(map_coord, constrained_pos_map, constrained_pos, widget);
		
		previous_pos_map = click_pos_map;
		previous_drag_map = constrained_pos_map;
		previous_point_direction = calculateRotation(constrained_pos.toPoint(), constrained_pos_map);
	}
	
	updateAngleHelper();
	updateDirtyRect();
	
	create_spline_corner = false;
	path_has_preview_point = false;
	dragging = false;
	
	if ((event->button() == Qt::RightButton) && drawOnRightClickEnabled())
		finishDrawing();
	return true;
}

bool DrawPathTool::mouseDoubleClickEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(map_coord);
	Q_UNUSED(widget);
	
	if (event->button() != Qt::LeftButton)
		return false;
	
	if (editingInProgress())
	{
		if (created_point_at_last_mouse_press)
			undoLastPoint();
		if (editingInProgress())
			finishDrawing();
	}
	return true;
}

bool DrawPathTool::keyPressEvent(QKeyEvent* event)
{
	switch (event->key())
	{
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
			return true;
		}
		else if (finished_path_is_selected)
		{
			if (removeLastPointFromSelectedPath())
				return true;
		}
		break;
		
	case Qt::Key_Return:
		if (editingInProgress())
		{
			if (allow_closing_paths && !(event->modifiers() & Qt::ControlModifier))
				closeDrawing();
			finishDrawing();
			return true;
		}
		break;
		
	case Qt::Key_Tab:
		deactivate();
		return true;
		
	case Qt::Key_Space:
		if (event->modifiers() & Qt::ControlModifier)
		{
			updateDirtyRect();
			azimuth_helper->setActive(!azimuth_helper->isActive());
			if (azimuth_button)
				azimuth_button->setChecked(azimuth_helper->isActive());
			updateDirtyRect();
		}
		else
		{
			draw_dash_points = !draw_dash_points;
			if (dash_points_button)
				dash_points_button->setChecked(draw_dash_points);
			updateStatusText();
		}
		return true;
		
	case Qt::Key_Control:
		ctrl_pressed = true;
		angle_helper->setActive(true);
		if (editingInProgress() && !dragging)
			updateDrawHover();
		picked_angle = false;
		updateStatusText();
		return false; // not consuming Ctrl
		
	case Qt::Key_Shift:
		shift_pressed = true;
		if (!dragging)
		{
			updateHover();
			updateDirtyRect();
		}
		updateStatusText();
		return false; // not consuming Shift
		
	}
	return false;
}

bool DrawPathTool::keyReleaseEvent(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_Control:
		ctrl_pressed = false;
		if (!picked_angle)
			angle_helper->setActive(false);
		if (editingInProgress() && !dragging)
			updateDrawHover();
		updateStatusText();
		return false; // not consuming Ctrl
	
	case Qt::Key_Shift:
		shift_pressed = false;
		if (!dragging && !following)
		{
			updateHover();
			updateDirtyRect();
		}
		updateStatusText();
		return false; // not consuming Shift
		
	}
	return false;
}

void DrawPathTool::draw(QPainter* painter, MapWidget* widget)
{
	drawPreviewObjects(painter, widget);
	
	if (editingInProgress())
	{
		painter->setRenderHint(QPainter::Antialiasing);
		
		auto azimuth_info_pending = azimuth_helper->isActive();
		
		if (dragging && (cur_pos - click_pos).manhattanLength() >= startDragDistance())
		{
			auto line = QLineF{ widget->mapToViewport(click_pos_map),
			                    widget->mapToViewport(constrained_pos_map) };
			QPen pen(qRgb(255, 255, 255));
			pen.setWidth(3);
			painter->setPen(pen);
			painter->drawLine(line);
			painter->setPen(active_color);
			painter->drawLine(line);
			if (azimuth_info_pending)
			{
				azimuth_helper->draw(painter, widget, map(), click_pos_map, constrained_pos_map);
				azimuth_info_pending = false;
			}
		}
		
		if (previous_point_is_curve_point)
		{
			auto line = QLineF{ widget->mapToViewport(previous_pos_map),
			                    widget->mapToViewport(previous_drag_map) };
			QPen pen(qRgb(255, 255, 255));
			pen.setWidth(3);
			painter->setPen(pen);
			painter->drawLine(line);
			painter->setPen(active_color);
			painter->drawLine(line);
			if (azimuth_info_pending)
			{
				if ((cur_pos - line.p2()).manhattanLength() < startDragDistance())
				{
					azimuth_helper->draw(painter, widget, map(), previous_pos_map, previous_drag_map);
					QPen pen(qRgb(255, 0, 0));
					painter->setPen(pen);
					painter->drawRect(QRectF{line.p2(), QSizeF{8.0, 8.0}}.translated(-4.0, -4.0));
				}
				else
				{
					azimuth_helper->draw(painter, widget, map(), click_pos_map, constrained_pos_map);
				}
				azimuth_info_pending = false;
			}
		}
		
		if (azimuth_info_pending && preview_path && preview_path->getCoordinateCount() >= 2)
		{
			auto start_pos = MapCoordF{preview_path->getCoordinate(preview_path->getCoordinateCount() - 2)};
			auto end_pos = MapCoordF{preview_path->getCoordinate(preview_path->getCoordinateCount() - 1)};
			azimuth_helper->draw(painter, widget, map(), start_pos, end_pos);
			azimuth_info_pending = false;
		}
		
		if (!shift_pressed)
			angle_helper->draw(painter, widget);
	}
	
	if (shift_pressed && !dragging)
	{
		snap_helper->draw(painter, widget);
	}
	else if (!editingInProgress() && (picking_angle || picked_angle))
	{
		if (picking_angle)
			snap_helper->draw(painter, widget);
		angle_helper->draw(painter, widget);
	}
}

void DrawPathTool::updatePreviewPath()
{
	DrawLineAndAreaTool::updatePreviewPath();
	updateStatusText();
}

void DrawPathTool::updateHover()
{
	if (shift_pressed)
		constrained_pos_map = MapCoordF(snap_helper->snapToObject(cur_pos_map, cur_map_widget));
	else
		constrained_pos_map = cur_pos_map;
	
	if (!editingInProgress())
	{
		// Show preview objects at this position
		setPreviewPointsPosition(constrained_pos_map);
		if (picked_angle)
			angle_helper->setCenter(constrained_pos_map);
		updateDirtyRect();
	}
	else // if (draw_in_progress)
	{
		updateDrawHover();
	}
}

void DrawPathTool::updateDrawHover()
{
	if (!shift_pressed)
		angle_helper->getConstrainedCursorPosMap(cur_pos_map, constrained_pos_map);
	if (!previous_point_is_curve_point && !left_mouse_down && editingInProgress())
	{
		// Show a line to the cursor position as preview
		hidePreviewPoints();
		
		if (!path_has_preview_point)
		{
			preview_path->addCoordinate(MapCoord(constrained_pos_map));
			path_has_preview_point = true;
		}
		preview_path->setCoordinate(preview_path->getCoordinateCount() - 1, MapCoord(constrained_pos_map));
		
		updatePreviewPath();
		updateDirtyRect();	// TODO: Possible optimization: mark only the last segment as dirty
	}
	else if (previous_point_is_curve_point && !left_mouse_down && editingInProgress())
	{
		setPreviewPointsPosition(constrained_pos_map, 1);
		updateDirtyRect();
	}
}

void DrawPathTool::createPreviewCurve(MapCoord position, qreal direction)
{
	if (!path_has_preview_point)
	{
		auto last = preview_path->getCoordinateCount() - 1;
		(preview_path->getCoordinate(last)).setCurveStart(true);
		
		preview_path->addCoordinate(MapCoord(0, 0));
		preview_path->addCoordinate(MapCoord(0, 0));
		if (draw_dash_points)
			position.setDashPoint(true);
		position.setCurveStart(false);
		preview_path->addCoordinate(position);
		
		path_has_preview_point = true;
	}
	
	// Adjust the preview curve
	auto last = preview_path->getCoordinateCount() - 1;
	MapCoord previous_point = preview_path->getCoordinate(last - 3);
	MapCoord last_point = preview_path->getCoordinate(last);
	
	double bezier_handle_distance = BEZIER_HANDLE_DISTANCE * previous_point.distanceTo(last_point);
	
	preview_path->setCoordinate(last - 2, MapCoord(previous_point.x() - bezier_handle_distance * sin(previous_point_direction),
	                                               previous_point.y() - bezier_handle_distance * cos(previous_point_direction)));
	preview_path->setCoordinate(last - 1, MapCoord(last_point.x() + bezier_handle_distance * sin(direction),
	                                               last_point.y() + bezier_handle_distance * cos(direction)));
	updatePreviewPath();	
}

void DrawPathTool::undoLastPoint()
{
	Q_ASSERT(editingInProgress());
	
	if (preview_path->getCoordinateCount() <= (preview_path->parts().front().isClosed() ? 3 : (path_has_preview_point ? 2 : 1)))
	{
		abortDrawing();
		return;
	}
	
	auto& part = preview_path->parts().back();
	auto last_index = part.last_index;
	auto prev_coord_index = part.prevCoordIndex(part.last_index);
	auto prev_coord = preview_path->getCoordinate(prev_coord_index);
	
	// Pre-undo preparation
	if (path_has_preview_point)
	{
		if (prev_coord.isCurveStart())
		{
			// Undo just the preview point
			path_has_preview_point = false;
		}
		else
		{
			// Remove the preview point from a straight edge, preparing for re-adding.
			Q_ASSERT(!previous_point_is_curve_point);
			
			preview_path->deleteCoordinate(last_index, false);
			last_index = prev_coord_index;
			prev_coord_index = part.prevCoordIndex(part.last_index);
			prev_coord = preview_path->getCoordinate(prev_coord_index);
			
			path_has_preview_point = !prev_coord.isCurveStart();
		}
	}
	
	if (prev_coord.isCurveStart())
	{
		// Removing last point of a curve, no re-adding of preview point.
		MapCoord prev_drag = preview_path->getCoordinate(prev_coord_index+1);
		previous_point_direction = -atan2(prev_drag.x() - prev_coord.x(), prev_coord.y() - prev_drag.y());
		previous_pos_map = MapCoordF(prev_coord);
		previous_drag_map = MapCoordF((prev_coord.x() + prev_drag.x()) / 2, (prev_coord.y() + prev_drag.y()) / 2);
		previous_point_is_curve_point = true;
		path_has_preview_point = false;
		
		click_pos_map = previous_pos_map;  // for azimuth info
	}
	else if (!path_has_preview_point)
	{
		// Removing last point from a straight edge, no re-adding of preview point.
		previous_point_is_curve_point = false;
	}
	
	// Actually delete the last point of the edge.
	preview_path->deleteCoordinate(last_index, false);
	if (preview_path->getRawCoordinateVector().empty())
	{
		// Re-add first point.
		prev_coord.setCurveStart(false);
		preview_path->addCoordinate(prev_coord);
	}
	
	// Post-undo
	if (path_has_preview_point)
	{
		// Re-add preview point.
		preview_path->addCoordinate(MapCoord(cur_pos_map));
	}
	else if (previous_point_is_curve_point && dragging)
	{
		cur_pos = click_pos;
		cur_pos_map = click_pos_map;
	}
	
	dragging = false;
	
	updateHover();
	updatePreviewPath();
	updateAngleHelper();
	updateDirtyRect();
}

bool DrawPathTool::removeLastPointFromSelectedPath()
{
	if (editingInProgress() || map()->getNumSelectedObjects() != 1)
	{
		return false;
	}
	
	Object* object = map()->getFirstSelectedObject();
	if (object->getType() != Object::Path)
	{
		return false;
	}
	
	PathObject* path = object->asPath();
	if (path->parts().size() != 1)
	{
		return false;
	}
	
	int points_on_path = 0;
	auto num_coords = path->getCoordinateCount();
	for (MapCoordVector::size_type i = 0; i < num_coords && points_on_path < 3; ++i)
	{
		++points_on_path;
		if (path->getCoordinate(i).isCurveStart())
		{
			i += 2; // Skip the control points.
		}
	}
	
	if (points_on_path < 3)
	{
		// Too few points after deleting the last: delete the whole object.
		map()->deleteSelectedObjects();
		return true;
	}
	
	auto undo_step = new ReplaceObjectsUndoStep(map());
	auto undo_duplicate = object->duplicate();
	undo_duplicate->setMap(map());
	undo_step->addObject(object, undo_duplicate);
	map()->push(undo_step);
	updateDirtyRect();
	
	path->parts().front().setClosed(false);
	path->deleteCoordinate(num_coords - 1, false);
	
	path->update();
	map()->setObjectsDirty();
	map()->emitSelectionEdited();
	return true;
}

void DrawPathTool::closeDrawing()
{
	Q_ASSERT(editingInProgress());
	
	if (preview_path->getCoordinateCount() <= 1)
		return;
	
	if (previous_point_is_curve_point && preview_path->getCoordinate(0).isCurveStart())
	{
		// Finish with a curve
		path_has_preview_point = false;
		
		if (dragging)
			previous_point_direction = -atan2(cur_pos_map.x() - click_pos_map.x(), click_pos_map.y() - cur_pos_map.y());
		
		MapCoord first = preview_path->getCoordinate(0);
		MapCoord second = preview_path->getCoordinate(1);
		createPreviewCurve(first, -atan2(second.x() - first.x(), first.y() - second.y()));
		path_has_preview_point = false;
	}
	
	if (!preview_path->parts().empty())
		preview_path->parts().front().setClosed(true, true);
}

void DrawPathTool::finishDrawing()
{
	Q_ASSERT(editingInProgress());
	
	// Does the symbols contain only areas? If so, auto-close the path if not done yet
	bool contains_only_areas = !is_helper_tool && (drawing_symbol->getContainedTypes() & ~(Symbol::Area | Symbol::Combined)) == 0 && (drawing_symbol->getContainedTypes() & Symbol::Area);
	if (contains_only_areas && !preview_path->parts().empty())
		preview_path->parts().front().setClosed(true, true);
	
	// Remove last point if closed and first and last points are equal, or if the last point was just a preview
	if (path_has_preview_point && !dragging)
		preview_path->deleteCoordinate(preview_path->getCoordinateCount() - (preview_path->parts().front().isClosed() ? 2 : 1), false);
	
	if (preview_path->getCoordinateCount() < (contains_only_areas ? 3 : 2))
	{
		renderables->removeRenderablesOfObject(preview_path, false);
		delete preview_path;
		preview_path = nullptr;
	}
	
	dragging = false;
	following = false;
	setEditingInProgress(false);
	if (!ctrl_pressed)
		angle_helper->setActive(false);
	updateSnapHelper();
	updateStatusText();
	hidePreviewPoints();
	
	DrawLineAndAreaTool::finishDrawing(appending ? append_to_object : nullptr);
	
	finished_path_is_selected = true;
}

void DrawPathTool::abortDrawing()
{
	dragging = false;
	following = false;
	setEditingInProgress(false);
	if (!ctrl_pressed)
		angle_helper->setActive(false);
	updateSnapHelper();
	updateStatusText();
	hidePreviewPoints();
	
	DrawLineAndAreaTool::abortDrawing();
}

void DrawPathTool::updateDirtyRect()
{
	QRectF rect;
	
	if (azimuth_helper->isActive() && editingInProgress())
	{
		rectIncludeSafe(rect, azimuth_helper->dirtyRect(mapWidget(), constrained_pos_map));
	}
	
	if (dragging)
	{
		rectIncludeSafe(rect, click_pos_map);
		rectInclude(rect, cur_pos_map);
	}
	if (editingInProgress() && previous_point_is_curve_point)
	{
		rectIncludeSafe(rect, previous_pos_map);
		rectInclude(rect, previous_drag_map);
	}
	if ((editingInProgress() && !dragging) ||
		(!editingInProgress() && !shift_pressed && ctrl_pressed) ||
		(!editingInProgress() && (picking_angle || picked_angle)))
	{
		angle_helper->includeDirtyRect(rect);
	}
	if (shift_pressed || (!editingInProgress() && ctrl_pressed))
		snap_helper->includeDirtyRect(rect);
	includePreviewRects(rect);
	
	if (is_helper_tool)
		emit dirtyRectChanged(rect);
	else
	{
		if (rect.isValid())
			map()->setDrawingBoundingBox(rect, qMax(qMax(dragging ? 1 : 0, angle_helper->getDisplayRadius()), snap_helper->getDisplayRadius()), true);
		else
			map()->clearDrawingBoundingBox();
	}
}

void DrawPathTool::setDrawingSymbol(const Symbol* symbol)
{
	if (is_helper_tool)
		return;
	DrawLineAndAreaTool::setDrawingSymbol(symbol);
	
	updateDashPointDrawing();
}

void DrawPathTool::objectSelectionChanged()
{
	finished_path_is_selected = false;
}

void DrawPathTool::updateAngleHelper()
{
	if (picked_angle)
		return;
	
	if (!preview_path
	    || (static_cast<void>(updatePreviewPath()), preview_path->parts().empty()))
	{
		angle_helper->clearAngles();
		angle_helper->addDefaultAnglesDeg(0);
		return;
	}
	
	const auto& part = preview_path->parts().back();
	
	bool rectangular_stepping = true;
	double angle;
	if (part.size() >= 2)
	{
		bool ok = false;
		MapCoordF tangent = part.calculateTangent(part.size()-1, true, ok);
		if (!ok)
			tangent = MapCoordF(1, 0);
		angle = -tangent.angle();
	}
	else if (previous_point_is_curve_point)
	{
		angle = previous_point_direction;
	}
	else
	{
		angle = 0;
		rectangular_stepping = false;
	}
	
	if (!part.empty())
		angle_helper->setCenter(part.coords.back());
	angle_helper->clearAngles();
	if (rectangular_stepping)
		angle_helper->addAnglesDeg(angle * 180 / M_PI, 45);
	else
		angle_helper->addDefaultAnglesDeg(angle * 180 / M_PI);
}

bool DrawPathTool::pickAngle(MapCoordF coord, MapWidget* widget)
{
	MapCoord snap_position;
	bool picked = snap_helper->snapToDirection(coord, widget, angle_helper.get(), &snap_position);
	if (picked)
	{
		angle_helper->setCenter(MapCoordF(snap_position));
	}
	else
	{
		updateAngleHelper();
		angle_helper->setCenter(constrained_pos_map);
	}
	hidePreviewPoints();
	updateDirtyRect();
	return picked;
}

void DrawPathTool::updateSnapHelper()
{
	if (editingInProgress())
		snap_helper->setFilter(SnappingToolHelper::AllTypes);
	else
	{
		//snap_helper.setFilter((SnappingToolHelper::SnapObjects)(SnappingToolHelper::GridCorners | SnappingToolHelper::ObjectCorners));
		snap_helper->setFilter(SnappingToolHelper::AllTypes);
	}
}

void DrawPathTool::startAppending(SnappingToolHelperSnapInfo& snap_info)
{
	append_to_object = snap_info.object->asPath();
}

void DrawPathTool::startFollowing(SnappingToolHelperSnapInfo& snap_info, const MapCoord& snap_coord)
{
	following = true;
	auto followed_object = snap_info.object->asPath();
	create_segment = false;
	
	if (snap_info.type == SnappingToolHelper::ObjectCorners)
		follow_helper->startFollowingFromCoord(followed_object, snap_info.coord_index);
	else // if (snap_info.type == SnappingToolHelper::ObjectPaths)
		follow_helper->startFollowingFromPathCoord(followed_object, snap_info.path_coord);
	
	if (path_has_preview_point)
		preview_path->setCoordinate(preview_path->getCoordinateCount() - 1, snap_coord);
	else
		preview_path->addCoordinate(snap_coord);
	path_has_preview_point = false;
	previous_point_is_curve_point = false;
	updatePreviewPath();
	follow_start_index = preview_path->getCoordinateCount() - 1;
}

void DrawPathTool::updateFollowing()
{
	PathCoord path_coord;
	float distance_sq;
	const auto followed_object = follow_helper->followed_object();
	const auto& part = followed_object->parts()[follow_helper->partIndex()];
	followed_object->calcClosestPointOnPath(cur_pos_map, distance_sq, path_coord, part.first_index, part.last_index);
	auto followed_path = follow_helper->updateFollowing(path_coord);
	
	// Append the temporary object to the preview object at follow_start_index
	// 1. Delete everything appended, except for the point where following started
	//    (thus avoiding deletion of the whole part).
	// \todo Implement as truncate()
	for (auto i = preview_path->getCoordinateCount() - 1;
	     i > follow_start_index + 1;
	     i = preview_path->getCoordinateCount() - 1)
	{
		preview_path->deleteCoordinate(i, false);
	}
	// 2. Merge segments at the point where following started.
	if (followed_path)
	{
		preview_path->connectPathParts(preview_path->findPartIndexForIndex(follow_start_index),
		                               followed_path.get(), 0, false, true);
	}
	updatePreviewPath();
	hidePreviewPoints();
	updateDirtyRect();
}

void DrawPathTool::finishFollowing()
{
	following = false;
	
	auto last = preview_path->getCoordinateCount() - 1;
	
	previous_point_is_curve_point = (last >= 3 && preview_path->getCoordinate(last - 3).isCurveStart());
	if (previous_point_is_curve_point)
	{
		MapCoord first = preview_path->getCoordinate(last - 1);
		MapCoord second = preview_path->getCoordinate(last);
		
		previous_point_direction = -atan2(second.x() - first.x(), first.y() - second.y());
		previous_pos_map = MapCoordF(second);
		previous_drag_map = MapCoordF(2*second.x() - first.x(), 2*second.y() +  - first.y());
	}
	
	updateAngleHelper();
}

qreal DrawPathTool::calculateRotation(QPoint mouse_pos, MapCoordF mouse_pos_map) const
{
	if (dragging && (mouse_pos - click_pos).manhattanLength() >= startDragDistance())
		return -atan2(mouse_pos_map.x() - click_pos_map.x(), click_pos_map.y() - mouse_pos_map.y());
	else
		return 0;
}

void DrawPathTool::updateDashPointDrawing()
{
	if (is_helper_tool)
		return;
	
	Symbol* symbol = editor->activeSymbol();
	if (symbol && symbol->getType() == Symbol::Line)
	{
		// Auto-activate dash points depending on if the selected symbol has a dash symbol.
		// TODO: instead of just looking if it is a line symbol with dash points,
		// could also check for combined symbols containing lines with dash points
		draw_dash_points = (symbol->asLine()->getDashSymbol());
		
		updateStatusText();
	}
	else if (symbol &&
		(symbol->getType() == Symbol::Area ||
		 symbol->getType() == Symbol::Combined))
	{
		draw_dash_points = false;
	}
	
	if (dash_points_button)
		dash_points_button->setChecked(draw_dash_points);
}

void DrawPathTool::updateStatusText()
{
	QString text;
	if (editingInProgress() && preview_path && preview_path->getCoordinateCount() >= 2)
	{
		//Q_ASSERT(!preview_path->isDirty());
		float length = map()->getScaleDenominator() * preview_path->parts().front().path_coords.back().clen * 0.001f;
		text += tr("<b>Length:</b> %1 m ").arg(QLocale().toString(length, 'f', 1)) + QLatin1String("| ");
	}
	
	if (draw_dash_points && !is_helper_tool)
	{
		text += ::OpenOrienteering::DrawLineAndAreaTool::tr("<b>Dash points on.</b> ") + QLatin1String("| ");
	}
	
	QVarLengthArray<QString, 3> modifier_keys;
	if (!editingInProgress())
	{
		if (shift_pressed)
		{
			text += ::OpenOrienteering::DrawLineAndAreaTool::tr("<b>%1+Click</b>: Snap or append to existing objects. ").arg(ModifierKey::shift());
		}
		else
		{
			modifier_keys.append(ModifierKey::shift());
		
			if (ctrl_pressed)
			{
				text += ::OpenOrienteering::DrawLineAndAreaTool::tr("<b>%1+Click</b>: Pick direction from existing objects. ").arg(ModifierKey::control());
				text += ::OpenOrienteering::DrawLineAndAreaTool::tr("<b>%1+%2</b>: Segment azimuth and length. ").arg(ModifierKey::control(), ModifierKey::space());
			}
			else
			{
				modifier_keys.append(ModifierKey::control());
			
				text += tr("<b>Click</b>: Start a straight line. <b>Drag</b>: Start a curve. ");
				
				// text += ::OpenOrienteering::DrawLineAndAreaTool::tr(draw_dash_points ? "<b>%1</b> Disable dash points. " : "<b>%1</b>: Enable dash points. ").arg(ModifierKey::space());
			}
		}
	}
	else
	{
		if (shift_pressed)
		{
			text += ::OpenOrienteering::DrawLineAndAreaTool::tr("<b>%1+Click</b>: Snap to existing objects. ").arg(ModifierKey::shift())
			        + tr("<b>%1+Drag</b>: Follow existing objects. ").arg(ModifierKey::shift());
		}
		else
		{
			modifier_keys.append(ModifierKey::shift());
			
			if (ctrl_pressed)
			{
				if (angle_helper->isActive())
					text += ::OpenOrienteering::DrawLineAndAreaTool::tr("<b>%1</b>: Fixed angles. ").arg(ModifierKey::control());
				text += ::OpenOrienteering::DrawLineAndAreaTool::tr("<b>%1+%2</b>: Segment azimuth and length. ").arg(ModifierKey::control(), ModifierKey::space());
			}
			else
			{
				modifier_keys.append(ModifierKey::control());
				
				text += tr("<b>Click</b>: Draw a straight line. <b>Drag</b>: Draw a curve. "
				           "<b>Right or double click</b>: Finish the path. "
				           "<b>%1</b>: Close the path. ").arg(ModifierKey::return_key())
				        + ::OpenOrienteering::DrawLineAndAreaTool::tr("<b>%1</b>: Undo last point. ").arg(ModifierKey::backspace())
				        + ::OpenOrienteering::MapEditorTool::tr("<b>%1</b>: Abort. ").arg(ModifierKey::escape());
			}
		}
	}
	
	if (!is_helper_tool && !ctrl_pressed)
	{
		modifier_keys.append(ModifierKey::space());
	}
	
	if (!modifier_keys.isEmpty())
	{
		QString text_more;
		switch (modifier_keys.length())
		{
		case 1:
			text_more = ::OpenOrienteering::MapEditorTool::tr("More: %1").arg(modifier_keys[0]);
			break;
		case 2:
			text_more = ::OpenOrienteering::MapEditorTool::tr("More: %1, %2").arg(modifier_keys[0], modifier_keys[1]);
			break;
		case 3:
			text_more = ::OpenOrienteering::MapEditorTool::tr("More: %1, %2, %3").arg(modifier_keys[0], modifier_keys[1], modifier_keys[2]);
			break;
		default:
			Q_UNREACHABLE();
		}
		text += QLatin1String("| ") + text_more;
	}
	
	setStatusBarText(text);
}


}  // namespace OpenOrienteering
