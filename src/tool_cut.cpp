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

#include "map_widget.h"
#include "util.h"
#include "symbol.h"
#include "map_undo.h"

QCursor* CutTool::cursor = NULL;

CutTool::CutTool(MapEditorController* editor, QAction* tool_button) : MapEditorTool(editor, Other, tool_button), renderables(editor->getMap())
{
	dragging = false;
	hover_object = NULL;
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-cut.png"), 11, 11);
}
void CutTool::init()
{
	connect(editor->getMap(), SIGNAL(selectedObjectsChanged()), this, SLOT(selectedObjectsChanged()));
	updateDirtyRect();
    updateStatusText();
}
CutTool::~CutTool()
{
}

bool CutTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (!(event->buttons() & Qt::LeftButton))
		return false;
	
	updateHoverPoint(widget->mapToViewport(map_coord), widget);
	
	dragging = false;
	click_pos = event->pos();
	click_pos_map = map_coord;
	cur_pos = event->pos();
	cur_pos_map = map_coord;
	return true;
}
bool CutTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	updateHoverPoint(widget->mapToViewport(map_coord), widget);
	
	bool mouse_down = event->buttons() & Qt::LeftButton;
	if (mouse_down)
	{
		if (!dragging && (event->pos() - click_pos).manhattanLength() >= QApplication::startDragDistance())
		{
			// Start dragging
			dragging = true;
			
			// TODO
		}
		else if (dragging)
			updateDragging(map_coord);
		return true;
	}
	return false;
}
bool CutTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	Map* map = editor->getMap();
	
	if (!dragging)
	{
		bool do_split = false;
		PathCoord split_pos;
		PathObject* split_object;
		
		if (hover_point >= 0 && hover_object->getSymbol()->getContainedTypes() & Symbol::Line)
		{
			// Hovering over a point of a line - cut it into two
			if (hover_object->getType() != Object::Path)
			{
				assert(!"TODO: make this work for non-path objects");
			}
			PathObject* path = reinterpret_cast<PathObject*>(hover_object);
			split_pos = PathCoord::findPathCoordForCoorinate(&path->getPathCoordinateVector(), hover_point);
			split_object = path;
			do_split = true;
		}
		else
		{
			// Check if a line segment was clicked
			float smallest_distance_sq = 999999;
			Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
			for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
			{
				if (!((*it)->getSymbol()->getContainedTypes() & Symbol::Line))
					continue;
				if ((*it)->getType() != Object::Path)
				{
					assert(!"TODO: make this work for non-path objects");
				}
				
				PathObject* path = reinterpret_cast<PathObject*>(*it);
				float distance_sq;
				PathCoord path_coord;
				path->calcClosestPointOnPath(map_coord, distance_sq, path_coord);
				
				float click_tolerance_map = widget->getMapView()->pixelToLength(click_tolerance);
				if (distance_sq < smallest_distance_sq && distance_sq <= click_tolerance_map*click_tolerance_map)
				{
					smallest_distance_sq = distance_sq;
					split_pos = path_coord;
					split_object = path;
					do_split = true;
				}
			}
		}
		
		if (do_split)
		{
			MapLayer* layer = map->getCurrentLayer();
			AddObjectsUndoStep* add_step = new AddObjectsUndoStep(map);
			DeleteObjectsUndoStep* delete_step = new DeleteObjectsUndoStep(map);
			
			Object* out1 = NULL;
			Object* out2 = NULL;
			split_object->splitAt(split_pos, out1, out2);
			
			add_step->addObject(layer->findObjectIndex(split_object), split_object);
			map->removeObjectFromSelection(split_object, false);
			map->deleteObject(split_object, true);
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
		}
	}
	
	dragging = false;
	return true;
}

void CutTool::draw(QPainter* painter, MapWidget* widget)
{
	Map* map = editor->getMap();
	map->drawSelection(painter, true, widget, renderables.isEmpty() ? NULL : &renderables);
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
		drawPointHandles((hover_object == *it) ? hover_point : -2, painter, *it, widget);
}
void CutTool::updateDirtyRect()
{
	Map* map = editor->getMap();
	QRectF rect;
	//editor->getMap()->includeSelectionRect(rect);
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
		includeControlPointRect(rect, *it, NULL);
	
	if (rect.isValid())
		editor->getMap()->setDrawingBoundingBox(rect, 6, true);
	else
		editor->getMap()->clearDrawingBoundingBox();
}

void CutTool::updateDragging(MapCoordF cursor_pos_map)
{
	// TODO
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
void CutTool::updatePreviewObjects()
{
	// TODO
	
	updateDirtyRect();
}
void CutTool::selectedObjectsChanged()
{
	updatePreviewObjects();
}
void CutTool::updateStatusText()
{
	setStatusBarText(tr("<b>Click</b> to split a line into two, <b>Drag</b> starting from an area boundary to draw a area split line"));
}

#include "tool_cut.moc"
