/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014-2018 Kai Pastor
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

#include <algorithm>
#include <iterator>
#include <limits>
#include <set>

#include <QtGlobal>
#include <QFlags>
#include <QIODevice>
#include <QLatin1String>
#include <QMessageBox>
#include <QString>
#include <QStringRef>
#include <QXmlStreamReader>

#include "core/map.h"
#include "undo/undo.h"
#include "util/xml_stream_util.h"


namespace OpenOrienteering {

Q_STATIC_ASSERT(UndoManager::max_undo_steps < std::numeric_limits<int>::max());


namespace
{

template <class iterator>
void saveSteps(QXmlStreamWriter& xml, iterator first, iterator last)
{
	for (auto step = first; step != last; ++step)
		(*step)->save(xml);
}

}  // namespace



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
, clean_state_index(-1)
, loaded_state_index(-1)
{
	undo_steps.reserve(max_undo_steps + 1);  // +1 is for push before trim
	if (map)
	{
		connect(map, &Map::symbolDeleted, this, [this]() {
			validateUndoSteps();
			validateRedoSteps();
		}, Qt::QueuedConnection);
	}
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
		
		undo_steps.erase(begin(undo_steps), end(undo_steps));
		current_index = 0;
		clean_state_index = old_state.is_clean ? 0 : -1;
		loaded_state_index = old_state.is_loaded ? 0 : -1;
		
		emitChangedSignals(old_state);
	}
	
	Q_ASSERT(undo_steps.empty());
}



void UndoManager::push(std::unique_ptr<UndoStep>&& step)
{
	Q_ASSERT(step);
	
	clearRedoSteps();
	
	UndoManager::State const old_state(this);
	undo_steps.emplace_back(std::move(step));
	++current_index;
	validateUndoSteps();
	emitChangedSignals(old_state);
}



bool UndoManager::canUndo() const
{
	return current_index > 0 && nextUndoStep()->isValid();
}


bool UndoManager::undo(QWidget* dialog_parent)
{
	UndoManager::State const old_state(this);
	
	if (!old_state.can_undo)
	{
		Q_ASSERT(!bool("Undo must not be called when no step is available"));
		return false;
	}
	
	UndoStep* step = nextUndoStep();
	if (!step->isValid())
	{
		QMessageBox::warning(dialog_parent, tr("Error"), tr("Cannot undo because the last undo step became invalid. This can for example happen if you change the symbol of an object to another and then delete the old symbol."));
		validateUndoSteps();  // removes all undo steps and updates the state.
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
	
	--current_index;
	undo_steps[StepList::size_type(current_index)].reset(redo_step);
	
	emitChangedSignals(old_state);
	
	return true;
}



bool UndoManager::canRedo() const
{
	return current_index < int(undo_steps.size()) && nextRedoStep()->isValid();
}


bool UndoManager::redo(QWidget* dialog_parent)
{
	UndoManager::State const old_state(this);
	
	if (!old_state.can_redo)
	{
		Q_ASSERT(!bool("redo must not be called when no step is available"));
		return false;
	}
	
	UndoStep* step = nextRedoStep();
	if (!step->isValid())
	{
		QMessageBox::warning(dialog_parent, tr("Error"), tr("Cannot redo because the first redo step became invalid. This can for example happen if you delete the symbol of an object you have drawn."));
		validateRedoSteps();
		return false;
	}
	
	UndoStep* undo_step = step->undo();
	updateMapState(step);
	
	undo_steps[StepList::size_type(current_index)].reset(undo_step);
	++current_index;
	
	emitChangedSignals(old_state);
	
	return true;
}



bool UndoManager::isClean() const
{
	return clean_state_index == UndoManager::StepList::difference_type(current_index);
}


void UndoManager::setClean()
{
	if (!isClean())
	{
		clean_state_index = current_index;
		emit cleanChanged(true);
	}
}



bool UndoManager::isLoaded() const
{
	return loaded_state_index == UndoManager::StepList::difference_type(current_index);
}


void UndoManager::setLoaded()
{
	if (!isLoaded())
	{
		loaded_state_index = current_index;
		emit loadedChanged(true);
	}
}



int UndoManager::undoStepCount() const
{
	return current_index;
}


UndoStep* UndoManager::nextUndoStep() const
{
	Q_ASSERT(current_index > 0);
	return undo_steps[StepList::size_type(current_index) - 1].get();
}



int UndoManager::redoStepCount() const
{
	return int(undo_steps.size()) - current_index;
}


UndoStep* UndoManager::nextRedoStep() const
{
	Q_ASSERT(StepList::size_type(current_index) < undo_steps.size());
	return undo_steps[StepList::size_type(current_index)].get();
}



void UndoManager::updateMapState(const UndoStep *step) const
{
	// Do nothing for a null map (which is the case for tests)
	if (!map)
		return;
	
	// Make a modified part the current one
	UndoStep::PartSet result_parts;
	bool have_modified_objects = step->getModifiedParts(result_parts);
	if (have_modified_objects && result_parts.find(map->getCurrentPartIndex()) == end(result_parts))
		map->setCurrentPartIndex(*begin(result_parts));
	
	// Select affected objects and ensure that they are visible
	UndoStep::ObjectSet result_objects;
	step->getModifiedObjects(map->getCurrentPartIndex(), result_objects);
	map->clearObjectSelection(false);
	for (auto object : result_objects)
		map->addObjectToSelection(object, false);
	emit map->objectSelectionChanged();
	
	map->ensureVisibilityOfSelectedObjects(Map::PartialVisibility);
}


void UndoManager::clearRedoSteps()
{
	if (current_index < int(undo_steps.size()))
	{
		undo_steps.erase(begin(undo_steps) + StepList::difference_type(current_index), end(undo_steps));
		if (clean_state_index > StepList::difference_type(current_index))
			clean_state_index = -1;
		if (loaded_state_index > StepList::difference_type(current_index))
			loaded_state_index = -1;
		emit canRedoChanged(false);
	}
}



void UndoManager::validateUndoSteps()
{
	if (current_index > 0)
	{
		int num_removed_undo_steps = 0;
		if (current_index > int(max_undo_steps))
			num_removed_undo_steps += current_index - int(max_undo_steps);
		
		auto rfirst = undo_steps.rend() - StepList::difference_type(current_index);
		Q_ASSERT(rfirst->get() == nextUndoStep());
		auto rlast = undo_steps.rend() - num_removed_undo_steps;
		rfirst = std::find_if(rfirst, rlast, [](auto&& step) { return !step->isValid(); });
		if (rfirst != rlast)
			num_removed_undo_steps += std::distance(rfirst, rlast);
		
		if (num_removed_undo_steps == 0)
			return;
		
		auto first = begin(undo_steps);
		undo_steps.erase(first, first + num_removed_undo_steps);
		current_index -= StepList::size_type(num_removed_undo_steps);
		
		if (clean_state_index >= 0)
			clean_state_index -= num_removed_undo_steps;
		
		if (loaded_state_index >= 0)
			loaded_state_index -= num_removed_undo_steps;
		
		if (!canUndo())
			emit canUndoChanged(false);
	}
}

void UndoManager::validateRedoSteps()
{
	if (current_index < int(undo_steps.size()))
	{
		auto num_removed_redo_steps = StepList::difference_type(0);
		
		auto first = begin(undo_steps) + StepList::difference_type(current_index);
		auto last = end(undo_steps);
		first = std::find_if(first, last, [](auto&& step) { return !step->isValid(); });
		if (first != last)
			num_removed_redo_steps += std::distance(first, last);
		
		if (num_removed_redo_steps == 0)
			return;
		
		undo_steps.erase(first, last);
		
		if (clean_state_index > StepList::difference_type(undo_steps.size()))
			clean_state_index = -1;
		
		if (loaded_state_index > StepList::difference_type(undo_steps.size()))
			loaded_state_index = -1;
		
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

#ifndef NO_NATIVE_FILE_FORMAT

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
			std::move(loaded_steps.rbegin(), loaded_steps.rend(), std::back_inserter(undo_steps)); 
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
		steps[i].reset(step);
	}
	return success;
}

#endif

void UndoManager::saveUndo(QXmlStreamWriter& xml)
{
	XmlElementWriter undo_element(xml, QLatin1String("undo"));
	
	validateUndoSteps();
	auto first = begin(undo_steps);
	auto last  = first + StepList::difference_type(undoStepCount());
	first = std::find_if(first, last, [](auto&& undo_step) { return undo_step->isValid(); });
	saveSteps(xml, first, last);
}

void UndoManager::saveRedo(QXmlStreamWriter& xml)
{
	XmlElementWriter redo_element(xml, QLatin1String("redo"));
	
	validateRedoSteps();
	auto first = undo_steps.rbegin();
	auto last  = undo_steps.rbegin() + StepList::difference_type(redoStepCount());
	saveSteps(xml, first, last);
}


void UndoManager::loadUndo(QXmlStreamReader& xml, SymbolDictionary& symbol_dict)
{
	Q_ASSERT(xml.name() == QLatin1String("undo"));
	
	auto loaded_steps = loadSteps(xml, symbol_dict);
	if (loaded_steps.size() > max_undo_steps)
		loaded_steps.erase(begin(loaded_steps), begin(loaded_steps) + StepList::difference_type(loaded_steps.size() - max_undo_steps));
	
	clear();
	UndoManager::State old_state(this);
	using std::swap;
	swap(undo_steps, loaded_steps);
	current_index = int(undo_steps.size());
	setLoaded();
	setClean();
	emitChangedSignals(old_state);
}


void UndoManager::loadRedo(QXmlStreamReader& xml, SymbolDictionary& symbol_dict)
{
	Q_ASSERT(xml.name() == QLatin1String("redo"));
	
	auto loaded_steps = loadSteps(xml, symbol_dict);
	auto capacity = max_undo_steps - undo_steps.size();
	if (loaded_steps.size() > capacity)
		loaded_steps.erase(begin(loaded_steps) + StepList::difference_type(loaded_steps.size() - capacity), end(loaded_steps));
		
	clearRedoSteps();
	UndoManager::State old_state(this);
	std::move(loaded_steps.rbegin(), loaded_steps.rend(), std::back_inserter(undo_steps)); 
	emitChangedSignals(old_state);
}


UndoManager::StepList UndoManager::loadSteps(QXmlStreamReader& xml, SymbolDictionary& symbol_dict) const
{
	StepList steps;
	steps.reserve(max_undo_steps + 1);
	while (xml.readNextStartElement())
	{
		if (xml.name() == QLatin1String("step"))
			steps.emplace_back(UndoStep::load(xml, map, symbol_dict));
		else
			xml.skipCurrentElement(); // unknown
	}
	return steps;
}


}  // namespace OpenOrienteering
