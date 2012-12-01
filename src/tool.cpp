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


#include "tool.h"

#include <QApplication>
#include <QPainter>
#include <QMouseEvent>

#include "map.h"
#include "map_widget.h"
#include "object.h"
#include "object_text.h"
#include "util.h"
#include "settings.h"
#include "map_editor.h"
#include "map_undo.h"

// ### MapEditorTool ###

const QRgb MapEditorTool::inactive_color = qRgb(0, 0, 255);
const QRgb MapEditorTool::active_color = qRgb(255, 150, 0);
const QRgb MapEditorTool::selection_color = qRgb(210, 0, 229);
QImage* MapEditorTool::point_handles = NULL;

MapEditorTool::MapEditorTool(MapEditorController* editor, Type type, QAction* tool_button): QObject(NULL), tool_button(tool_button), type(type), editor(editor)
{
}
MapEditorTool::~MapEditorTool()
{
	if (tool_button)
		tool_button->setChecked(false);
}

void MapEditorTool::loadPointHandles()
{
	if (!point_handles)
		point_handles = new QImage(":/images/point-handles.png");
}

void MapEditorTool::setStatusBarText(const QString& text)
{
	editor->getWindow()->setStatusBarText(text);
}

void MapEditorTool::startEditingSelection(MapRenderables& old_renderables, std::vector< Object* >* undo_duplicates)
{
	Map* map = editor->getMap();
	
	// Temporarily take the edited objects out of the map so their map renderables are not updated, and make duplicates of them before for the edit step
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if (undo_duplicates)
			undo_duplicates->push_back((*it)->duplicate());
		
		(*it)->setMap(NULL);
		
		// Cache old renderables until the object is inserted into the map again
		old_renderables.insertRenderablesOfObject(*it);
 		(*it)->takeRenderables();
	}
	
	editor->setEditingInProgress(true);
}
void MapEditorTool::resetEditedObjects(std::vector< Object* >* undo_duplicates)
{
	assert(undo_duplicates);
	Map* map = editor->getMap();
	
	size_t i = 0;
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		*(*it) = *undo_duplicates->at(i);
		(*it)->setMap(NULL);
		++i;
	}
}
void MapEditorTool::finishEditingSelection(MapRenderables& renderables, MapRenderables& old_renderables, bool create_undo_step, std::vector< Object* >* undo_duplicates, bool delete_objects)
{
	ReplaceObjectsUndoStep* undo_step = create_undo_step ? new ReplaceObjectsUndoStep(editor->getMap()) : NULL;
	
	int i = 0;
	Map::ObjectSelection::const_iterator it_end = editor->getMap()->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = editor->getMap()->selectedObjectsBegin(); it != it_end; ++it)
	{
		if (!delete_objects)
		{
			(*it)->setMap(editor->getMap());
			(*it)->update(true);
		}
		
		if (create_undo_step)
			undo_step->addObject(*it, undo_duplicates->at(i));
		else
			delete undo_duplicates->at(i);
		++i;
	}
	renderables.clear();
	deleteOldSelectionRenderables(old_renderables, true);
	
	undo_duplicates->clear();
	if (create_undo_step)
		editor->getMap()->objectUndoManager().addNewUndoStep(undo_step);
	
	editor->setEditingInProgress(false);
}
void MapEditorTool::updateSelectionEditPreview(MapRenderables& renderables)
{
	Map::ObjectSelection::const_iterator it_end = editor->getMap()->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = editor->getMap()->selectedObjectsBegin(); it != it_end; ++it)
	{
		(*it)->update(true);
		// NOTE: only necessary because of setMap(NULL) in startEditingSelection(..)
		renderables.insertRenderablesOfObject(*it);
	}
}
void MapEditorTool::deleteOldSelectionRenderables(MapRenderables& old_renderables, bool set_area_dirty)
{
	old_renderables.clear(set_area_dirty);
}

void MapEditorTool::includeControlPointRect(QRectF& rect, Object* object, QPointF* box_text_handles)
{
	if (object->getType() == Object::Path)
	{
		PathObject* path = reinterpret_cast<PathObject*>(object);
		int size = path->getCoordinateCount();
		for (int i = 0; i < size; ++i)
			rectInclude(rect, path->getCoordinate(i).toQPointF());
	}
	else if (object->getType() == Object::Text)
	{
		TextObject* text_object = reinterpret_cast<TextObject*>(object);
		if (text_object->hasSingleAnchor())
			rectInclude(rect, text_object->getAnchorCoordF());
		else
		{
			for (int i = 0; i < 4; ++i)
				rectInclude(rect, box_text_handles[i]);
		}
	}
}
void MapEditorTool::drawPointHandles(int hover_point, QPainter* painter, Object* object, MapWidget* widget)
{
	if (object->getType() == Object::Point)
	{
		PointObject* point = reinterpret_cast<PointObject*>(object);
		drawPointHandle(painter, widget->mapToViewport(point->getCoordF()), NormalHandle, hover_point == 0);
	}
	else if (object->getType() == Object::Text)
	{
		TextObject* text = reinterpret_cast<TextObject*>(object);
		if (text->hasSingleAnchor())
			drawPointHandle(painter, widget->mapToViewport(text->getAnchorCoordF()), NormalHandle, hover_point == 0);
		else
		{
			QPointF box_text_handles[4];
			calculateSelectedBoxTextHandles(box_text_handles, widget->getMapView()->getMap());	// TODO: object->getMap() has lead to a crash here ...
			for (int i = 0; i < 4; ++i)
				drawPointHandle(painter, widget->mapToViewport(box_text_handles[i]), NormalHandle, hover_point == i);
		}
	}
	else if (object->getType() == Object::Path)
	{
		painter->setBrush(Qt::NoBrush); // for handle lines
		
		PathObject* path = reinterpret_cast<PathObject*>(object);
		
		int num_parts = path->getNumParts();
		for (int part_index = 0; part_index < num_parts; ++part_index)
		{
			PathObject::PathPart& part = path->getPart(part_index);
			bool have_curve = part.isClosed() && part.getNumCoords() > 3 && path->getCoordinate(part.end_index - 3).isCurveStart();
			
			for (int i = part.start_index; i <= part.end_index; ++i)
			{
				MapCoord coord = path->getCoordinate(i);
				if (coord.isClosePoint())
					continue;
				QPointF point = widget->mapToViewport(coord);
				bool is_active = hover_point == i;
				
				if (have_curve)
				{
					int curve_index = (i == part.start_index) ? (part.end_index - 1) : (i - 1);
					QPointF curve_handle = widget->mapToViewport(path->getCoordinate(curve_index));
					drawCurveHandleLine(painter, point, curve_handle, is_active);
					drawPointHandle(painter, curve_handle, CurveHandle, is_active || hover_point == curve_index);
					have_curve = false;
				}
				
				if (i == part.start_index && !part.isClosed()) // || (i > part.start_index && path->getCoordinate(i-1).isHolePoint()))
					drawPointHandle(painter, point, StartHandle, is_active);
				else if (i == part.end_index && !part.isClosed()) // || coord.isHolePoint())
					drawPointHandle(painter, point, EndHandle, is_active);
				else
					drawPointHandle(painter, point, coord.isDashPoint() ? DashHandle : NormalHandle, is_active);
				
				if (coord.isCurveStart())
				{
					QPointF curve_handle = widget->mapToViewport(path->getCoordinate(i+1));
					drawCurveHandleLine(painter, point, curve_handle, is_active);
					drawPointHandle(painter, curve_handle, CurveHandle, is_active || hover_point == i + 1);
					i += 2;
					have_curve = true;
				}
			}
		}
	}
	else
		assert(false);
}
void MapEditorTool::drawPointHandle(QPainter* painter, QPointF point, MapEditorTool::PointHandleType type, bool active)
{
	painter->drawImage(qRound(point.x()) - 5, qRound(point.y()) - 5, *point_handles, (int)type * 11, active ? 11 : 0, 11, 11);
}
void MapEditorTool::drawCurveHandleLine(QPainter* painter, QPointF point, QPointF curve_handle, bool active)
{
	const float handle_radius = 3;
	painter->setPen(active ? active_color : inactive_color);
	
	QPointF to_handle = curve_handle - point;
	float to_handle_len = to_handle.x()*to_handle.x() + to_handle.y()*to_handle.y();
	if (to_handle_len > 0.00001f)
	{
		to_handle_len = sqrt(to_handle_len);
		QPointF change = to_handle * (handle_radius / to_handle_len);
		
		point += change;
		curve_handle -= change;
	}
	
	painter->drawLine(point, curve_handle);
}
bool MapEditorTool::calculateSelectedBoxTextHandles(QPointF* out, Map* map)
{
	Object* single_selected_object = (map->getNumSelectedObjects() == 1) ? *map->selectedObjectsBegin() : NULL;
	if (single_selected_object && single_selected_object->getType() == Object::Text)
	{
		TextObject* text_object = single_selected_object->asText();
		if (!text_object->hasSingleAnchor())
		{
			QTransform transform;
			transform.rotate(-text_object->getRotation() * 180 / M_PI);
			out[0] = transform.map(QPointF(text_object->getBoxWidth() / 2, -text_object->getBoxHeight() / 2)) + text_object->getAnchorCoordF().toQPointF();
			out[1] = transform.map(QPointF(text_object->getBoxWidth() / 2, text_object->getBoxHeight() / 2)) + text_object->getAnchorCoordF().toQPointF();
			out[2] = transform.map(QPointF(-text_object->getBoxWidth() / 2, text_object->getBoxHeight() / 2)) + text_object->getAnchorCoordF().toQPointF();
			out[3] = transform.map(QPointF(-text_object->getBoxWidth() / 2, -text_object->getBoxHeight() / 2)) + text_object->getAnchorCoordF().toQPointF();
			return true;
		}
	}
	return false;
}

int MapEditorTool::findHoverPoint(QPointF cursor, Object* object, bool include_curve_handles, QPointF* box_text_handles, QRectF* selection_extent, MapWidget* widget)
{
	int click_tolerance = Settings::getInstance().getSettingCached(Settings::MapEditor_ClickTolerance).toInt();
	const float click_tolerance_squared = click_tolerance * click_tolerance;
	
	// Check object
	if (object)
	{
		if (object->getType() == Object::Point)
		{
			PointObject* point = reinterpret_cast<PointObject*>(object);
			if (distanceSquared(widget->mapToViewport(point->getCoordF()), cursor) <= click_tolerance_squared)
				return 0;
		}
		else if (object->getType() == Object::Text)
		{
			TextObject* text = reinterpret_cast<TextObject*>(object);
			if (text->hasSingleAnchor() && distanceSquared(widget->mapToViewport(text->getAnchorCoordF()), cursor) <= click_tolerance_squared)
				return 0;
			else if (!text->hasSingleAnchor())
			{
				for (int i = 0; i < 4; ++i)
				{
					if (box_text_handles && distanceSquared(widget->mapToViewport(box_text_handles[i]), cursor) <= click_tolerance_squared)
						return i;
				}
			}
		}
		else if (object->getType() == Object::Path)
		{
			PathObject* path = reinterpret_cast<PathObject*>(object);
			int size = path->getCoordinateCount();
			
			int best_index = -1;
			float best_dist_sq = click_tolerance_squared;
			for (int i = size - 1; i >= 0; --i)
			{
				if (!path->getCoordinate(i).isClosePoint())
				{
					float distance_sq = distanceSquared(widget->mapToViewport(path->getCoordinate(i)), cursor);
					bool is_handle = (i >= 1 && path->getCoordinate(i - 1).isCurveStart()) ||
					                 (i >= 2 && path->getCoordinate(i - 2).isCurveStart());
					if (distance_sq < best_dist_sq || (distance_sq == best_dist_sq && is_handle))
					{
						best_index = i;
						best_dist_sq = distance_sq;
					}
				}
				if (!include_curve_handles && i >= 3 && path->getCoordinate(i - 3).isCurveStart())
					i -= 2;
			}
			if (best_index >= 0)
				return best_index;
		}
	}
	
	// Check bounding box
	if (!selection_extent)
		return -2;
	QRectF selection_extent_viewport = widget->mapToViewport(*selection_extent);
	if (cursor.x() < selection_extent_viewport.left() - click_tolerance) return -2;
	if (cursor.y() < selection_extent_viewport.top() - click_tolerance) return -2;
	if (cursor.x() > selection_extent_viewport.right() + click_tolerance) return -2;
	if (cursor.y() > selection_extent_viewport.bottom() + click_tolerance) return -2;
	if (cursor.x() > selection_extent_viewport.left() + click_tolerance &&
		cursor.y() > selection_extent_viewport.top() + click_tolerance &&
		cursor.x() < selection_extent_viewport.right() - click_tolerance &&
		cursor.y() < selection_extent_viewport.bottom() - click_tolerance) return -2;
	return -1;
}

bool MapEditorTool::drawMouseButtonHeld(QMouseEvent* event)
{
	if ((event->buttons() & Qt::LeftButton) ||
		(Settings::getInstance().getSettingCached(Settings::MapEditor_DrawLastPointOnRightClick).toBool() == true &&
		 event->buttons() & Qt::RightButton))
	{
		return true;
	}
	return false;
}

bool MapEditorTool::drawMouseButtonClicked(QMouseEvent* event)
{
	if ((event->button() == Qt::LeftButton) ||
		(Settings::getInstance().getSettingCached(Settings::MapEditor_DrawLastPointOnRightClick).toBool() == true &&
		 event->button() == Qt::RightButton))
	{
		return true;
	}
	return false;
}

// ### MapEditorToolBase ###

MapEditorToolBase::MapEditorToolBase(const QCursor cursor, MapEditorTool::Type type, MapEditorController* editor, QAction* tool_button)
 : MapEditorTool(editor, type, tool_button),
   dragging(false),
   cur_map_widget(editor->getMainWidget()),
   cursor(cursor),
   editing(false),
   old_renderables(new MapRenderables(editor->getMap())), 
   renderables(new MapRenderables(editor->getMap()))
{
}
MapEditorToolBase::~MapEditorToolBase()
{
	deleteOldSelectionRenderables(*old_renderables, false);
}

void MapEditorToolBase::init()
{
	initImpl();
	connect(editor->getMap(), SIGNAL(objectSelectionChanged()), this, SLOT(objectSelectionChanged()));
	connect(editor->getMap(), SIGNAL(selectedObjectEdited()), this, SLOT(updateDirtyRect()));
	updateDirtyRect();
	updateStatusText();
}

bool MapEditorToolBase::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (!(event->buttons() & Qt::LeftButton))
		return false;
	cur_map_widget = widget;
	
	click_pos = event->pos();
	click_pos_map = map_coord;
	cur_pos = click_pos;
	cur_pos_map = click_pos_map;
	clickPress();
	return true;
}
bool MapEditorToolBase::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	cur_pos = event->pos();
	cur_pos_map = map_coord;
	if (!(event->buttons() & Qt::LeftButton))
	{
		mouseMove();
		return false;
	}
	
	if (dragging)
		dragMove();
	else if ((event->pos() - click_pos).manhattanLength() >= QApplication::startDragDistance())
	{
		dragging = true;
		dragStart();
	}
	
	return true;
}
bool MapEditorToolBase::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	if (dragging)
	{
		dragging = false;
		dragMove();
		dragFinish();
	}
	else
		clickRelease();
	
	return true;
}

void MapEditorToolBase::draw(QPainter* painter, MapWidget* widget)
{
	editor->getMap()->drawSelection(painter, true, widget, renderables->isEmpty() ? NULL : renderables.data());
}

void MapEditorToolBase::updateDirtyRect()
{
	QRectF rect;
	editor->getMap()->includeSelectionRect(rect);
	
	int result = updateDirtyRectImpl(rect);
	if (result >= 0)
		editor->getMap()->setDrawingBoundingBox(rect, result, true);
	else
		editor->getMap()->clearDrawingBoundingBox();
}

void MapEditorToolBase::objectSelectionChanged()
{
	objectSelectionChangedImpl();
}

void MapEditorToolBase::updatePreviewObjects()
{
	updateSelectionEditPreview(*renderables);
	updateDirtyRect();
}

void MapEditorToolBase::startEditing()
{
	assert(!editing);
	editing = true;
	startEditingSelection(*old_renderables, &undo_duplicates);
}
void MapEditorToolBase::abortEditing()
{
	assert(editing);
	editing = false;
	finishEditingSelection(*renderables, *old_renderables, false, &undo_duplicates);
}
void MapEditorToolBase::finishEditing()
{
	assert(editing);
	editing = false;
	finishEditingSelection(*renderables, *old_renderables, true, &undo_duplicates);
	editor->getMap()->setObjectsDirty();
	editor->getMap()->emitSelectionEdited();
}
