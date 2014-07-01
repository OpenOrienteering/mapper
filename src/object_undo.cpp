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

#include <QIODevice>

#include "map.h"
#include "object.h"
#include "symbol.h"

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
		for (std::vector<int>::const_iterator it = modified_objects.begin(), end = modified_objects.end(); it != end; ++it)
		{
			Q_ASSERT(*it >= 0 && *it < map_part->getNumObjects());
			out.insert(map_part->getObject(*it));
		}
	}
}

bool ObjectModifyingUndoStep::load(QIODevice* file, int version)
{
	Q_UNUSED(version);
	file->read((char*)&part_index, sizeof(int));
	int size;
	file->read((char*)&size, sizeof(int));
	modified_objects.resize(size);
	for (int i = 0; i < size; ++i)
		file->read((char*)&modified_objects[i], sizeof(int));
	return true;
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
	if (xml.name() == "affected_objects")
	{
		XmlElementReader element(xml);
		part_index = element.attribute<int>(QLatin1String("part"));
		int size = element.attribute<int>(QLatin1String("count"));
		if (size)
			modified_objects.reserve(size);
		while (xml.readNextStartElement())
		{
			if (xml.name() == "ref")
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
	connect(map, SIGNAL(symbolChanged(int,Symbol*,Symbol*)), this, SLOT(symbolChanged(int,Symbol*,Symbol*)));
	connect(map, SIGNAL(symbolDeleted(int,Symbol*)), this, SLOT(symbolDeleted(int,Symbol*)));
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

void ObjectCreatingUndoStep::addObject(int index)
{
	Q_ASSERT(false && "This implementation must not be called");
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

bool ObjectCreatingUndoStep::load(QIODevice* file, int version)
{
	if (!ObjectModifyingUndoStep::load(file, version))
		return false;
	
	int size = (int)modified_objects.size();
	objects.resize(size);
	for (int i = 0; i < size; ++i)
	{
		int save_type;
		file->read((char*)&save_type, sizeof(int));
		objects[i] = Object::getObjectForType(static_cast<Object::Type>(save_type), NULL);
		if (!objects[i])
			return false;
		objects[i]->load(file, version, map);
	}
	return true;
}

void ObjectCreatingUndoStep::getModifiedObjects(int part_index, ObjectSet& out) const
{
	if (part_index == getPartIndex())
		out.insert(objects.begin(), objects.end());
}

void ObjectCreatingUndoStep::saveImpl(QXmlStreamWriter& xml) const
{
	ObjectModifyingUndoStep::saveImpl(xml);
	
	xml.writeStartElement("contained_objects");
	int size = (int)objects.size();
	xml.writeAttribute("count", QString::number(size));
	for (int i = 0; i < size; ++i)
	{
		objects[i]->setMap(map);	// IMPORTANT: only if the object's map pointer is set it will save its symbol index correctly
		objects[i]->save(xml);
	}
	xml.writeEndElement(/*contained_objects*/);
}

void ObjectCreatingUndoStep::loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict)
{
	if (xml.name() == "contained_objects")
	{
		int size = xml.attributes().value("count").toString().toInt();
		objects.reserve(qMin(size, 1000)); // 1000 is not a limit
		while (xml.readNextStartElement())
		{
			if (xml.name() == "object")
				objects.push_back(Object::load(xml, map, symbol_dict));
			else
				xml.skipCurrentElement(); // unknown
		}
	}
	else
		ObjectModifyingUndoStep::loadImpl(xml, symbol_dict);
}

void ObjectCreatingUndoStep::symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol)
{
	Q_UNUSED(pos);
	int size = (int)objects.size();
	for (int i = 0; i < size; ++i)
	{
		if (objects[i]->getSymbol() == old_symbol)
			objects[i]->setSymbol(new_symbol, true);
	}
}

void ObjectCreatingUndoStep::symbolDeleted(int pos, Symbol* old_symbol)
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
{
	; // nothing else
}

ReplaceObjectsUndoStep::~ReplaceObjectsUndoStep()
{
	; // nothing
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
	
	objects.clear();
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
{
	; // nothing else
}

AddObjectsUndoStep::~AddObjectsUndoStep()
{
	; // nothing
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
	
	objects.clear();
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



// ### SwitchSymbolUndoStep ###

SwitchSymbolUndoStep::SwitchSymbolUndoStep(Map* map)
: ObjectModifyingUndoStep(SwitchSymbolUndoStepType, map)
, valid(true)
{
	connect(map, SIGNAL(symbolChanged(int,Symbol*,Symbol*)), this, SLOT(symbolChanged(int,Symbol*,Symbol*)));
	connect(map, SIGNAL(symbolDeleted(int,Symbol*)), this, SLOT(symbolDeleted(int,Symbol*)));
}

SwitchSymbolUndoStep::~SwitchSymbolUndoStep()
{
	; // nothing
}

bool SwitchSymbolUndoStep::isValid() const
{
	return valid;
}

void SwitchSymbolUndoStep::addObject(int index, Symbol* target_symbol)
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
		assert(ok);
	}
	
	return undo_step;
}

bool SwitchSymbolUndoStep::load(QIODevice* file, int version)
{
	if (!ObjectModifyingUndoStep::load(file, version))
		return false;
	
	int size = (int)modified_objects.size();
	target_symbols.resize(size);
	for (int i = 0; i < size; ++i)
	{
		int index;
		file->read((char*)&index, sizeof(int));
		target_symbols[i] = map->getSymbol(index);
	}
	return true;
}

void SwitchSymbolUndoStep::saveImpl(QXmlStreamWriter& xml) const
{
	ObjectModifyingUndoStep::saveImpl(xml);
	
	xml.writeStartElement("switch_symbol");
	int size = (int)target_symbols.size();
	xml.writeAttribute("count", QString::number(size));
	for (int i = 0; i < size; ++i)
	{
		int index = map->findSymbolIndex(target_symbols[i]);
		xml.writeEmptyElement("ref");
		xml.writeAttribute("symbol", QString::number(index));
	}
	xml.writeEndElement(/*switch_symbol*/);
}

void SwitchSymbolUndoStep::loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict)
{
	if (xml.name() == "switch_symbol")
	{
		int size = xml.attributes().value("count").toString().toInt();
		target_symbols.reserve(qMin(size, 1000)); // 1000 is not a limit
		while (xml.readNextStartElement())
		{
			if (xml.name() == "ref")
			{
				QString key = xml.attributes().value("symbol").toString();
				target_symbols.push_back(symbol_dict[key]);
			}
			
			xml.skipCurrentElement(); // unknown
		}
	}
	else
		ObjectModifyingUndoStep::loadImpl(xml, symbol_dict);
}

void SwitchSymbolUndoStep::symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol)
{
	Q_UNUSED(pos);
	int size = (int)target_symbols.size();
	for (int i = 0; i < size; ++i)
	{
		if (target_symbols[i] == old_symbol)
			target_symbols[i] = new_symbol;
	}
}
void SwitchSymbolUndoStep::symbolDeleted(int pos, Symbol* old_symbol)
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
	for (ObjectList::iterator it = modified_objects.begin(), end = modified_objects.end(); it != end; ++it)
	{
		PathObject* object = reinterpret_cast<PathObject*>(part->getObject(*it));
		object->reverse();
		object->update(true);
		
		undo_step->addObject(*it);
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
	for (ObjectTagsMap::iterator it = object_tags_map.begin(), end = object_tags_map.end(); it != end; ++it)
	{
		redo_step->addObject(it->first);
		map_part->getObject(it->first)->setTags(it->second);
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
	
	for (ObjectTagsMap::const_iterator it = object_tags_map.begin(), end = object_tags_map.end(); it != end; ++it)
	{
		namespace literal = XmlStreamLiteral;
		
		XmlElementWriter tags_element(xml, QLatin1String("ref"));
		tags_element.writeAttribute(literal::object, it->first);
		tags_element.write(it->second);
	}
}

void ObjectTagsUndoStep::loadImpl(QXmlStreamReader &xml, SymbolDictionary &symbol_dict)
{
	namespace literal = XmlStreamLiteral;
	
	// Note: This implementation copies, not calls, the parent's implementation.
	if (xml.name() == "affected_objects")
	{
		XmlElementReader element(xml);
		setPartIndex(element.attribute<int>(QLatin1String("part")));
		int size = element.attribute<int>(QLatin1String("count"));
		if (size)
			modified_objects.reserve(size);
		
		while (xml.readNextStartElement())
		{
			if (xml.name() == "ref")
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
