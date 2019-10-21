/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2013, 2014, 2017-2019 Kai Pastor
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


#include "cutout_operation.h"

#include <iterator>
#include <vector>

#include <QtGlobal>
#include <QFlags>
#include <QRectF>

#include "core/map.h"
#include "core/map_coord.h"
#include "core/map_part.h"
#include "core/objects/boolean_tool.h"
#include "core/objects/object.h"
#include "core/symbols/symbol.h"
#include "undo/object_undo.h"
#include "undo/undo.h"


namespace OpenOrienteering {

CutoutOperation::CutoutOperation(Map* map, PathObject* cutout_object, bool cut_away)
: map(map)
, cutout_object(cutout_object)
, add_step(new AddObjectsUndoStep(map))
, boolean_tool(cut_away ? BooleanTool::Difference : BooleanTool::Intersection, map)
, cut_away(cut_away)
{
	// nothing else
}


CutoutOperation::~CutoutOperation()
{
	Q_ASSERT(!add_step);  // cleared by commit()
}


void CutoutOperation::operator()(Object* object)
{
	// If there is a selection, only clip selected objects
	if (!map->selectedObjects().empty() && !map->isObjectSelected(object))
		return;
	
	// Don't clip object itself
	if (object == cutout_object)
		return;
	
	// Early out
	if (!object->getExtent().intersects(cutout_object->getExtent()))
	{
		if (!cut_away)
			add_step->addObject(object, object);
		return;
	}
	
	switch (object->getType())
	{
	case Object::Point:
	case Object::Text:
		// Simple check if the (first) point is inside the area
		if (cutout_object->isPointInsideArea(MapCoordF(object->getRawCoordinateVector().at(0))) == cut_away)
			add_step->addObject(object, object);
		break;
		
	case Object::Path:
		out_objects.clear();
		if (object->getSymbol()->getContainedTypes() & Symbol::Area)
		{
			// Use the Clipper library to clip the area
			BooleanTool::PathObjects in_objects;
			in_objects.push_back(cutout_object);
			in_objects.push_back(object->asPath());
			if (!boolean_tool.executeForObjects(object->asPath(), in_objects, out_objects))
				break;
		}
		else
		{
			// Use some custom code to clip the line
			boolean_tool.executeForLine(cutout_object, object->asPath(), out_objects);
		}
		
		add_step->addObject(object, object);
		new_objects.insert(end(new_objects), begin(out_objects), end(out_objects));
		break;
	}
	
	return;
}


void CutoutOperation::commit()
{
	if (auto undo_step = finish())
	{
		map->setObjectsDirty();
		map->push(undo_step);
		map->emitSelectionEdited();
	}
	add_step = nullptr;
}


UndoStep* CutoutOperation::finish()
{
	// Whenever operator() adds to new_objects, it adds to add_step, too.
	Q_ASSERT(new_objects.empty() || !add_step->isEmpty());
	
	if (add_step->isEmpty())
	{
		delete add_step;
		return nullptr;
	}
	
	map->clearObjectSelection(false);
	add_step->removeContainedObjects(false);
	if (new_objects.empty())
	{
		map->emitSelectionChanged();
		return add_step;
	}
	
	for (auto object : new_objects)
	{
		map->addObject(object);
	}
	// Do not merge this loop into the upper one;
	// theoretically undo step indices could be wrong this way.
	auto part = map->getCurrentPart();
	auto delete_step = new DeleteObjectsUndoStep(map);
	for (auto object : new_objects)
	{
		delete_step->addObject(part->findObjectIndex(object));
	}
	map->emitSelectionChanged();
	
	auto combined_step = new CombinedUndoStep(map);
	combined_step->push(add_step);
	combined_step->push(delete_step);
	return combined_step;
}


}  // namespace OpenOrienteering
