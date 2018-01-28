/*
 *    Copyright 2014 Kai Pastor
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

#include "map_part_undo.h"

#include <QtGlobal>
#include <QLatin1String>
#include <QStringRef>
#include <QXmlStreamReader>

#include "core/map.h"
#include "core/map_part.h"
#include "util/xml_stream_util.h"


namespace literal
{
	const QLatin1String change("change");
	const QLatin1String type("type");
	const QLatin1String part("part");
	const QLatin1String add("add");
	const QLatin1String remove("remove");
	const QLatin1String modify("modify");
	const QLatin1String name("name");
}



namespace OpenOrienteering {

MapPartUndoStep::MapPartUndoStep(Map* map, MapPartChange change, const MapPart* part)
: UndoStep(UndoStep::MapPartUndoStepType, map)
, change(change)
, index(map->findPartIndex(part))
, name(part->getName())
{
	// nothing else
}

MapPartUndoStep::MapPartUndoStep(Map* map, MapPartChange change, int index)
: UndoStep(UndoStep::MapPartUndoStepType, map)
, change(change)
, index(index)
, name(map->getPart(index)->getName())
{
	// nothing else
}

MapPartUndoStep::MapPartUndoStep(Map* map)
: UndoStep(UndoStep::MapPartUndoStepType, map)
, change(UndefinedChange)
, index(0)
, name()
{
	// nothing else
}

// virtual
MapPartUndoStep::~MapPartUndoStep()
{
	// nothing
}

// virtual
bool MapPartUndoStep::isValid() const
{
	return change != UndefinedChange;
}

// virtual
UndoStep* MapPartUndoStep::undo()
{
	UndoStep* redo_step = nullptr;
	switch (change)
	{
	case AddMapPart:
		Q_ASSERT(map->getNumParts()+1 > index);
		map->addPart(new MapPart(name, map), index);
		redo_step = new MapPartUndoStep(map, RemoveMapPart, index);
	    break;
	case RemoveMapPart:
		Q_ASSERT(map->getPart(index)->getNumObjects() == 0);
		redo_step = new MapPartUndoStep(map, AddMapPart, index);
		map->removePart(index);
		break;
	case ModifyMapPart:
		Q_ASSERT(map->getNumParts() > index);
		redo_step = new MapPartUndoStep(map, ModifyMapPart, index);
		map->getPart(index)->setName(name);
		break;
	case UndefinedChange:
		Q_ASSERT(false);
		redo_step = new NoOpUndoStep(map, true);
		break;
	// default: nothing left (but watch compiler warnings).
	}
	
	Q_ASSERT(redo_step);
	return redo_step;
}

// virtual
bool MapPartUndoStep::getModifiedParts(PartSet &out) const
{
	bool modified = false;
	switch (change)
	{
	case AddMapPart:
	case ModifyMapPart:
		out.insert(index);
		modified = true;
		break;
	default:
		; // nothing
	}

	return modified;
}

// virtual
void MapPartUndoStep::getModifiedObjects(int, ObjectSet &) const
{
	// nothing
}



// virtual
void MapPartUndoStep::saveImpl(QXmlStreamWriter &xml) const
{
	UndoStep::saveImpl(xml);
	XmlElementWriter change_element(xml, literal::change);
	change_element.writeAttribute(literal::type, change);
	change_element.writeAttribute(literal::part, index);
	
	switch (change)
	{
	case AddMapPart:
	case ModifyMapPart:
		change_element.writeAttribute(literal::name, name);
		break;
	case RemoveMapPart:
	case UndefinedChange:
		break;
	// default: nothing left (but watch compiler warnings).
	}		
}

// virtual
void MapPartUndoStep::loadImpl(QXmlStreamReader &xml, SymbolDictionary &)
{
	if (xml.name() == literal::change)
	{
		XmlElementReader change_element(xml);
		unsigned int type = change_element.attribute<unsigned int>(literal::type);
		if (type <= 3)
			change = (MapPartChange)type;
		index = change_element.attribute<int>(literal::part);
		name  = change_element.attribute<QString>(literal::name);
	}
}


}  // namespace OpenOrienteering
