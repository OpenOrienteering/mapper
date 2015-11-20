/*
 *    Copyright 2012, 2013 Thomas Schöps
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

#include "undo.h"

/** Base class for all map undo steps */
class MapUndoStep : public UndoStep
{
Q_OBJECT
public:
	/** Creates a MapUndoStep for the given map with the given type */
	MapUndoStep(Map* map, Type type);
	
	/** Loads the MapUndoStep in the old "native" file format from the given file */
	virtual bool load(QIODevice* file, int version);
	
	/** Returns the map part affected by the MapUndoStep */
	int getPart() const {return part;}
	
	/** Returns a list of affected objects in out. */
	virtual void getAffectedOutcome(std::vector<Object*>& out) const;
	
protected:
	virtual void saveImpl(QXmlStreamWriter& xml) const;
	virtual void loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	
	int part;
	/** Indices of the existing objects that are affected */
	std::vector<int> affected_objects;
	Map* map;
};

/**
 * Base class for undo steps which store an array of objects internally.
 * Takes care of correctly reacting to symbol changes in the map, which are not
 * covered by the undo / redo system.
 */
class ObjectContainingUndoStep : public MapUndoStep
{
Q_OBJECT
public:
	/** Constructor, calls MapUndoStep(map, type). */
	ObjectContainingUndoStep(Map* map, Type type);
	
	/** Destructor. */
	virtual ~ObjectContainingUndoStep();
	
	/** Adds an object to the undo step with given index. */
	void addObject(int existing_index, Object* object);
	/** Adds an object to the undo step with the index of the existing object. */
	void addObject(Object* existing, Object* object);
	
	/** Loads the MapUndoStep in the old "native" file format from the given file */
	virtual bool load(QIODevice* file, int version);
	
	/** Returns a list of affected objects in out. */
	virtual void getAffectedOutcome(std::vector< Object* >& out) const {out = objects;}
	/** Returns true if no objects are stored in the undo step. */
	inline bool isEmpty() const {return objects.empty();}
	
public slots:
	/** Adapts the symbol pointers of objects referencing the changed symbol */
	virtual void symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol);
	
	/**
	 * Invalidates the undo step if a contained object
	 * referenced the deleted symbol
	 */
	virtual void symbolDeleted(int pos, Symbol* old_symbol);
	
protected:
	virtual void saveImpl(QXmlStreamWriter& xml) const;
	virtual void loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	
	std::vector<Object*> objects;
};

/** Map undo step which replaces all affected objects by another set of objects. */
class ReplaceObjectsUndoStep : public ObjectContainingUndoStep
{
Q_OBJECT
public:
	ReplaceObjectsUndoStep(Map* map);
	virtual UndoStep* undo();
};

/**
 * Map undo step which deletes the referenced objects.
 * 
 * Take care of correct application order when mixing with an add step to
 * make sure that the object indices are preserved.
 */
class DeleteObjectsUndoStep : public MapUndoStep
{
Q_OBJECT
public:
	DeleteObjectsUndoStep(Map* map);
	
	void addObject(int index);
	virtual UndoStep* undo();
	
	virtual void getAffectedOutcome(std::vector< Object* >& out) const {out.clear();}
	inline bool isEmpty() const {return affected_objects.empty();}
};

/**
 * Map undo step which adds the contained objects.
 * 
 * Take care of correct application order when mixing with a delete step to
 * make sure that the object indices are preserved.
 */
class AddObjectsUndoStep : public ObjectContainingUndoStep
{
Q_OBJECT
public:
	AddObjectsUndoStep(Map* map);
	virtual UndoStep* undo();
	
	/**
	 * Removes all contained objects from the map.
	 * This can be useful after constructing the undo step,
	 * as it is often impractical to remove the objects directly
	 * when adding them as the indexing would be changed this way.
	 */
	void removeContainedObjects(bool emit_selection_changed);
protected:
	static bool sortOrder(const std::pair<int, int>& a, const std::pair<int, int>& b);
};

/** Map undo step which changes the symbols of referenced objects. */
class SwitchSymbolUndoStep : public MapUndoStep
{
Q_OBJECT
public:
	SwitchSymbolUndoStep(Map* map);
	
	void addObject(int index, Symbol* target_symbol);
	virtual UndoStep* undo();
	
	virtual bool load(QIODevice* file, int version);
	
public slots:
	virtual void symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol);
	virtual void symbolDeleted(int pos, Symbol* old_symbol);
	
protected:
	virtual void saveImpl(QXmlStreamWriter& xml) const;
	virtual void loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	
	std::vector<Symbol*> target_symbols;
};

/**
 * Map undo step which reverses the directions of referenced objects,
 * thereby switching the side of any line mid symbols (often dashes of fences).
 */
class SwitchDashesUndoStep : public MapUndoStep
{
Q_OBJECT
public:
	SwitchDashesUndoStep(Map* map);
	
	void addObject(int index);
	virtual UndoStep* undo();
};

#endif
