/*
 *    Copyright 2012 Thomas Schöps
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

/// Base class for all map undo steps
class MapUndoStep : public UndoStep
{
Q_OBJECT
public:
	MapUndoStep(Map* map, Type type);
	
	virtual void save(QIODevice* file);
	virtual bool load(QIODevice* file, int version);
	
	int getPart() const {return part;}
	virtual void getAffectedOutcome(std::vector<Object*>& out) const;
	
protected:
	int part;
	std::vector<int> affected_objects;	// indices of the existing objects that are affected
	Map* map;
};

/// Base class for undo steps which contain an array of objects
class ObjectContainingUndoStep : public MapUndoStep
{
Q_OBJECT
public:
	ObjectContainingUndoStep(Map* map, Type type);
	virtual ~ObjectContainingUndoStep();
	
	void addObject(int existing_index, Object* object);
	void addObject(Object* existing, Object* object);
	
	virtual void save(QIODevice* file);
	virtual bool load(QIODevice* file, int version);
	
	virtual void getAffectedOutcome(std::vector< Object* >& out) const {out = objects;}
	
public slots:
	virtual void symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol);
	virtual void symbolDeleted(int pos, Symbol* old_symbol);
	
protected:
	std::vector<Object*> objects;
};

/// Undo step which replaces all affected objects with another set of objects
class ReplaceObjectsUndoStep : public ObjectContainingUndoStep
{
Q_OBJECT
public:
	ReplaceObjectsUndoStep(Map* map);
	virtual UndoStep* undo();
};

class DeleteObjectsUndoStep : public MapUndoStep
{
Q_OBJECT
public:
	DeleteObjectsUndoStep(Map* map);
	
	void addObject(int index);
	virtual UndoStep* undo();
	
	virtual void getAffectedOutcome(std::vector< Object* >& out) const {out.clear();}
};

class AddObjectsUndoStep : public ObjectContainingUndoStep
{
Q_OBJECT
public:
	AddObjectsUndoStep(Map* map);
	virtual UndoStep* undo();
protected:
	static bool sortOrder(const std::pair<int, int>& a, const std::pair<int, int>& b);
};

class SwitchSymbolUndoStep : public MapUndoStep
{
Q_OBJECT
public:
	SwitchSymbolUndoStep(Map* map);
	
	void addObject(int index, Symbol* target_symbol);
	virtual UndoStep* undo();
	
	virtual void save(QIODevice* file);
	virtual bool load(QIODevice* file, int version);
	
public slots:
	virtual void symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol);
	virtual void symbolDeleted(int pos, Symbol* old_symbol);
	
protected:
	std::vector<Symbol*> target_symbols;
};

class SwitchDashesUndoStep : public MapUndoStep
{
Q_OBJECT
public:
	SwitchDashesUndoStep(Map* map);
	
	void addObject(int index);
	virtual UndoStep* undo();
};

#endif
