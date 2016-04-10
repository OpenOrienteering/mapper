/*
 *    Copyright 2013 Thomas Schöps
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


#include "tool_cutout.h"

#include <QKeyEvent>

#include "object.h"
#include "map.h"
#include "map_widget.h"
#include "settings.h"
#include "symbol_combined.h"
#include "tool_edit.h"
#include "util.h"
#include "object_undo.h"
#include "tool_boolean.h"
#include "map_editor.h"
#include "gui/modifier_key.h"

CutoutTool::CutoutTool(MapEditorController* editor, QAction* tool_button, bool cut_away)
 : MapEditorToolBase(QCursor(QPixmap(QString::fromLatin1(":/images/cursor-hollow.png")), 1, 1), Other, editor, tool_button),
   cut_away(cut_away),
   object_selector(new ObjectSelector(map()))
{

}

CutoutTool::~CutoutTool()
{

}

void CutoutTool::finishEditing()
{
	if (editingInProgress())
	{
		if (cutout_object_index >= 0)
			map()->getCurrentPart()->addObject(cutout_object, cutout_object_index);
		map()->clearObjectSelection(false);
		map()->addObjectToSelection(cutout_object, true);
		
		abortEditing();
	}
}

void CutoutTool::initImpl()
{
	// Take cutout shape object out of the map
	Q_ASSERT(map()->getNumSelectedObjects() == 1);
	cutout_object = (*(map()->selectedObjectsBegin()))->asPath();
	
	startEditing();
	cutout_object->setSymbol(Map::getCoveringCombinedLine(), true);
	updatePreviewObjects();
	
	cutout_object_index = map()->getCurrentPart()->findObjectIndex(cutout_object);
	map()->removeObjectFromSelection(cutout_object, true);
	map()->deleteObject(cutout_object, true);
}

void CutoutTool::drawImpl(QPainter* painter, MapWidget* widget)
{
	// Draw selection renderables
	map()->drawSelection(painter, true, widget, NULL, false);
	
	// Draw cutout shape renderables
	drawSelectionOrPreviewObjects(painter, widget, true);

	// Box selection
	if (isDragging())
		drawSelectionBox(painter, widget, click_pos_map, cur_pos_map);
}

bool CutoutTool::keyPress(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Return)
	{
		// Insert cutout object again at its original index to keep the objects order
		// (necessary for not breaking undo / redo)
		map()->getCurrentPart()->addObject(cutout_object, cutout_object_index);
		cutout_object_index = -1;
		
		// Apply tool via static function and deselect this tool
		apply(map(), cutout_object, cut_away);
		editor->setEditTool();
	}
	else if (event->key() == Qt::Key_Escape)
	{
		editor->setEditTool();
	}
	
	return false;
}

int CutoutTool::updateDirtyRectImpl(QRectF& rect)
{
	rectInclude(rect, cutout_object->getExtent());
	
	map()->includeSelectionRect(rect);
	
	// Box selection
	if (isDragging())
	{
		rectIncludeSafe(rect, click_pos_map);
		rectIncludeSafe(rect, cur_pos_map);
	}
	
	return 0;
}

void CutoutTool::updateStatusText()
{
	QString text;
	if (map()->getNumSelectedObjects() == 0)
	{
		text = tr("<b>%1</b>: Clip the whole map. ").arg(ModifierKey::return_key()) +
		       tr("<b>%1+Click or drag</b>: Select the objects to be clipped. ").arg(ModifierKey::shift());
	}
	else
	{
		text = tr("<b>%1+Click or drag</b>: Select the objects to be clipped. ").arg(ModifierKey::shift()) +
		       tr("<b>%1</b>: Clip the selected objects. ").arg(ModifierKey::return_key());
	}
	text += MapEditorTool::tr("<b>%1</b>: Abort. ").arg(ModifierKey::escape());
	setStatusBarText(text);	
}

void CutoutTool::objectSelectionChangedImpl()
{
	updateDirtyRect();
}

void CutoutTool::clickRelease()
{
	object_selector->selectAt(cur_pos_map, cur_map_widget->getMapView()->pixelToLength(clickTolerance()), active_modifiers & Qt::ShiftModifier);
	updateStatusText();
}

void CutoutTool::dragStart()
{
	
}

void CutoutTool::dragMove()
{
	updateDirtyRect();
}

void CutoutTool::dragFinish()
{
	object_selector->selectBox(click_pos_map, cur_pos_map, active_modifiers & Qt::ShiftModifier);
	updateStatusText();
}


struct PhysicalCutoutOperation
{
	inline PhysicalCutoutOperation(Map* map, PathObject* cutout_object, bool cut_away)
	: map(map), cutout_object(cutout_object), cut_away(cut_away)
	{
		add_step = new AddObjectsUndoStep(map);
		delete_step = new DeleteObjectsUndoStep(map);
	}
	
	inline bool operator()(Object* object, MapPart* part, int object_index)
	{
		Q_UNUSED(part);
		Q_UNUSED(object_index);
		
		// If there is a selection, only clip selected objects
		if (map->getNumSelectedObjects() > 0 && !map->isObjectSelected(object))
			return true;
		// Don't clip object itself
		if (object == cutout_object)
			return true;
		
		// Early out
		if (!object->getExtent().intersects(cutout_object->getExtent()))
		{
			if (!cut_away)
				add_step->addObject(object, object);
			return true;
		}
		
		if (object->getType() == Object::Point ||
			object->getType() == Object::Text)
		{
			// Simple check if the (first) point is inside the area
			if (cutout_object->isPointInsideArea(MapCoordF(object->getRawCoordinateVector().at(0))) == cut_away)
				add_step->addObject(object, object);
		}
		else if (object->getType() == Object::Path)
		{
			if (object->getSymbol()->getContainedTypes() & Symbol::Area)
			{
				// Use the Clipper library to clip the area
				BooleanTool boolean_tool(cut_away ? BooleanTool::Difference : BooleanTool::Intersection, map);
				BooleanTool::PathObjects in_objects;
				in_objects.push_back(cutout_object);
				in_objects.push_back(object->asPath());
				BooleanTool::PathObjects out_objects;
				if (!boolean_tool.executeForObjects(object->asPath(), in_objects, out_objects))
					return true;
				
				add_step->addObject(object, object);
				new_objects.insert(new_objects.end(), out_objects.begin(), out_objects.end());
			}
			else
			{
				// Use some custom code to clip the line
				BooleanTool boolean_tool(cut_away ? BooleanTool::Difference : BooleanTool::Intersection, map);
				BooleanTool::PathObjects out_objects;
				boolean_tool.executeForLine(cutout_object, object->asPath(), out_objects);
				
				add_step->addObject(object, object);
				new_objects.insert(new_objects.end(), out_objects.begin(), out_objects.end());
			}
		}
		
		return true;
	}
	
	UndoStep* finish()
	{
		MapPart* part = map->getCurrentPart();
		
		map->clearObjectSelection(false);
		add_step->removeContainedObjects(false);
		for (size_t i = 0; i < new_objects.size(); ++i)
		{
			Object* object = new_objects[i];
			map->addObject(object);
		}
		// Do not merge this loop into the upper one;
		// theoretically undo step indices could be wrong this way.
		for (size_t i = 0; i < new_objects.size(); ++i)
		{
			Object* object = new_objects[i];
			delete_step->addObject(part->findObjectIndex(object));
		}
		map->emitSelectionChanged();
		
		// Return undo step
		if (delete_step->isEmpty())
		{
			delete delete_step;
			if (add_step->isEmpty())
			{
				delete add_step;
				return NULL;
			}
			else
				return add_step;
		}
		else
		{
			if (add_step->isEmpty())
			{
				delete add_step;
				return delete_step;	
			}
			else
			{
				CombinedUndoStep* combined_step = new CombinedUndoStep(map);
				combined_step->push(add_step);
				combined_step->push(delete_step);
				return combined_step;
			}
		}
	}
private:
	Map* map;
	PathObject* cutout_object;
	bool cut_away;
	
	std::vector<PathObject*> new_objects;
	AddObjectsUndoStep* add_step;
	DeleteObjectsUndoStep* delete_step;
};

void CutoutTool::apply(Map* map, PathObject* cutout_object, bool cut_away)
{
	PhysicalCutoutOperation operation(map, cutout_object, cut_away);
	map->getCurrentPart()->applyOnAllObjects(operation);
	UndoStep* undo_step = operation.finish();
	if (undo_step)
	{
		map->setObjectsDirty();
		map->push(undo_step);
		map->emitSelectionEdited();
	}
}
