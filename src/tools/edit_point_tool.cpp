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


#include "edit_point_tool.h"

#include <map>
#include <memory>
#include <limits>
#include <vector>

#include <QtGlobal>
#include <QtMath>
#include <QCursor>
#include <QEvent>
#include <QFlags>
#include <QKeyEvent>
#include <QLatin1String>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QToolButton>

#include "settings.h"
#include "core/map.h"
#include "core/map_part.h"
#include "core/map_view.h"
#include "core/path_coord.h"
#include "core/objects/object.h"
#include "core/objects/object_mover.h"
#include "core/objects/text_object.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/symbol.h"
#include "gui/modifier_key.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "gui/widgets/key_button_bar.h"
#include "tools/object_selector.h"
#include "tools/point_handles.h"
#include "tools/text_object_editor_helper.h"
#include "tools/tool.h"
#include "tools/tool_base.h"
#include "tools/tool_helpers.h"
#include "undo/object_undo.h"
#include "util/util.h"


namespace OpenOrienteering {

namespace {
	
	/**
	 * Maximum number of objects in the selection for which point handles
	 * will still be displayed (and can be edited).
	 */
	static unsigned int max_objects_for_handle_display = 10;
	
	/**
	 * The value which indicates that no point of the current object is hovered.
	 */
	static auto no_point = std::numeric_limits<MapCoordVector::size_type>::max();
	
	
}



EditPointTool::EditPointTool(MapEditorController* editor, QAction* tool_action)
 : EditTool { editor, EditPoint, tool_action }
{
	// noting else
}

EditPointTool::~EditPointTool()
{
	delete text_editor;
}

bool EditPointTool::addDashPointDefault() const
{
	// Toggle dash points depending on if the selected symbol has a dash symbol.
	// TODO: instead of just looking if it is a line symbol with dash points,
	// could also check for combined symbols containing lines with dash points
	return ( hover_object &&
	         hover_object->getSymbol()->getType() == Symbol::Line &&
	         hover_object->getSymbol()->asLine()->getDashSymbol() != nullptr );
}

bool EditPointTool::mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	// TODO: port TextObjectEditorHelper to MapEditorToolBase
	if (text_editor && text_editor->mousePressEvent(event, map_coord, widget))
		return true;
	
	if (waiting_for_mouse_release)
	{
		if (event->buttons() & ~event->button())
			return true;
		waiting_for_mouse_release = false;
	}
	
	return MapEditorToolBase::mousePressEvent(event, map_coord, widget);
}

bool EditPointTool::mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	// TODO: port TextObjectEditorHelper to MapEditorToolBase
	if (text_editor && text_editor->mouseMoveEvent(event, map_coord, widget))
		return true;
	
	if (waiting_for_mouse_release)
	{
		if (event->buttons())
			return true;
		waiting_for_mouse_release = false;
	}
	
	return MapEditorToolBase::mouseMoveEvent(event, map_coord, widget);
}

bool EditPointTool::mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	// TODO: port TextObjectEditorHelper to MapEditorToolBase
	if (text_editor && text_editor->mouseReleaseEvent(event, map_coord, widget))
		return true;
	
	if (waiting_for_mouse_release)
	{
		waiting_for_mouse_release = !event->buttons();
		return true;
	}
	
	return MapEditorToolBase::mouseReleaseEvent(event, map_coord, widget);
}

void EditPointTool::mouseMove()
{
	updateHoverState(cur_pos_map);
	
	// For texts, decide whether to show the beam cursor
	if (hoveringOverSingleText())
		cur_map_widget->setCursor(QCursor(Qt::IBeamCursor));
	else
		cur_map_widget->setCursor(getCursor());
}

void EditPointTool::clickPress()
{
	/// \todo Consider calling updateHoverState from an override of (virtual) mousePositionEvent
	updateHoverState(click_pos_map);
	
	if (hover_state.testFlag(OverPathEdge) &&
	    active_modifiers & Qt::ControlModifier)
	{
		Q_ASSERT(hover_object);
		// Add new point to path
		PathObject* path = hover_object->asPath();
		
		float distance_sq;
		PathCoord path_coord;
		path->calcClosestPointOnPath(cur_pos_map, distance_sq, path_coord);
		
		auto click_tolerance_map_sq = cur_map_widget->getMapView()->pixelToLength(clickTolerance());
		click_tolerance_map_sq = click_tolerance_map_sq * click_tolerance_map_sq;
		
		if (distance_sq <= click_tolerance_map_sq)
		{
			startDragging();
			hover_state = OverObjectNode;
			hover_point = path->subdivide(path_coord);
			if (addDashPointDefault() ^ switch_dash_points)
			{
				auto point = path->getCoordinate(hover_point);
				point.setDashPoint(true);
				path->setCoordinate(hover_point, point);
				map()->emitSelectionEdited();
			}
			startEditingSetup();
			angle_helper->setActive(false);
			updatePreviewObjects();
		}
	}
    else if (hover_state.testFlag(OverObjectNode) &&
	         hover_object->getType() == Object::Path)
	{
		Q_ASSERT(hover_object);
		PathObject* hover_object = this->hover_object->asPath();
		Q_ASSERT(hover_point < hover_object->getCoordinateCount());
		
		if (switch_dash_points &&
		    !hover_object->isCurveHandle(hover_point))
		{
			// Switch point between dash / normal point
			createReplaceUndoStep(hover_object);
			
			auto hover_coord = hover_object->getCoordinate(hover_point);
			hover_coord.setDashPoint(!hover_coord.isDashPoint());
			hover_object->setCoordinate(hover_point, hover_coord);
			hover_object->update();
			updateDirtyRect();
			waiting_for_mouse_release = true;
		}
		else if (active_modifiers & Qt::ControlModifier)
		{
			auto hover_point_part_index = hover_object->findPartIndexForIndex(hover_point);
			PathPart& hover_point_part = hover_object->parts()[hover_point_part_index];
			
			if (hover_object->isCurveHandle(hover_point))
			{
				// Convert the curve into a straight line
				createReplaceUndoStep(hover_object);
				hover_object->deleteCoordinate(hover_point, false);
				hover_object->update();
				map()->emitSelectionEdited();
				updateHoverState(cur_pos_map);
				updateDirtyRect();
				waiting_for_mouse_release = true;
			}
			else
			{
				// Delete the point
				if (hover_point_part.countRegularNodes() <= 2 ||
				    ( !(hover_object->getSymbol()->getContainedTypes() & Symbol::Line) &&
				      hover_point_part.size() <= 3 ) )
				{
					// Not enough remaining points -> delete the part and maybe object
					if (hover_object->parts().size() == 1)
					{
						map()->removeObjectFromSelection(hover_object, false);
						auto undo_step = new AddObjectsUndoStep(map());
						auto part = map()->getCurrentPart();
						int index = part->findObjectIndex(hover_object);
						Q_ASSERT(index >= 0);
						undo_step->addObject(index, hover_object);
						map()->deleteObject(hover_object, true);
						map()->push(undo_step);
						map()->setObjectsDirty();
						map()->emitSelectionEdited();
						updateHoverState(cur_pos_map);
					}
					else
					{
						createReplaceUndoStep(hover_object);
						hover_object->deletePart(hover_point_part_index);
						hover_object->update();
						map()->emitSelectionEdited();
						updateHoverState(cur_pos_map);
						updateDirtyRect();
					}
					waiting_for_mouse_release = true;
				}
				else
				{
					// Delete the point only
					createReplaceUndoStep(hover_object);
					int delete_bezier_spline_point_setting;
					if (active_modifiers & Qt::ShiftModifier)
						delete_bezier_spline_point_setting = Settings::EditTool_DeleteBezierPointActionAlternative;
					else
						delete_bezier_spline_point_setting = Settings::EditTool_DeleteBezierPointAction;
					hover_object->deleteCoordinate(hover_point, true, Settings::getInstance().getSettingCached((Settings::SettingsEnum)delete_bezier_spline_point_setting).toInt());
					hover_object->update();
					map()->emitSelectionEdited();
					updateHoverState(cur_pos_map);
					updateDirtyRect();
					waiting_for_mouse_release = true;
				}
			}
		}
	}
	else if (hoveringOverSingleText())
	{
		box_selection = false;
		
		if (key_button_bar)
			key_button_bar->hide();
		
		TextObject* hover_object = map()->getFirstSelectedObject()->asText();
		startEditing(hover_object);
		
		// Don't show the original text while editing
		map()->removeRenderablesOfObject(hover_object, true);
		
		old_text = hover_object->getText();
		old_horz_alignment = (int)hover_object->getHorizontalAlignment();
		old_vert_alignment = (int)hover_object->getVerticalAlignment();
		text_editor = new TextObjectEditorHelper(hover_object, editor);
		connect(text_editor, &TextObjectEditorHelper::stateChanged, this, &EditPointTool::updatePreviewObjects);
		connect(text_editor, &TextObjectEditorHelper::finished, this, &EditPointTool::finishEditing);
		
		// Send clicked position
		QMouseEvent event { QEvent::MouseButtonPress, click_pos, Qt::LeftButton, Qt::LeftButton, Qt::KeyboardModifiers{} };
		text_editor->mousePressEvent(&event, click_pos_map, cur_map_widget);
	}
	
	click_timer.restart();
}

void EditPointTool::clickRelease()
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

void EditPointTool::dragStart()
{
	updateHoverState(click_pos_map);
	
	if (hover_state == OverNothing)
	{
		box_selection = true;
	}
	else
	{
		startEditing(map()->selectedObjects());
		startEditingSetup();
		
		if (active_modifiers & Qt::ControlModifier)
			activateAngleHelperWhileEditing();
		if (active_modifiers & Qt::ShiftModifier && !hoveringOverCurveHandle())
			activateSnapHelperWhileEditing();
	}
}

void EditPointTool::dragMove()
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
		
		object_mover->move(constrained_pos_map, moveOppositeHandle());
		updatePreviewObjectsAsynchronously();
	}
	else if (box_selection)
	{
		updateDirtyRect();
	}
}

void EditPointTool::dragFinish()
{
	if (editingInProgress())
	{
		finishEditing();
		angle_helper->setActive(false);
		snap_helper->setFilter(SnappingToolHelper::NoSnapping);
	}
	else if (box_selection)
	{
		object_selector->selectBox(click_pos_map, cur_pos_map, active_modifiers & Qt::ShiftModifier);
	}
	box_selection = false;
}

void EditPointTool::dragCanceled()
{
	if (editingInProgress())
	{
		abortEditing();
		angle_helper->setActive(false);
		snap_helper->setFilter(SnappingToolHelper::NoSnapping);
	}
	waiting_for_mouse_release = true;
	box_selection = false;
	updateDirtyRect();
}

void EditPointTool::focusOutEvent(QFocusEvent* event)
{
	Q_UNUSED(event);
	
	// Deactivate modifiers - not always correct, but should be
	// wrong only in unusual cases and better than leaving the modifiers on forever
	switch_dash_points = false;
	if (dash_points_button)
		dash_points_button->setChecked(switch_dash_points);
	updateStatusText();
}


bool EditPointTool::keyPress(QKeyEvent* event)
{
	if (text_editor && text_editor->keyPressEvent(event))
	{
		return true;
	}
	
	switch(event->key())
	{
	case EditTool::DeleteObjectKey:
		if (map()->getNumSelectedObjects() > 0)
			deleteSelectedObjects();
		updateStatusText();
		return true;
		
	case Qt::Key_Escape:
		if (text_editor)
			finishEditing(); 
		else if (isDragging())
			cancelDragging();
		else if (map()->getNumSelectedObjects() > 0 && !waiting_for_mouse_release)
			map()->clearObjectSelection(true);
		updateStatusText();
		return true;
		
	case Qt::Key_Space:
		if (event->modifiers().testFlag(Qt::ControlModifier))
			switch_dash_points = !switch_dash_points;
		else
			switch_dash_points = true;
		if (dash_points_button)
			dash_points_button->setChecked(switch_dash_points);
		updateStatusText();
		return true;
		
	case Qt::Key_Control:
		if (editingInProgress())
			activateAngleHelperWhileEditing();
		updateStatusText();
		return false; // not consuming Ctrl
	
	case Qt::Key_Shift:
		if (hoveringOverCurveHandle())
			reapplyConstraintHelpers();
		else if (editingInProgress())
			activateSnapHelperWhileEditing();
		updateStatusText();
		return false; // not consuming Shift
		
	}
	return false;
}


bool EditPointTool::keyRelease(QKeyEvent* event)
{
	if (text_editor && text_editor->keyReleaseEvent(event))
	{
		return true;
	}
	
	switch(event->key())
	{
	case Qt::Key_Control:
		angle_helper->setActive(false);
		if (editingInProgress())
			reapplyConstraintHelpers();
		updateStatusText();
		return false; // not consuming Ctrl
		
	case Qt::Key_Shift:
		snap_helper->setFilter(SnappingToolHelper::NoSnapping);
		if (hoveringOverCurveHandle())
			reapplyConstraintHelpers();
		else if (editingInProgress())
			reapplyConstraintHelpers();
		updateStatusText();
		return false; // not consuming Shift
		
	case Qt::Key_Space:
		if (event->modifiers().testFlag(Qt::ControlModifier))
			return true;
		
		switch_dash_points = false;
		if (dash_points_button)
			dash_points_button->setChecked(switch_dash_points);
		updateStatusText();
		return true;
		
	}
	return false;
}


bool EditPointTool::inputMethodEvent(QInputMethodEvent* event)
{
	if (text_editor && text_editor->inputMethodEvent(event))
		return true;
	
	return MapEditorTool::inputMethodEvent(event);
}

QVariant EditPointTool::inputMethodQuery(Qt::InputMethodQuery property, const QVariant& argument) const
{
	auto result = QVariant { };
	if (text_editor)
		result = text_editor->inputMethodQuery(property, argument);
	return result;
}

void EditPointTool::initImpl()
{
	objectSelectionChanged();
	
	if (editor->isInMobileMode())
	{
		// Create key replacement bar
		key_button_bar = new KeyButtonBar(editor->getMainWidget());
		key_button_bar->addModifierButton(Qt::ShiftModifier, tr("Snap", "Snap to existing objects"));
		key_button_bar->addModifierButton(Qt::ControlModifier, tr("Point / Angle", "Modify points or use constrained angles"));
		dash_points_button = key_button_bar->addKeyButton(Qt::Key_Space, Qt::ControlModifier, tr("Toggle dash", "Toggle dash points"));
		dash_points_button->setCheckable(true);
		dash_points_button->setChecked(switch_dash_points);
		editor->showPopupWidget(key_button_bar, QString{});
	}
}

void EditPointTool::objectSelectionChangedImpl()
{
	if (text_editor)
	{
		// This case can be reproduced by using "select all objects of symbol" for any symbol while editing a text.
		// Revert selection to text object in order to be able to finish editing. Not optimal, but better than crashing.
		map()->clearObjectSelection(false);
		map()->addObjectToSelection(text_editor->object(), false);
		finishEditing();
		map()->emitSelectionChanged();
		return;
	}
	
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

int EditPointTool::updateDirtyRectImpl(QRectF& rect)
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
	
	// Text selection
	if (text_editor)
		text_editor->includeDirtyRect(rect);
	
	// Box selection
	if (isDragging() && box_selection)
	{
		rectIncludeSafe(rect, click_pos_map);
		rectIncludeSafe(rect, cur_pos_map);
	}
	
	return pixel_border;
}

void EditPointTool::drawImpl(QPainter* painter, MapWidget* widget)
{
	auto num_selected_objects = map()->selectedObjects().size();
	if (num_selected_objects > 0)
	{
		drawSelectionOrPreviewObjects(painter, widget, bool(text_editor));
		
		if (!text_editor)
		{
			Object* object = *map()->selectedObjectsBegin();
			if (num_selected_objects == 1 &&
			    object->getType() == Object::Text &&
			    !object->asText()->hasSingleAnchor())
			{
				drawBoundingPath(painter, widget, object->asText()->controlPoints(), hover_state.testFlag(OverFrame) ? active_color : selection_color);
			}
			else if (selection_extent.isValid())
			{
				auto active = hover_state.testFlag(OverFrame) && !hover_state.testFlag(OverObjectNode);
				drawBoundingBox(painter, widget, selection_extent, active ? active_color : selection_color);
			}
			
			if (num_selected_objects <= max_objects_for_handle_display)
			{
				for (const auto* object: map()->selectedObjects())
				{
					auto active = hover_state.testFlag(OverObjectNode) && hover_object == object;
					auto hover_point = active ? this->hover_point : no_point;
					pointHandles().draw(painter, widget, object, hover_point, true, PointHandles::NormalHandleState);
				}
			}
		}
	}
	
	// Text editor
	if (text_editor)
	{
		painter->save();
		widget->applyMapTransform(painter);
		text_editor->draw(painter, widget);
		painter->restore();
	}
	
	// Box selection
	if (isDragging() && box_selection)
		drawSelectionBox(painter, widget, click_pos_map, cur_pos_map);
}

void EditPointTool::finishEditing()
{
	if (text_editor)
	{
		updateDirtyRect();
		auto text_object = text_editor->object();
		
		delete text_editor;
		text_editor = nullptr;
		
		waiting_for_mouse_release = true;
		
		if (key_button_bar)
			key_button_bar->show();
		
		if (!editedObjectsModified())
		{
			abortEditing();
			updateStatusText();
			return;
		}
		else if (text_object->getText().isEmpty())
		{
			abortEditing();
			updateStatusText();
			
			auto map = this->map();
			auto num_selected_objects = map->selectedObjects().size();
			map->removeObjectFromSelection(text_object, false);
			
			MapPart* part = map->getCurrentPart();
			int index = part->findObjectIndex(text_object);
			if (index >= 0)
			{
				part->deleteObject(index, true);
				
				auto undo_step = new AddObjectsUndoStep(map);
				undo_step->addObject(index, text_object);
				map->push(undo_step);
				map->setObjectsDirty();
			}
			else
			{
				qDebug("text_object not found in current map part.");
			}
			
			if (map->selectedObjects().size() != num_selected_objects)
				emit map->objectSelectionChanged();
			
			return;
		}
	}
	
	MapEditorToolBase::finishEditing();
	updateStatusText();
}

void EditPointTool::updatePreviewObjects()
{
	MapEditorToolBase::updatePreviewObjects();
	updateStatusText();
}

void EditPointTool::updateStatusText()
{
	QString text;
	if (text_editor)
	{
		text = tr("<b>%1</b>: Finish editing. ").arg(ModifierKey::escape());
	}
	else if (editingInProgress())
	{
		MapCoordF drag_vector = constrained_pos_map - click_pos_map;
		text = ::OpenOrienteering::EditTool::tr("<b>Coordinate offset:</b> %1, %2 mm  <b>Distance:</b> %3 m ").
		       arg(QLocale().toString(drag_vector.x(), 'f', 1),
		           QLocale().toString(-drag_vector.y(), 'f', 1),
		           QLocale().toString(0.001 * map()->getScaleDenominator() * drag_vector.length(), 'f', 1)) +
		       QLatin1String("| ");
		
		if (!angle_helper->isActive())
			text += ::OpenOrienteering::EditTool::tr("<b>%1</b>: Fixed angles. ").arg(ModifierKey::control());
		
		if (!(active_modifiers & Qt::ShiftModifier))
		{
			if (hoveringOverCurveHandle())
			{
				//: Actually, this means: "Keep the opposite handle's position. "
				text += tr("<b>%1</b>: Keep opposite handle positions. ").arg(ModifierKey::shift());
			}	
			else 
			{
				text += ::OpenOrienteering::EditTool::tr("<b>%1</b>: Snap to existing objects. ").arg(ModifierKey::shift());
			}
		}
	}
	else
	{
		text = ::OpenOrienteering::EditTool::tr("<b>Click</b>: Select a single object. <b>Drag</b>: Select multiple objects. <b>%1+Click</b>: Toggle selection. ").arg(ModifierKey::shift());
		if (map()->getNumSelectedObjects() > 0)
		{
			text += ::OpenOrienteering::EditTool::tr("<b>%1</b>: Delete selected objects. ").arg(ModifierKey(DeleteObjectKey));
			
			if (map()->selectedObjects().size() <= max_objects_for_handle_display)
			{
				// TODO: maybe show this only if at least one PathObject among the selected objects?
				if (active_modifiers & Qt::ControlModifier)
				{
					if (addDashPointDefault())
						text = tr("<b>%1+Click</b> on point: Delete it; on path: Add a new dash point; with <b>%2</b>: Add a normal point. ").
						       arg(ModifierKey::control(), ModifierKey::space());
					else
						text = tr("<b>%1+Click</b> on point: Delete it; on path: Add a new point; with <b>%2</b>: Add a dash point. ").
						       arg(ModifierKey::control(), ModifierKey::space());
				}
				else if (switch_dash_points)
					text = tr("<b>%1+Click</b> on point to switch between dash and normal point. ").arg(ModifierKey::space());
				else
					text += QLatin1String("| ") + ::OpenOrienteering::MapEditorTool::tr("More: %1, %2").arg(ModifierKey::control(), ModifierKey::space());
			}
		}
	}
	setStatusBarText(text);
}

void EditPointTool::updateHoverState(const MapCoordF& cursor_pos)
{
	HoverState new_hover_state = OverNothing;
	const Object* new_hover_object = nullptr;
	MapCoordVector::size_type new_hover_point = no_point;
	
	if (text_editor)
	{
		handle_offset = MapCoordF(0, 0);
	}
	else if (!map()->selectedObjects().empty())
	{
		if (map()->selectedObjects().size() <= max_objects_for_handle_display)
		{
			// Try to find object node.
			auto best_distance_sq = std::numeric_limits<double>::max();
			for (const auto* object : map()->selectedObjects())
			{
				MapCoordF handle_pos;
				auto hover_point = findHoverPoint(cur_map_widget->mapToViewport(cursor_pos), cur_map_widget, object, true, &handle_pos);
				if (hover_point == no_point)
					continue;
				
				auto distance_sq = cursor_pos.distanceSquaredTo(handle_pos);
				if (distance_sq < best_distance_sq)
				{
					new_hover_state |= OverObjectNode;
					new_hover_object = object;
					new_hover_point  = hover_point;
					best_distance_sq = distance_sq;
					handle_offset    = handle_pos - cursor_pos;
				}
			}
			
			if (!new_hover_state.testFlag(OverObjectNode))
			{
				// No object node found. Try to find path object edge.
				/// \todo De-duplicate: Copied from EditLineTool
				auto click_tolerance_sq = qPow(0.001 * cur_map_widget->getMapView()->pixelToLength(clickTolerance()), 2);
				
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
						    distance_sq < qMax(click_tolerance_sq, qPow(path->getSymbol()->calculateLargestLineExtent(), 2)))
						{
							new_hover_state |= OverPathEdge;
							new_hover_object = path;
							new_hover_point  = path_coord.index;
							best_distance_sq = distance_sq;
							handle_offset    = path_coord.pos - cursor_pos;
						}
					}
				}
			}
		}
		
		if (!new_hover_state.testFlag(OverObjectNode) && selection_extent.isValid())
		{
			QRectF selection_extent_viewport = cur_map_widget->mapToViewport(selection_extent);
			if (pointOverRectangle(cur_map_widget->mapToViewport(cursor_pos), selection_extent_viewport))
			{
				new_hover_state |= OverFrame;
				handle_offset    = closestPointOnRect(cursor_pos, selection_extent) - cursor_pos;
			}
		}
	}
	
	if (new_hover_state  != hover_state  ||
	    new_hover_object != hover_object ||
	    new_hover_point  != hover_point)
	{
		hover_state = new_hover_state;
		// We have got a Map*, so we may get an non-const Object*.
		hover_object = const_cast<Object*>(new_hover_object);
		hover_point  = new_hover_point;
		effective_start_drag_distance = (hover_state == OverNothing) ? startDragDistance() : 0;
		updateDirtyRect();
	}
	
	Q_ASSERT((hover_state.testFlag(OverObjectNode) || hover_state.testFlag(OverPathEdge)) == bool(hover_object));
}

void EditPointTool::setupAngleHelperFromHoverObject()
{
	Q_ASSERT(hover_object->getType() == Object::Path);
	
	angle_helper->clearAngles();
	bool forward_ok = false;
	auto part = hover_object->asPath()->findPartForIndex(hover_point);
	MapCoordF forward_tangent = part->calculateTangent(hover_point, false, forward_ok);
	if (forward_ok)
		angle_helper->addAngles(-forward_tangent.angle(), M_PI/2);
	bool backward_ok = false;
	MapCoordF backward_tangent = part->calculateTangent(hover_point, true, backward_ok);
	if (backward_ok)
		angle_helper->addAngles(-backward_tangent.angle(), M_PI/2);
	
	if (forward_ok && backward_ok)
	{
		double angle = (-backward_tangent.angle() - forward_tangent.angle()) / 2;
		angle_helper->addAngle(angle);
		angle_helper->addAngle(angle + M_PI/2);
		angle_helper->addAngle(angle + M_PI);
		angle_helper->addAngle(angle + 3*M_PI/2);
	}
}

void EditPointTool::startEditingSetup()
{
	// This method operates on MapEditorToolBase::editedItems().
	Q_ASSERT(editingInProgress());
	
	snap_exclude_object = hover_object;
	
	// Collect elements to move
	object_mover.reset(new ObjectMover(map(), click_pos_map));
	if (hover_state.testFlag(OverObjectNode))
	{
		switch (hover_object->getType())
		{
		case Object::Point:
			object_mover->addObject(hover_object);
			setupAngleHelperFromEditedObjects();
			break;
			
		case Object::Path:
			object_mover->addPoint(hover_object->asPath(), hover_point);
			setupAngleHelperFromHoverObject();
			break;
			
		case Object::Text:
			if (hover_object->asText()->hasSingleAnchor())
				object_mover->addObject(hover_object);
			else
				object_mover->addTextHandle(hover_object->asText(), hover_point);
			setupAngleHelperFromEditedObjects();
			break;
			
		default:
			Q_UNREACHABLE();
		}
		angle_helper->setCenter(click_pos_map);
	}
	else if (hover_state.testFlag(OverFrame))
	{
		for (auto object : editedObjects())
			object_mover->addObject(object);
		setupAngleHelperFromEditedObjects();
		angle_helper->setCenter(click_pos_map);
	}
}

bool EditPointTool::hoveringOverSingleText() const
{
	return hover_state == OverNothing &&
	       map()->selectedObjects().size() == 1 &&
	       map()->getFirstSelectedObject()->getType() == Object::Text &&
	       map()->getFirstSelectedObject()->asText()->calcTextPositionAt(cur_pos_map, true) >= 0;
}


bool EditPointTool::hoveringOverCurveHandle() const
{
	return hover_state == OverObjectNode
	       && hover_object->getType() == Object::Path
	       && hover_object->asPath()->isCurveHandle(hover_point);
}


bool EditPointTool::moveOppositeHandle() const
{
	return !(active_modifiers & Qt::ShiftModifier)
	       && hoveringOverCurveHandle();
}


}  // namespace OpenOrienteering
