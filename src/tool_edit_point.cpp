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


#include "tool_edit_point.h"

#include <limits>

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "util.h"
#include "symbol.h"
#include "object.h"
#include "object_text.h"
#include "map.h"
#include "map_widget.h"
#include "map_undo.h"
#include "symbol_dock_widget.h"
#include "tool_draw_text.h"
#include "tool_helpers.h"
#include "symbol_line.h"
#include "symbol_text.h"
#include "renderable.h"
#include "settings.h"
#include "map_editor.h"
#include "gui/main_window.h"
#include "gui/modifier_key.h"
#include "gui/widgets/key_button_bar.h"


int EditPointTool::max_objects_for_handle_display = 10;

EditPointTool::EditPointTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget)
: EditTool(editor, EditPoint, symbol_widget, tool_button)
{
	hover_point = -2;
	hover_object = NULL;
	text_editor = NULL;
	box_selection = false;
	space_pressed = false;
	no_more_effect_on_click = false;
}

EditPointTool::~EditPointTool()
{
	if (text_editor)
		delete text_editor;
}

bool EditPointTool::addDashPointDefault() const
{
	// Toggle dash points depending on if the selected symbol has a dash symbol.
	// TODO: instead of just looking if it is a line symbol with dash points,
	// could also check for combined symbols containing lines with dash points
	return ( hover_object &&
	         hover_object->getSymbol()->getType() == Symbol::Line &&
	         hover_object->getSymbol()->asLine()->getDashSymbol() != NULL );
}

bool EditPointTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	// TODO: port TextObjectEditorHelper to MapEditorToolBase
	if (text_editor)
	{
		if (!text_editor->mousePressEvent(event, map_coord, widget))
			finishEditing();
		return true;
	}
	else
		return MapEditorToolBase::mousePressEvent(event, map_coord, widget);
}
bool EditPointTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	// TODO: port TextObjectEditorHelper to MapEditorToolBase
	if (text_editor)
		return text_editor->mouseMoveEvent(event, map_coord, widget);
	else
		return MapEditorToolBase::mouseMoveEvent(event, map_coord, widget);
}
bool EditPointTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	// TODO: port TextObjectEditorHelper to MapEditorToolBase
	if (text_editor)
	{
		if (event->button() == Qt::LeftButton)
		{
			dragging = false;
			box_selection = false;
		}
		if (!text_editor->mouseReleaseEvent(event, map_coord, widget))
			finishEditing();
		return true;
	}
	else
		return MapEditorToolBase::mouseReleaseEvent(event, map_coord, widget);
}

void EditPointTool::mouseMove()
{
	updateHoverPoint(cur_pos_map);
	
	// For texts, decide whether to show the beam cursor
	if (hoveringOverSingleText())
		cur_map_widget->setCursor(QCursor(Qt::IBeamCursor));
	else
		cur_map_widget->setCursor(*getCursor());
}

void EditPointTool::clickPress()
{
	if (space_pressed &&
		hover_point >= 0 &&
		hover_object &&
		hover_object->getType() == Object::Path &&
		!hover_object->asPath()->isCurveHandle(hover_point))
	{
		// Switch point between dash / normal point
		createReplaceUndoStep(hover_object);
		
		MapCoord& hover_coord = hover_object->asPath()->getCoordinate(hover_point);
		hover_coord.setDashPoint(!hover_coord.isDashPoint());
		hover_object->update(true);
		updateDirtyRect();
		no_more_effect_on_click = true;
	}
	else if (hover_object &&
		hover_object->getType() == Object::Path &&
		hover_point < 0 &&
		active_modifiers & Qt::ControlModifier)
	{
		// Add new point to path
		PathObject* path = hover_object->asPath();
		
		float distance_sq;
		PathCoord path_coord;
		path->calcClosestPointOnPath(cur_pos_map, distance_sq, path_coord);
		
		float click_tolerance = Settings::getInstance().getMapEditorClickTolerancePx();
		float click_tolerance_map_sq = cur_map_widget->getMapView()->pixelToLength(click_tolerance);
		click_tolerance_map_sq = click_tolerance_map_sq * click_tolerance_map_sq;
		
		if (distance_sq <= click_tolerance_map_sq)
		{
			startEditing();
			dragging = true;	// necessary to prevent second call to startEditing()
			hover_point = path->subdivide(path_coord.index, path_coord.param);
			if (addDashPointDefault() ^ space_pressed)
			{
				MapCoord point = path->getCoordinate(hover_point);
				point.setDashPoint(true);
				path->setCoordinate(hover_point, point);
				map()->emitSelectionEdited();
			}
			startEditingSetup();
			updatePreviewObjects();
		}
	}
	else if (hover_object &&
		hover_object->getType() == Object::Path &&
		hover_point >= 0 &&
		active_modifiers & Qt::ControlModifier)
	{
		PathObject* path = hover_object->asPath();
		int hover_point_part_index = path->findPartIndexForIndex(hover_point);
		PathObject::PathPart& hover_point_part = path->getPart(hover_point_part_index);
		
		if (path->isCurveHandle(hover_point))
		{
			// Convert the curve into a straight line
			createReplaceUndoStep(path);
			path->deleteCoordinate(hover_point, false);
			if (path->getCoordinate(hover_point - 1).isCurveStart())
			{
				path->getCoordinate(hover_point - 1).setCurveStart(false);
				path->deleteCoordinate(hover_point, false);
			}
			else // if (path->getCoordinate(hover_point - 2).isCurveStart())
			{
				path->getCoordinate(hover_point - 2).setCurveStart(false);
				path->deleteCoordinate(hover_point - 1, false);
			}
			
			path->update(true);
			map()->emitSelectionEdited();
			updateHoverPoint(cur_pos_map);
			updateDirtyRect();
			no_more_effect_on_click = true;
		}
		else
		{
			// Delete the point
			if (hover_point_part.calcNumRegularPoints() <= 2 || (!(path->getSymbol()->getContainedTypes() & Symbol::Line) && hover_point_part.getNumCoords() <= 3))
			{
				// Not enough remaining points -> delete the part and maybe object
				if (path->getNumParts() == 1)
					deleteSelectedObjects();
				else
				{
					createReplaceUndoStep(path);
					path->deletePart(hover_point_part_index);
					path->update(true);
					map()->emitSelectionEdited();
					updateHoverPoint(cur_pos_map);
					updateDirtyRect();
				}
				no_more_effect_on_click = true;
			}
			else
			{
				// Delete the point only
				createReplaceUndoStep(path);
				int delete_bezier_spline_point_setting;
				if (active_modifiers & Qt::ShiftModifier)
					delete_bezier_spline_point_setting = Settings::EditTool_DeleteBezierPointActionAlternative;
				else
					delete_bezier_spline_point_setting = Settings::EditTool_DeleteBezierPointAction;
				path->deleteCoordinate(hover_point, true, Settings::getInstance().getSettingCached((Settings::SettingsEnum)delete_bezier_spline_point_setting).toInt());
				path->update(true);
				map()->emitSelectionEdited();
				updateHoverPoint(cur_pos_map);
				updateDirtyRect();
				no_more_effect_on_click = true;
			}
		}
	}
	else if (hover_point == -2 &&
		hover_object &&
		hover_object->getType() == Object::Text &&
		hoveringOverSingleText())
	{
		TextObject* text_object = hover_object->asText();
		startEditing();
		
		// Don't show the original text while editing
		map()->removeRenderablesOfObject(text_object, true);
		
		// Make sure that the TextObjectEditorHelper remembers the correct standard cursor
		cur_map_widget->setCursor(*getCursor());
		
		old_text = text_object->getText();
		old_horz_alignment = (int)text_object->getHorizontalAlignment();
		old_vert_alignment = (int)text_object->getVerticalAlignment();
		text_editor = new TextObjectEditorHelper(text_object, editor);
		connect(text_editor, SIGNAL(selectionChanged(bool)), this, SLOT(textSelectionChanged(bool)));
		
		// Select clicked position
		int pos = text_object->calcTextPositionAt(cur_pos_map, false);
		text_editor->setSelection(pos, pos);
		
		updatePreviewObjects();
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
	
	if (no_more_effect_on_click)
	{
		no_more_effect_on_click = false;
		return;
	}
	if (hover_point >= -1
		&& click_timer.elapsed() >= selection_click_time_threshold)
		return;
	
	float click_tolerance = Settings::getInstance().getMapEditorClickTolerancePx();
	object_selector->selectAt(cur_pos_map, cur_map_widget->getMapView()->pixelToLength(click_tolerance), active_modifiers & Qt::ShiftModifier);
	updateHoverPoint(cur_pos_map);
}

void EditPointTool::dragStart()
{
	if (no_more_effect_on_click)
		return;
	updateHoverPoint(click_pos_map);
	
	if (hover_point >= -1)
	{
		startEditing();
		startEditingSetup();
		
		if (active_modifiers & Qt::ControlModifier)
			activateAngleHelperWhileEditing();
		if (active_modifiers & Qt::ShiftModifier)
			activateSnapHelperWhileEditing();
	}
	else if (hover_point == -2)
	{
		box_selection = true;
	}
}

void EditPointTool::dragMove()
{
	if (no_more_effect_on_click)
		return;
	if (editing)
	{
		if (snapped_to_pos && handle_offset != MapCoordF(0, 0))
		{
			// Snapped to a position. Correct the handle offset, so the
			// object moves to this position exactly.
			click_pos_map += handle_offset;
			object_mover->setStartPos(click_pos_map);
			handle_offset = MapCoordF(0, 0);
		}
		
		object_mover->move(constrained_pos_map, !(active_modifiers & Qt::ShiftModifier));
		updatePreviewObjectsAsynchronously();
	}
	else if (box_selection)
	{
		updateDirtyRect();
	}
}

void EditPointTool::dragFinish()
{
	if (no_more_effect_on_click)
		no_more_effect_on_click = false;
	
	if (editing)
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

void EditPointTool::focusOutEvent(QFocusEvent* event)
{
	Q_UNUSED(event);
	
	// Deactivate modifiers - not always correct, but should be
	// wrong only in unusual cases and better than leaving the modifiers on forever
	space_pressed = false;
	updateStatusText();
}

bool EditPointTool::keyPress(QKeyEvent* event)
{
	if (text_editor)
	{
		if (event->key() == Qt::Key_Escape)
		{
			finishEditing(); 
			return true;
		}
		return text_editor->keyPressEvent(event);
	}
	
	int num_selected_objects = map()->getNumSelectedObjects();
	
	if (num_selected_objects > 0 && event->key() == delete_object_key)
		deleteSelectedObjects();
	else if (num_selected_objects > 0 && event->key() == Qt::Key_Escape)
		map()->clearObjectSelection(true);
	else if (event->key() == Qt::Key_Control)
	{
		if (editing)
			activateAngleHelperWhileEditing();
	}
	else if (event->key() == Qt::Key_Shift && editing)
	{
		if (hover_object != NULL &&
			hover_point >= 0 &&
			hover_object->getType() == Object::Path &&
			hover_object->asPath()->isCurveHandle(hover_point))
		{
			// In this case, Shift just activates deactivates
			// the opposite curve handle constraints
			return true;
		}
		activateSnapHelperWhileEditing();
	}
	else if (event->key() == Qt::Key_Space)
	{
		space_pressed = true;
	}
	else
		return false;
	updateStatusText();
	return true;
}

bool EditPointTool::keyRelease(QKeyEvent* event)
{
	if (text_editor)
		return text_editor->keyReleaseEvent(event);
	
	if (event->key() == Qt::Key_Control)
	{
		angle_helper->setActive(false);
		if (editing)
		{
			calcConstrainedPositions(cur_map_widget);
			dragMove();
		}
	}
	else if (event->key() == Qt::Key_Shift)
	{
		snap_helper->setFilter(SnappingToolHelper::NoSnapping);
		if (editing)
		{
			calcConstrainedPositions(cur_map_widget);
			dragMove();
		}
	}
	else if (event->key() == Qt::Key_Space)
	{
		space_pressed = false;
	}
	else
		return false;
	updateStatusText();
	return true;
}

void EditPointTool::initImpl()
{
	objectSelectionChanged();
	
	if (editor->isInMobileMode())
	{
		// Create key replacement bar
		key_button_bar = new KeyButtonBar(this, editor->getMainWidget());
		key_button_bar->addModifierKey(Qt::Key_Shift, Qt::ShiftModifier, tr("Snap", "Snap to existing objects"));
		key_button_bar->addModifierKey(Qt::Key_Control, Qt::ControlModifier, tr("Point / Angle", "Modify points or use constrained angles"));
		key_button_bar->addModifierKey(Qt::Key_Space, 0, tr("Toggle dash", "Toggle dash points"));
		editor->showPopupWidget(key_button_bar, "");
	}
}

void EditPointTool::objectSelectionChangedImpl()
{
	if (text_editor)
	{
		// This case can be reproduced by using "select all objects of symbol" for any symbol while editing a text.
		// Revert selection to text object in order to be able to finish editing. Not optimal, but better than crashing.
		map()->clearObjectSelection(false);
		map()->addObjectToSelection(text_editor->getObject(), false);
		finishEditing();
		map()->emitSelectionChanged();
		return;
	}
	
	updateHoverPoint(cur_pos_map);
	updateDirtyRect();
	updateStatusText();
}

int EditPointTool::updateDirtyRectImpl(QRectF& rect)
{
	bool show_object_points = map()->getNumSelectedObjects() <= max_objects_for_handle_display;
	
	selection_extent = QRectF();
	map()->includeSelectionRect(selection_extent);
	
	rectInclude(rect, selection_extent);
	int pixel_border = show_object_points ? (resolution_scale_factor * 6) : 1;
	
	// Control points
	if (show_object_points)
	{
		for (Map::ObjectSelection::const_iterator it = map()->selectedObjectsBegin(), end = map()->selectedObjectsEnd(); it != end; ++it)
			includeControlPointRect(rect, *it);
	}
	
	// Text selection
	if (text_editor)
		text_editor->includeDirtyRect(rect);
	
	// Box selection
	if (dragging && box_selection)
	{
		rectIncludeSafe(rect, click_pos_map.toQPointF());
		rectIncludeSafe(rect, cur_pos_map.toQPointF());
	}
	
	return pixel_border;
}

void EditPointTool::drawImpl(QPainter* painter, MapWidget* widget)
{
	int num_selected_objects = map()->getNumSelectedObjects();
	if (num_selected_objects > 0)
	{
		drawSelectionOrPreviewObjects(painter, widget, text_editor != NULL);
		
		if (!text_editor)
		{
			if (selection_extent.isValid())
				drawBoundingBox(painter, widget, selection_extent, hoveringOverFrame() ? active_color : selection_color);
			
			if (num_selected_objects <= max_objects_for_handle_display)
			{
				for (Map::ObjectSelection::const_iterator it = map()->selectedObjectsBegin(), end = map()->selectedObjectsEnd(); it != end; ++it)
					drawPointHandles((hover_object == *it) ? hover_point : -2, painter, *it, widget, true, MapEditorTool::NormalHandleState);
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
	if (dragging && box_selection)
		drawSelectionBox(painter, widget, click_pos_map, cur_pos_map);
}

void EditPointTool::textSelectionChanged(bool text_change)
{
	Q_UNUSED(text_change);
	updatePreviewObjects();
}

void EditPointTool::finishEditing()
{
	bool create_undo_step = true;
	bool delete_objects = false;
	
	if (text_editor)
	{
		delete text_editor;
		text_editor = NULL;
		
		TextObject* text_object = reinterpret_cast<TextObject*>(*map()->selectedObjectsBegin());
		if (text_object->getText().isEmpty())
		{
			text_object->setText(old_text);
			text_object->setHorizontalAlignment((TextObject::HorizontalAlignment)old_horz_alignment);
			text_object->setVerticalAlignment((TextObject::VerticalAlignment)old_vert_alignment);
			create_undo_step = false;
			delete_objects = true;
		}
		else if (text_object->getText() == old_text && (int)text_object->getHorizontalAlignment() == old_horz_alignment && (int)text_object->getVerticalAlignment() == old_vert_alignment)
			create_undo_step = false;
	}
	
	MapEditorToolBase::finishEditing(delete_objects, create_undo_step);
	
	if (delete_objects)
		deleteSelectedObjects();
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
	if (editing)
	{
		MapCoordF drag_vector = constrained_pos_map - click_pos_map;
		text = EditTool::tr("<b>Coordinate offset:</b> %1, %2 mm  <b>Distance:</b> %3 m ").
		       arg(QLocale().toString(drag_vector.getX(), 'f', 1)).
		       arg(QLocale().toString(-drag_vector.getY(), 'f', 1)).
		       arg(QLocale().toString(0.001 * map()->getScaleDenominator() * drag_vector.length(), 'f', 1)) +
		       "| ";
		
		if (!angle_helper->isActive())
			text += EditTool::tr("<b>%1</b>: Fixed angles. ").arg(ModifierKey::control());
		
		if (!(active_modifiers & Qt::ShiftModifier))
		{
			if (hover_object != NULL &&
				hover_point >= 0 &&
				hover_object->getType() == Object::Path &&
				hover_object->asPath()->isCurveHandle(hover_point))
			{
				text += tr("<b>%1</b>: Keep opposite handle positions. ").arg(ModifierKey::shift());
			}
			else
			{
				text += EditTool::tr("<b>%1</b>: Snap to existing objects. ").arg(ModifierKey::shift());
			}
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
				else if (space_pressed)
					text = tr("<b>%1+Click</b> on point to switch between dash and normal point. ").arg(ModifierKey::space());
				else
					text += "| " + MapEditorTool::tr("More: %1, %2").arg(ModifierKey::control(), ModifierKey::space());
			}
		}
	}
	setStatusBarText(text);
}

void EditPointTool::updateHoverPoint(MapCoordF cursor_pos)
{
	Object* new_hover_object = NULL;
	int new_hover_point = -2;
	if (map()->getNumSelectedObjects() <= max_objects_for_handle_display)
	{
		float new_hover_point_dist_sq = std::numeric_limits<float>::max();
		for (Map::ObjectSelection::const_iterator it = map()->selectedObjectsBegin(), end = map()->selectedObjectsEnd(); it != end; ++it)
		{
			MapCoordF handle_pos;
			int hover_point = findHoverPoint(cur_map_widget->mapToViewport(cursor_pos), *it, true, &selection_extent, cur_map_widget, &handle_pos);
			float distance_sq = (hover_point >= 0) ? cursor_pos.lengthToSquared(handle_pos) : -1;
			if (hover_point >= 0 && distance_sq < new_hover_point_dist_sq)
			{
				new_hover_object = *it;
				new_hover_point = hover_point;
				new_hover_point_dist_sq = distance_sq;
				handle_offset = handle_pos - cursor_pos;
			}
		}
	}
	if (new_hover_point < 0 &&
		map()->getNumSelectedObjects() > 0 &&
		selection_extent.isValid())
	{
		QRectF selection_extent_viewport = cur_map_widget->mapToViewport(selection_extent);
		new_hover_point = pointOverRectangle(cur_map_widget->mapToViewport(cursor_pos), selection_extent_viewport) ? -1 : -2;
		handle_offset = closestPointOnRect(cursor_pos, selection_extent) - cursor_pos;
	}
	
	// TODO: this is a HACK to make it possible to create new points with Ctrl
	// (there has to be a hover_object for that).
	// Make it possible to insert points into any selected object (and multiple at once,
	// if at the same position)
	if (new_hover_point < 0 && map()->getNumSelectedObjects() == 1)
		new_hover_object = map()->getFirstSelectedObject();
	
	if (text_editor)
	{
		new_hover_point = -2;
		new_hover_object = NULL;
		handle_offset = MapCoordF(0, 0);
	}
	if (new_hover_object != hover_object ||
		new_hover_point != hover_point)
	{
		updateDirtyRect();
		hover_object = new_hover_object;
		hover_point = new_hover_point;
		start_drag_distance = (hover_point >= -1) ? 0 : Settings::getInstance().getStartDragDistancePx();
	}
}

void EditPointTool::startEditingSetup()
{
	snap_exclude_object = hover_object;
	
	// Collect elements to move
	object_mover.reset(new ObjectMover(map(), click_pos_map));
	if (hoveringOverFrame())
	{
		for (Map::ObjectSelection::const_iterator it = map()->selectedObjectsBegin(), it_end = map()->selectedObjectsEnd(); it != it_end; ++it)
			object_mover->addObject(*it);
		setupAngleHelperFromSelectedObjects();
	}
	else
	{
		if (hover_object->getType() == Object::Point)
		{
			object_mover->addObject(hover_object);
			setupAngleHelperFromSelectedObjects();
		}
		else if (hover_object->getType() == Object::Path)
			object_mover->addPoint(hover_object->asPath(), hover_point);
		else if (hover_object->getType() == Object::Text)
		{
			TextObject* text = hover_object->asText();
			if (text->hasSingleAnchor())
				object_mover->addObject(hover_object);
			else
				object_mover->addTextHandle(text, hover_point);
			setupAngleHelperFromSelectedObjects();
		}
	}
	
	// Set up angle tool helper (for path points)
	angle_helper->setCenter(click_pos_map);
	if (hover_point >= 0 &&
		hover_object &&
		hover_object->getType() == Object::Path)
	{
		angle_helper->clearAngles();
		bool forward_ok = false;
		MapCoordF forward_tangent = PathCoord::calculateTangent(hover_object->getRawCoordinateVector(), hover_point, false, forward_ok);
		if (forward_ok)
			angle_helper->addAngles(-forward_tangent.getAngle(), M_PI/2);
		bool backward_ok = false;
		MapCoordF backward_tangent = PathCoord::calculateTangent(hover_object->getRawCoordinateVector(), hover_point, true, backward_ok);
		if (backward_ok)
			angle_helper->addAngles(-backward_tangent.getAngle(), M_PI/2);
		
		if (forward_ok && backward_ok)
		{
			double angle = (-backward_tangent.getAngle() - forward_tangent.getAngle()) / 2;
			angle_helper->addAngle(angle);
			angle_helper->addAngle(angle + M_PI/2);
			angle_helper->addAngle(angle + M_PI);
			angle_helper->addAngle(angle + 3*M_PI/2);
		}
	}
}

bool EditPointTool::hoveringOverSingleText()
{
	if (hover_point != -2)
		return false;
	
	// TODO: make editing of text objects possible when multiple objects are selected
	Object* single_selected_object = (map()->getNumSelectedObjects() == 1) ? *map()->selectedObjectsBegin() : NULL;
	if (single_selected_object && single_selected_object->getType() == Object::Text)
	{
		TextObject* text_object = reinterpret_cast<TextObject*>(single_selected_object);
		return text_object->calcTextPositionAt(cur_pos_map, true) >= 0;
	}
	return false;
}
