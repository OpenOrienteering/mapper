/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2017 Kai Pastor
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


#include "cut_tool.h"

#include <cmath>
#include <limits>
#include <memory>

#include <Qt>
#include <QtGlobal>
#include <QCursor>
#include <QEvent>
#include <QFlags>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QString>

#include "core/map.h"
#include "core/map_part.h"
#include "core/map_view.h"
#include "core/virtual_path.h"
#include "core/objects/boolean_tool.h"
#include "core/renderables/renderable.h"
#include "core/symbols/combined_symbol.h"
#include "core/symbols/symbol.h"
#include "gui/map/map_widget.h"
#include "tools/draw_line_and_area_tool.h"
#include "tools/draw_path_tool.h"
#include "tools/point_handles.h"
#include "tools/tool.h"
#include "undo/object_undo.h"
#include "undo/undo.h"
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
	
	
}  // namespace OpenOrienteering



CutTool::CutTool(MapEditorController* editor, QAction* tool_action)
 : MapEditorToolBase { scaledToScreen(QCursor{ QString::fromLatin1(":/images/cursor-cut.png"), 11, 11 }), Other, editor, tool_action }
 , renderables { new MapRenderables(map()) }
{
	// nothing
}


CutTool::~CutTool()
{
	deletePreviewObject();
	delete path_tool;
}



void CutTool::initImpl()
{
	updateDirtyRect();
	updateStatusText();
}



// This function contains translations. Keep it close to the top of the file so
// that line numbers remain stable here when changing other parts of the file.
void CutTool::updateStatusText()
{
	QString text;
	if (!editingInProgress())
		text = tr("<b>Click</b> on a line: Split it into two. <b>Drag</b> along a line: Remove this line part. <b>Click or Drag</b> at an area boundary: Start a split line. ");
	
	setStatusBarText(text);
}



void CutTool::objectSelectionChangedImpl()
{
	for (const auto* object : map()->selectedObjects())
	{
		if (object->getSymbol()->getContainedTypes() & (Symbol::Line | Symbol::Area))
		{
			updateDirtyRect();
			return;
		}
	}
	
	deactivate();
}



bool CutTool::mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	if (path_tool)
		return path_tool->mousePressEvent(event, map_coord, widget);
	
	if (waiting_for_mouse_release)
	{
		if (event->buttons() & ~event->button())
			return true;
		waiting_for_mouse_release = false;
	}
	
	return MapEditorToolBase::mousePressEvent(event, map_coord, widget);
}


bool CutTool::mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	if (path_tool)
		return path_tool->mouseMoveEvent(event, map_coord, widget);
	
	if (waiting_for_mouse_release)
	{
		if (event->buttons())
			return true;
		waiting_for_mouse_release = false;
	}
	
	return MapEditorToolBase::mouseMoveEvent(event, map_coord, widget);
}


bool CutTool::mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	if (path_tool)
		return path_tool->mouseReleaseEvent(event, map_coord, widget);
	
	if (waiting_for_mouse_release)
	{
		waiting_for_mouse_release = !event->buttons();
		return true;
	}
	
	return MapEditorToolBase::mouseReleaseEvent(event, map_coord, widget);
}


bool CutTool::mouseDoubleClickEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	if (path_tool)
		return path_tool->mouseDoubleClickEvent(event, map_coord, widget);
	
	return false;
}


void CutTool::leaveEvent(QEvent* event)
{
	if (path_tool)
		path_tool->leaveEvent(event);
}


void CutTool::focusOutEvent(QFocusEvent* event)
{
	if (path_tool)
		path_tool->focusOutEvent(event);
}



void CutTool::mouseMove()
{
	updateHoverState(cur_pos_map);
}


void CutTool::clickPress()
{
	if (auto area_point = findEditPoint(click_pos_map, Symbol::Area, Symbol::NoSymbol))
	{
		// Click on area
		if (!startCuttingArea(area_point))
		{
			// Not on area outline
			waiting_for_mouse_release = true;
		}
	}
}


void CutTool::clickRelease()
{
	if (auto line_point = findEditPoint(click_pos_map, Symbol::Line, Symbol::NoSymbol))
	{
		// Line split in single point
		auto split_objects = line_point.object->splitLineAt(line_point);
		if (!split_objects.empty())
			replaceObject(line_point.object, split_objects);
	}
}


void CutTool::dragStart()
{
	if (auto line_point = findEditPoint(click_pos_map, Symbol::Line, Symbol::Area))
	{
		Q_ASSERT(line_point.object->getType() == Object::Path);
		
		// Line split at ends of drawn segment
		startCuttingLine(line_point);
	}
}


void CutTool::dragMove()
{
	if (editingInProgress())
		updateCuttingLine(cur_pos_map);
	else
		mouseMove();
}


void CutTool::dragFinish()
{
	if (editingInProgress())
	{
		finishCuttingLine();
	}
}



bool CutTool::keyPress(QKeyEvent* event)
{
	if (path_tool)
		return path_tool->keyPressEvent(event);
	
	return false;
}


bool CutTool::keyRelease(QKeyEvent* event)
{
	if (path_tool)
		return path_tool->keyReleaseEvent(event);
	
	return false;
}



void CutTool::startCuttingLine(const ObjectPathCoord& point)
{
	startEditing();
	
	edit_object     = point.object;
	drag_part_index = point.object->findPartIndexForIndex(point.index);
	drag_start_len  = point.clen;
	drag_end_len    = point.clen;
	reverse_drag    = false;
}


void CutTool::updateCuttingLine(const MapCoordF& cursor_pos)
{
	Q_ASSERT(editingInProgress());
	
	updateHoverState(cursor_pos);
	PathCoord path_coord;
	if (hover_state != EditTool::OverObjectNode)
	{
		float distance_sq;
		edit_object->calcClosestPointOnPath(cursor_pos, distance_sq, path_coord);
	}
	else
	{
		path_coord = edit_object->findPathCoordForIndex(hover_point);
	}
	
	if (edit_object->findPartIndexForIndex(path_coord.index) == drag_part_index)
	{
		auto new_drag_end_len = path_coord.clen;
		const PathPart& drag_part = edit_object->parts()[drag_part_index];
		auto path_length = double(drag_part.path_coords.back().clen);
		bool delta_forward; 
		if (drag_part.isClosed())
		{
			auto value = std::fmod(double(new_drag_end_len) - double(drag_end_len) + path_length, path_length);
			delta_forward = value >= 0 && value < 0.5 * path_length;
		}
		else
		{
			delta_forward = new_drag_end_len >= drag_end_len;
		}
		
		if (delta_forward && reverse_drag &&
		    std::fmod(double(drag_end_len) - double(drag_start_len) + path_length, path_length) > 0.5 * path_length &&
		    std::fmod(double(new_drag_end_len) - double(drag_start_len) + path_length, path_length) <= 0.5 * path_length)
		{
			reverse_drag = false;
		}
		else if (!delta_forward && !reverse_drag &&
		         std::fmod(double(drag_end_len) - double(drag_start_len) + path_length, path_length) <= 0.5 * path_length &&
		         std::fmod(double(new_drag_end_len) - double(drag_start_len) + path_length, path_length) > 0.5 * path_length)
		{
			reverse_drag = true;
		}
		
		drag_end_len = new_drag_end_len;
		updatePreviewObjects();
	}
}


void CutTool::finishCuttingLine()
{
	Q_ASSERT(editingInProgress());
	
	if (reverse_drag)
		qSwap(drag_start_len, drag_end_len);
	
	auto split_objects = edit_object->removeFromLine(drag_part_index, qreal(drag_start_len), qreal(drag_end_len));
	replaceObject(edit_object, split_objects);
	
	finishEditing();
	updatePreviewObjects();
}



bool CutTool::startCuttingArea(const ObjectPathCoord& point)
{
	drag_part_index = point.object->findPartIndexForIndex(point.index);
	if (drag_part_index > 0)
	{
		QMessageBox::warning(window(), tr("Error"), tr("Splitting holes of area objects is not supported yet!"));
		return false;
	}
	
	startEditing();
	edit_object = point.object;
	drag_start_len = point.clen;
	path_tool = new DrawPathTool(editor, nullptr, true, false);
	connect(path_tool, &DrawLineAndAreaTool::dirtyRectChanged, this, [this](const QRectF& rect){
		path_tool_rect = rect;
		updateDirtyRect();
	});
	connect(path_tool, &DrawLineAndAreaTool::pathAborted, this, &CutTool::abortCuttingArea);
	connect(path_tool, &DrawLineAndAreaTool::pathFinished, this, &CutTool::finishCuttingArea);
	path_tool->init();
	
	QMouseEvent event { QEvent::MouseButtonPress, cur_map_widget->mapToViewport(point.pos), Qt::LeftButton, QGuiApplication::mouseButtons(), active_modifiers };
	path_tool->mousePressEvent(&event, point.pos, cur_map_widget);
	
	return true;
}


void CutTool::abortCuttingArea()
{
	path_tool->deleteLater();
	path_tool = nullptr;
	abortEditing();
	updateDirtyRect();
	waiting_for_mouse_release = true;
}


void CutTool::finishCuttingArea(PathObject* split_path)
{
	Map* map = this->map();
	
	// Get path endpoint and check if it is on the area boundary
	const MapCoordVector& path_coords = split_path->getRawCoordinateVector();
	MapCoord path_end = path_coords.at(path_coords.size() - 1);
	
	PathCoord end_path_coord;
	float distance_sq;
	edit_object->calcClosestPointOnPath(MapCoordF(path_end), distance_sq, end_path_coord);
	
	auto click_tolerance_map = 0.001 * cur_map_widget->getMapView()->pixelToLength(clickTolerance());
	if (double(distance_sq) > click_tolerance_map*click_tolerance_map)
	{
		QMessageBox::warning(window(), tr("Error"), tr("The split line must end on the area boundary!"));
		abortCuttingArea();
		return;
	}
	else if (drag_part_index != edit_object->findPartIndexForIndex(end_path_coord.index))
	{
		QMessageBox::warning(window(), tr("Error"), tr("Start and end of the split line are at different parts of the object!"));
		abortCuttingArea();
		return;
	}
	else if (qAbs(drag_start_len - end_path_coord.clen) < 0.001f)
	{
		QMessageBox::warning(window(), tr("Error"), tr("Start and end of the split line are at the same position!"));
		abortCuttingArea();
		return;
	}
	
	Q_ASSERT(split_path->parts().size() == 1);
	split_path->parts().front().setClosed(false);
	split_path->setCoordinate(split_path->getCoordinateCount() - 1, MapCoord(end_path_coord.pos));
	
	// Do the splitting
	const double split_threshold = 0.01;
	
	PathObject* holes = nullptr; // if the edited path contains holes, they are saved in this temporary object
	if (edit_object->parts().size() > 1)
	{
		holes = edit_object->duplicate()->asPath();
		holes->deletePart(0);
	}
	
	bool ok; Q_UNUSED(ok) // "ok" is only used in Q_ASSERT.
	std::vector<PathObject*> out_paths = { new PathObject { edit_object->parts().front() }, nullptr };
	const PathPart& drag_part = edit_object->parts()[drag_part_index];
	if (drag_part.isClosed())
	{
		out_paths[1] = out_paths[0]->duplicate();
		
		out_paths[0]->changePathBounds(drag_part_index, drag_start_len, end_path_coord.clen);
		ok = out_paths[0]->connectIfClose(split_path, split_threshold);
		Q_ASSERT(ok);

		out_paths[1]->changePathBounds(drag_part_index, end_path_coord.clen, drag_start_len);
		ok = out_paths[1]->connectIfClose(split_path, split_threshold);
		Q_ASSERT(ok);
	}
	else
	{
		auto min_cut_pos = qMin(drag_start_len, end_path_coord.clen);
		auto max_cut_pos = qMax(drag_start_len, end_path_coord.clen);
		auto path_len = drag_part.path_coords.back().clen;
		if (min_cut_pos <= 0 && max_cut_pos >= path_len)
		{
			ok = out_paths[0]->connectIfClose(split_path, split_threshold);
			Q_ASSERT(ok);
			
			out_paths[1] = split_path->duplicate();
			out_paths[1]->setSymbol(edit_object->getSymbol(), false);
		}
		else if (min_cut_pos <= 0 || max_cut_pos >= path_len)
		{
			float cut_pos = (min_cut_pos <= 0) ? max_cut_pos : min_cut_pos;
			out_paths[1] = out_paths[0]->duplicate();
			
			out_paths[0]->changePathBounds(drag_part_index, 0, cut_pos);
			ok = out_paths[0]->connectIfClose(split_path, split_threshold);
			Q_ASSERT(ok);
			
			out_paths[1]->changePathBounds(drag_part_index, cut_pos, path_len);
			ok = out_paths[1]->connectIfClose(split_path, split_threshold);
			Q_ASSERT(ok);
		}
		else
		{
			out_paths[1] = out_paths[0]->duplicate();
			PathObject* temp_path = out_paths[0]->duplicate();
			
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
	
	for (auto& object : out_paths)
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
	}
	replaceObject(edit_object, out_paths);	
	
	delete holes;
	
	path_tool->deleteLater();
	path_tool = nullptr;
	finishEditing();
	updateDirtyRect();
	waiting_for_mouse_release = true;
}



void CutTool::updatePreviewObjects()
{
	Q_ASSERT(!path_tool);
	if (editingInProgress() && qAbs(drag_end_len - drag_start_len) >= 0.001f)
	{
		if (preview_path)
		{
			renderables->removeRenderablesOfObject(preview_path, false);
		}
		else
		{
			preview_path = new PathObject { Map::getCoveringCombinedLine() };
		}
		
		preview_path->clearCoordinates();
		preview_path->appendPathPart(edit_object->parts()[drag_part_index]);
		if (reverse_drag)
			preview_path->changePathBounds(0, drag_end_len, drag_start_len);
		else
			preview_path->changePathBounds(0, drag_start_len, drag_end_len);
		
		preview_path->update();
		renderables->insertRenderablesOfObject(preview_path);
	}
	else
	{
		deletePreviewObject();
	}
		
	updateDirtyRect();
}


void CutTool::deletePreviewObject()
{
	if (preview_path)
	{
		renderables->removeRenderablesOfObject(preview_path, false);
		delete preview_path;
		preview_path = nullptr;
	}
}


int CutTool::updateDirtyRectImpl(QRectF& rect)
{
	auto map = this->map();
	map->includeSelectionRect(rect);
	
	if (editingInProgress())
	{
		edit_object->includeControlPointsRect(rect);
	}
	else if (map->selectedObjects().size() <= max_objects_for_handle_display)
	{
		for (const auto* object : map->selectedObjects())
			object->includeControlPointsRect(rect);
	}
	
	if (path_tool && path_tool_rect.isValid())
		rectIncludeSafe(rect, path_tool_rect);
	
	return rect.isValid() ? 6 : -1;
}


void CutTool::drawImpl(QPainter* painter, MapWidget* widget)
{
	auto map = this->map();
	map->drawSelection(painter, true, widget, nullptr);
	
	if (editingInProgress())
	{
		pointHandles().draw(painter, widget, edit_object, hover_point, false);
	}
	else if (map->selectedObjects().size() <= max_objects_for_handle_display)
	{
		for (const auto* object: map->selectedObjects())
		{
			auto hover_point = MapCoordVector::size_type { hover_object == object ? this->hover_point : no_point };
			pointHandles().draw(painter, widget, object, hover_point, false);
		}
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



void CutTool::updateHoverState(const MapCoordF& cursor_pos)
{
	auto new_hover_state = HoverState { HoverFlag::OverNothing };
	PathObject* new_hover_object = nullptr;
	auto new_hover_point = MapCoordVector::size_type { no_point };
	
	if (editingInProgress())
	{
		Q_ASSERT(edit_object);
		new_hover_object = edit_object;
		new_hover_point = findHoverPoint(cur_map_widget->mapToViewport(cursor_pos), cur_map_widget, edit_object, false);
		if (new_hover_point != no_point)
		{
			new_hover_state = HoverFlag::OverObjectNode;
		}
	}
	else if (map()->selectedObjects().size() <= max_objects_for_handle_display)
	{
		auto best_distance_sq = std::numeric_limits<double>::max();
		for (auto* object : map()->selectedObjects())
		{
			if (object->getType() != Object::Path)
				continue;
			
			MapCoordF handle_pos;
			auto hover_point = findHoverPoint(cur_map_widget->mapToViewport(cursor_pos), cur_map_widget, object, false, &handle_pos);
			if (hover_point == no_point)
				continue;
			
			auto distance_sq = cursor_pos.distanceSquaredTo(handle_pos);
			if (distance_sq < best_distance_sq)
			{
				new_hover_state  = HoverFlag::OverObjectNode;
				new_hover_object = object->asPath();
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
		hover_object = new_hover_object;
		hover_point  = new_hover_point;
		updateDirtyRect();
	}
}


ObjectPathCoord CutTool::findEditPoint(const MapCoordF& cursor_pos_map, int with_type, int without_type) const
{
	ObjectPathCoord result;
	if (hover_state == HoverFlag::OverObjectNode
	    && hover_object->getSymbol()->getContainedTypes() & with_type
	    && !(hover_object->getSymbol()->getContainedTypes() & without_type))
	{
		result = { hover_object, hover_point };
	}
	else
	{
		// Check if a line segment was clicked
		const auto click_tolerance_map = 0.001 * cur_map_widget->getMapView()->pixelToLength(clickTolerance());
		auto min_distance_sq = float(qMin(999999.9, click_tolerance_map * click_tolerance_map));
		for (auto* object : map()->selectedObjects())
		{
			if (object->getType() != Object::Path
			    || !(object->getSymbol()->getContainedTypes() & with_type)
			    || object->getSymbol()->getContainedTypes() & without_type)
			{
				continue;
			}
			
			auto object_path_coord = ObjectPathCoord { object->asPath() };
			auto distance_sq = object_path_coord.findClosestPointTo(cursor_pos_map);
			if (distance_sq < min_distance_sq)
			{
				min_distance_sq = distance_sq;
				result = object_path_coord;
			}
		}
	}
	return result;
}



void CutTool::replaceObject(Object* object, const std::vector<PathObject*>& replacement) const
{
	auto map = this->map();
	auto map_part = map->getCurrentPart();
	
	auto add_step = new AddObjectsUndoStep(map);
	add_step->addObject(map_part->findObjectIndex(object), object);
	map->removeObjectFromSelection(object, false);
	map->deleteObject(object, true);
	
	auto delete_step = new DeleteObjectsUndoStep(map);
	for (auto new_object : replacement)
	{
		map->addObject(new_object);
		map->addObjectToSelection(new_object, false);
		delete_step->addObject(map_part->findObjectIndex(new_object));
	}
	
	auto undo_step = new CombinedUndoStep(map);
	undo_step->push(add_step);
	undo_step->push(delete_step);
	map->push(undo_step);
	
	map->emitSelectionChanged();
}


}  // namespace OpenOrienteering
