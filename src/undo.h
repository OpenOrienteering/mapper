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


#ifndef _OPENORIENTEERING_UNDO_H_
#define _OPENORIENTEERING_UNDO_H_

#include <QObject>

#include <cassert>
#include <deque>
#include <vector>

#include "symbol.h"

QT_BEGIN_NAMESPACE
class QIODevice;
class QXmlStreamReader;
class QXmlStreamWriter;
QT_END_NAMESPACE

class Map;
struct MapColor;
class Object;

class UndoStep : public QObject
{
Q_OBJECT
public:
	enum Type
	{
		ReplaceObjectsUndoStepType = 0,
		DeleteObjectsUndoStepType = 1,
		AddObjectsUndoStepType = 2,
		SwitchSymbolUndoStepType = 3,
		SwitchDashesUndoStepType = 4,
		CombinedUndoStepType = 5
	};
	
	UndoStep(Type type);
	virtual ~UndoStep() {}
	
	/// Must undo the action and generate another UndoStep that can redo the action again
	virtual UndoStep* undo() = 0;
	
	virtual void save(QIODevice* file) = 0;
	virtual bool load(QIODevice* file, int version) = 0;
	
	void save(QXmlStreamWriter& xml);
	static UndoStep* load(QXmlStreamReader& xml, void* owner, SymbolDictionary& symbol_dict);
	
	/// Returns if the step can still be undone. This must be true after generating the step
	/// (otherwise it would not make sense to generate it) but can change to false if an object the step depends on,
	/// which is not tracked by the undo system, is deleted. Example: changing a map object's symbol to a different one, then deleting the first one.
	virtual bool isValid() const {return valid;}
	
	inline Type getType() const {return type;}
	
	static UndoStep* getUndoStepForType(Type type, void* owner);
	
protected:
	virtual void saveImpl(QXmlStreamWriter& xml) const;
	virtual void loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	
	bool valid;
	Type type;
};

class CombinedUndoStep : public UndoStep
{
Q_OBJECT
public:
	CombinedUndoStep(void* owner);
	virtual ~CombinedUndoStep();
	
	inline int getNumSubSteps() const {return (int)steps.size();}
	inline void addSubStep(UndoStep* step) {steps.push_back(step);}
	inline UndoStep* getSubStep(int i) {return steps[i];}
	
	virtual UndoStep* undo();
	virtual void save(QIODevice* file);
	virtual bool load(QIODevice* file, int version);
	
	virtual bool isValid() const;
	
protected:
	virtual void saveImpl(QXmlStreamWriter& xml) const;
	virtual void loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	
	std::vector<UndoStep*> steps;
	void* owner;
};

class UndoManager : public QObject
{
Q_OBJECT
public:
	/// The owner pointer is a way to pass the map pointer to the steps without making this class dependent on Map
	UndoManager(void* owner = NULL);
	inline void setOwner(void* owner) {this->owner = owner;}
	~UndoManager();
	
	void save(QIODevice* file);
	bool load(QIODevice* file, int version);
	
	void saveUndo(QXmlStreamWriter& xml);
	bool loadUndo(QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	void saveRedo(QXmlStreamWriter& xml);
	bool loadRedo(QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	
	/// Call this to add a new step resulting from an edit action
	void addNewUndoStep(UndoStep* step);
	
	/// Deletes all undo and redo steps. Can be necessary if changes are made to objects which are not tracked by the undo system but related to it.
	void clear(bool current_state_is_saved);
	
	/// undo() and redo() return true if, as the result of the action, the file is in the state where it was saved the last time
	bool undo(QWidget* dialog_parent = NULL, bool* done = NULL);
	bool redo(QWidget* dialog_parent = NULL, bool* done = NULL);
	
	/// Call this when the currently edited file is saved
	void notifyOfSave();
	
	inline int getNumUndoSteps() const {return (int)undo_steps.size();}
	inline UndoStep* getLastUndoStep() const {assert(getNumUndoSteps() > 0); return undo_steps[undo_steps.size() - 1];}
	inline int getNumRedoSteps() const {return (int)redo_steps.size();}
	inline UndoStep* getLastRedoStep() const {assert(getNumRedoSteps() > 0); return redo_steps[redo_steps.size() - 1];}
	
	static const int max_undo_steps = 128;	// TODO: Make configurable (maybe by used memory instead of step count)
	
signals:
	void undoStepAvailabilityChanged();	// Emitted when the number of undo (or redo) steps changes from 0 to positive or vice versa
	
private:
	void addUndoStep(UndoStep* step);
	void addRedoStep(UndoStep* step);
	bool clearUndoSteps();	// returns if at least one step was deleted
	bool clearRedoSteps();	// returns if at least one step was deleted
	void validateSteps(std::deque<UndoStep*>& steps);
	void saveSteps(std::deque<UndoStep*>& steps, QIODevice* file);
	bool loadSteps(std::deque< UndoStep* >& steps, QIODevice* file, int version);
	void saveSteps(std::deque< UndoStep* >& steps, QXmlStreamWriter& xml);
	bool loadSteps(std::deque< UndoStep* >& steps, QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	
	int saved_step_index;				// 0 would be the current state, negative indices stand for the undo steps, positive indices for the redo steps
	int loaded_step_index;				// indexing like for saved_step_index
	std::deque<UndoStep*> undo_steps;	// the last item is the first step to undo
	std::deque<UndoStep*> redo_steps;	// the last item is the first step to redo
	
	void* owner;
};

#endif
