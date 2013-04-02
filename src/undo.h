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
class MapColor;
class Object;

/**
 * Abstract base class for undo steps.
 * 
 * Stores information which is necessary for executing an undo step.
 * While executing the step, creates a new UndoStep for the corresponding
 * redo step.
 */
class UndoStep : public QObject
{
Q_OBJECT
public:
	/** Types of undo steps for identification. */
	enum Type
	{
		ReplaceObjectsUndoStepType = 0,
		DeleteObjectsUndoStepType = 1,
		AddObjectsUndoStepType = 2,
		SwitchSymbolUndoStepType = 3,
		SwitchDashesUndoStepType = 4,
		CombinedUndoStepType = 5
	};
	
	/** Constructs an undo step having the given type. */
	UndoStep(Type type);
	virtual ~UndoStep() {}
	
	/** Must undo the action and return a new UndoStep
	 *  which can redo the action again. */
	virtual UndoStep* undo() = 0;
	
	/** Must save the undo step to the file in the old "native" format. */
	virtual void save(QIODevice* file) = 0;
	/** Must load the undo step from the file in the old "native" format. */
	virtual bool load(QIODevice* file, int version) = 0;
	
	/** Must save the undo step to the stream in xml format. */
	void save(QXmlStreamWriter& xml);
	/** Must load the undo step from the stream in xml format. */
	static UndoStep* load(QXmlStreamReader& xml, void* owner, SymbolDictionary& symbol_dict);
	
	/**
	 * Returns if the step can still be undone. This must be true after
	 * generating the step (otherwise it would not make sense to generate it)
	 * but can change to false if an object the step depends on,
	 * which is not tracked by the undo system, is deleted.
	 * 
	 * Example: changing a map object's symbol to a different one,
	 * then deleting the first symbol. Then changing the symbol cannot be undone
	 * as the old symbol does not exist anymore.
	 */
	virtual bool isValid() const {return valid;}
	
	/** Returns the undo step's type. */
	inline Type getType() const {return type;}
	
	/**
	 * Constructs an undo step of the given type.
	 * The owner pointer can be used to pass in additional information,
	 * usually the Map in which the step will be.
	 */
	static UndoStep* getUndoStepForType(Type type, void* owner);
	
protected:
	virtual void saveImpl(QXmlStreamWriter& xml) const;
	virtual void loadImpl(QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	
	bool valid;
	Type type;
};

/**
 * Undo step which internally consists of one or more other UndoSteps (sub steps),
 * which it executes in order.
 */
class CombinedUndoStep : public UndoStep
{
Q_OBJECT
public:
	CombinedUndoStep(void* owner);
	virtual ~CombinedUndoStep();
	
	/** Returns the number of sub steps. */
	inline int getNumSubSteps() const {return (int)steps.size();}
	
	/** Adds the sub step. */
	inline void addSubStep(UndoStep* step) {steps.push_back(step);}
	
	/** Returns the i-th sub step. */
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

/** Keeps an undo and redo step list, thus storing a complete undo history. */
class UndoManager : public QObject
{
Q_OBJECT
public:
	/** Constructor.
	 * 
	 *  The owner pointer is a way to pass the map pointer to the steps
	 *  without making this class dependent on Map. */
	UndoManager(void* owner = NULL);
	inline void setOwner(void* owner) {this->owner = owner;}
	~UndoManager();
	
	/** Saves the UndoManager to the file in the old "native" format. */
	void save(QIODevice* file);
	/** Loads the UndoManager from the file in the old "native" format. */
	bool load(QIODevice* file, int version);
	
	/** Saves the undo steps to the file in xml format. */
	void saveUndo(QXmlStreamWriter& xml);
	/** Loads the undo steps from the file in xml format. */
	bool loadUndo(QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	/** Saves the redo steps to the file in xml format. */
	void saveRedo(QXmlStreamWriter& xml);
	/** Loads the redo steps from the file in xml format. */
	bool loadRedo(QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	
	/** Call this to add a new step resulting from an edit action. */
	void addNewUndoStep(UndoStep* step);
	
	/** Deletes all undo and redo steps. Can be necessary if changes are made
	 *  to objects which are not tracked by the undo system but related to it. */
	void clear(bool current_state_is_saved);
	
	/** Executes the most recently added undo step.
	 *  @param dialog_parent Optional QWidget parent for any dialogs
	 *      shown by the method.
	 *  @param done If set, will return whether an undo step actually has been
	 *      executed, or if not because of a problem.
	 * 
	 *  undo() and redo() return true if, as the result of the action,
	 *  the file is in the state where it was saved the last time */
	bool undo(QWidget* dialog_parent = NULL, bool* done = NULL);
	
	/** Executes the most recently added redo step.
	 *  See undo() for a description of the parameters. */
	bool redo(QWidget* dialog_parent = NULL, bool* done = NULL);
	
	/** Call this when the currently edited file is saved. */
	void notifyOfSave();
	
	inline int getNumUndoSteps() const {return (int)undo_steps.size();}
	inline UndoStep* getLastUndoStep() const {assert(getNumUndoSteps() > 0); return undo_steps[undo_steps.size() - 1];}
	inline int getNumRedoSteps() const {return (int)redo_steps.size();}
	inline UndoStep* getLastRedoStep() const {assert(getNumRedoSteps() > 0); return redo_steps[redo_steps.size() - 1];}
	
	/** The maximum number of undo steps stored in an UndoManager.
	 *  If more steps are added, the first steps will be deleted again.
	 * 
	 *  TODO: Make configurable (maybe by used memory instead of step count) */
	static const int max_undo_steps = 128;
	
signals:
	/** Emitted when the number of undo (or redo) steps changes
	 *  from 0 to positive or vice versa */
	void undoStepAvailabilityChanged();
	
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
