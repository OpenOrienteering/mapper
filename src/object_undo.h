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


#ifndef _OPENORIENTEERING_MAP_UNDO_H_
#define _OPENORIENTEERING_MAP_UNDO_H_

#include <map>

#include "object.h"
#include "symbol.h"
#include "undo.h"

QT_BEGIN_NAMESPACE
class QIODevice;
class QXmlStreamReader;
class QXmlStreamWriter;
QT_END_NAMESPACE


/** 
 * Base class for undo steps which modify objects of a single map part.
 */
class ObjectModifyingUndoStep : public UndoStep
{
public:
	/**
	 * Creates an ObjectModifyingUndoStep for the given map and its current part.
	 */
	ObjectModifyingUndoStep(Type type, Map* map);
	
	/**
	 * Creates an ObjectModifyingUndoStep for the given map and part.
	 */
	ObjectModifyingUndoStep(Type type, Map* map, int part_index);
	
	/**
	 * Destructor.
	 */
	virtual ~ObjectModifyingUndoStep();
	
	
	/**
	 * Returns the index of the map part modified by this undo step.
	 */
	int getPartIndex() const;
	
	/**
	 * Set the index of the map part modified by this undo step.
	 * 
	 * This must not be called after object have been added.
	 */
	void setPartIndex(int part_index);
	
	
	/**
	 * Returns true if no objects are modified by this undo step.
	 */
	virtual bool isEmpty() const;
	
	/**
	 * Adds an object (by index) to this step.
	 * 
	 * The object must belong to the step's part.
	 */
	virtual void addObject(int index);
	
	
	/**
	 * Adds the the step's modified part to the container provided by out.
	 * 
	 * @return True if there are objects modified by this undo step, false otherwise.
	 */
	virtual bool getModifiedParts(PartSet& out) const;
	
	/**
	 * Adds the list of the step's modified objects to the container provided by out.
	 * 
	 * Only adds objects when the given part_index matches this step's part index.
	 */
	virtual void getModifiedObjects(int part_index, ObjectSet& out) const;
	
	
	/**
	 * Loads the undo step from the file in the old "native" format.
	 * @deprecated Old file format.
	 */
	virtual bool load(QIODevice* file, int version);
	
protected:
	/**
	 * Saves undo properties to the the xml stream.
	 * 
	 * Implementations in derived classes shall first call the parent class'
	 * implementation, and then start a new element for additional properties.
	 */
	virtual void saveImpl(QXmlStreamWriter& xml) const;
	
	/**
	 * Loads undo properties from the the xml stream.
	 * 
	 * Implementations in derived classes shall first check the element's name
	 * for one of their own elements, and otherwise call the parent class'
	 * implementation.
	 */
	virtual void loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	
	
private:
	/**
	 * The index of the part all modified objects belong to.
	 */
	int part_index;
	
protected:
	/**
	 * A list of indices referring to objects in a map part.
	 */
	typedef std::vector<int> ObjectList;
	
	/**
	 * Indices of the existing objects that are modified by this step.
	 */
	ObjectList modified_objects;
};



/**
 * Base class for undo steps which add new objects to the map.
 * 
 * Takes care of correctly reacting to symbol changes in the map,
 * which are not covered by the undo / redo system.
 * 
 */
class ObjectCreatingUndoStep : public QObject, public ObjectModifyingUndoStep
{
Q_OBJECT
public:
	/**
	 * Constructor.
	 */
	ObjectCreatingUndoStep(Type type, Map* map);
	
	/**
	 * Destructor.
	 */
	virtual ~ObjectCreatingUndoStep();
	
	
	/**
	 * Returns true if the step can still be undone.
	 * 
	 * The value returned by this function is taken from the "valid" member variable.
	 */
	virtual bool isValid() const;
	
	/**
	 * Must not be called.
	 * 
	 * Use the two-parameter signatures instead of this one.
	 * 
	 * Does nothing in release builds. Aborts the program in non-release builds.
	 * Reimplemented from ObjectModifyingUndoStep::addObject()).
	 */
	virtual void addObject(int index);
	
	/**
	 * Adds an object to the undo step with given index.
	 */
	void addObject(int existing_index, Object* object);
	
	/**
	 * Adds an object to the undo step with the index of the existing object.
	 */
	void addObject(Object* existing, Object* object);
	
	/**
	 * @copybrief ObjectModifyingUndoStep::getModifiedObjects
	 */
	virtual void getModifiedObjects(int, ObjectSet&) const;
	
	
	/**
	 * @copybrief UndoStep::load()
	 * @deprecated Old file format.
	 */
	virtual bool load(QIODevice* file, int version);
	
public slots:
	/**
	 * Adapts the symbol pointers of objects referencing the changed symbol.
	 */
	virtual void symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol);
	
	/**
	 * Invalidates the undo step if a contained object references the deleted symbol.
	 */
	virtual void symbolDeleted(int pos, Symbol* old_symbol);
	
protected:
	/**
	 * @copybrief UndoStep::saveImpl()
	 */
	virtual void saveImpl(QXmlStreamWriter& xml) const;
	
	/**
	 * @copybrief UndoStep::loadImpl()
	 */
	virtual void loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	
	/**
	 * A list of object instance which are currently not part of the map.
	 */
	std::vector<Object*> objects;
	
	/**
	 * A flag indicating whether this step is still valid.
	 */
	bool valid;
};



/**
 * Map undo step which replaces all affected objects by another set of objects.
 */
class ReplaceObjectsUndoStep : public ObjectCreatingUndoStep
{
Q_OBJECT
public:
	ReplaceObjectsUndoStep(Map* map);
	
	virtual ~ReplaceObjectsUndoStep();
	
	virtual UndoStep* undo();
};

/**
 * Map undo step which deletes the referenced objects.
 * 
 * Take care of correct application order when mixing with an add step to
 * make sure that the object indices are preserved.
 */
class DeleteObjectsUndoStep : public ObjectModifyingUndoStep
{
public:
	DeleteObjectsUndoStep(Map* map);
	
	virtual ~DeleteObjectsUndoStep();
	
	virtual UndoStep* undo();
	
	virtual bool getModifiedParts(PartSet& out) const;
	
	virtual void getModifiedObjects(int part_index, ObjectSet& out) const;
};

/**
 * Map undo step which adds the contained objects.
 * 
 * Take care of correct application order when mixing with a delete step to
 * make sure that the object indices are preserved.
 */
class AddObjectsUndoStep : public ObjectCreatingUndoStep
{
Q_OBJECT
public:
	AddObjectsUndoStep(Map* map);
	
	~AddObjectsUndoStep();
	
	virtual UndoStep* undo();
	
	/**
	 * Removes all contained objects from the map.
	 * 
	 * This can be useful after constructing the undo step,
	 * as it is often impractical to remove the objects directly
	 * when adding them as the indexing would be changed this way.
	 */
	void removeContainedObjects(bool emit_selection_changed);
	
protected:
	static bool sortOrder(const std::pair<int, int>& a, const std::pair<int, int>& b);
};



/**
 * Map undo step which changes the symbols of referenced objects.
 */
class SwitchSymbolUndoStep : public QObject, public ObjectModifyingUndoStep
{
Q_OBJECT
public:
	SwitchSymbolUndoStep(Map* map);
	
	virtual ~SwitchSymbolUndoStep();
	
	bool isValid() const;
	
	virtual void addObject(int index, Symbol* target_symbol);
	
	virtual UndoStep* undo();
	
	virtual bool load(QIODevice* file, int version);
	
public slots:
	virtual void symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol);
	
	virtual void symbolDeleted(int pos, Symbol* old_symbol);
	
protected:
	virtual void saveImpl(QXmlStreamWriter& xml) const;
	
	virtual void loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	
	std::vector<Symbol*> target_symbols;
	
	bool valid;
};

/**
 * Map undo step which reverses the directions of referenced objects,
 * 
 * thereby switching the side of any line mid symbols (often dashes of fences).
 */
class SwitchDashesUndoStep : public ObjectModifyingUndoStep
{
public:
	SwitchDashesUndoStep(Map* map);
	
	virtual ~SwitchDashesUndoStep();
	
	virtual UndoStep* undo();
};



/**
 * Undo step which modifies object tags.
 * 
 * Note: For reduced file size, this implementation copies, not calls,
 * ObjectModifyingUndoStep::saveImpl()/ObjectModifyingUndoStep::loadImpl().
 */
class ObjectTagsUndoStep : public ObjectModifyingUndoStep
{
public:
	ObjectTagsUndoStep(Map* map);
	
	virtual ~ObjectTagsUndoStep();
	
	virtual void addObject(int index);
	
	virtual UndoStep* undo();
	
protected:
	virtual void saveImpl(QXmlStreamWriter& xml) const;
	
	virtual void loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	
	typedef std::map<int, Object::Tags> ObjectTagsMap;
	
	ObjectTagsMap object_tags_map;
};


// ### ObjectModifyingUndoStep inline code ###

inline
int ObjectModifyingUndoStep::getPartIndex() const
{
	return part_index;
}


#endif
