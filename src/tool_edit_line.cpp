/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013, 2014 Kai Pastor
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


#include "tool_edit_line.h"

#include <algorithm>
#include <limits>

#include <QKeyEvent>

#include "map.h"
#include "object_undo.h"
#include "map_widget.h"
#include "object.h"
#include "object_text.h"
#include "renderable.h"
#include "symbol_combined.h"
#include "symbol_line.h"
#include "settings.h"
#include "symbol.h"
#include "tool_helpers.h"
#include "util.h"
#include "map_editor.h"
#include "gui/main_window.h"
#include "gui/modifier_key.h"
#include "gui/widgets/key_button_bar.h"

class SymbolWidget;


int EditLineTool::max_objects_for_handle_display = 10;

EditLineTool::EditLineTool(MapEditorController* editor, QAction* tool_button)
: EditTool(editor, EditLine, tool_button)
{
	hover_line = -2;
	hover_object = NULL;
	highlight_object = NULL;
	highlight_renderables.reset(new MapRenderables(map()));
	box_selection = false;
	no_more_effect_on_click = false;
}

EditLineTool::~EditLineTool()
{
	if (highlight_object)
		highlight_renderables->removeRenderablesOfObject(highlight_object, false);
	delete highlight_object;
}

void EditLineTool::mouseMove()
{
	updateHoverLine(cur_pos_map);
}

void EditLineTool::clickPress()
{
	if (hover_object &&
		hover_line >= 0 &&
		active_modifiers & Qt::ControlModifier)
	{
		// Toggle segment between straight line and curve
		createReplaceUndoStep(hover_object);
		
		MapCoord& start_coord = hover_object->getCoordinate(hover_line);
		if (start_coord.isCurveStart())
		{
			// Convert to straight segment
			start_coord.setCurveStart(false);
			hover_object->deleteCoordinate(hover_line + 2, false);
			hover_object->deleteCoordinate(hover_line + 1, false);
		}
		else
		{
			// Convert to curve
			start_coord.setCurveStart(true);
			MapCoord end_coord = hover_object->getCoordinate(hover_line + 1);
			double baseline = start_coord.lengthTo(end_coord);
			
			bool tangent_ok = false;
			MapCoordF start_tangent = PathCoord::calculateTangent(hover_object->getRawCoordinateVector(), hover_line, true, tangent_ok);
			if (!tangent_ok)
				start_tangent = MapCoordF(end_coord - start_coord);
			start_tangent.normalize();
			
			MapCoordF end_tangent = PathCoord::calculateTangent(hover_object->getRawCoordinateVector(), hover_line + 1, false, tangent_ok);
			if (!tangent_ok)
				end_tangent = MapCoordF(start_coord - end_coord);
			else
				end_tangent = -end_tangent;
			end_tangent.normalize();
			
			double handle_distance = baseline * BEZIER_HANDLE_DISTANCE;
			MapCoord handle = start_coord + (start_tangent * handle_distance).toMapCoord();
			handle.setFlags(0);
			hover_object->addCoordinate(hover_line + 1, handle);
			handle = end_coord + (end_tangent * handle_distance).toMapCoord();
			handle.setFlags(0);
			hover_object->addCoordinate(hover_line + 2, handle);
		}
		
		hover_object->update(true);
		map()->emitSelectionEdited();
		
		// Make sure that the highlight object is recreated
		deleteHighlightObject();
		updateHoverLine(cur_pos_map);
		no_more_effect_on_click = true;
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
	
	if (no_more_effect_on_click)
	{
		no_more_effect_on_click = false;
		return;
	}
	if (hover_line >= -1
		&& click_timer.elapsed() >= selection_click_time_threshold)
		return;
	
	float click_tolerance = Settings::getInstance().getMapEditorClickTolerancePx();
	object_selector->selectAt(cur_pos_map, cur_map_widget->getMapView()->pixelToLength(click_tolerance), active_modifiers & Qt::ShiftModifier);
	updateHoverLine(cur_pos_map);
}

void EditLineTool::dragStart()
{
	if (no_more_effect_on_click)
		return;
	
	updateHoverLine(click_pos_map);
	
	Map* map = this->map();
	if (hover_line >= -1)
	{
		startEditing();
		snap_exclude_object = hover_object;
		
		// Collect elements to move
		object_mover.reset(new ObjectMover(map, click_pos_map));
		if (hoveringOverFrame())
		{
			for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(), it_end = map->selectedObjectsEnd(); it != it_end; ++it)
				object_mover->addObject(*it);
		}
		else
		{
			object_mover->addLine(hover_object, hover_line);
			
			if (!hover_object->getCoordinate(hover_line).isCurveStart())
				angle_helper->setActive(true);
		}
		
		// Set up angle tool helper
		angle_helper->setCenter(click_pos_map);
		if (hover_line == -1)
			setupAngleHelperFromSelectedObjects();
		else
		{
			angle_helper->clearAngles();
			bool ok = false;
			MapCoordF tangent = PathCoord::calculateTangent(hover_object->getRawCoordinateVector(), hover_line, false, ok);
			if (ok)
				angle_helper->addAngles(-tangent.getAngle(), M_PI/2);
			tangent = PathCoord::calculateTangent(hover_object->getRawCoordinateVector(), hover_line, true, ok);
			if (ok)
				angle_helper->addAngles(-tangent.getAngle(), M_PI/2);
			
			int end_index = hover_line + (hover_object->getCoordinate(hover_line).isCurveStart() ? 3 : 1);
			tangent = PathCoord::calculateTangent(hover_object->getRawCoordinateVector(), end_index, false, ok);
			if (ok)
				angle_helper->addAngles(-tangent.getAngle(), M_PI/2);
			tangent = PathCoord::calculateTangent(hover_object->getRawCoordinateVector(), end_index, true, ok);
			if (ok)
				angle_helper->addAngles(-tangent.getAngle(), M_PI/2);
		}
		
		// Activate tool helpers if modifier pressed
		if (active_modifiers & Qt::ControlModifier)
			toggleAngleHelper();
		if (active_modifiers & Qt::ShiftModifier)
			activateSnapHelperWhileEditing();
	}
	else if (hover_line == -2)
	{
		box_selection = true;
	}
}

void EditLineTool::dragMove()
{
	if (no_more_effect_on_click)
		return;
	
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
		
		qint64 dx, dy;
		object_mover->move(constrained_pos_map, !(active_modifiers & Qt::ShiftModifier), &dx, &dy);
		if (highlight_object)
		{
			highlight_renderables->removeRenderablesOfObject(highlight_object, false);
			highlight_object->move(dx, dy);
			highlight_object->update(true);
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
	if (no_more_effect_on_click)
	{
		no_more_effect_on_click = false;
		return;
	}
	
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

bool EditLineTool::keyPress(QKeyEvent* event)
{
	int num_selected_objects = map()->getNumSelectedObjects();
	
	if (num_selected_objects > 0 && event->key() == delete_object_key)
		deleteSelectedObjects();
	else if (num_selected_objects > 0 && event->key() == Qt::Key_Escape)
		map()->clearObjectSelection(true);
	else if (event->key() == Qt::Key_Control)
	{
		if (editingInProgress())
			toggleAngleHelper();
	}
	else if (event->key() == Qt::Key_Shift && editingInProgress())
		activateSnapHelperWhileEditing();
	else
		return false;
	updateStatusText();
	return true;
}

bool EditLineTool::keyRelease(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Control)
	{
		toggleAngleHelper();
	}
	else if (event->key() == Qt::Key_Shift)
	{
		snap_helper->setFilter(SnappingToolHelper::NoSnapping);
		if (editingInProgress())
		{
			dragMove();
			calcConstrainedPositions(cur_map_widget);
		}
	}
	else
		return false;
	updateStatusText();
	return true;
}

void EditLineTool::initImpl()
{
	objectSelectionChangedImpl();
	
	if (editor->isInMobileMode())
	{
		// Create key replacement bar
		key_button_bar = new KeyButtonBar(this, editor->getMainWidget());
		key_button_bar->addModifierKey(Qt::Key_Shift, Qt::ShiftModifier, tr("Snap", "Snap to existing objects"));
		key_button_bar->addModifierKey(Qt::Key_Control, Qt::ControlModifier, tr("Toggle curve", "Toggle between curved and flat segment"));
		editor->showPopupWidget(key_button_bar, "");
	}
}

void EditLineTool::objectSelectionChangedImpl()
{
	updateHoverLine(cur_pos_map);
	updateDirtyRect();
	updateStatusText();
}

int EditLineTool::updateDirtyRectImpl(QRectF& rect)
{
	bool show_object_points = map()->getNumSelectedObjects() <= max_objects_for_handle_display;
	
	selection_extent = QRectF();
	map()->includeSelectionRect(selection_extent);
	
	rectInclude(rect, selection_extent);
	int pixel_border = show_object_points ? (scaleFactor() * 6) : 1;
	
	// Control points
	if (show_object_points)
	{
		for (Map::ObjectSelection::const_iterator it = map()->selectedObjectsBegin(), end = map()->selectedObjectsEnd(); it != end; ++it)
			(*it)->includeControlPointsRect(rect);
	}
	
	// Box selection
	if (isDragging() && box_selection)
	{
		rectIncludeSafe(rect, click_pos_map.toQPointF());
		rectIncludeSafe(rect, cur_pos_map.toQPointF());
	}
	
	return pixel_border;
}

void EditLineTool::drawImpl(QPainter* painter, MapWidget* widget)
{
	int num_selected_objects = map()->getNumSelectedObjects();
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
			for (Map::ObjectSelection::const_iterator it = map()->selectedObjectsBegin(), end = map()->selectedObjectsEnd(); it != end; ++it)
				pointHandles().draw(painter, widget, *it, -2, false, PointHandles::DisabledHandleState);
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
		highlight_object = NULL;
	}
}

void EditLineTool::updateStatusText()
{
	QString text;
	if (editingInProgress())
	{
		MapCoordF drag_vector = constrained_pos_map - click_pos_map;
		text = EditTool::tr("<b>Coordinate offset:</b> %1, %2 mm  <b>Distance:</b> %3 m ").
		       arg(QLocale().toString(drag_vector.getX(), 'f', 1)).
		       arg(QLocale().toString(-drag_vector.getY(), 'f', 1)).
		       arg(QLocale().toString(0.001 * map()->getScaleDenominator() * drag_vector.length(), 'f', 1)) +
		       "| ";
		
		if (!(active_modifiers & Qt::ShiftModifier))
		{
			if ( ((active_modifiers & Qt::ControlModifier) > 0) ^ angle_helper->isActive() )
				text += tr("<b>%1</b>: Free movement. ").arg(ModifierKey::control());
			else
				text += EditTool::tr("<b>%1</b>: Fixed angles. ").arg(ModifierKey::control());
		}
		if (!(active_modifiers & Qt::ControlModifier))
		{
			text += EditTool::tr("<b>%1</b>: Snap to existing objects. ").arg(ModifierKey::shift());
		}
	}
	else
	{
		text = EditTool::tr("<b>Click</b>: Select a single object. <b>Drag</b>: Select multiple objects. <b>%1+Click</b>: Toggle selection. ").arg(ModifierKey::shift());
		if (map()->getNumSelectedObjects() > 0)
		{
			text += EditTool::tr("<b>%1</b>: Delete selected objects. ").arg(ModifierKey(delete_object_key));
			
			if (map()->getNumSelectedObjects() <= max_objects_for_handle_display)
			{
				if (active_modifiers & Qt::ControlModifier)
					text = tr("<b>%1+Click</b> on segment: Toggle between straight and curved. ").arg(ModifierKey::control());
				else
					text += MapEditorTool::tr("More: %1").arg(ModifierKey::control());
			}
		}
	}
	setStatusBarText(text);
}

void EditLineTool::updateHoverLine(MapCoordF cursor_pos)
{
	float click_tolerance = Settings::getInstance().getMapEditorClickTolerancePx();
	float click_tolerance_sq = qPow(0.001f * cur_map_widget->getMapView()->pixelToLength(click_tolerance), 2);
	float best_distance_sq = std::numeric_limits<float>::max();
	PathObject* new_hover_object = NULL;
	int new_hover_line = -2;
	PathCoord hover_path_coord;
	
	// Check objects
	if (map()->getNumSelectedObjects() <= max_objects_for_handle_display)
	{
		for (Map::ObjectSelection::const_iterator it = map()->selectedObjectsBegin(), end = map()->selectedObjectsEnd(); it != end; ++it)
		{
			Object* object = *it;
			if (object->getType() != Object::Path)
				continue;
			
			float distance_sq;
			PathCoord path_coord;
			PathObject* path = object->asPath();
			path->calcClosestPointOnPath(cursor_pos, distance_sq, path_coord);
			
			if (distance_sq < best_distance_sq &&
				distance_sq < qMax(click_tolerance_sq, (float)qPow(path->getSymbol()->calculateLargestLineExtent(map()), 2)))
			{
				best_distance_sq = distance_sq;
				new_hover_object = path;
				new_hover_line = path_coord.index;
				hover_path_coord = path_coord;
				handle_offset = hover_path_coord.pos - cursor_pos;
			}
		}
	}
	
	// Check bounding box
	if (new_hover_line < 0 &&
		map()->getNumSelectedObjects() > 0 &&
		selection_extent.isValid())
	{
		QRectF selection_extent_viewport = cur_map_widget->mapToViewport(selection_extent);
		new_hover_line = pointOverRectangle(cur_map_widget->mapToViewport(cursor_pos), selection_extent_viewport) ? -1 : -2;
		handle_offset = closestPointOnRect(cursor_pos, selection_extent) - cursor_pos;
	}
	
	// Apply possible changes
	if (new_hover_line != hover_line ||
		new_hover_object != hover_object)
	{
		hover_line = new_hover_line;
		hover_object = new_hover_object;
		
		deleteHighlightObject();
		if (hover_object)
		{
			// Extract hover line
			highlight_object = new PathObject(map()->getCoveringCombinedLine());
			MapCoord& start_coord = hover_object->getCoordinate(hover_line);
			highlight_object->addCoordinate(start_coord);
			int end_coord_offset = 1;
			if (start_coord.isCurveStart())
			{
				highlight_object->addCoordinate(hover_object->getCoordinate(hover_line + 1));
				highlight_object->addCoordinate(hover_object->getCoordinate(hover_line + 2));
				end_coord_offset = 3;
			}
			MapCoord end_coord = hover_object->getCoordinate(hover_line + end_coord_offset);
			end_coord.setClosePoint(false);
			end_coord.setCurveStart(false);
			end_coord.setHolePoint(false);
			highlight_object->addCoordinate(end_coord);
			
			highlight_object->update(true);
			highlight_renderables->insertRenderablesOfObject(highlight_object);
		}
		
		start_drag_distance = (hover_line >= -1) ? 0 : Settings::getInstance().getStartDragDistancePx();
		updateDirtyRect();
	}
}

void EditLineTool::toggleAngleHelper()
{
	if (!editingInProgress())
		angle_helper->setActive(false);
	else
		activateAngleHelperWhileEditing(!angle_helper->isActive());
}
