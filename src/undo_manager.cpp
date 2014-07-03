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


#include "undo_manager.h"

#include <QMessageBox>
#include <QWidget>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "map.h"
#include "object_undo.h"
#include "util/xml_stream_util.h"



// ### UndoManager::State ###

UndoManager::State::State(UndoManager const *manager)
: is_loaded(manager->isLoaded())
, is_clean(manager->isClean())
, can_undo(manager->canUndo())
, can_redo(manager->canRedo())
{
	; // nothing else
}



// ### UndoManager ###

UndoManager::UndoManager(Map* map)
: QObject()
, map(map)
, current_index(0)
, clean_state_reachable(false)
, loaded_state_reachable(false)
{
	; // nothing else
}

UndoManager::~UndoManager()
{
	clear();
}


void UndoManager::clear()
{
	if (!undo_steps.empty())
	{
		UndoManager::State const old_state(this);
		
		clear(undo_steps, undo_steps.begin(), undo_steps.end());
		
		current_index = 0;
		
		if (old_state.is_clean)
			clean_state_index = 0;
		else
			clean_state_reachable = false;
		
		if (old_state.is_loaded)
			loaded_state_index = 0;
		else
			loaded_state_reachable = false;
		
		emitChangedSignals(old_state);
	}
	
	Q_ASSERT(undo_steps.empty());
}

void UndoManager::push(UndoStep* step)
{
	Q_ASSERT(step);
	
	clearRedoSteps();
	
	UndoManager::State const old_state(this);
	undo_steps.push_back(step);
	++current_index;
	validateUndoSteps();
	emitChangedSignals(old_state);
}

bool UndoManager::undo(QWidget* dialog_parent)
{
	UndoManager::State const old_state(this);
	
	if (!old_state.can_undo)
	{
		Q_ASSERT(false && "Undo must not be called when no step is available");
		return false;
	}
	
	UndoStep* step = nextUndoStep();
	if (!step->isValid())
	{
		QMessageBox::warning(dialog_parent, tr("Error"), tr("Cannot undo because the last undo step became invalid. This can for example happen if you change the symbol of an object to another and then delete the old symbol."));
		return false;
	}
	
	if (old_state.is_loaded)
	{
		if (QMessageBox::warning(dialog_parent, tr("Confirmation"), tr("Undoing this step will go beyond the point where the file was loaded. Are you sure?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
		{
			return false;
		}
	}
	
	UndoStep* redo_step = step->undo();
	updateMapState(step);
	delete step;
	
	--current_index;
	StepList::iterator entry = undo_steps.begin() + current_index;
	*entry = redo_step;
	
	emitChangedSignals(old_state);
	
	return true;
}

bool UndoManager::redo(QWidget* dialog_parent)
{
	UndoManager::State const old_state(this);
	
	if (!old_state.can_redo)
	{
		Q_ASSERT(false && "redo must not be called when no step is available");
		return false;
	}
	
	UndoStep* step = nextRedoStep();
	if (!step->isValid())
	{
		QMessageBox::warning(dialog_parent, tr("Error"), tr("Cannot redo because the first redo step became invalid. This can for example happen if you delete the symbol of an object you have drawn."));
		return false;
	}
	
	UndoStep* undo_step = step->undo();
	updateMapState(step);
	delete step;
	
	StepList::iterator entry = undo_steps.begin() + current_index;
	*entry = undo_step;
	++current_index;
	
	emitChangedSignals(old_state);
	
	return true;
}

void UndoManager::updateMapState(const UndoStep *step) const
{
	// Make a modified part the current one
	UndoStep::PartSet result_parts;
	bool have_modified_objects = step->getModifiedParts(result_parts);
	if (have_modified_objects && result_parts.find(map->getCurrentPartIndex()) == result_parts.end())
		map->setCurrentPartIndex(*result_parts.begin());
	
	// Select affected objects and ensure that they are visible
	UndoStep::ObjectSet result_objects;
	step->getModifiedObjects(map->getCurrentPartIndex(), result_objects);
	map->clearObjectSelection(result_objects.size() == (std::size_t)0);
	
	UndoStep::ObjectSet::iterator object = result_objects.begin();
	for (std::size_t i = 1, size = result_objects.size(); i <= size; ++i, ++object)
	{
		map->addObjectToSelection(*object, i == size);
	}
	
	map->ensureVisibilityOfSelectedObjects();
}


void UndoManager::clearRedoSteps()
{
	if (canRedo())
	{
		clear(undo_steps, undo_steps.begin() + current_index, undo_steps.end());
		clean_state_reachable  &= (clean_state_index <= current_index);
		loaded_state_reachable &= (loaded_state_index <= current_index);
		emit canRedoChanged(false);
	}
}

void UndoManager::clear(StepList &list, StepList::iterator begin, StepList::iterator end)
{
	for (StepList::iterator step = begin; step != end; ++step)
		delete *step;
	
	list.erase(begin, end);
}



void UndoManager::setClean()
{
	if (!isClean())
	{
		clean_state_reachable = true;
		clean_state_index = current_index;
		emit cleanChanged(true);
	}
}

void UndoManager::setLoaded()
{
	if (!isLoaded())
	{
		loaded_state_reachable = true;
		loaded_state_index = current_index;
		emit loadedChanged(true);
	}
}


void UndoManager::validateUndoSteps()
{
	if (canUndo())
	{
		std::size_t count = 0;
		StepList::reverse_iterator step = undo_steps.rbegin() + redoStepCount(),
		                           end  = undo_steps.rend();
		
		while (count < max_undo_steps && step != end && (*step)->isValid())
		{
			++count;
			++step;
		}
		
		for (; step != end; ++step)
		{
			if ((*step)->getType() != UndoStep::InvalidUndoStepType)
			{
				delete *step;
				*step = new NoOpUndoStep(map, false);
			}
		}
		
		if (clean_state_reachable)
			clean_state_reachable = (clean_state_index  >= current_index) || undo_steps[clean_state_index]->isValid();
		
		if (loaded_state_reachable)
			loaded_state_reachable = (loaded_state_index >= current_index) || undo_steps[loaded_state_index]->isValid();
		
		if (!canUndo())
			emit canUndoChanged(false);
	}
}

void UndoManager::validateRedoSteps()
{
	if (canRedo())
	{
		std::size_t count = 0;
		StepList::iterator step = undo_steps.begin() + current_index,
		                   end  = undo_steps.end();
		
		while (count < max_undo_steps && step != end && (*step)->isValid())
		{
			++count;
			++step;
		}
		
		clear(undo_steps, step, end);
		
		if (clean_state_reachable)
			clean_state_reachable = (clean_state_index <= undo_steps.size());
		
		if (loaded_state_reachable)
			loaded_state_reachable = (loaded_state_index <= undo_steps.size());
		
		if (!canRedo())
			emit canRedoChanged(false);
	}
}

void UndoManager::emitChangedSignals(const UndoManager::State& old_state)
{
	bool const is_clean = isClean();
	if (is_clean != old_state.is_clean)
		emit cleanChanged(is_clean);
	
	bool const is_loaded = isLoaded();
	if (is_loaded != old_state.is_loaded)
		emit loadedChanged(is_loaded);
	
	bool const can_undo = canUndo();
	if (can_undo != old_state.can_undo)
		emit canUndoChanged(can_undo);
	
	bool const can_redo = canRedo();
	if (can_redo != old_state.can_redo)
		emit canRedoChanged(can_redo);
}

bool UndoManager::load(QIODevice* file, int version)
{
	clear();
	
	StepList loaded_steps;
	bool success = loadSteps(loaded_steps, file, version);
	if (success)
	{
		UndoManager::State old_state(this);
		undo_steps.swap(loaded_steps);
		current_index = undo_steps.size();
		setLoaded();
		setClean();
		
		Q_ASSERT(loaded_steps.empty()); // as per above clear() + swap()
		success = loadSteps(loaded_steps, file, version);
		if (success)
		{
			undo_steps.insert(undo_steps.end(), loaded_steps.rbegin(), loaded_steps.rend());
		}
		
		emitChangedSignals(old_state);
	}
	
	return success;
}

bool UndoManager::loadSteps(StepList& steps, QIODevice* file, int version)
{
	int size;
	file->read((char*)&size, sizeof(int));
	steps.resize(size);
	bool success = true;
	for (int i = 0; i < size && success; ++i)
	{
		int type;
		file->read((char*)&type, sizeof(int));
		UndoStep* step = UndoStep::getUndoStepForType((UndoStep::Type)type, map);
		success = step->load(file, version);
		steps[i] = step;
	}
	return success;
}


void UndoManager::saveUndo(QXmlStreamWriter& xml)
{
	XmlElementWriter undo_element(xml, QLatin1String("undo"));
	
	validateUndoSteps();
	StepList::iterator begin = undo_steps.begin();
	StepList::iterator end   = begin + undoStepCount();
	while (begin != end && !(*begin)->isValid())
		++begin;
	
	saveSteps(begin, end, xml);
}

void UndoManager::saveRedo(QXmlStreamWriter& xml)
{
	XmlElementWriter redo_element(xml, QLatin1String("redo"));
	
	validateRedoSteps();
	saveSteps(undo_steps.rbegin(), undo_steps.rbegin() + redoStepCount(), xml);
}

template <class iterator>
void UndoManager::saveSteps(iterator begin, iterator end, QXmlStreamWriter& xml)
{
	for (iterator step = begin; step != end; ++step)
		(*step)->save(xml);
}


bool UndoManager::loadUndo(QXmlStreamReader& xml, SymbolDictionary& symbol_dict)
{
	Q_ASSERT(xml.name() == "undo");
	
	clear();
	
	StepList loaded_steps;
	bool success = loadSteps(loaded_steps, xml, symbol_dict);
	if (success)
	{
		if (loaded_steps.size() > max_undo_steps)
			clear(loaded_steps, loaded_steps.begin(), loaded_steps.begin() + (loaded_steps.size() - max_undo_steps));
		
		UndoManager::State old_state(this);
		undo_steps.swap(loaded_steps);
		current_index = undo_steps.size();
		setLoaded();
		setClean();
		emitChangedSignals(old_state);
	}
	return success;
}

bool UndoManager::loadRedo(QXmlStreamReader& xml, SymbolDictionary& symbol_dict)
{
	Q_ASSERT(xml.name() == "redo");
	
	clearRedoSteps();
	
	StepList loaded_steps;
	bool success = loadSteps(loaded_steps, xml, symbol_dict);
	if (success)
	{
		if (loaded_steps.size() > max_undo_steps)
			clear(loaded_steps, loaded_steps.begin() + (loaded_steps.size() - max_undo_steps), loaded_steps.end());
		
		UndoManager::State old_state(this);
		undo_steps.insert(undo_steps.end(), loaded_steps.rbegin(), loaded_steps.rend());
		emitChangedSignals(old_state);
	}
	return success;
}

bool UndoManager::loadSteps(StepList& steps, QXmlStreamReader& xml, SymbolDictionary& symbol_dict)
{
	while (xml.readNextStartElement())
	{
		if (xml.name() == "step")
			steps.push_back(UndoStep::load(xml, map, symbol_dict));
		else
			xml.skipCurrentElement(); // unknown
	}
	return true;
}
