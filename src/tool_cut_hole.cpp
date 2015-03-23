/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2015 Kai Pastor
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


#include "tool_cut_hole.h"

#include <QApplication>
#include <QMouseEvent>
#include <QMessageBox>

#include "map.h"
#include "object_undo.h"
#include "map_widget.h"
#include "object.h"
#include "symbol.h"
#include "symbol_combined.h"
#include "tool_boolean.h"
#include "tool_draw_circle.h"
#include "tool_draw_path.h"
#include "tool_draw_rectangle.h"
#include "util.h"
#include "gui/modifier_key.h"

QCursor* CutHoleTool::cursor = NULL;

CutHoleTool::CutHoleTool(MapEditorController* editor, QAction* tool_button, CutHoleTool::HoleType hole_type)
 : MapEditorTool(editor, Other, tool_button), hole_type(hole_type)
{
	path_tool = NULL;
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-cut.png"), 11, 11);
}

void CutHoleTool::init()
{
	connect(map(), SIGNAL(objectSelectionChanged()), this, SLOT(objectSelectionChanged()));
	updateDirtyRect();
	updateStatusText();
	
	MapEditorTool::init();
}

CutHoleTool::~CutHoleTool()
{
	delete path_tool;
}

bool CutHoleTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (path_tool)
		return path_tool->mousePressEvent(event, map_coord, widget);
	
	if (!(event->buttons() & Qt::LeftButton))
		return false;
	
	// Start a new hole
	edit_widget = widget;
	
	switch (hole_type)
	{
	case CutHoleTool::Path:
		path_tool = new DrawPathTool(editor, NULL, true, true);
		break;
	case CutHoleTool::Circle:
		path_tool = new DrawCircleTool(editor, NULL, true);
		break;
	case CutHoleTool::Rect:
		path_tool = new DrawRectangleTool(editor, NULL, true);
		break;
	/* no default; watch compiler warnings for unhandled cases! */
	}
	
	connect(path_tool, SIGNAL(dirtyRectChanged(QRectF)), this, SLOT(pathDirtyRectChanged(QRectF)));
	connect(path_tool, SIGNAL(pathAborted()), this, SLOT(pathAborted()));
	connect(path_tool, SIGNAL(pathFinished(PathObject*)), this, SLOT(pathFinished(PathObject*)));
	
	path_tool->init();
	path_tool->mousePressEvent(event, map_coord, widget);
	
	return true;
}

bool CutHoleTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (path_tool)
		return path_tool->mouseMoveEvent(event, map_coord, widget);
	
	return false;
}

bool CutHoleTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (path_tool)
		return path_tool->mouseReleaseEvent(event, map_coord, widget);
	
	return false;
}

bool CutHoleTool::mouseDoubleClickEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (path_tool)
		return path_tool->mouseDoubleClickEvent(event, map_coord, widget);
	return false;
}

void CutHoleTool::leaveEvent(QEvent* event)
{
	if (path_tool)
		return path_tool->leaveEvent(event);
}

bool CutHoleTool::keyPressEvent(QKeyEvent* event)
{
	if (path_tool)
		return path_tool->keyPressEvent(event);
	return false;
}

bool CutHoleTool::keyReleaseEvent(QKeyEvent* event)
{
	if (path_tool)
		return path_tool->keyReleaseEvent(event);
	return false;
}

void CutHoleTool::focusOutEvent(QFocusEvent* event)
{
	if (path_tool)
		return path_tool->focusOutEvent(event);
}

void CutHoleTool::draw(QPainter* painter, MapWidget* widget)
{
	map()->drawSelection(painter, true, widget, NULL);
	
	if (path_tool)
		path_tool->draw(painter, widget);
}

void CutHoleTool::updateDirtyRect(const QRectF* path_rect)
{
	QRectF rect;
	if (path_rect)
		rect = *path_rect;
	map()->includeSelectionRect(rect);
	
	if (rect.isValid())
		map()->setDrawingBoundingBox(rect, 6, true);
	else
		map()->clearDrawingBoundingBox();
}

void CutHoleTool::objectSelectionChanged()
{
	Map* map = this->map();
	if (map->getNumSelectedObjects() != 1 || !((*map->selectedObjectsBegin())->getSymbol()->getContainedTypes() & Symbol::Area))
		deactivate();
	else
		updateDirtyRect();
}

void CutHoleTool::pathDirtyRectChanged(const QRectF& rect)
{
	updateDirtyRect(&rect);
}

void CutHoleTool::pathAborted()
{
	delete path_tool;
	path_tool = NULL;
	updateDirtyRect();
	updateStatusText();
}

void CutHoleTool::pathFinished(PathObject* hole_path)
{
	if (map()->getNumSelectedObjects() == 0)
	{
		pathAborted();
		return;
	}
	
	Object* edited_object = *map()->selectedObjectsBegin();
	Object* undo_duplicate = edited_object->duplicate();
	
	// Close the hole path
	Q_ASSERT(hole_path->parts().size() == 1);
	hole_path->parts().front().setClosed(true, true);
	
	BooleanTool::PathObjects in_objects, out_objects;
	in_objects.push_back(hole_path);
	
	PathObject* edited_path = reinterpret_cast<PathObject*>(edited_object);
	BooleanTool(BooleanTool::Difference, map()).executeForObjects(edited_path, in_objects, out_objects);
	
	edited_path->clearCoordinates();
	edited_path->appendPath(out_objects.front());
	edited_path->update();
	updateDirtyRect();
	
	while (!out_objects.empty())
	{
		delete out_objects.back();
		out_objects.pop_back();
	}
	
	ReplaceObjectsUndoStep* undo_step = new ReplaceObjectsUndoStep(map());
	undo_step->addObject(edited_object, undo_duplicate);
	map()->push(undo_step);
	map()->setObjectsDirty();
	map()->emitSelectionEdited();
	
	pathAborted();
}

void CutHoleTool::updateStatusText()
{
	if (!path_tool)
	{
		// FIXME: The path_tool would have better instruction, but is not initialized yet.
		setStatusBarText(tr("<b>Click or drag</b>: Start drawing the hole. "));
	}
}
