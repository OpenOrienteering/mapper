/*
 *    Copyright 2014 Kai Pastor
 *    Copyright 2026 Matthias Kühlewein
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

#include <algorithm>
#include <initializer_list>
#include <memory>

#include <QtTest>
#include <QBuffer>
#include <QIODevice>

#include "core/map.h"
#include "core/objects/object.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"
#include "fileformats/file_format.h"
#include "fileformats/file_import_export.h"
#include "fileformats/xml_file_format.h"
#include "undo/object_undo.h"
#include "undo/undo.h"
#include "undo/undo_manager.h"


using namespace OpenOrienteering;

namespace
{
	bool saveAndLoadMap(const Map& input, Map& out)
	{
		XMLFileFormat format;
		auto exporter = format.makeExporter({}, &input, nullptr);
		auto importer = format.makeImporter({}, &out, nullptr);
		if (exporter && importer)
		{
			QBuffer buffer;
			exporter->setDevice(&buffer);
			importer->setDevice(&buffer);
			if (buffer.open(QIODevice::ReadWrite)
				&& exporter->doExport()
				&& buffer.seek(0)
				&& importer->doImport())
			{
				return true;  // success
			}
		}
		out.reset();  // failure
		return false;
	}
}


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


// test
void UndoManagerTest::testSaveAndLoad()
{
	Map map;
	map.undoManager().max_undo_steps = 3;
	
	auto point_symbol = new PointSymbol();
	point_symbol->setRotatable(true);
	map.addSymbol(point_symbol, 0);
	
	auto addObject = [&point_symbol](Map* map, double rotation)
	{
		auto object = new PointObject(point_symbol);
		object->setRotation(rotation);
		auto undo_step = new DeleteObjectsUndoStep(map);
		map->addObject(object);
		undo_step->addObject(map->getCurrentPart()->findObjectIndex(object));
		map->push(undo_step);
	};
	
	for(double rotation : {1.0, 2.0, 3.0})
	{
		addObject(&map, rotation);
	}
	QCOMPARE(map.getNumObjects(), 3);
	auto existsObject = [](const Map* map, double rotation) { 
		return map->existsObject([rotation](auto const* o) {
			return qFuzzyCompare(o->getRotation(), rotation);
		});
	};
	QVERIFY(existsObject(&map, 1.0));
	auto existsAllObjects = [&existsObject](const Map* map, std::initializer_list<double> members) {
		return std::all_of(begin(members), end(members), [&existsObject, map](auto& member) {
			return existsObject(map, member);
		});
	};
	QVERIFY(existsAllObjects(&map, {1.0, 2.0, 3.0}));
	QVERIFY(map.undoManager().canUndo());
	QVERIFY(!map.undoManager().canRedo());
	QCOMPARE(map.undoManager().undoStepCount(), 3);
	
	map.undoManager().undo();
	QCOMPARE(map.getNumObjects(), 2);
	QVERIFY(map.undoManager().canUndo());
	QVERIFY(map.undoManager().canRedo());
	QCOMPARE(map.undoManager().undoStepCount(), 2);
	QCOMPARE(map.undoManager().redoStepCount(), 1);
	
	map.undoManager().redo();
	QCOMPARE(map.getNumObjects(), 3);
	QVERIFY(map.undoManager().canUndo());
	QVERIFY(!map.undoManager().canRedo());
	QCOMPARE(map.undoManager().undoStepCount(), 3);
	
	// add another object, causing the oldest undo step to be deleted
	addObject(&map, 4.0);
	while (map.undoManager().canUndo())
	{
		map.undoManager().undo();
	}
	QCOMPARE(map.getNumObjects(), 1);
	QVERIFY(existsAllObjects(&map, {1.0}));
	while (map.undoManager().canRedo())
	{
		map.undoManager().redo();
	}
	QCOMPARE(map.getNumObjects(), 4);
	QVERIFY(existsAllObjects(&map, {1.0, 2.0, 3.0, 4.0}));
	
	// Normal save and load cycle with 3 undo steps and 0 redo steps
	Map reloaded_map;
	reloaded_map.undoManager().max_undo_steps = 3;
	QVERIFY(saveAndLoadMap(map, reloaded_map));
	QCOMPARE(reloaded_map.getNumObjects(), 4);
	QVERIFY(existsAllObjects(&reloaded_map, {1.0, 2.0, 3.0, 4.0}));
	reloaded_map.undoManager().loaded_state_index = 0;	// avoid message box "Undoing this step will go beyond the point..."
	QVERIFY(reloaded_map.undoManager().canUndo());
	QVERIFY(!reloaded_map.undoManager().canRedo());
	QCOMPARE(reloaded_map.undoManager().undoStepCount(), 3);
	
	while (reloaded_map.undoManager().canUndo())
	{
		reloaded_map.undoManager().undo();
	}
	QCOMPARE(reloaded_map.getNumObjects(), 1);
	QVERIFY(existsAllObjects(&reloaded_map, {1.0}));
	
	// Save and load cycle with 3 undo steps and 0 redo steps
	// where number of undo steps in loaded map is larger than max. undo/redo steps,
	// thus reducing the number of stored undo steps
	Map reloaded_map2;
	reloaded_map2.undoManager().max_undo_steps = 2;
	QVERIFY(saveAndLoadMap(map, reloaded_map2));
	QCOMPARE(reloaded_map2.getNumObjects(), 4);
	QVERIFY(existsAllObjects(&reloaded_map2, {1.0, 2.0, 3.0, 4.0}));
	reloaded_map2.undoManager().loaded_state_index = 0;	// avoid message box "Undoing this step will go beyond the point..."
	QVERIFY(reloaded_map2.undoManager().canUndo());
	QVERIFY(!reloaded_map2.undoManager().canRedo());
	QCOMPARE(reloaded_map2.undoManager().undoStepCount(), 2);
	
	reloaded_map2.undoManager().undo();
	QVERIFY(existsAllObjects(&reloaded_map2, {1.0, 2.0, 3.0}));
	reloaded_map2.undoManager().undo();
	QCOMPARE(reloaded_map2.getNumObjects(), 2);
	QVERIFY(existsAllObjects(&reloaded_map2, {1.0, 2.0}));
	
	// now create source map with 0 undo steps and 3 redo steps
	while (map.undoManager().canUndo())
	{
		map.undoManager().undo();
	}
	QCOMPARE(map.getNumObjects(), 1);
	
	// Normal save and load cycle with 0 undo steps and 3 redo steps
	Map reloaded_map3;
	reloaded_map3.undoManager().max_undo_steps = 3;
	QVERIFY(saveAndLoadMap(map, reloaded_map3));
	QCOMPARE(reloaded_map3.getNumObjects(), 1);
	QVERIFY(existsAllObjects(&reloaded_map3, {1.0}));
	QVERIFY(!reloaded_map3.undoManager().canUndo());
	QVERIFY(reloaded_map3.undoManager().canRedo());
	QCOMPARE(reloaded_map3.undoManager().redoStepCount(), 3);
	reloaded_map3.undoManager().redo();
	QVERIFY(existsAllObjects(&reloaded_map3, {1.0, 2.0}));
	reloaded_map3.undoManager().redo();
	QVERIFY(existsAllObjects(&reloaded_map3, {1.0, 2.0, 3.0}));
	reloaded_map3.undoManager().redo();
	QVERIFY(existsAllObjects(&reloaded_map3, {1.0, 2.0, 3.0, 4.0}));
	
	// Save and load cycle with 0 undo steps and 3 redo steps
	// where number of redo steps in loaded map is larger than max. undo/redo steps,
	// thus reducing the number of stored redo steps
	Map reloaded_map4;
	reloaded_map4.undoManager().max_undo_steps = 2;
	QVERIFY(saveAndLoadMap(map, reloaded_map4));
	QCOMPARE(reloaded_map4.getNumObjects(), 1);
	QVERIFY(existsAllObjects(&reloaded_map4, {1.0}));
	QVERIFY(!reloaded_map4.undoManager().canUndo());
	QVERIFY(reloaded_map4.undoManager().canRedo());
	QCOMPARE(reloaded_map4.undoManager().redoStepCount(), 2);
	reloaded_map4.undoManager().redo();
	QVERIFY(existsAllObjects(&reloaded_map4, {1.0, 2.0}));
	reloaded_map4.undoManager().redo();
	QVERIFY(existsAllObjects(&reloaded_map4, {1.0, 2.0, 3.0}));
}

/*
 * We don't need a real GUI window.
 */
namespace {
	auto const Q_DECL_UNUSED qpa_selected = qputenv("QT_QPA_PLATFORM", "offscreen");  // clazy:exclude=non-pod-global-static
}


QTEST_MAIN(UndoManagerTest)
