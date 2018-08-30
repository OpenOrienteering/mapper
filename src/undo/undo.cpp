/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#include "undo.h"

#include <vector>

#include <QXmlStreamReader>

#include "object_undo.h"
#include "map_part_undo.h"
#include "util/xml_stream_util.h"


namespace literal
{
	const QLatin1String step("step");
	const QLatin1String steps("steps");
}



namespace OpenOrienteering {

// ### UndoStep ###

// static
UndoStep* UndoStep::getUndoStepForType(Type type, Map* map)
{
	switch (type)
	{
	case CombinedUndoStepType:
		return new CombinedUndoStep(map);
		
	case ReplaceObjectsUndoStepType:
		return new ReplaceObjectsUndoStep(map);
		
	case DeleteObjectsUndoStepType:
		return new DeleteObjectsUndoStep(map);
		
	case AddObjectsUndoStepType:
		return new AddObjectsUndoStep(map);
		
	case SwitchSymbolUndoStepType:
		return new SwitchSymbolUndoStep(map);
		
	case SwitchDashesUndoStepType:
		return new SwitchDashesUndoStep(map);
		
	case ValidNoOpUndoStepType:
		return new NoOpUndoStep(map, true);
		
	case ObjectTagsUndoStepType:
		return new ObjectTagsUndoStep(map);
		
	case SwitchPartUndoStepType:
		return new SwitchPartUndoStep(map);
		
	case MapPartUndoStepType:
		return new MapPartUndoStep(map);
		
	default:
		qWarning("Undefined undo step type");
		// fall through
	case SwitchPartUndoStepTypeV0:
		return new NoOpUndoStep(map, false);
	}
}


UndoStep::UndoStep(Type type, Map* map)
: type(type)
, map(map)
{
	; // nothing else
}

UndoStep::~UndoStep()
{
	; // nothing
}

bool UndoStep::isValid() const
{
	return true;
}

bool UndoStep::getModifiedParts(PartSet& out) const
{
	Q_UNUSED(out);
	return false;
}

void UndoStep::getModifiedObjects(int, ObjectSet&) const
{
	; // nothing
}

// static
UndoStep* UndoStep::load(QXmlStreamReader& xml, Map* map, SymbolDictionary& symbol_dict)
{
	Q_ASSERT(xml.name() == QLatin1String("step"));
	
	int type = xml.attributes().value(QLatin1String("type")).toInt();
	UndoStep* step = UndoStep::getUndoStepForType((Type)type, map);
	while (xml.readNextStartElement())
		step->loadImpl(xml, symbol_dict);
	
	return step;
}

void UndoStep::save(QXmlStreamWriter& xml)
{
	XmlElementWriter element(xml, QLatin1String("step"));
	element.writeAttribute(QLatin1String("type"), type);
	saveImpl(xml);
}

void UndoStep::saveImpl(QXmlStreamWriter& xml) const
{
	Q_UNUSED(xml);
	; // nothing yet
}

void UndoStep::loadImpl(QXmlStreamReader& xml, SymbolDictionary &symbol_dict)
{
	Q_UNUSED(symbol_dict);
	
	xml.skipCurrentElement(); // unknown
}



// ### CombinedUndoStep ###

CombinedUndoStep::CombinedUndoStep(Map* map)
: UndoStep(CombinedUndoStepType, map)
{
	; // nothing else
}

CombinedUndoStep::~CombinedUndoStep()
{
	for (const auto step : steps)
		delete step;
}

bool CombinedUndoStep::isValid() const
{
	return std::all_of(begin(steps), end(steps), [](const auto& step) {
		return step->isValid();
	});
}

UndoStep* CombinedUndoStep::undo()
{
	CombinedUndoStep* undo_step = new CombinedUndoStep(map);
	undo_step->steps.reserve(steps.size());
	std::for_each(steps.rbegin(), steps.rend(), [undo_step](auto step) {
		undo_step->push(step->undo());
	});
	return undo_step;
}

bool CombinedUndoStep::getModifiedParts(PartSet &out) const
{
	for (const auto step : steps)
	{
		step->getModifiedParts(out);
	}
	return !out.empty();
}

void CombinedUndoStep::getModifiedObjects(int part_index, ObjectSet &out) const
{
	for (const auto step : steps)
	{
		step->getModifiedObjects(part_index, out);
	}
}

#ifndef NO_NATIVE_FILE_FORMAT

bool CombinedUndoStep::load(QIODevice* file, int version)
{
	int size;
	file->read((char*)&size, sizeof(int));
	steps.resize(size);
	bool success = true;
	for (std::size_t i = 0; i < steps.size() && success; ++i)
	{
		int type;
		file->read((char*)&type, sizeof(int));
		steps.insert(steps.begin(), UndoStep::getUndoStepForType((UndoStep::Type)type, map));
		success = steps.front()->load(file, version);
	}
	return success;
}

#endif

void CombinedUndoStep::saveImpl(QXmlStreamWriter& xml) const
{
	UndoStep::saveImpl(xml);
	
	// From Mapper 0.6, use the same XML element and order as UndoManager.
	// (A barrier element prevents older versions from loading this element.)
	XmlElementWriter steps_element(xml, literal::steps);
	steps_element.writeAttribute(XmlStreamLiteral::count, steps.size());
	for (const auto step : steps)
		step->save(xml);
}

void CombinedUndoStep::loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict)
{
	if (xml.name() == QLatin1String("substeps"))
	{
		// Mapper before 0.6
		// @todo Remove in a future version
		int size = xml.attributes().value(QLatin1String("count")).toInt();
		steps.reserve(qMin(size, 10)); // 10 is not a limit
		while (xml.readNextStartElement())
		{
			if (xml.name() == literal::step)
				steps.insert(steps.begin(), UndoStep::load(xml, map, symbol_dict));
			else
				xml.skipCurrentElement(); // unknown
		}
	}
	else if (xml.name() == literal::steps)
	{
		// From Mapper 0.6, use the same XML element and order as UndoManager.
		XmlElementReader steps_element(xml);
		std::size_t size = steps_element.attribute<std::size_t>(XmlStreamLiteral::count);
		steps.reserve(qMin(size, (std::size_t)50)); // 50 is not a limit
		while (xml.readNextStartElement())
		{
			if (xml.name() == literal::step)
				steps.push_back(UndoStep::load(xml, map, symbol_dict));
			else
				xml.skipCurrentElement(); // unknown
		}
	}
	else
		UndoStep::loadImpl(xml, symbol_dict);
}



// ### InvalidUndoStep ###

NoOpUndoStep::NoOpUndoStep(Map *map, bool valid)
: UndoStep(valid ? ValidNoOpUndoStepType : InvalidUndoStepType, map)
, valid(valid)
{
	// nothing else
}

NoOpUndoStep::~NoOpUndoStep()
{
	// nothing
}

bool NoOpUndoStep::isValid() const
{
	return valid;
}

UndoStep* NoOpUndoStep::undo()
{
	if (!valid)
		qWarning("InvalidUndoStep::undo() must not be called");
	
	return new NoOpUndoStep(map, true);
}

#ifndef NO_NATIVE_FILE_FORMAT

bool NoOpUndoStep::load(QIODevice*, int)
{
	qWarning("InvalidUndoStep::load(QIODevice*, int) must not be called");
	return false;
}

#endif  // NO_NATIVE_FILE_FORMAT

}  // namespace OpenOrienteering

