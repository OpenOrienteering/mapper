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


#include "tool_cut.h"

#include <QApplication>
#include <QMouseEvent>
#include <QMessageBox>
#include <QPainter>

#include "map_editor.h"
#include "map_widget.h"
#include "util.h"
#include "object.h"
#include "renderable.h"
#include "symbol.h"
#include "symbol_combined.h"
#include "map_undo.h"
#include "tool_draw_path.h"
#include "settings.h"

QCursor* CutTool::cursor = NULL;

CutTool::CutTool(MapEditorController* editor, QAction* tool_button) : MapEditorTool(editor, Other, tool_button), renderables(new MapRenderables(editor->getMap()))
{
	dragging = false;
	hover_object = NULL;
	hover_point = -2;
	preview_path = NULL;
	path_tool = NULL;
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-cut.png"), 11, 11);
}
void CutTool::init()
{
	connect(editor->getMap(), SIGNAL(objectSelectionChanged()), this, SLOT(objectSelectionChanged()));
	updateDirtyRect();
    updateStatusText();
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
	
	updateHoverPoint(widget->mapToViewport(map_coord), widget);
	
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
		updateHoverPoint(widget->mapToViewport(map_coord), widget);
	
	if (mouse_down)
	{
		if (!dragging && (event->pos() - click_pos).manhattanLength() >= QApplication::startDragDistance())
		{
			// Start dragging
			dragging = true;
			dragging_on_line = false;
			editor->setEditingInProgress(true);
			
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
			updateDragging(map_coord, widget);
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
	Map* map = editor->getMap();
	
	if (dragging)
	{
		updateDragging(map_coord, widget);
		
		if (dragging_on_line)
		{
			if (drag_start_len != drag_end_len)
			{
				MapLayer* layer = map->getCurrentLayer();
				PathObject* split_object = reinterpret_cast<PathObject*>(edit_object);
				
				if (split_object->getPart(drag_part_index).isClosed())
				{
					Object* undo_duplicate = split_object->duplicate();
					
					if (!drag_forward)
						split_object->changePathBounds(drag_part_index, drag_start_len, drag_end_len);
					else
						split_object->changePathBounds(drag_part_index, drag_end_len, drag_start_len);
					
					ReplaceObjectsUndoStep* replace_step = new ReplaceObjectsUndoStep(map);
					replace_step->addObject(layer->findObjectIndex(split_object), undo_duplicate);
					map->objectUndoManager().addNewUndoStep(replace_step);
					split_object->update(); // Make sure that the map display is updated
				}
				else
				{
					AddObjectsUndoStep* add_step = new AddObjectsUndoStep(map);
					add_step->addObject(layer->findObjectIndex(split_object), split_object);
					map->removeObjectFromSelection(split_object, false);
					map->deleteObject(split_object, true);
					
					float min_cut_pos = qMin(drag_start_len, drag_end_len);
					float max_cut_pos = qMax(drag_start_len, drag_end_len);
					float path_len = split_object->getPathCoordinateVector().at(split_object->getPathCoordinateVector().size() - 1).clen;
					if (min_cut_pos <= 0 && max_cut_pos >= path_len)
						map->objectUndoManager().addNewUndoStep(add_step);
					else
					{
						DeleteObjectsUndoStep* delete_step = new DeleteObjectsUndoStep(map);
						
						if (min_cut_pos > 0)
						{
							PathObject* part1 = reinterpret_cast<PathObject*>(split_object->duplicate());
							part1->changePathBounds(drag_part_index, 0, min_cut_pos);
							map->addObject(part1);
							delete_step->addObject(layer->findObjectIndex(part1));
							map->addObjectToSelection(part1, !(max_cut_pos < path_len));
						}
						if (max_cut_pos < path_len)
						{
							PathObject* part2 = reinterpret_cast<PathObject*>(split_object->duplicate());
							part2->changePathBounds(drag_part_index, max_cut_pos, path_len);
							map->addObject(part2);
							delete_step->addObject(layer->findObjectIndex(part2));
							map->addObjectToSelection(part2, true);
						}
						
						CombinedUndoStep* undo_step = new CombinedUndoStep((void*)map);
						undo_step->addSubStep(delete_step);
						undo_step->addSubStep(add_step);
						map->objectUndoManager().addNewUndoStep(undo_step);
					}
				}
				
				map->setObjectsDirty();
				map->emitSelectionEdited();
			}
			
			deletePreviewPath();
			updateDirtyRect();
		}
	}
	else
	{
		PathCoord split_pos;
		PathObject* split_object;
		
		if (findEditPoint(split_pos, edit_object, map_coord, (int)Symbol::Line, 0, widget))
		{
			if (edit_object->getType() != Object::Path)
				assert(!"TODO: make this work for non-path objects");
			split_object = reinterpret_cast<PathObject*>(edit_object);
			
			MapLayer* layer = map->getCurrentLayer();
			AddObjectsUndoStep* add_step = new AddObjectsUndoStep(map);
			
			Object* out1 = NULL;
			Object* out2 = NULL;
			split_object->splitAt(split_pos, out1, out2);
			
			add_step->addObject(layer->findObjectIndex(split_object), split_object);
			map->deleteObject(split_object, true);
			map->removeObjectFromSelection(split_object, false);
			if (!out1 && !out2)
			{
				map->objectUndoManager().addNewUndoStep(add_step);
				map->emitSelectionChanged();
				map->emitSelectionEdited();
				return true;
			}
			
			if (out1)
			{
				map->addObject(out1);
				map->addObjectToSelection(out1, !out2);
			}
			if (out2)
			{
				map->addObject(out2);
				map->addObjectToSelection(out2, true);
			}
			DeleteObjectsUndoStep* delete_step = new DeleteObjectsUndoStep(map);
			if (out1)
				delete_step->addObject(layer->findObjectIndex(out1));
			if (out2)
				delete_step->addObject(layer->findObjectIndex(out2));
			
			CombinedUndoStep* undo_step = new CombinedUndoStep((void*)map);
			if (out1 || out2)
				undo_step->addSubStep(delete_step);
			else
				delete delete_step;
			if (add_step)
				undo_step->addSubStep(add_step);
			map->objectUndoManager().addNewUndoStep(undo_step);
			
			updateDirtyRect();
			map->emitSelectionEdited();
		}
	}
	
	dragging = false;
	editor->setEditingInProgress(false);
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
	Map* map = editor->getMap();
	map->drawSelection(painter, true, widget, NULL);
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
		drawPointHandles((hover_object == *it) ? hover_point : -2, painter, *it, widget);
	
	if (preview_path)
	{
		painter->save();
		painter->translate(widget->width() / 2.0 + widget->getMapView()->getDragOffset().x(),
						   widget->height() / 2.0 + widget->getMapView()->getDragOffset().y());
		widget->getMapView()->applyTransform(painter);
		
		renderables->draw(painter, widget->getMapView()->calculateViewedRect(widget->viewportToView(widget->rect())), true, widget->getMapView()->calculateFinalZoomFactor(), true, 0.5f);
		
		painter->restore();
	}
	
	if (path_tool)
		path_tool->draw(painter, widget);
}
void CutTool::updateDirtyRect(const QRectF* path_rect)
{
	Map* map = editor->getMap();
	QRectF rect;
	if (path_rect)
		rect = *path_rect;
	editor->getMap()->includeSelectionRect(rect);
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
		includeControlPointRect(rect, *it, NULL);
	
	if (rect.isValid())
		editor->getMap()->setDrawingBoundingBox(rect, 6, true);
	else
		editor->getMap()->clearDrawingBoundingBox();
}

void CutTool::updateDragging(MapCoordF cursor_pos_map, MapWidget* widget)
{
	if (dragging_on_line)
	{
		PathObject* path = reinterpret_cast<PathObject*>(edit_object);
		PathCoord path_coord;
		
		if (hover_point < 0)
		{
			float distance_sq;
			path->calcClosestPointOnPath(cursor_pos_map, distance_sq, path_coord);
		}
		else
			path_coord = PathCoord::findPathCoordForCoorinate(&path->getPathCoordinateVector(), hover_point);
		
		if (edit_object->findPartIndexForIndex(path_coord.index) != drag_part_index)
			return;	// dragging on a different part
		
		//float click_tolerance_map = 0.001 * widget->getMapView()->pixelToLength(click_tolerance);
		//if (distance_sq <= click_tolerance_map*click_tolerance_map)
		{
			float new_drag_end_len = path_coord.clen;
			float path_length = path->getPathCoordinateVector().at(path->getPart(drag_part_index).path_coord_end_index).clen;
			bool delta_forward; 
			if (path->getPart(drag_part_index).isClosed())
			{
				delta_forward = fmod(new_drag_end_len - drag_end_len + path_length, path_length) >= 0 &&
								 fmod(new_drag_end_len - drag_end_len + path_length, path_length) < 0.5f * path_length;
			}
			else
				delta_forward = new_drag_end_len >= drag_end_len;
			
			if (delta_forward && !drag_forward &&
				fmod(drag_end_len - drag_start_len + path_length, path_length) > 0.5f * path_length &&
				fmod(new_drag_end_len - drag_start_len + path_length, path_length) <= 0.5f * path_length)
				drag_forward = true;
			else if (!delta_forward && drag_forward &&
				fmod(drag_end_len - drag_start_len + path_length, path_length) <= 0.5f * path_length &&
				fmod(new_drag_end_len - drag_start_len + path_length, path_length) > 0.5f * path_length)
				drag_forward = false;
			drag_end_len = new_drag_end_len;
			
			if (drag_end_len != drag_start_len)
				updatePreviewObjects();
		}
	}
}
void CutTool::updateHoverPoint(QPointF cursor_pos_screen, MapWidget* widget)
{
	Map* map = editor->getMap();
	bool has_hover_point = false;
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		int new_hover_point = findHoverPoint(cursor_pos_screen, *it, false, NULL, NULL, widget);
		if (new_hover_point > -2)
		{
			has_hover_point = true;
			if (new_hover_point != hover_point || *it != hover_object)
			{
				updateDirtyRect();
				hover_point = new_hover_point;
				hover_object = *it;
			}
		}
	}
	if (!has_hover_point && hover_point > -2)
	{
		updateDirtyRect();
		hover_point = -2;
		hover_object = NULL;
	}
}
bool CutTool::findEditPoint(PathCoord& out_edit_point, PathObject*& out_edit_object, MapCoordF cursor_pos_map, int with_type, int without_type, MapWidget* widget)
{
	int click_tolerance = Settings::getInstance().getSettingCached(Settings::MapEditor_ClickTolerance).toInt();
	Map* map = editor->getMap();
	
	out_edit_object = NULL;
	if (hover_point >= 0 && hover_object->getSymbol()->getContainedTypes() & with_type && !(hover_object->getSymbol()->getContainedTypes() & without_type))
	{
		// Hovering over a point of a line
		if (hover_object->getType() != Object::Path)
		{
			assert(!"TODO: make this work for non-path objects");
		}
		PathObject* path = reinterpret_cast<PathObject*>(hover_object);
		out_edit_point = PathCoord::findPathCoordForCoorinate(&path->getPathCoordinateVector(), hover_point);
		out_edit_object = path;
	}
	else
	{
		// Check if a line segment was clicked
		float smallest_distance_sq = 999999;
		Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
		for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
		{
			if (!((*it)->getSymbol()->getContainedTypes() & with_type && !((*it)->getSymbol()->getContainedTypes() & without_type)))
				continue;
			if ((*it)->getType() != Object::Path)
			{
				assert(!"TODO: make this work for non-path objects");
			}
			
			PathObject* path = reinterpret_cast<PathObject*>(*it);
			float distance_sq;
			PathCoord path_coord;
			path->calcClosestPointOnPath(cursor_pos_map, distance_sq, path_coord);
			
			float click_tolerance_map = 0.001f * widget->getMapView()->pixelToLength(click_tolerance);
			if (distance_sq < smallest_distance_sq && distance_sq <= click_tolerance_map*click_tolerance_map)
			{
				smallest_distance_sq = distance_sq;
				out_edit_point = path_coord;
				out_edit_object = path;
			}
		}
	}
	return out_edit_object != NULL;
}
void CutTool::updatePreviewObjects()
{
	deletePreviewPath();
	
	preview_path = reinterpret_cast<PathObject*>(edit_object->duplicate());
	preview_path->setSymbol(Map::getCoveringCombinedLine(), false);
	for (int i = preview_path->getNumParts() - 1; i > drag_part_index; --i)
		preview_path->deletePart(i);
	for (int i = drag_part_index - 1; i >= 0; --i)
		preview_path->deletePart(i);
	if (drag_forward)
		preview_path->changePathBounds(0, drag_start_len, drag_end_len);
	else
		preview_path->changePathBounds(0, drag_end_len, drag_start_len);
	
	preview_path->update(true);
	renderables->insertRenderablesOfObject(preview_path);
	
	updateDirtyRect();
}
void CutTool::deletePreviewPath()
{
	if (preview_path)
	{
		renderables->removeRenderablesOfObject(preview_path, false);
		delete preview_path;
		preview_path = NULL;
	}
}
void CutTool::objectSelectionChanged()
{
	Map* map = editor->getMap();
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
		editor->setEditTool();
	else
		updateDirtyRect();
}
void CutTool::pathDirtyRectChanged(const QRectF& rect)
{
	updateDirtyRect(&rect);
}
void CutTool::pathAborted()
{
	delete path_tool;
	path_tool = NULL;
	cutting_area = false;
	updateDirtyRect();
}
void CutTool::pathFinished(PathObject* split_path)
{
	int click_tolerance = Settings::getInstance().getSettingCached(Settings::MapEditor_ClickTolerance).toInt();
	Map* map = editor->getMap();
	
	// Get path endpoint and check if it is on the area boundary
	const MapCoordVector& path_coords = split_path->getRawCoordinateVector();
	MapCoord path_end = path_coords.at(path_coords.size() - 1);
	
	PathObject* edited_path = reinterpret_cast<PathObject*>(edit_object);
	PathCoord end_path_coord;
	float distance_sq;
	edited_path->calcClosestPointOnPath(MapCoordF(path_end), distance_sq, end_path_coord);
	
	float click_tolerance_map = 0.001 * edit_widget->getMapView()->pixelToLength(click_tolerance);
	if (distance_sq > click_tolerance_map*click_tolerance_map)
	{
		QMessageBox::warning(editor->getWindow(), tr("Error"), tr("The split line must end on the area boundary!"));
		pathAborted();
		return;
	}
	else if (drag_part_index != edited_path->findPartIndexForIndex(end_path_coord.index))
	{
		QMessageBox::warning(editor->getWindow(), tr("Error"), tr("Start and end of the split line are at different parts of the object!"));
		pathAborted();
		return;
	}
	else if (drag_start_len == end_path_coord.clen)
	{
		QMessageBox::warning(editor->getWindow(), tr("Error"), tr("Start and end of the split line are at the same position!"));
		pathAborted();
		return;
	}
	
	assert(split_path->getNumParts() == 1);
	split_path->getPart(0).setClosed(false);
	split_path->setCoordinate(split_path->getCoordinateCount() - 1, end_path_coord.pos.toMapCoord());
	
	// Do the splitting
	const double split_threshold = 0.01;
	
	MapLayer* layer = map->getCurrentLayer();
	AddObjectsUndoStep* add_step = new AddObjectsUndoStep(map);
	add_step->addObject(layer->findObjectIndex(edited_path), edited_path);
	map->removeObjectFromSelection(edited_path, false);
	map->deleteObject(edited_path, true);
	
	DeleteObjectsUndoStep* delete_step = new DeleteObjectsUndoStep(map);
	
	PathObject* holes = NULL; // if the edited path contains holes, these are saved in this temporary object
	if (edited_path->getNumParts() > 1)
	{
		holes = reinterpret_cast<PathObject*>(edited_path->duplicate());
		holes->deletePart(0);
	}
	
	PathObject* parts[2];
	if (edited_path->getPart(drag_part_index).isClosed())
	{
		parts[0] = edited_path->duplicateFirstPart();
		parts[0]->changePathBounds(drag_part_index, drag_start_len, end_path_coord.clen);
		assert(parts[0]->connectIfClose(split_path, split_threshold));

		parts[1] = edited_path->duplicateFirstPart();
		parts[1]->changePathBounds(drag_part_index, end_path_coord.clen, drag_start_len);
		assert(parts[1]->connectIfClose(split_path, split_threshold));
	}
	else
	{
		float min_cut_pos = qMin(drag_start_len, end_path_coord.clen);
		float max_cut_pos = qMax(drag_start_len, end_path_coord.clen);
		float path_len = edited_path->getPathCoordinateVector().at(edited_path->getPart(drag_part_index).path_coord_end_index).clen;
		if (min_cut_pos <= 0 && max_cut_pos >= path_len)
		{
			parts[0] = edited_path->duplicateFirstPart();
			assert(parts[0]->connectIfClose(split_path, split_threshold));
			
			parts[1] = reinterpret_cast<PathObject*>(split_path->duplicate());
			parts[1]->setSymbol(edited_path->getSymbol(), false);
		}
		else if (min_cut_pos <= 0 || max_cut_pos >= path_len)
		{
			float cut_pos = (min_cut_pos <= 0) ? max_cut_pos : min_cut_pos;
			
			parts[0] = edited_path->duplicateFirstPart();
			parts[0]->changePathBounds(drag_part_index, 0, cut_pos);
			assert(parts[0]->connectIfClose(split_path, split_threshold));
			
			parts[1] = edited_path->duplicateFirstPart();
			parts[1]->changePathBounds(drag_part_index, cut_pos, path_len);
			assert(parts[1]->connectIfClose(split_path, split_threshold));
		}
		else
		{
			parts[0] = edited_path->duplicateFirstPart();
			parts[0]->changePathBounds(drag_part_index, min_cut_pos, max_cut_pos);
			assert(parts[0]->connectIfClose(split_path, split_threshold));
			
			parts[1] = edited_path->duplicateFirstPart();
			parts[1]->changePathBounds(drag_part_index, 0, min_cut_pos);
			assert(parts[1]->connectIfClose(split_path, split_threshold));
			PathObject* temp_path = edited_path->duplicateFirstPart();
			temp_path->changePathBounds(drag_part_index, max_cut_pos, path_len);
			assert(parts[1]->connectIfClose(temp_path, split_threshold));
			delete temp_path;
		}
	}
	
	// If the object had holes, check into which parts they go
	if (holes)
	{
		int num_holes = holes->getNumParts();
		for (int i = 0; i < num_holes; ++i)
		{
			int part_index = (parts[0]->isPointOnPath(MapCoordF(holes->getCoordinate(holes->getPart(i).start_index)), 0) != Symbol::NoSymbol) ? 0 : 1;
			parts[part_index]->getCoordinate(parts[part_index]->getCoordinateCount() - 1).setHolePoint(true);
			parts[part_index]->appendPathPart(holes, i);
		}
	}
	
	for (int i = 0; i < 2; ++i)
	{
		map->addObject(parts[i]);
		delete_step->addObject(layer->findObjectIndex(parts[i]));
		map->addObjectToSelection(parts[i], false);
	}
	
	CombinedUndoStep* undo_step = new CombinedUndoStep((void*)map);
	undo_step->addSubStep(delete_step);
	undo_step->addSubStep(add_step);
	map->objectUndoManager().addNewUndoStep(undo_step);
	
	pathAborted();
}
void CutTool::updateStatusText()
{
	setStatusBarText(tr("<b>Click</b> on a line to split it into two, <b>Drag</b> along a line to remove this line part, <b>Click or Drag</b> at an area boundary to start drawing a split line"));
}

void CutTool::startCuttingArea(const PathCoord& coord, MapWidget* widget)
{
	drag_part_index = edit_object->findPartIndexForIndex(coord.index);
	if (drag_part_index > 0)
	{
		QMessageBox::warning(editor->getWindow(), tr("Error"), tr("Splitting holes of area objects is not supported yet!"));
		return;
	}
	cutting_area = true;
	drag_start_len = coord.clen;
	edit_widget = widget;
	
	path_tool = new DrawPathTool(editor, NULL, NULL, false);
	connect(path_tool, SIGNAL(dirtyRectChanged(QRectF)), this, SLOT(pathDirtyRectChanged(QRectF)));
	connect(path_tool, SIGNAL(pathAborted()), this, SLOT(pathAborted()));
	connect(path_tool, SIGNAL(pathFinished(PathObject*)), this, SLOT(pathFinished(PathObject*)));
}
