/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#include "edit_line_tool.h"

#include <map>
#include <memory>
#include <limits>
#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QtMath>
#include <QFlags>
#include <QLatin1String>
#include <QKeyEvent>
#include <QLocale>
#include <QMouseEvent>
#include <QPoint>
#include <QPointF>
#include <QPointer>
#include <QString>

#include "core/map.h"
#include "core/map_view.h"
#include "core/path_coord.h"
#include "core/objects/object.h"
#include "core/objects/object_mover.h"
#include "core/objects/text_object.h"
#include "core/renderables/renderable.h"
#include "core/symbols/combined_symbol.h"
#include "core/symbols/symbol.h"
#include "gui/modifier_key.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "gui/widgets/key_button_bar.h"
#include "tools/object_selector.h"
#include "tools/point_handles.h"
#include "tools/tool.h"
#include "tools/tool_base.h"
#include "tools/tool_helpers.h"
#include "util/util.h"


namespace
{
	/**
	 * Maximum number of objects in the selection for which point handles
	 * will still be displayed (and can be edited).
	 */
	static unsigned int max_objects_for_handle_display = 10;
}


namespace OpenOrienteering {

EditLineTool::EditLineTool(MapEditorController* editor, QAction* tool_action)
 : EditTool(editor, EditLine, tool_action)
 , highlight_renderables(new MapRenderables(map()))
{
	// nothing
}

EditLineTool::~EditLineTool()
{
	if (highlight_object)
		highlight_renderables->removeRenderablesOfObject(highlight_object, false);
	delete highlight_object;
}

bool EditLineTool::mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	if (waiting_for_mouse_release)
	{
		if (event->buttons() & ~event->button())
			return true;
		waiting_for_mouse_release = false;
	}
	
	return MapEditorToolBase::mousePressEvent(event, map_coord, widget);
}

bool EditLineTool::mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	if (waiting_for_mouse_release)
	{
		if (event->buttons())
			return true;
		waiting_for_mouse_release = false;
	}
	
	return MapEditorToolBase::mouseMoveEvent(event, map_coord, widget);
}

bool EditLineTool::mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	if (waiting_for_mouse_release)
	{
		waiting_for_mouse_release = !event->buttons();
		return true;
	}
	
	return MapEditorToolBase::mouseReleaseEvent(event, map_coord, widget);
}

void EditLineTool::mouseMove()
{
	updateHoverState(cur_pos_map);
}

void EditLineTool::clickPress()
{
	if (hover_state == OverPathEdge &&
		active_modifiers & Qt::ControlModifier)
	{
		Q_ASSERT(hover_object);
		Q_ASSERT(hover_object->getType() == Object::Path);
		
		auto path = hover_object->asPath()->findPartForIndex(hover_line);
		
		// Toggle segment between straight line and curve
		createReplaceUndoStep(hover_object);
		
		// hover_object is going to be modified. Non-const getCoordinate is fine.
		MapCoord& start_coord = hover_object->getCoordinateRef(hover_line);
		if (start_coord.isCurveStart())
		{
			// Convert to straight segment
			/// \todo Provide a PathObject::convertToStraight(hover_line) ?
			hover_object->deleteCoordinate(hover_line + 1, false);
		}
		else
		{
			// Convert to curve
			/// \todo Provide a PathObject::convertToCurve(hover_line) ?
			start_coord.setCurveStart(true);
			const MapCoord end_coord = hover_object->getCoordinate(hover_line + 1);
			double baseline = start_coord.distanceTo(end_coord);
			
			bool tangent_ok = false;
			MapCoordF start_tangent = path->calculateTangent(hover_line, true, tangent_ok);
			if (!tangent_ok)
				start_tangent = MapCoordF(end_coord - start_coord);
			start_tangent.normalize();
			
			MapCoordF end_tangent = path->calculateTangent(hover_line + 1, false, tangent_ok);
			if (!tangent_ok)
				end_tangent = MapCoordF(start_coord - end_coord);
			else
				end_tangent = -end_tangent;
			end_tangent.normalize();
			
			double handle_distance = baseline * BEZIER_HANDLE_DISTANCE;
			MapCoord handle = start_coord + MapCoord(start_tangent * handle_distance);
			handle.setFlags(0);
			hover_object->addCoordinate(hover_line + 1, handle);
			handle = end_coord + MapCoord(end_tangent * handle_distance);
			handle.setFlags(0);
			hover_object->addCoordinate(hover_line + 2, handle);
		}
		
		hover_object->update();
		map()->emitSelectionEdited();
		
		// Make sure that the highlight object is recreated
		deleteHighlightObject();
		updateHoverState(cur_pos_map);
		waiting_for_mouse_release = true;
	}
	
	click_timer.restart();
}

void EditLineTool::clickRelease()
{
	// Maximum time in milliseconds a click may take to cause
	// a selection change if hovering over a handle / frame.
	// If the click takes longer, it is assumed that the user
	// wanted to move the objects instead and no selection change is done.
	const int selection_click_time_threshold = 150;
	
	if (hover_state != OverNothing &&
		click_timer.elapsed() >= selection_click_time_threshold)
	{
		return;
	}
	
	object_selector->selectAt(cur_pos_map, cur_map_widget->getMapView()->pixelToLength(clickTolerance()), active_modifiers & Qt::ShiftModifier);
	updateHoverState(cur_pos_map);
}

void EditLineTool::dragStart()
{
	updateHoverState(click_pos_map);
	
	Map* map = this->map();
	if (hover_state == OverNothing)
	{
		box_selection = true;
	}
	else
	{
		startEditing(map->selectedObjects());
		snap_exclude_object = hover_object;
		
		// Collect elements to move
		object_mover.reset(new ObjectMover(map, click_pos_map));
		if (hoveringOverFrame())
		{
			for (auto object : map->selectedObjects())
				object_mover->addObject(object);
		}
		else
		{
			Q_ASSERT(hover_object);
			
			object_mover->addLine(hover_object, hover_line);
			
			if (!hover_object->getCoordinate(hover_line).isCurveStart())
				angle_helper->setActive(true);
		}
		
		// Set up angle tool helper
		angle_helper->setCenter(click_pos_map);
		if (hoveringOverFrame())
		{
			setupAngleHelperFromEditedObjects();
		}
		else if (hover_object->getType() == Object::Path)
		{
			auto path = hover_object->asPath()->findPartForIndex(hover_line);
			
			angle_helper->clearAngles();
			bool ok = false;
			MapCoordF tangent = path->calculateTangent(hover_line, false, ok);
			if (ok)
				angle_helper->addAngles(-tangent.angle(), M_PI/2);
			tangent = path->calculateTangent(hover_line, true, ok);
			if (ok)
				angle_helper->addAngles(-tangent.angle(), M_PI/2);
			
			int end_index = hover_line + (hover_object->getCoordinate(hover_line).isCurveStart() ? 3 : 1);
			tangent = path->calculateTangent(end_index, false, ok);
			if (ok)
				angle_helper->addAngles(-tangent.angle(), M_PI/2);
			tangent = path->calculateTangent(end_index, true, ok);
			if (ok)
				angle_helper->addAngles(-tangent.angle(), M_PI/2);
		}
		
		// Apply modifier keys to tool helpers
		// using inverted logic for angle helper
		activateAngleHelperWhileEditing(!(active_modifiers & Qt::ControlModifier));
		if (active_modifiers & Qt::ShiftModifier)
			activateSnapHelperWhileEditing();
	}
}

void EditLineTool::dragMove()
{
	if (editingInProgress())
	{
		if (snapped_to_pos && handle_offset != MapCoordF(0, 0))
		{
			// Snapped to a position. Correct the handle offset, so the
			// object moves to this position exactly.
			click_pos_map += handle_offset;
			object_mover->setStartPos(click_pos_map);
			handle_offset = MapCoordF(0, 0);
		}
		
		qint32 dx, dy;
		object_mover->move(constrained_pos_map, false, &dx, &dy);
		if (highlight_object)
		{
			highlight_renderables->removeRenderablesOfObject(highlight_object, false);
			highlight_object->move(dx, dy);
			highlight_object->update();
			highlight_renderables->insertRenderablesOfObject(highlight_object);
		}
		
		updatePreviewObjectsAsynchronously();
	}
	else if (box_selection)
	{
		updateDirtyRect();
	}
}

void EditLineTool::dragFinish()
{
	if (editingInProgress())
	{
		finishEditing();
		angle_helper->setActive(false);
		snap_helper->setFilter(SnappingToolHelper::NoSnapping);
		updateStatusText();
	}
	else if (box_selection)
	{
		object_selector->selectBox(click_pos_map, cur_pos_map, active_modifiers & Qt::ShiftModifier);
		box_selection = false;
	}
}

void EditLineTool::dragCanceled()
{
	if (editingInProgress())
	{
		updateDirtyRect(); // Catch the selection extent including the highlight_object
		abortEditing();
		angle_helper->setActive(false);
		snap_helper->setFilter(SnappingToolHelper::NoSnapping);
		hover_state = OverNothing;
		deleteHighlightObject();
	}
	waiting_for_mouse_release = true;
	box_selection = false;
	updateDirtyRect(); // Catch the changed selection extent
}

bool EditLineTool::keyPress(QKeyEvent* event)
{
	switch(event->key())
	{
	case EditTool::DeleteObjectKey:
		if (map()->getNumSelectedObjects() > 0)
			deleteSelectedObjects();
		updateStatusText();
		return true;
		
	case Qt::Key_Escape:
		if (isDragging())
			cancelDragging();
		else if (map()->getNumSelectedObjects() > 0 && !waiting_for_mouse_release)
			map()->clearObjectSelection(true);
		else
			return false;
		updateStatusText();
		return true;
		
	case Qt::Key_Control:
		if (editingInProgress())
			// This tool uses inverted logic.
			activateAngleHelperWhileEditing(false);
		updateStatusText();
		return false; // not consuming Ctrl
	
	case Qt::Key_Shift:
		if (editingInProgress())
			activateSnapHelperWhileEditing();
		updateStatusText();
		return false; // not consuming Shift
		
	}
	return false;
}

bool EditLineTool::keyRelease(QKeyEvent* event)
{
	switch(event->key())
	{
	case Qt::Key_Control:
		if (editingInProgress())
			// This tool uses inverted logic.
			activateAngleHelperWhileEditing(true);
		updateStatusText();
		return false; // not consuming Ctrl
		
	case Qt::Key_Shift:
		if (editingInProgress())
			activateSnapHelperWhileEditing(false);
		updateStatusText();
		return false; // not consuming Shift
		
	}
	return false;
}

void EditLineTool::initImpl()
{
	objectSelectionChangedImpl();
	
	if (editor->isInMobileMode())
	{
		// Create key replacement bar
		key_button_bar = new KeyButtonBar(editor->getMainWidget());
		key_button_bar->addModifierButton(Qt::ShiftModifier, tr("Snap", "Snap to existing objects"));
		key_button_bar->addModifierButton(Qt::ControlModifier, tr("Toggle curve", "Toggle between curved and flat segment"));
		editor->showPopupWidget(key_button_bar, QString{});
	}
}

void EditLineTool::objectSelectionChangedImpl()
{
	if (isDragging())
	{
		cancelDragging();
	}
	else
	{
		updateHoverState(cur_pos_map);
		updateDirtyRect();
		updateStatusText();
	}
}

int EditLineTool::updateDirtyRectImpl(QRectF& rect)
{
	bool show_object_points = map()->selectedObjects().size() <= max_objects_for_handle_display;
	
	selection_extent = QRectF();
	map()->includeSelectionRect(selection_extent);
	
	rectInclude(rect, selection_extent);
	int pixel_border = show_object_points ? pointHandles().displayRadius() : 1;
	
	// Control points
	if (show_object_points)
	{
		for (auto object : map()->selectedObjects())
			object->includeControlPointsRect(rect);
	}
	
	// Box selection
	if (isDragging() && box_selection)
	{
		rectIncludeSafe(rect, click_pos_map);
		rectIncludeSafe(rect, cur_pos_map);
	}
	
	return pixel_border;
}

void EditLineTool::drawImpl(QPainter* painter, MapWidget* widget)
{
	auto num_selected_objects = map()->selectedObjects().size();
	if (num_selected_objects > 0)
	{
		drawSelectionOrPreviewObjects(painter, widget);
		
		Object* object = *map()->selectedObjectsBegin();
		if (num_selected_objects == 1 &&
		    object->getType() == Object::Text &&
		    !object->asText()->hasSingleAnchor())
		{
			drawBoundingPath(painter, widget, object->asText()->controlPoints(), hoveringOverFrame() ? active_color : selection_color);
		}
		else if (selection_extent.isValid())
		{
			drawBoundingBox(painter, widget, selection_extent, hoveringOverFrame() ? active_color : selection_color);
		}
		
		if (num_selected_objects <= max_objects_for_handle_display)
		{
			for (const auto* object: map()->selectedObjects())
			{
				auto hover_point = std::numeric_limits<MapCoordVector::size_type>::max();
				pointHandles().draw(painter, widget, object, hover_point, false, PointHandles::DisabledHandleState);
			}
		}
		
		if (!highlight_renderables->empty())
			map()->drawSelection(painter, true, widget, highlight_renderables.data(), true);
	}
	
	// Box selection
	if (isDragging() && box_selection)
		drawSelectionBox(painter, widget, click_pos_map, cur_pos_map);
}

void EditLineTool::updatePreviewObjects()
{
	updateStatusText();
	MapEditorToolBase::updatePreviewObjects();
}

void EditLineTool::deleteHighlightObject()
{
	if (highlight_object)
	{
		highlight_renderables->removeRenderablesOfObject(highlight_object, false);
		delete highlight_object;
		highlight_object = nullptr;
	}
}

void EditLineTool::updateStatusText()
{
	QString text;
	if (editingInProgress())
	{
		MapCoordF drag_vector = constrained_pos_map - click_pos_map;
		text = ::OpenOrienteering::EditTool::tr("<b>Coordinate offset:</b> %1, %2 mm  <b>Distance:</b> %3 m ").arg(
		         QLocale().toString(drag_vector.x(), 'f', 1),
		         QLocale().toString(-drag_vector.y(), 'f', 1),
		         QLocale().toString(0.001 * map()->getScaleDenominator() * drag_vector.length(), 'f', 1))
		       + QLatin1String("| ");
		
		if (angle_helper->isActive())
			text += tr("<b>%1</b>: Free movement. ").arg(ModifierKey::control());
		if (!(active_modifiers & Qt::ShiftModifier))
			text += ::OpenOrienteering::EditTool::tr("<b>%1</b>: Snap to existing objects. ").arg(ModifierKey::shift());
		else
			text.chop(2); // Remove "| "
	}
	else
	{
		text = ::OpenOrienteering::EditTool::tr("<b>Click</b>: Select a single object. <b>Drag</b>: Select multiple objects. <b>%1+Click</b>: Toggle selection. ").arg(ModifierKey::shift());
		if (map()->getNumSelectedObjects() > 0)
		{
			text += ::OpenOrienteering::EditTool::tr("<b>%1</b>: Delete selected objects. ").arg(ModifierKey(DeleteObjectKey));
			
			if (map()->selectedObjects().size() <= max_objects_for_handle_display)
			{
				if (active_modifiers & Qt::ControlModifier)
					text = tr("<b>%1+Click</b> on segment: Toggle between straight and curved. ").arg(ModifierKey::control());
				else
					text += ::OpenOrienteering::MapEditorTool::tr("More: %1").arg(ModifierKey::control());
			}
		}
	}
	setStatusBarText(text);
}

void EditLineTool::updateHoverState(const MapCoordF& cursor_pos)
{
	HoverState new_hover_state = OverNothing;
	const PathObject* new_hover_object = nullptr;
	MapCoordVector::size_type new_hover_line = 0;
	
	if (!map()->selectedObjects().empty())
	{
		// Check selected objects
		if (map()->selectedObjects().size() <= max_objects_for_handle_display)
		{
			float click_tolerance_sq = qPow(0.001f * cur_map_widget->getMapView()->pixelToLength(clickTolerance()), 2);
			float best_distance_sq = std::numeric_limits<float>::max();
			
			for (auto object : map()->selectedObjects())
			{
				if (object->getType() == Object::Path)
				{
					PathObject* path = object->asPath();
					float distance_sq;
					PathCoord path_coord;
					path->calcClosestPointOnPath(cursor_pos, distance_sq, path_coord);
					
					if (distance_sq >= +0.0 &&
					    distance_sq < best_distance_sq &&
					    distance_sq < qMax(click_tolerance_sq, (float)qPow(path->getSymbol()->calculateLargestLineExtent(), 2)))
					{
						new_hover_state  = OverPathEdge;
						new_hover_object = path;
						new_hover_line   = path_coord.index;
						best_distance_sq = distance_sq;
						handle_offset    = path_coord.pos - cursor_pos;
						
						const auto part = path->findPartForIndex(new_hover_line);
						if (new_hover_line == part->last_index)
						{
							new_hover_line = part->prevCoordIndex(new_hover_line);
						}
					}
				}
			}
		}
		
		// Check bounding box
		if (new_hover_state != OverPathEdge && selection_extent.isValid())
		{
			QRectF selection_extent_viewport = cur_map_widget->mapToViewport(selection_extent);
			if (pointOverRectangle(cur_map_widget->mapToViewport(cursor_pos), selection_extent_viewport))
			{
				new_hover_state = OverFrame;
				new_hover_object  = nullptr;
				handle_offset = closestPointOnRect(cursor_pos, selection_extent) - cursor_pos;
			}
		}
	}
		
	// Apply possible changes
	if (new_hover_state  != hover_state ||
	    new_hover_line   != hover_line  ||
		new_hover_object != hover_object)
	{
		deleteHighlightObject();
		
		hover_state  = new_hover_state;
		// We have got a Map*, so we may get a PathObject*.
		hover_object = const_cast<PathObject*>(new_hover_object);
		hover_line   = new_hover_line;
		
		if (hover_state == OverPathEdge)
		{
			Q_ASSERT(hover_object);
			
			// Extract hover line
			highlight_object = new PathObject(map()->getCoveringCombinedLine(), *hover_object, hover_line);
			highlight_object->update();
			highlight_renderables->insertRenderablesOfObject(highlight_object);
		}
		
		effective_start_drag_distance = (hover_state == OverNothing) ? startDragDistance() : 0;
		updateDirtyRect();
	}
}


}  // namespace OpenOrienteering
