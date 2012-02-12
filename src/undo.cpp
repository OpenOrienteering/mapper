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


#include "undo.h"

#include <assert.h>

#include <QtGui>

#include "map_undo.h"

// ### UndoStep ###

UndoStep::UndoStep(Type type) : valid(true), type(type)
{
}

UndoStep* UndoStep::getUndoStepForType(UndoStep::Type type, void* owner)
{
	if (type == ReplaceObjectsUndoStepType)
		return new ReplaceObjectsUndoStep(reinterpret_cast<Map*>(owner));
	else if (type == DeleteObjectsUndoStepType)
		return new DeleteObjectsUndoStep(reinterpret_cast<Map*>(owner));
	else if (type == AddObjectsUndoStepType)
		return new AddObjectsUndoStep(reinterpret_cast<Map*>(owner));
	else if (type == SwitchSymbolUndoStepType)
		return new SwitchSymbolUndoStep(reinterpret_cast<Map*>(owner));
	else if (type == SwitchDashesUndoStepType)
		return new SwitchDashesUndoStep(reinterpret_cast<Map*>(owner));
	
	assert(false);
	return NULL;
}

// ### UndoManager ###

UndoManager::UndoManager(void* owner) : QObject(NULL), owner(owner)
{
	saved_step_index = 0;
	loaded_step_index = 0;
}
UndoManager::~UndoManager()
{
	clearUndoSteps();
	clearRedoSteps();
}

void UndoManager::save(QFile* file)
{
	saveSteps(undo_steps, file);
	saveSteps(redo_steps, file);
}
void UndoManager::saveSteps(std::deque< UndoStep* >& steps, QFile* file)
{
	// Delete all invalid steps so we do not have to deal with them
	int size = (int)steps.size();
	for (int i = size - 1; i >= 0; --i)
	{
		if (steps[i]->isValid())
			continue;
		
		for (int k = 0; k <= i; ++k)
		{
			delete steps[0];
			steps.pop_front();
		}
		size = (int)steps.size();
		break;
	}
	
	file->write((char*)&size, sizeof(int));
	for (int i = 0; i < size; ++i)
	{
		int type = (int)steps[i]->getType();
		file->write((char*)&type, sizeof(int));
		steps[i]->save(file);
	}
}
bool UndoManager::load(QFile* file)
{
	clearUndoSteps();
	clearRedoSteps();
	
	if (!loadSteps(undo_steps, file))
		return false;
	if (!loadSteps(redo_steps, file))
		return false;
	saved_step_index = 0;
	loaded_step_index = 0;
	return true;
}
bool UndoManager::loadSteps(std::deque< UndoStep* >& steps, QFile* file)
{
	int size;
	file->read((char*)&size, sizeof(int));
	steps.resize(size);
	for (int i = 0; i < size; ++i)
	{
		int type;
		file->read((char*)&type, sizeof(int));
		steps[i] = UndoStep::getUndoStepForType((UndoStep::Type)type, owner);
		if (!steps[i]->load(file))
			return false;
	}
	return true;
}

void UndoManager::addNewUndoStep(UndoStep* step)
{
	clearRedoSteps();
	addUndoStep(step);
	
	if (saved_step_index > 0)
		saved_step_index = -max_undo_steps - 1;
	else
		--saved_step_index;
	
	if (loaded_step_index > 0)
		loaded_step_index = -max_undo_steps - 1;
	else
		--loaded_step_index;
	
	if (undo_steps.size() == 1)
		emit(undoStepAvailabilityChanged());
}

void UndoManager::clear()
{
	bool have_steps = !undo_steps.empty() || !redo_steps.empty();
	clearUndoSteps();
	clearRedoSteps();
	saved_step_index = -1;
	loaded_step_index = -1;
	if (have_steps)
		emit(undoStepAvailabilityChanged());
}

bool UndoManager::undo(QWidget* dialog_parent, bool* done)
{
	assert(getNumUndoSteps() > 0);
	
	if (!undo_steps[undo_steps.size() - 1]->isValid())
	{
		QMessageBox::warning(dialog_parent, tr("Error"), tr("Cannot undo because the last undo step became invalid. This can for example happen if you change the symbol of an object to another and then delete the old symbol."));
		if (done)
			*done = false;
		return saved_step_index == 0;
	}
	if (loaded_step_index == 0)
	{
		if (QMessageBox::warning(dialog_parent, tr("Confirmation"), tr("Undoing this step will go beyond the point where the file was loaded. Are you sure?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
		{
			if (done)
				*done = false;
			return saved_step_index == 0;
		}
	}
	
	UndoStep* step = undo_steps[undo_steps.size() - 1];
	undo_steps.pop_back();
	
	++saved_step_index;
	++loaded_step_index;
	
	UndoStep* redo_step = step->undo();
	delete step;
	addRedoStep(redo_step);
	
	if (undo_steps.size() == 0 || redo_steps.size() == 1)
		emit(undoStepAvailabilityChanged());
	
	if (done)
		*done = true;
	
	return saved_step_index == 0;
}
bool UndoManager::redo(QWidget* dialog_parent, bool* done)
{
	assert(getNumRedoSteps() > 0);
	
	if (!redo_steps[redo_steps.size() - 1]->isValid())
	{
		QMessageBox::warning(dialog_parent, tr("Error"), tr("Cannot redo because the first redo step became invalid. This can for example happen if you delete the symbol of an object you have drawn."));
		if (done)
			*done = false;
		return saved_step_index == 0;
	}
	
	UndoStep* step = redo_steps[redo_steps.size() - 1];
	redo_steps.pop_back();
	
	--saved_step_index;
	--loaded_step_index;
	
	UndoStep* undo_step = step->undo();
	delete step;
	addUndoStep(undo_step);
	
	if (undo_steps.size() == 1 || redo_steps.size() == 0)
		emit(undoStepAvailabilityChanged());
	
	if (done)
		*done = true;
	
	return saved_step_index == 0;
}

void UndoManager::notifyOfSave()
{
	saved_step_index = 0;
}

void UndoManager::addUndoStep(UndoStep* step)
{
	undo_steps.push_back(step);
	while ((int)undo_steps.size() > max_undo_steps)
	{
		delete undo_steps[0];
		undo_steps.pop_front();
	}
}
void UndoManager::addRedoStep(UndoStep* step)
{
	redo_steps.push_back(step);
	while ((int)redo_steps.size() > max_undo_steps)
	{
		delete redo_steps[0];
		redo_steps.pop_front();
	}
}

void UndoManager::clearUndoSteps()
{
	int size = (int)undo_steps.size();
	for (int i = 0; i < size; ++i)
		delete undo_steps[i];
	undo_steps.clear();
}
void UndoManager::clearRedoSteps()
{
	int size = (int)redo_steps.size();
	for (int i = 0; i < size; ++i)
		delete redo_steps[i];
	redo_steps.clear();
}

#include "moc_undo.cpp"
