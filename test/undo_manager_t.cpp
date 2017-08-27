/*
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

#include "undo_manager_t.h"

#include <QtTest>

#include "undo/undo.h"
#include "undo/undo_manager.h"

class Map;


// test
void UndoManagerTest::testUndoRedo()
{
	Map* const map = nullptr;
	UndoManager undo_manager(map);
	connect(&undo_manager, &UndoManager::loadedChanged,  this, &UndoManagerTest::loadedChanged);
	connect(&undo_manager, &UndoManager::cleanChanged,   this, &UndoManagerTest::cleanChanged);
	connect(&undo_manager, &UndoManager::canRedoChanged, this, &UndoManagerTest::canRedoChanged);
	connect(&undo_manager, &UndoManager::canUndoChanged, this, &UndoManagerTest::canUndoChanged);
	
	undo_manager.setClean();
	QVERIFY(undo_manager.isClean());
	QVERIFY(!undo_manager.canRedo());
	QVERIFY(!undo_manager.canUndo());
	
	undo_manager.setLoaded();
	QVERIFY(undo_manager.isLoaded());
	
	// State: [loaded,clean,current]:EOL
	
	// Add undo step A
	resetAllChanged();
	undo_manager.push(std::unique_ptr<UndoStep>(new NoOpUndoStep(map, true)));
	
	// State: [current, loaded]:A, [clean]:EOL
	QVERIFY(loaded_changed);
	QVERIFY(!loaded);
	QVERIFY(!undo_manager.isLoaded());
	QVERIFY(clean_changed);
	QVERIFY(!clean);
	QVERIFY(!undo_manager.isClean());
	QVERIFY(can_undo_changed);
	QVERIFY(can_undo);
	QVERIFY(undo_manager.canUndo());
	QVERIFY(!can_redo_changed);
	QVERIFY(!undo_manager.canRedo());
	
	// Add undo step B
	resetAllChanged();
	undo_manager.push(std::unique_ptr<UndoStep>(new NoOpUndoStep(map, true)));
	
	// State: [loaded,clean]:A, B, [current]:EOL
	QVERIFY(!loaded_changed);
	QVERIFY(!undo_manager.isLoaded());
	QVERIFY(!clean_changed);
	QVERIFY(!undo_manager.isClean());
	QVERIFY(!can_undo_changed);
	QVERIFY(undo_manager.canUndo());
	QVERIFY(!can_redo_changed);
	QVERIFY(!undo_manager.canRedo());
	
	// Perform undo
	resetAllChanged();
	QVERIFY(undo_manager.undo());
	
	// State: [loaded,clean]:A, [current]:B, EOL
	QVERIFY(!loaded_changed);
	QVERIFY(!undo_manager.isLoaded());
	QVERIFY(!clean_changed);
	QVERIFY(!undo_manager.isClean());
	QVERIFY(!can_undo_changed);
	QVERIFY(undo_manager.canUndo());
	QVERIFY(can_redo_changed);
	QVERIFY(can_redo);
	QVERIFY(undo_manager.canRedo());
	
	// Perform another undo
	resetAllChanged();
	QVERIFY(undo_manager.undo());
	
	// State: [loaded,clean,current]:A, B, EOL
	QVERIFY(loaded_changed);
	QVERIFY(loaded);
	QVERIFY(undo_manager.isLoaded());
	QVERIFY(clean_changed);
	QVERIFY(clean);
	QVERIFY(undo_manager.isClean());
	QVERIFY(can_undo_changed);
	QVERIFY(!can_undo);
	QVERIFY(!undo_manager.canUndo());
	QVERIFY(!can_redo_changed);
	QVERIFY(undo_manager.canRedo());
	
	// Perform redo
	resetAllChanged();
	QVERIFY(undo_manager.redo());
	
	// State: [loaded,clean]:A, [current]:B, EOL
	QVERIFY(loaded_changed);
	QVERIFY(!loaded);
	QVERIFY(!undo_manager.isLoaded());
	QVERIFY(clean_changed);
	QVERIFY(!clean);
	QVERIFY(!undo_manager.isClean());
	QVERIFY(can_undo_changed);
	QVERIFY(can_undo);
	QVERIFY(undo_manager.canUndo());
	QVERIFY(!can_redo_changed);
	QVERIFY(undo_manager.canRedo());
	
	// Mark as clean
	resetAllChanged();
	undo_manager.setClean();
	
	// State: [loaded]:A, [clean,current]:B, EOL
	QVERIFY(clean_changed);
	QVERIFY(clean);
	QVERIFY(undo_manager.isClean());
	QVERIFY(!can_undo_changed);
	QVERIFY(!can_redo_changed);
	
	// Add undo step C
	resetAllChanged();
	undo_manager.push(std::unique_ptr<UndoStep>(new NoOpUndoStep(map, true)));
	
	// State: [loaded]:A, [clean]:B, [current]:C, EOL
	QVERIFY(!loaded_changed);
	QVERIFY(!undo_manager.isLoaded());
	QVERIFY(clean_changed);
	QVERIFY(!clean);
	QVERIFY(!undo_manager.isClean());
	QVERIFY(!can_undo_changed);
	QVERIFY(undo_manager.canUndo());
	QVERIFY(can_redo_changed);
	QVERIFY(!can_redo);
	QVERIFY(!undo_manager.canRedo());
	
	// Perform undo
	resetAllChanged();
	QVERIFY(undo_manager.undo());
	
	// State: [loaded]:A, [clean,current]:B, C, EOL
	QVERIFY(!loaded_changed);
	QVERIFY(!undo_manager.isLoaded());
	QVERIFY(clean_changed);
	QVERIFY(clean);
	QVERIFY(undo_manager.isClean());
	QVERIFY(!can_undo_changed);
	QVERIFY(undo_manager.canUndo());
	QVERIFY(can_redo_changed);
	QVERIFY(can_redo);
	QVERIFY(undo_manager.canRedo());
	
	// Perform another undo
	resetAllChanged();
	QVERIFY(undo_manager.undo());
	
	// State: [loaded,current]:A, [clean]:B, C, EOL
	QVERIFY(loaded_changed);
	QVERIFY(loaded);
	QVERIFY(undo_manager.isLoaded());
	QVERIFY(clean_changed);
	QVERIFY(!clean);
	QVERIFY(!undo_manager.isClean());
	QVERIFY(can_undo_changed);
	QVERIFY(!can_undo);
	QVERIFY(!undo_manager.canUndo());
	QVERIFY(!can_redo_changed);
	QVERIFY(undo_manager.canRedo());
	
	// Add undo step D: clean state now no longer reachable
	resetAllChanged();
	undo_manager.push(std::unique_ptr<UndoStep>(new NoOpUndoStep(map, true)));
	
	// State: [loaded,current]:D, EOL [clean:invalid]
	QVERIFY(loaded_changed);
	QVERIFY(!loaded);
	QVERIFY(!undo_manager.isLoaded());
	QVERIFY(!clean_changed);
	QVERIFY(!undo_manager.isClean());
	QVERIFY(can_undo_changed);
	QVERIFY(can_undo);
	QVERIFY(undo_manager.canUndo());
	QVERIFY(can_redo_changed);
	QVERIFY(!can_redo);
	QVERIFY(!undo_manager.canRedo());
}

void UndoManagerTest::resetAllChanged()
{
	loaded_changed   = false;
	clean_changed    = false;
	can_undo_changed = false;
	can_redo_changed = false;
}

// slot
void UndoManagerTest::loadedChanged(bool loaded)
{
	this->loaded = loaded;
	this->loaded_changed = true;
}

// slot
void UndoManagerTest::cleanChanged(bool clean)
{
	this->clean = clean;
	this->clean_changed = true;
}

// slot
void UndoManagerTest::canUndoChanged(bool can_undo)
{
	this->can_undo = can_undo;
	this->can_undo_changed = true;
}

// slot
void UndoManagerTest::canRedoChanged(bool can_redo)
{
	this->can_redo = can_redo;
	this->can_redo_changed = true;
}


QTEST_GUILESS_MAIN(UndoManagerTest)
