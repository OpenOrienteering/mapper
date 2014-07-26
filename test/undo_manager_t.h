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

#ifndef _OPENORIENTEERING_UNDO_MANAGER_T_H
#define _OPENORIENTEERING_UNDO_MANAGER_T_H

#include <QtTest/QtTest>

#include "../src/undo_manager.h"


/**
 * @test Tests UndoManager.
 */
class UndoManagerTest : public QObject
{
	Q_OBJECT
	
private slots:
	/**
	 * Performs actions on an UndoManager and observes its behaviour.
	 */
	void testUndoRedo();
	
private:
	bool clean_changed;
	bool clean;
	
	bool loaded_changed;
	bool loaded;
	
	bool can_redo_changed;
	bool can_redo;
	
	bool can_undo_changed;
	bool can_undo;
	
protected:
	/**
	 * Resets all flags named *_changed.
	 */
	void resetAllChanged();
	
protected slots:
	/**
	 * Observes UndoManager::loadedChanged(bool) signals.
	 */
	void loadedChanged(bool loaded);
	
	/**
	 * Observes UndoManager::cleanChanged(bool) signals.
	 */
	void cleanChanged(bool clean);
	
	/**
	 * Observes UndoManager::canUndoChanged(bool) signals.
	 */
	void canUndoChanged(bool can_undo);
	
	/**
	 * Observes UndoManager::canRedoChanged(bool) signals.
	 */
	void canRedoChanged(bool can_redo);
};

#endif // _OPENORIENTEERING_UNDO_MANAGER_T_H
