/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014, 2017, 2018 Kai Pastor
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


#ifndef OPENORIENTEERING_UNDO_MANAGER_H
#define OPENORIENTEERING_UNDO_MANAGER_H


#include <cstddef>
#include <memory>
#include <vector>

#include <QObject>

#include "core/symbols/symbol.h"

class QWidget;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {

class Map;
class UndoStep;


/**
 * Manages the history of steps for undoing and redoing changes to a map.
 * 
 * This API is intentionally similar to QUndoStack.
 * (QUndoStack is part of Qt since 4.2 and available under the GPL3.)
 */
class UndoManager : public QObject
{
Q_OBJECT
public:
	/**
	 * Constructs an empty undo manager.
	 */
	UndoManager(Map* map);
	
	/**
	 * Destroys the undo manager, deleting all managed steps.
	 */
	~UndoManager() override;
	
	
	/** 
	 * Deletes all undo and redo steps. 
	 * 
	 * Can be necessary if changes are made to objects which are not tracked
	 * by the undo system but related to it.
	 */
	void clear();
	
	
	/**
	 * Adds a new undo step to the manager.
	 * 
	 * All recorded redo steps will be deleted.
	 */
	void push(std::unique_ptr<UndoStep>&& step);
	
	
	/**
	 * Returns true iff valid undo steps are available.
	 */
	bool canUndo() const;
	
	/** 
	 * Executes the current undo step.
	 * 
	 * If the UndoManager is in loaded state, the user is offered the choice to
	 * cancel the operation.
	 * 
	 *  @param dialog_parent Optional QWidget parent for any dialogs
	 *      shown by the method.
	 *  @returns True if an undo step actually has been executed, false otherwise.
	 */
	bool undo(QWidget* dialog_parent = nullptr);
	
	
	/**
	 * Returns true iff valid redo steps are available.
	 */
	bool canRedo() const;
	
	/**
	 * Executes the current redo step.
	 */
	bool redo(QWidget* dialog_parent = nullptr);
	
	
	/**
	 * Returns true iff the current state is the clean state.
	 */
	bool isClean() const;
	
	/**
	 * Marks the current state as the clean state.
	 * 
	 * Whenever the manager returns to this state through the use of undo()/redo(),
	 * it emits the signal cleanChanged().
	 * 
	 * Emits cleanChanged(true) if the current state was not the clean state.
	 * 
	 * Call this when the currently edited file is loaded or saved.
	 */
	void setClean();
	
	
	/**
	 * Returns true iff the current state is the loaded state.
	 */
	bool isLoaded() const;
	
	/**
	 * Marks the current state as the loaded state.
	 * 
	 * When calling undo() while in loaded state, an extra warning will be displayed.
	 * 
	 * Call this when the currently edited file is loaded.
	 */
	void setLoaded();
	
	
	/**
	 * Returns the current number of undo steps.
	 * 
	 * This figure may include invalid steps.
	 * So canUndo() may return false even if undoStepCount() is positive.
	 */
	int undoStepCount() const;
	
	/**
	 * Returns the step performed by the next call to undo().
	 */
	UndoStep* nextUndoStep() const;
	
	
	/**
	 * Returns the current number of redo steps.
	 * 
	 * This figure may include invalid steps.
	 * So canRedo() may return false even if redoStepCount() is positive.
	 */
	int redoStepCount() const;
	
	/**
	 * Returns the step performed by the next call to redo().
	 */
	UndoStep* nextRedoStep() const;
	
	
	/**
	 * Saves the undo steps to the file in xml format.
	 */
	void saveUndo(QXmlStreamWriter& xml) const;
	
	/**
	 * Saves the undo steps to the file in xml format.
	 */
	void saveRedo(QXmlStreamWriter& xml) const;
	
	/**
	 * Loads the undo steps from the file in xml format.
	 * 
	 * Any existing undo steps and redo steps will be deleted first.
	 */
	void loadUndo(QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	
	/**
	 * Loads the redo steps from the file in xml format.
	 * 
	 * Any existing redo steps will be deleted first, but undo steps are left untouched.
	 */
	void loadRedo(QXmlStreamReader& xml, SymbolDictionary& symbol_dict);
	
	
	/**
	 * The maximum number of steps kept for undo() and redo(), respectively.
	 * 
	 * This limits the amount of memory occupied by undo steps.
	 * 
	 * @todo Make this configurable (maybe by used memory instead of step count)
	 */
	static constexpr std::size_t max_undo_steps = 128;
	
signals:
	/**
	 * This signal is emitted whenever the value of canUndo() changes.
	 */
	void canUndoChanged(bool can_undo);
	
	/**
	 * This signal is emitted whenever the value of canRedo() changes.
	 */
	void canRedoChanged(bool can_redo);
	
	/**
	 * This signal is emitted whenever the value of isClean() changes.
	 */
	void cleanChanged(bool clean);
	
	/**
	 * This signal is emitted whenever the value of isLoaded() changes.
	 */
	void loadedChanged(bool loaded);
	
protected:
	/**
	 * A list of UndoSteps.
	 */
	using StepList = std::vector<std::unique_ptr<UndoStep>>;
	
	/**
	 * Deletes all redo steps and removes them from undo_steps.
	 */
	void clearRedoSteps();
	
	/**
	 * Validates the list of steps available for undo().
	 * 
	 * In order to maintain the validness of current_index etc., this
	 * method does not remove elements from undo_steps.
	 * Instead, it replaces steps which are no longer reachable via valid steps,
	 * or which exceed the max_undo_steps limit, with invalid NoOpUndoStep objects,
	 * thus releasing the memory which was orginally occupied by now obsolete
	 * undo steps.
	 */
	void validateUndoSteps();
	
	/**
	 * Validates the list of steps available for redo().
	 * 
	 * This method removes elements from undo_steps which are no longer reachable
	 * via valid steps, or which exceed the max_undo_steps limit, with invalid
	 * NoOpUndoStep objects, thus releasing the memory which was orginally
	 * occupied by now obsolete undo steps.
	 */
	void validateRedoSteps();
	
	
	/**
	 * Keeps the state of an UndoManager.
	 * 
	 * Together with UndoManager::emitChangedSignals(), this allows to easily
	 * emit the correct signals after changes to the state of an UndoManager.
	 */
	struct State
	{
		bool is_loaded;
		bool is_clean;
		bool can_undo;
		bool can_redo;
		
		/**
		 * Constructs a state which is a snapshot of the given UndoManager.
		 */
		State(UndoManager const *manager);
	};
	
	/**
	 * Emits "changed" signals for all properties which are different from old_state.
	 */
	void emitChangedSignals(UndoManager::State const &old_state);
	
	/**
	 * Set the map's current part and selection from the given undo step.
	 * 
	 * This method relies on the step already being applied to the map,
	 * i.e. all affected parts and objects do exist.
	 */
	void updateMapState(const UndoStep* step) const;
	
private:
	StepList loadSteps(QXmlStreamReader& xml, SymbolDictionary& symbol_dict) const;
	
	/**
	 * The list of all steps available for undo() and redo().
	 * 
	 * @see current_index
	 */
	StepList undo_steps;
	
	/**
	 * The map which this UndoManager operates on.
	 */
	Map* const map;
	
	/**
	 * The index where push will place the next undo step.
	 * 
	 * The latest undo step (if any) is just before this index.
	 * The current redo step (if any) is just at this index.
	 * This redo step will be executed on the next call to redo().
	 */
	int current_index;
	
	/**
	 * The index of the clean state.
	 * 
	 * A negative value indicates that the state clean state is not reachable
	 * through undo() or redo().
	 * 
	 * @see setClean()
	 */
	int clean_state_index;
	
	/**
	 * The index of the loaded state.
	 * 
	 * A negative value indicates that the loaded clean state is not reachable
	 * through undo() or redo().
	 * 
	 * @see setLoaded()
	 */
	int loaded_state_index;
	
};


}  // namespace OpenOrienteering

#endif
