/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2015 Kai Pastor
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


#include "tool_cut.h"

#include <QApplication>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>

#include "map.h"
#include "map_widget.h"
#include "object.h"
#include "object_undo.h"
#include "renderable.h"
#include "settings.h"
#include "symbol.h"
#include "symbol_combined.h"
#include "tool_boolean.h"
#include "tool_draw_path.h"
#include "util.h"


namespace
{
	/**
	 * Maximum number of objects in the selection for which point handles
	 * will still be displayed (and can be edited).
	 */
	static unsigned int max_objects_for_handle_display = 10;
}



CutTool::CutTool(MapEditorController* editor, QAction* tool_action)
 : MapEditorTool { editor, Other, tool_action }
 , dragging { false }
 , hover_state { HoverFlag::OverNothing }
 , hover_object { nullptr }
 , hover_point { 0 }
 , path_tool { nullptr }
 , preview_path { nullptr }
 , renderables { new MapRenderables(map()) }
{
	// nothing
}

void CutTool::init()
{
	connect(map(), SIGNAL(objectSelectionChanged()), this, SLOT(objectSelectionChanged()));
	updateDirtyRect();
	updateStatusText();
	
	MapEditorTool::init();
}

const QCursor& CutTool::getCursor() const
{
	static auto const cursor = scaledToScreen(QCursor{ QPixmap(QString::fromLatin1(":/images/cursor-cut.png")), 11, 11 });
	return cursor;
}

CutTool::~CutTool()
{
	deletePreviewPath();
	delete path_tool;
}

bool CutTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (path_tool)
		return path_tool->mousePressEvent(event, map_coord, widget);
	
	if (!(event->buttons() & Qt::LeftButton))
		return false;
	
	updateHoverState(widget->mapToViewport(map_coord), widget);
	
	dragging = false;
	click_pos = event->pos();
	click_pos_map = map_coord;
	cur_pos = event->pos();
	cur_pos_map = map_coord;
	cutting_area = false;
	
	PathCoord path_point;
	if (findEditPoint(path_point, edit_object, map_coord, (int)Symbol::Area, 0, widget))
	{
		startCuttingArea(path_point, widget);
		// The check is actually necessary to prevent segfaults,
		// even though path_tool is created in the method called above!
		if (path_tool)
			path_tool->mousePressEvent(event, path_point.pos, widget);
	}
	
	return true;
}

bool CutTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (path_tool)
		return path_tool->mouseMoveEvent(event, map_coord, widget);
	
	bool mouse_down = event->buttons() & Qt::LeftButton;
	if (!mouse_down || dragging)
		updateHoverState(widget->mapToViewport(map_coord), widget);
	
	if (mouse_down)
	{
		if (!dragging && (event->pos() - click_pos).manhattanLength() >= Settings::getInstance().getStartDragDistancePx())
		{
			// Start dragging
			dragging = true;
			dragging_on_line = false;
			setEditingInProgress(true);
			
			PathCoord split_pos;
			if (findEditPoint(split_pos, edit_object, click_pos_map, (int)Symbol::Line, (int)Symbol::Area, widget))
			{
				drag_start_len = split_pos.clen;
				drag_end_len = drag_start_len;
				drag_part_index = edit_object->findPartIndexForIndex(split_pos.index);
				drag_forward = true;
				dragging_on_line = true;
			}
		}
		else if (dragging)
		{
			updateDragging(map_coord, widget);
		}
		return true;
	}
	return false;
}

bool CutTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (path_tool)
		return path_tool->mouseReleaseEvent(event, map_coord, widget);
	
	if (event->button() != Qt::LeftButton)
		return false;
	
	if (dragging)
	{
		updateDragging(map_coord, widget);
		
		if (dragging_on_line)
		{
			Q_ASSERT(edit_object->getType() == Object::Path);
			splitLine(edit_object, drag_part_index, drag_start_len, drag_end_len);
		}
		
		deletePreviewPath();
		dragging = false;
	}
	else
	{
		PathCoord split_pos;
		if (findEditPoint(split_pos, edit_object, map_coord, (int)Symbol::Line, 0, widget))
		{
			Q_ASSERT(edit_object->getType() == Object::Path);
			splitLine(edit_object, split_pos);
		}
	}
	
	setEditingInProgress(false);
	return true;
}

bool CutTool::mouseDoubleClickEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (path_tool)
		return path_tool->mouseDoubleClickEvent(event, map_coord, widget);
	
	return false;
}

void CutTool::leaveEvent(QEvent* event)
{
	if (path_tool)
		return path_tool->leaveEvent(event);
}

bool CutTool::keyPressEvent(QKeyEvent* event)
{
	if (path_tool)
		return path_tool->keyPressEvent(event);
	
	return false;
}

bool CutTool::keyReleaseEvent(QKeyEvent* event)
{
	if (path_tool)
		return path_tool->keyReleaseEvent(event);
	
	return false;
}

void CutTool::focusOutEvent(QFocusEvent* event)
{
	if (path_tool)
		return path_tool->focusOutEvent(event);
}

void CutTool::draw(QPainter* painter, MapWidget* widget)
{
	Map* map = this->map();
	map->drawSelection(painter, true, widget, nullptr);
	for (const auto object: map->selectedObjects())
	{
		auto hover_point = MapCoordVector::size_type
		                   { (hover_object == object) ? this->hover_point : std::numeric_limits<MapCoordVector::size_type>::max() };
		pointHandles().draw(painter, widget, object, hover_point);
	}
	
	if (preview_path)
	{
		const MapView* map_view = widget->getMapView();
		painter->save();
		painter->translate(widget->width() / 2.0 + map_view->panOffset().x(),
						   widget->height() / 2.0 + map_view->panOffset().y());
		painter->setWorldTransform(map_view->worldTransform(), true);
		
		RenderConfig config = { *map, map_view->calculateViewedRect(widget->viewportToView(widget->rect())), map_view->calculateFinalZoomFactor(), RenderConfig::Tool, 0.5 };
		renderables->draw(painter, config);
		
		painter->restore();
	}
	
	if (path_tool)
		path_tool->draw(painter, widget);
}

void CutTool::updateDirtyRect(const QRectF* path_rect) const
{
	Map* map = this->map();
	QRectF rect;
	if (path_rect)
		rect = *path_rect;
	
	map->includeSelectionRect(rect);
	
	for (auto object : map->selectedObjects())
		object->includeControlPointsRect(rect);
	
	if (rect.isValid())
		map->setDrawingBoundingBox(rect, 6, true);
	else
		map->clearDrawingBoundingBox();
}

void CutTool::updateDragging(MapCoordF cursor_pos_map, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	if (dragging_on_line)
	{
		PathCoord path_coord;
		if (hover_state != EditTool::OverObjectNode)
		{
			float distance_sq;
			edit_object->calcClosestPointOnPath(cursor_pos_map, distance_sq, path_coord);
		}
		else
		{
			path_coord = edit_object->findPathCoordForIndex(hover_point);
		}
		
		if (edit_object->findPartIndexForIndex(path_coord.index) != drag_part_index)
			return;	// dragging on a different part
		
		//float click_tolerance_map = 0.001 * widget->getMapView()->pixelToLength(click_tolerance);
		//if (distance_sq <= click_tolerance_map*click_tolerance_map)
		{
			auto new_drag_end_len = path_coord.clen;
			const PathPart& drag_part = edit_object->parts()[drag_part_index];
			auto path_length = drag_part.path_coords.back().clen;
			bool delta_forward; 
			if (drag_part.isClosed())
			{
				auto value = fmod(new_drag_end_len - drag_end_len + path_length, path_length);
				delta_forward = value >= 0 && value < 0.5 * path_length;
			}
			else
			{
				delta_forward = new_drag_end_len >= drag_end_len;
			}
			
			if (delta_forward && !drag_forward &&
				fmod(drag_end_len - drag_start_len + path_length, path_length) > 0.5f * path_length &&
				fmod(new_drag_end_len - drag_start_len + path_length, path_length) <= 0.5f * path_length)
			{
				drag_forward = true;
			}
			else if (!delta_forward && drag_forward &&
				fmod(drag_end_len - drag_start_len + path_length, path_length) <= 0.5f * path_length &&
				fmod(new_drag_end_len - drag_start_len + path_length, path_length) > 0.5f * path_length)
			{
				drag_forward = false;
			}
			
			drag_end_len = new_drag_end_len;
			if (drag_end_len != drag_start_len)
				updatePreviewObjects();
		}
	}
}

void CutTool::updateHoverState(QPointF cursor_pos_screen, MapWidget* widget)
{
	HoverState new_hover_state = HoverFlag::OverNothing;
	const Object* new_hover_object = nullptr;
	MapCoordVector::size_type new_hover_point = 0;
	
	if (map()->selectedObjects().size() <= max_objects_for_handle_display)
	{
		auto best_distance_sq = std::numeric_limits<double>::max();
		for (const auto object : map()->selectedObjects())
		{
			MapCoordF handle_pos;
			auto hover_point = findHoverPoint(cursor_pos_screen, widget, object, false, &handle_pos);
			if (hover_point == std::numeric_limits<MapCoordVector::size_type>::max())
				continue;
			
			auto distance_sq = widget->viewportToMapF(cursor_pos_screen).distanceSquaredTo(handle_pos);
			if (distance_sq < best_distance_sq)
			{
				new_hover_state  = HoverFlag::OverObjectNode;
				new_hover_object = object;
				new_hover_point  = hover_point;
				best_distance_sq = distance_sq;
			}
		}
	}	
	
	if (new_hover_state  != hover_state  ||
	    new_hover_object != hover_object ||
	    new_hover_point  != hover_point)
	{
		hover_state  = new_hover_state;
		// We have got a Map*, so we may get a Object*.
		hover_object = const_cast<Object*>(new_hover_object);
		hover_point  = new_hover_point;
		updateDirtyRect();
	}
}

bool CutTool::findEditPoint(PathCoord& out_edit_point, PathObject*& out_edit_object, MapCoordF cursor_pos_map, int with_type, int without_type, MapWidget* widget)
{
	Map* map = this->map();
	
	out_edit_object = nullptr;
	if (hover_state == HoverFlag::OverObjectNode &&
	    hover_object->getSymbol()->getContainedTypes() & with_type &&
	    !(hover_object->getSymbol()->getContainedTypes() & without_type))
	{
		// Hovering over a point of a line
		if (hover_object->getType() != Object::Path)
		{
			Q_ASSERT(!"TODO: make this work for non-path objects");
			return false;
		}
		
		out_edit_object = reinterpret_cast<PathObject*>(hover_object);
		out_edit_point  = out_edit_object->findPathCoordForIndex(hover_point);
	}
	else
	{
		// Check if a line segment was clicked
		float min_distance_sq = 999999;
		for (const auto object : map->selectedObjects())
		{
			if (!(object->getSymbol()->getContainedTypes() & with_type) ||
			    object->getSymbol()->getContainedTypes() & without_type)
			{
				continue;
			}
			
			if (object->getType() != Object::Path)
			{
				Q_ASSERT(!"TODO: make this work for non-path objects");
				continue;
			}
			
			PathObject* path = reinterpret_cast<PathObject*>(object);
			float distance_sq;
			PathCoord path_coord;
			path->calcClosestPointOnPath(cursor_pos_map, distance_sq, path_coord);
			
			float click_tolerance_map = 0.001f * widget->getMapView()->pixelToLength(clickTolerance());
			if (distance_sq < min_distance_sq && distance_sq <= click_tolerance_map*click_tolerance_map)
			{
				min_distance_sq = distance_sq;
				out_edit_object = path;
				out_edit_point  = path_coord;
			}
		}
	}
	return out_edit_object != nullptr;
}

void CutTool::updatePreviewObjects()
{
	deletePreviewPath();
	
	preview_path = new PathObject { edit_object->parts()[drag_part_index] };
	preview_path->setSymbol(Map::getCoveringCombinedLine(), false);
	if (drag_forward)
		preview_path->changePathBounds(0, drag_start_len, drag_end_len);
	else
		preview_path->changePathBounds(0, drag_end_len, drag_start_len);
	
	preview_path->update();
	renderables->insertRenderablesOfObject(preview_path);
	
	updateDirtyRect();
}

void CutTool::deletePreviewPath()
{
	if (preview_path)
	{
		renderables->removeRenderablesOfObject(preview_path, false);
		delete preview_path;
		preview_path = nullptr;
	}
}

void CutTool::objectSelectionChanged()
{
	Map* map = this->map();
	bool have_line_or_area = false;
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if ((*it)->getSymbol()->getContainedTypes() & (Symbol::Line | Symbol::Area))
		{
			have_line_or_area = true;
			break;
		}
	}
	
	if (!have_line_or_area)
		deactivate();
	else
		updateDirtyRect();
}

void CutTool::pathDirtyRectChanged(const QRectF& rect)
{
	updateDirtyRect(&rect);
}

void CutTool::pathAborted()
{
	path_tool->deleteLater();
	path_tool = nullptr;
	cutting_area = false;
	updateDirtyRect();
}

void CutTool::pathFinished(PathObject* split_path)
{
	Map* map = this->map();
	
	// Get path endpoint and check if it is on the area boundary
	const MapCoordVector& path_coords = split_path->getRawCoordinateVector();
	MapCoord path_end = path_coords.at(path_coords.size() - 1);
	
	PathObject* edited_path = reinterpret_cast<PathObject*>(edit_object);
	PathCoord end_path_coord;
	float distance_sq;
	edited_path->calcClosestPointOnPath(MapCoordF(path_end), distance_sq, end_path_coord);
	
	float click_tolerance_map = 0.001 * edit_widget->getMapView()->pixelToLength(clickTolerance());
	if (distance_sq > click_tolerance_map*click_tolerance_map)
	{
		QMessageBox::warning(window(), tr("Error"), tr("The split line must end on the area boundary!"));
		pathAborted();
		return;
	}
	else if (drag_part_index != edited_path->findPartIndexForIndex(end_path_coord.index))
	{
		QMessageBox::warning(window(), tr("Error"), tr("Start and end of the split line are at different parts of the object!"));
		pathAborted();
		return;
	}
	else if (drag_start_len == end_path_coord.clen)
	{
		QMessageBox::warning(window(), tr("Error"), tr("Start and end of the split line are at the same position!"));
		pathAborted();
		return;
	}
	
	Q_ASSERT(split_path->parts().size() == 1);
	split_path->parts().front().setClosed(false);
	split_path->setCoordinate(split_path->getCoordinateCount() - 1, MapCoord(end_path_coord.pos));
	
	// Do the splitting
	const double split_threshold = 0.01;
	
	MapPart* part = map->getCurrentPart();
	AddObjectsUndoStep* add_step = new AddObjectsUndoStep(map);
	add_step->addObject(part->findObjectIndex(edited_path), edited_path);
	map->removeObjectFromSelection(edited_path, false);
	map->deleteObject(edited_path, true);
	map->setObjectsDirty();
	
	DeleteObjectsUndoStep* delete_step = new DeleteObjectsUndoStep(map);
	
	PathObject* holes = nullptr; // if the edited path contains holes, they are saved in this temporary object
	if (edited_path->parts().size() > 1)
	{
		holes = edited_path->duplicate()->asPath();
		holes->deletePart(0);
	}
	
	bool ok; Q_UNUSED(ok); // "ok" is only used in Q_ASSERT.
	PathObject* out_paths[2] = { new PathObject { edited_path->parts().front() }, nullptr };
	const PathPart& drag_part = edited_path->parts()[drag_part_index];
	if (drag_part.isClosed())
	{
		out_paths[1] = new PathObject { *out_paths[0] };
		
		out_paths[0]->changePathBounds(drag_part_index, drag_start_len, end_path_coord.clen);
		ok = out_paths[0]->connectIfClose(split_path, split_threshold);
		Q_ASSERT(ok);

		out_paths[1]->changePathBounds(drag_part_index, end_path_coord.clen, drag_start_len);
		ok = out_paths[1]->connectIfClose(split_path, split_threshold);
		Q_ASSERT(ok);
	}
	else
	{
		float min_cut_pos = qMin(drag_start_len, end_path_coord.clen);
		float max_cut_pos = qMax(drag_start_len, end_path_coord.clen);
		float path_len = drag_part.path_coords.back().clen;
		if (min_cut_pos <= 0 && max_cut_pos >= path_len)
		{
			ok = out_paths[0]->connectIfClose(split_path, split_threshold);
			Q_ASSERT(ok);
			
			out_paths[1] = new PathObject { *split_path };
			out_paths[1]->setSymbol(edited_path->getSymbol(), false);
		}
		else if (min_cut_pos <= 0 || max_cut_pos >= path_len)
		{
			float cut_pos = (min_cut_pos <= 0) ? max_cut_pos : min_cut_pos;
			out_paths[1] = new PathObject { *out_paths[0] };
			
			out_paths[0]->changePathBounds(drag_part_index, 0, cut_pos);
			ok = out_paths[0]->connectIfClose(split_path, split_threshold);
			Q_ASSERT(ok);
			
			out_paths[1]->changePathBounds(drag_part_index, cut_pos, path_len);
			ok = out_paths[1]->connectIfClose(split_path, split_threshold);
			Q_ASSERT(ok);
		}
		else
		{
			out_paths[1] = new PathObject { *out_paths[0] };
			PathObject* temp_path = new PathObject { *out_paths[0] };
			
			out_paths[0]->changePathBounds(drag_part_index, min_cut_pos, max_cut_pos);
			ok = out_paths[0]->connectIfClose(split_path, split_threshold);
			Q_ASSERT(ok);
			
			out_paths[1]->changePathBounds(drag_part_index, 0, min_cut_pos);
			ok = out_paths[1]->connectIfClose(split_path, split_threshold);
			Q_ASSERT(ok);
			
			temp_path->changePathBounds(drag_part_index, max_cut_pos, path_len);
			ok = out_paths[1]->connectIfClose(temp_path, split_threshold);
			Q_ASSERT(ok);
			
			delete temp_path;
		}
	}
	
	for (auto&& object : out_paths)
	{
		if (holes)
		{
			BooleanTool hole_tool = { BooleanTool::Intersection, map };
			BooleanTool::PathObjects out_objects;
			for (auto&& hole : holes->parts())
			{
				out_objects.clear();
				PathObject hole_object(hole);
				hole_tool.executeForLine(object, &hole_object, out_objects);
				for (auto&& new_hole : out_objects)
					object->appendPathPart(new_hole->parts().front());
			}
		}
		
		map->addObject(object);
		delete_step->addObject(part->findObjectIndex(object));
		map->addObjectToSelection(object, false);
	}
	
	CombinedUndoStep* undo_step = new CombinedUndoStep(map);
	undo_step->push(add_step);
	undo_step->push(delete_step);
	map->push(undo_step);
	map->setObjectsDirty();
	
	pathAborted();
}

void CutTool::splitLine(PathObject* object, std::size_t part_index, qreal begin, qreal end) const
{
	if (!drag_forward)
		qSwap(begin, end);
	
	auto split_objects = object->asPath()->removeFromLine(part_index, begin, end);
	replaceObject(object, split_objects);
}

void CutTool::splitLine(PathObject* object, const PathCoord& split_pos) const
{
	auto split_objects = object->splitLineAt(split_pos);
	if (!split_objects.empty())
		replaceObject(object, split_objects);
}

void CutTool::replaceObject(PathObject* object, const std::vector<PathObject*>& replacement) const
{
	Map* map = this->map();
	MapPart* map_part = map->getCurrentPart();
	
	AddObjectsUndoStep* add_step = new AddObjectsUndoStep(map);
	add_step->addObject(map_part->findObjectIndex(object), object);
	map->removeObjectFromSelection(object, false);
	map->deleteObject(object, true);
	
	DeleteObjectsUndoStep* delete_step = new DeleteObjectsUndoStep(map);
	for (Object* new_object : replacement)
	{
		map->addObject(new_object);
		map->addObjectToSelection(new_object, false);
		delete_step->addObject(map_part->findObjectIndex(new_object));
	}
	
	CombinedUndoStep* undo_step = new CombinedUndoStep(map);
	undo_step->push(add_step);
	undo_step->push(delete_step);
	map->push(undo_step);
	
	map->emitSelectionChanged();
}

void CutTool::updateStatusText()
{
	setStatusBarText(tr("<b>Click</b> on a line: Split it into two. <b>Drag</b> along a line: Remove this line part. <b>Click or Drag</b> at an area boundary: Start a split line. "));
}

void CutTool::startCuttingArea(const PathCoord& coord, MapWidget* widget)
{
	drag_part_index = edit_object->findPartIndexForIndex(coord.index);
	if (drag_part_index > 0)
	{
		QMessageBox::warning(window(), tr("Error"), tr("Splitting holes of area objects is not supported yet!"));
		return;
	}
	cutting_area = true;
	drag_start_len = coord.clen;
	edit_widget = widget;
	
	path_tool = new DrawPathTool(editor, nullptr, true, false);
	connect(path_tool, SIGNAL(dirtyRectChanged(QRectF)), this, SLOT(pathDirtyRectChanged(QRectF)));
	connect(path_tool, SIGNAL(pathAborted()), this, SLOT(pathAborted()));
	connect(path_tool, SIGNAL(pathFinished(PathObject*)), this, SLOT(pathFinished(PathObject*)));
}
