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


#include "object_undo.h"

#include <algorithm>

#include "core/map.h"
#include "core/objects/object.h"
#include "core/symbols/symbol.h"
#include "util/xml_stream_util.h"


namespace literal
{
	const QLatin1String source("source");
	const QLatin1String part("part");
	const QLatin1String reverse("reverse");
}


namespace OpenOrienteering {

// ### ObjectModifyingUndoStep ###

ObjectModifyingUndoStep::ObjectModifyingUndoStep(Type type, Map* map)
: UndoStep(type, map)
, part_index(map->getCurrentPartIndex())
{
	; // nothing
}

ObjectModifyingUndoStep::ObjectModifyingUndoStep(Type type, Map *map, int part_index)
: UndoStep(type, map)
, part_index(part_index)
{
	; // nothing
}


ObjectModifyingUndoStep::~ObjectModifyingUndoStep()
{
	; // nothing
}

void ObjectModifyingUndoStep::setPartIndex(int part_index)
{
	Q_ASSERT(modified_objects.empty());
	this->part_index = part_index;
}

bool ObjectModifyingUndoStep::isEmpty() const
{
	return modified_objects.empty();
}

void ObjectModifyingUndoStep::addObject(int index)
{
	modified_objects.push_back(index);
}

bool ObjectModifyingUndoStep::getModifiedParts(PartSet& out) const
{
	out.insert(getPartIndex());
	return !modified_objects.empty();
}

void ObjectModifyingUndoStep::getModifiedObjects(int part_index, ObjectSet& out) const
{
	if (part_index == getPartIndex())
	{
		MapPart* const map_part = map->getPart(part_index);
		for (int object_index : modified_objects)
		{
			Q_ASSERT(object_index >= 0 && object_index < map_part->getNumObjects());
			out.insert(map_part->getObject(object_index));
		}
	}
}



void ObjectModifyingUndoStep::saveImpl(QXmlStreamWriter& xml) const
{
	UndoStep::saveImpl(xml);
	
	XmlElementWriter element(xml, QLatin1String("affected_objects"));
	element.writeAttribute(QLatin1String("part"), part_index);
	int size = modified_objects.size();
	if (size > 8)
		element.writeAttribute(QLatin1String("count"), size);
	
	for (int i = 0; i < size; ++i)
	{
		XmlElementWriter ref(xml, QLatin1String("ref"));
		ref.writeAttribute(QLatin1String("object"), modified_objects[i]);
	}
}

void ObjectModifyingUndoStep::loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict)
{
	if (xml.name() == QLatin1String("affected_objects"))
	{
		XmlElementReader element(xml);
		part_index = element.attribute<int>(QLatin1String("part"));
		int size = element.attribute<int>(QLatin1String("count"));
		if (size)
			modified_objects.reserve(size);
		while (xml.readNextStartElement())
		{
			if (xml.name() == QLatin1String("ref"))
			{
				XmlElementReader ref(xml);
				modified_objects.push_back(ref.attribute<int>(QLatin1String("object")));
			}
			else
			{
				xml.skipCurrentElement(); // unknown
			}
		}
	}
	else
	{
		UndoStep::loadImpl(xml, symbol_dict);
	}
}



// ### ObjectCreatingUndoStep ###

ObjectCreatingUndoStep::ObjectCreatingUndoStep(Type type, Map* map)
: ObjectModifyingUndoStep(type, map)
, valid(true)
{
	connect(map, &Map::symbolChanged, this, &ObjectCreatingUndoStep::symbolChanged);
	connect(map, &Map::symbolDeleted, this, &ObjectCreatingUndoStep::symbolDeleted);
}

ObjectCreatingUndoStep::~ObjectCreatingUndoStep()
{
	int size = (int)objects.size();
	for (int i = 0; i < size; ++i)
		delete objects[i];
}

bool ObjectCreatingUndoStep::isValid() const
{
	return valid;
}

void ObjectCreatingUndoStep::addObject(int)
{
	qWarning("This implementation must not be called");
	return;
}

void ObjectCreatingUndoStep::addObject(int existing_index, Object* object)
{
	ObjectModifyingUndoStep::addObject(existing_index);
	object->setMap(map); // this is necessary so the object will find the symbols and colors it references when the undo step is saved
	objects.push_back(object);
}

void ObjectCreatingUndoStep::addObject(Object* existing, Object* object)
{
	int index = map->getCurrentPart()->findObjectIndex(existing);
	Q_ASSERT(index >= 0);
	addObject(index, object);
}

void ObjectCreatingUndoStep::getModifiedObjects(int part_index, ObjectSet& out) const
{
	if (part_index == getPartIndex())
		out.insert(objects.begin(), objects.end());
}

void ObjectCreatingUndoStep::saveImpl(QXmlStreamWriter& xml) const
{
	ObjectModifyingUndoStep::saveImpl(xml);
	
	xml.writeStartElement(QLatin1String("contained_objects"));
	int size = (int)objects.size();
	xml.writeAttribute(QLatin1String("count"), QString::number(size));
	for (int i = 0; i < size; ++i)
	{
		objects[i]->setMap(map);	// IMPORTANT: only if the object's map pointer is set it will save its symbol index correctly
		objects[i]->save(xml);
	}
	xml.writeEndElement(/*contained_objects*/);
}

void ObjectCreatingUndoStep::loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict)
{
	if (xml.name() == QLatin1String("contained_objects"))
	{
		int size = xml.attributes().value(QLatin1String("count")).toInt();
		objects.reserve(qMin(size, 1000)); // 1000 is not a limit
		while (xml.readNextStartElement())
		{
			if (xml.name() == QLatin1String("object"))
				objects.push_back(Object::load(xml, map, symbol_dict));
			else
				xml.skipCurrentElement(); // unknown
		}
	}
	else
		ObjectModifyingUndoStep::loadImpl(xml, symbol_dict);
}

void ObjectCreatingUndoStep::symbolChanged(int pos, const Symbol* new_symbol, const Symbol* old_symbol)
{
	Q_UNUSED(pos);
	int size = (int)objects.size();
	for (int i = 0; i < size; ++i)
	{
		if (objects[i]->getSymbol() == old_symbol)
			objects[i]->setSymbol(new_symbol, true);
	}
}

void ObjectCreatingUndoStep::symbolDeleted(int pos,  const Symbol* old_symbol)
{
	Q_UNUSED(pos);
	int size = (int)objects.size();
	for (int i = 0; i < size; ++i)
	{
		if (objects[i]->getSymbol() == old_symbol)
		{
			valid = false;
			return;
		}
	}
}



// ### ReplaceObjectsUndoStep ###

ReplaceObjectsUndoStep::ReplaceObjectsUndoStep(Map* map)
: ObjectCreatingUndoStep(ReplaceObjectsUndoStepType, map)
, undone(false)
{
	; // nothing else
}

ReplaceObjectsUndoStep::~ReplaceObjectsUndoStep()
{
	// Save the objects from being deleted in ~ObjectCreatingUndoStep()
	if (undone)
		objects.clear();
}

UndoStep* ReplaceObjectsUndoStep::undo()
{
	int const part_index = getPartIndex();
	
	ReplaceObjectsUndoStep* undo_step = new ReplaceObjectsUndoStep(map);
	undo_step->setPartIndex(part_index);
	
	MapPart* part = map->getPart(part_index);
	std::size_t size = objects.size();
	for (std::size_t i = 0; i < size; ++i)
	{
		undo_step->addObject(modified_objects[i], part->getObject(modified_objects[i]));
		part->setObject(objects[i], modified_objects[i], false);
	}
	
	undone = true;
	return undo_step;
}



// ### DeleteObjectsUndoStep ###

DeleteObjectsUndoStep::DeleteObjectsUndoStep(Map* map)
: ObjectModifyingUndoStep(DeleteObjectsUndoStepType, map)
{
	; // nothing else
}

DeleteObjectsUndoStep::~DeleteObjectsUndoStep()
{
	; // nothing
}

UndoStep* DeleteObjectsUndoStep::undo()
{
	int const part_index = getPartIndex();
	
	AddObjectsUndoStep* undo_step = new AddObjectsUndoStep(map);
	undo_step->setPartIndex(part_index);
	
	// Make sure to delete the objects in the right order so the other objects' indices stay valid
	std::sort(modified_objects.begin(), modified_objects.end(), std::greater<int>());
	
	MapPart* part = map->getPart(part_index);
	int size = (int)modified_objects.size();
	for (int i = 0; i < size; ++i)
	{
		undo_step->addObject(modified_objects[i], part->getObject(modified_objects[i]));
		part->deleteObject(modified_objects[i], true);
	}
	
	return undo_step;
}

bool DeleteObjectsUndoStep::getModifiedParts(PartSet&) const
{
	return false;
}

void DeleteObjectsUndoStep::getModifiedObjects(int, ObjectSet&) const
{
	; // nothing
}



// ### AddObjectsUndoStep ###

AddObjectsUndoStep::AddObjectsUndoStep(Map* map)
: ObjectCreatingUndoStep(AddObjectsUndoStepType, map)
, undone(false)
{
	; // nothing else
}

AddObjectsUndoStep::~AddObjectsUndoStep()
{
	// Save the objects from being deleted in ~ObjectCreatingUndoStep()
	if (undone)
		objects.clear();
}

UndoStep* AddObjectsUndoStep::undo()
{
	int const part_index = getPartIndex();
	
	DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map);
	undo_step->setPartIndex(part_index);
	
	// Make sure to add the objects in the right order so the other objects' indices stay valid
	std::vector< std::pair<int, int> > order;	// index into affected_objects & objects, object index
	order.resize(modified_objects.size());
	for (int i = 0; i < (int)modified_objects.size(); ++i)
		order[i] = std::pair<int, int>(i, modified_objects[i]);
	std::sort(order.begin(), order.end(), sortOrder);
	
	MapPart* part = map->getPart(part_index);
	int size = (int)objects.size();
	for (int i = 0; i < size; ++i)
	{
		undo_step->addObject(modified_objects[order[i].first]);
		part->addObject(objects[order[i].first], order[i].second);
	}
	
	undone = true;
	return undo_step;
}

void AddObjectsUndoStep::removeContainedObjects(bool emit_selection_changed)
{
	MapPart* part = map->getPart(getPartIndex());
	int size = (int)objects.size();
	bool object_deselected = false;
	for (int i = 0; i < size; ++i)
	{
		if (map->isObjectSelected(objects[i]))
		{
			map->removeObjectFromSelection(objects[i], false);
			object_deselected = true;
		}
		part->deleteObject(objects[i], true);
		map->setObjectsDirty();
	}
	if (object_deselected && emit_selection_changed)
		map->emitSelectionChanged();
}

bool AddObjectsUndoStep::sortOrder(const std::pair< int, int >& a, const std::pair< int, int >& b)
{
	return a.second < b.second;
}



// ### SwitchPartUndoStep ###

SwitchPartUndoStep::SwitchPartUndoStep(Map *map, int source, int target_index)
: ObjectModifyingUndoStep(SwitchPartUndoStepType, map, target_index)
, source_index(source)
{
	// nothing else
}

SwitchPartUndoStep::SwitchPartUndoStep(Map *map)
: ObjectModifyingUndoStep(SwitchPartUndoStepType, map)
, source_index(map->getCurrentPartIndex())
{
	// nothing else
}

// virtual
SwitchPartUndoStep::~SwitchPartUndoStep() = default;


// virtual
bool SwitchPartUndoStep::getModifiedParts(UndoStep::PartSet& out) const
{
	out.insert(source_index);
	return ObjectModifyingUndoStep::getModifiedParts(out);
}

// virtual
void SwitchPartUndoStep::getModifiedObjects(int part_index, UndoStep::ObjectSet& out) const
{
	if (!reverse)
	{
		ObjectModifyingUndoStep::getModifiedObjects(part_index, out);
	}
	else if (part_index == source_index)
	{
		MapPart* const map_part = map->getPart(source_index);
		auto object_index = map_part->getNumObjects();
		for (auto&& counter : modified_objects)
		{
			Q_UNUSED(counter)
			Q_ASSERT(object_index > 0);
			if (auto* object = map_part->getObject(--object_index))
				out.insert(object);
		}
	}
}

// virtual
UndoStep* SwitchPartUndoStep::undo()
{
	auto selection_size = map->selectedObjects().size();
	
	auto source = map->getPart(source_index);
	auto target = map->getPart(getPartIndex());
	SwitchPartUndoStep* undo = new SwitchPartUndoStep(map, source_index, getPartIndex());
	undo->modified_objects = modified_objects;
	if (reverse)
	{
		std::for_each(begin(modified_objects), end(modified_objects), [this, source, target](auto i) {
			auto object = target->getObject(i);
			if (map->getCurrentPart() == target && map->isObjectSelected(object))
				map->removeObjectFromSelection(object, false);
			target->deleteObject(i, true);
			source->addObject(object);
		});
	}
	else
	{
		undo->reverse = true;
		auto i = source->getNumObjects();
		std::for_each(modified_objects.rbegin(), modified_objects.rend(), [this, source, target, &i](auto j) {
			--i;
			auto object = source->getObject(i);
			if (map->getCurrentPart() == source && map->isObjectSelected(object))
				map->removeObjectFromSelection(object, false);
			source->deleteObject(i, true);
			target->addObject(object, j);
		});
	}
	
	if (map->selectedObjects().size() != selection_size)
		map->emitSelectionChanged();
	
	return undo;
}


// virtual
void SwitchPartUndoStep::saveImpl(QXmlStreamWriter &xml) const
{
	ObjectModifyingUndoStep::saveImpl(xml);
	XmlElementWriter source_element(xml, literal::source);
	source_element.writeAttribute(literal::part, source_index);
	source_element.writeAttribute(literal::reverse, reverse);
}

// virtual
void SwitchPartUndoStep::loadImpl(QXmlStreamReader &xml, SymbolDictionary &symbol_dict)
{
	if (xml.name() == literal::source)
	{
		XmlElementReader source_element(xml);
		source_index = source_element.attribute<int>(literal::part);
		reverse = source_element.attribute<bool>(literal::reverse);
	}
	else
		ObjectModifyingUndoStep::loadImpl(xml, symbol_dict);
}



// ### SwitchSymbolUndoStep ###

SwitchSymbolUndoStep::SwitchSymbolUndoStep(Map* map)
: ObjectModifyingUndoStep(SwitchSymbolUndoStepType, map)
, valid(true)
{
	connect(map, &Map::symbolChanged, this, &SwitchSymbolUndoStep::symbolChanged);
	connect(map, &Map::symbolDeleted, this, &SwitchSymbolUndoStep::symbolDeleted);
}

SwitchSymbolUndoStep::~SwitchSymbolUndoStep()
{
	; // nothing
}

bool SwitchSymbolUndoStep::isValid() const
{
	return valid;
}

void SwitchSymbolUndoStep::addObject(int index, const Symbol* target_symbol)
{
	ObjectModifyingUndoStep::addObject(index);
	target_symbols.push_back(target_symbol);
}

UndoStep* SwitchSymbolUndoStep::undo()
{
	int const part_index = getPartIndex();
	
	SwitchSymbolUndoStep* undo_step = new SwitchSymbolUndoStep(map);
	undo_step->setPartIndex(part_index);
	
	MapPart* part = map->getPart(part_index);
	int size = (int)modified_objects.size();
	for (int i = 0; i < size; ++i)
	{
		Object* object = part->getObject(modified_objects[i]);
		undo_step->addObject(modified_objects[i], object->getSymbol());
		bool ok = object->setSymbol(target_symbols[i], false);
		Q_ASSERT(ok);
		Q_UNUSED(ok);
	}
	
	return undo_step;
}



void SwitchSymbolUndoStep::saveImpl(QXmlStreamWriter& xml) const
{
	ObjectModifyingUndoStep::saveImpl(xml);
	
	xml.writeStartElement(QLatin1String("switch_symbol"));
	int size = (int)target_symbols.size();
	xml.writeAttribute(QLatin1String("count"), QString::number(size));
	for (int i = 0; i < size; ++i)
	{
		int index = map->findSymbolIndex(target_symbols[i]);
		xml.writeEmptyElement(QLatin1String("ref"));
		xml.writeAttribute(QLatin1String("symbol"), QString::number(index));
	}
	xml.writeEndElement(/*switch_symbol*/);
}

void SwitchSymbolUndoStep::loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict)
{
	if (xml.name() == QLatin1String("switch_symbol"))
	{
		int size = xml.attributes().value(QLatin1String("count")).toInt();
		target_symbols.reserve(qMin(size, 1000)); // 1000 is not a limit
		while (xml.readNextStartElement())
		{
			if (xml.name() == QLatin1String("ref"))
			{
				QString key = xml.attributes().value(QLatin1String("symbol")).toString();
				target_symbols.push_back(symbol_dict[key]);
			}
			
			xml.skipCurrentElement(); // unknown
		}
	}
	else
		ObjectModifyingUndoStep::loadImpl(xml, symbol_dict);
}

void SwitchSymbolUndoStep::symbolChanged(int pos, const Symbol* new_symbol, const Symbol* old_symbol)
{
	Q_UNUSED(pos);
	int size = (int)target_symbols.size();
	for (int i = 0; i < size; ++i)
	{
		if (target_symbols[i] == old_symbol)
			target_symbols[i] = new_symbol;
	}
}
void SwitchSymbolUndoStep::symbolDeleted(int pos, const Symbol* old_symbol)
{
	Q_UNUSED(pos);
	int size = (int)target_symbols.size();
	for (int i = 0; i < size; ++i)
	{
		if (target_symbols[i] == old_symbol)
		{
			valid = false;
			return;
		}
	}
}



// ### SwitchDashesUndoStep ###

SwitchDashesUndoStep::SwitchDashesUndoStep(Map* map)
: ObjectModifyingUndoStep(SwitchDashesUndoStepType, map)
{
	; // nothing else
}

SwitchDashesUndoStep::~SwitchDashesUndoStep()
{
	; // nothing
}

UndoStep* SwitchDashesUndoStep::undo()
{
	int const part_index = getPartIndex();
	
	SwitchDashesUndoStep* undo_step = new SwitchDashesUndoStep(map);
	undo_step->setPartIndex(part_index);
	
	MapPart* part = map->getPart(part_index);
	for (const auto object_index : modified_objects)
	{
		PathObject* object = reinterpret_cast<PathObject*>(part->getObject(object_index));
		object->reverse();
		object->update();
		
		undo_step->addObject(object_index);
	}
	
	return undo_step;
}



// ### ObjectTagsUndoStep ###

ObjectTagsUndoStep::ObjectTagsUndoStep(Map *map)
: ObjectModifyingUndoStep(ObjectTagsUndoStepType, map)
{
	; // nothing else
}

ObjectTagsUndoStep::~ObjectTagsUndoStep()
{
	; // nothing
}

void ObjectTagsUndoStep::addObject(int index)
{
	ObjectModifyingUndoStep::addObject(index);
	
	MapPart* const map_part = map->getPart(getPartIndex());
	object_tags_map[index] = map_part->getObject(index)->tags();
}

UndoStep* ObjectTagsUndoStep::undo()
{
	int const part_index = getPartIndex();
	
	ObjectTagsUndoStep* redo_step = new ObjectTagsUndoStep(map);
	MapPart* const map_part = map->getPart(part_index);
	
	redo_step->setPartIndex(part_index);
	for (const auto& object_tags : object_tags_map)
	{
		redo_step->addObject(object_tags.first);
		map_part->getObject(object_tags.first)->setTags(object_tags.second);
	}
	
	return redo_step;
}

void ObjectTagsUndoStep::saveImpl(QXmlStreamWriter &xml) const
{
	UndoStep::saveImpl(xml);
	
	// Note: For reducing file size, this implementation copies, not calls,
	// the parent's implementation.
	XmlElementWriter element(xml, QLatin1String("affected_objects"));
	element.writeAttribute(QLatin1String("part"), getPartIndex());
	std::size_t size = modified_objects.size();
	if (size > 8)
		element.writeAttribute(QLatin1String("count"), size);
	
	for (const auto& object_tags : object_tags_map)
	{
		namespace literal = XmlStreamLiteral;
		
		XmlElementWriter tags_element(xml, QLatin1String("ref"));
		tags_element.writeAttribute(literal::object, object_tags.first);
		tags_element.write(object_tags.second);
	}
}

void ObjectTagsUndoStep::loadImpl(QXmlStreamReader &xml, SymbolDictionary &symbol_dict)
{
	namespace literal = XmlStreamLiteral;
	
	// Note: This implementation copies, not calls, the parent's implementation.
	if (xml.name() == QLatin1String("affected_objects"))
	{
		XmlElementReader element(xml);
		setPartIndex(element.attribute<int>(QLatin1String("part")));
		int size = element.attribute<int>(QLatin1String("count"));
		if (size)
			modified_objects.reserve(size);
		
		while (xml.readNextStartElement())
		{
			if (xml.name() == QLatin1String("ref"))
			{
				XmlElementReader tags_element(xml);
				int index = tags_element.attribute<int>(literal::object);
				modified_objects.push_back(index);
				tags_element.read(object_tags_map[index]);
			}
			else
			{
				xml.skipCurrentElement();
			}
		}
	}
	else
	{
		UndoStep::loadImpl(xml, symbol_dict);
	}
}


}  // namespace OpenOrienteering
