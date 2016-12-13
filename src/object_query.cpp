/*
 *    Copyright 2016 Mitchell Krome
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


#include "object_query.h"

#include "map.h"
#include "map_editor.h"
#include "map_part.h"
#include "object.h"
#include "tool.h"
#include "util.h"

// ### ObjectQuery ###

ObjectQuery::ObjectQuery()
: op(INVALID_OP)
{
	; // Nothing
}

ObjectQuery::ObjectQuery(const QString& key, ObjectQuery::Operation op, const QString& value)
: op(op)
, key_arg(key)
, value_arg(value)
{
	// Must be a comparison op
	if (op != IS_OP && op != CONTAINS_OP && op != NOT_OP)
		this->op = INVALID_OP;

	// Can't have an empty key (can have empty value but)
	if (key.length() == 0)
		this->op = INVALID_OP;
}

ObjectQuery::ObjectQuery(std::unique_ptr<ObjectQuery> left, ObjectQuery::Operation op, std::unique_ptr<ObjectQuery> right)
: op(op)
, left_arg(std::move(left))
, right_arg(std::move(right))
{
	// Must be a logical op
	if (op != OR_OP && op != AND_OP)
		this->op = INVALID_OP;

	// Can't have null queries
	if (left_arg.get() == nullptr || right_arg.get() == nullptr)
		this->op = INVALID_OP;
}

ObjectQuery::Operation ObjectQuery::getOp() const
{
	return op;
}

bool ObjectQuery::operator()(Object* object) const
{
	const auto tags = object->tags();

	switch(op)
	{
	case IS_OP:
		return tags.contains(key_arg) && tags.value(key_arg) == value_arg;
	case CONTAINS_OP:
		return tags.contains(key_arg) && tags.value(key_arg).contains(value_arg);
	case NOT_OP:
		// If the object does have the tag, not is true
		return !tags.contains(key_arg) || tags.value(key_arg) != value_arg;
	case OR_OP:
		return (*left_arg)(object) || (*right_arg)(object);
	case AND_OP:
		return (*left_arg)(object) && (*right_arg)(object);
	case INVALID_OP:
		return false;
	}

	return false;
}

void ObjectQuery::selectMatchingObjects(Map* map, MapEditorController* controller) const
{
	bool selection_changed = false;
	if (map->getNumSelectedObjects() > 0)
		selection_changed = true;
	map->clearObjectSelection(false);

	// Lambda to add objects to the selection
	auto select_object = [map](Object* const object, MapPart *part, int object_index)
	{
		Q_UNUSED(part);
		Q_UNUSED(object_index);
		map->addObjectToSelection(object, false);
		return false; // applyOnMatchingObjects tells us if this op fails, so if we fail we made a selection
	};

	MapPart *part = map->getCurrentPart();

	// This reports failure if we made a selection
	auto object_selected = !part->applyOnMatchingObjects(select_object, *this);

	selection_changed |= object_selected;
	if (selection_changed)
		map->emitSelectionChanged();

	if (object_selected)
	{
		auto current_tool = controller->getTool();
		if (current_tool && current_tool->isDrawTool())
			controller->setEditTool();
	}
}
