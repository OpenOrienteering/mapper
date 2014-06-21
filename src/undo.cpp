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

#include "map.h"
#include "map_undo.h"
#include "util/xml_stream_util.h"

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
		
	default:
		qWarning("Undefined undo step type");
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
	return false;
}

void UndoStep::getModifiedObjects(int, ObjectSet&) const
{
	; // nothing
}

// static
UndoStep* UndoStep::load(QXmlStreamReader& xml, Map* map, SymbolDictionary& symbol_dict)
{
	Q_ASSERT(xml.name() == "step");
	
	int type = xml.attributes().value("type").toString().toInt();
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
	for (StepList::iterator step=steps.begin(), end=steps.end(); step != end; ++step)
		delete *step;
}

bool CombinedUndoStep::isValid() const
{
	for (std::size_t i = 0; i < steps.size(); ++i)
	{
		if (!steps[i]->isValid())
			return false;
	}
	return true;
}

UndoStep* CombinedUndoStep::undo()
{
	CombinedUndoStep* undo_step = new CombinedUndoStep(map);
	undo_step->steps.reserve(steps.size());
	for (std::size_t i = 0; i < steps.size(); ++i)
		undo_step->steps.insert(undo_step->steps.begin(), steps[i]->undo());
	return undo_step;
}

bool CombinedUndoStep::getModifiedParts(PartSet &out) const
{
	for (StepList::const_iterator step = steps.begin(), end = steps.end(); step != end; ++step)
	{
		(*step)->getModifiedParts(out);
	}
	return !out.empty();
}

void CombinedUndoStep::getModifiedObjects(int part_index, ObjectSet &out) const
{
	for (StepList::const_iterator step = steps.begin(), end = steps.end(); step != end; ++step)
	{
		(*step)->getModifiedObjects(part_index, out);
	}
}

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
		steps[i] = UndoStep::getUndoStepForType((UndoStep::Type)type, map);
		success = steps[i]->load(file, version);
	}
	return success;
}

void CombinedUndoStep::saveImpl(QXmlStreamWriter& xml) const
{
	UndoStep::saveImpl(xml);
	
	xml.writeStartElement("substeps");
	int size = (int)steps.size();
	xml.writeAttribute("count", QString::number(size));
	for (int i = 0; i < size; ++i)
	{
		steps[i]->save(xml);
	}
	xml.writeEndElement(/*substeps*/);
}

void CombinedUndoStep::loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict)
{
	if (xml.name() == "substeps")
	{
		int size = xml.attributes().value("count").toString().toInt();
		steps.reserve(qMin(size, 10)); // 10 is not a limit
		while (xml.readNextStartElement())
		{
			if (xml.name() == "step")
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

bool NoOpUndoStep::load(QIODevice*, int)
{
	qWarning("InvalidUndoStep::load(QIODevice*, int) must not be called");
	return false;
}

