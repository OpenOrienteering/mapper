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


// ### ObjectQuery ###

ObjectQuery::ObjectQuery(const QString& key, ObjectQuery::Operator op, const QString& value)
: op        { op }
, key_arg   { key }
, value_arg { value }
{
	// Must be a key/value operator
	Q_ASSERT(op >= 16);
	Q_ASSERT(op <= 18);
	if (op < 16 || op > 18)
		this->op = OperatorInvalid;

	// Can't have an empty key (can have empty value but)
	if (key.length() == 0)
		this->op = OperatorInvalid;
}


ObjectQuery::ObjectQuery(std::unique_ptr<ObjectQuery> left, ObjectQuery::Operator op, std::unique_ptr<ObjectQuery> right)
: op        { op }
, left_arg  { std::move(left) }
, right_arg { std::move(right) }
{
	// Must be a logical operator
	Q_ASSERT(op >= 1);
	Q_ASSERT(op <= 2);
	if (op < 1 || op > 2)
		this->op = OperatorInvalid;

	// Can't have null queries
	if (!left_arg || !right_arg)
		this->op = OperatorInvalid;
}



ObjectQuery::Operator ObjectQuery::getOperator() const
{
	return op;
}



bool ObjectQuery::operator()(const Object* object) const
{
	const auto tags = object->tags();

	switch(op)
	{
	case OperatorIs:
		return tags.contains(key_arg) && tags.value(key_arg) == value_arg;
	case OperatorIsNot:
		// If the object does have the tag, not is true
		return !tags.contains(key_arg) || tags.value(key_arg) != value_arg;
	case OperatorContains:
		return tags.contains(key_arg) && tags.value(key_arg).contains(value_arg);
		
	case OperatorAnd:
		return (*left_arg)(object) && (*right_arg)(object);
	case OperatorOr:
		return (*left_arg)(object) || (*right_arg)(object);
		
	case OperatorInvalid:
		return false;
	}
	
	Q_UNREACHABLE();
}



void ObjectQuery::selectMatchingObjects(Map* map, MapEditorController* controller) const
{
	bool had_selection = !map->selectedObjects().empty();
	map->clearObjectSelection(false);

	// Lambda to add objects to the selection
	auto select_object = [map](Object* const object, MapPart*, int)
	{
		map->addObjectToSelection(object, false);
		return false; // applyOnMatchingObjects tells us if this op fails, so if we fail we made a selection
	};

	MapPart *part = map->getCurrentPart();

	// This reports failure if we made a selection
	auto object_selected = !part->applyOnMatchingObjects(select_object, *this);

	if (object_selected || had_selection)
		map->emitSelectionChanged();

	if (object_selected)
	{
		auto current_tool = controller->getTool();
		if (current_tool && current_tool->isDrawTool())
			controller->setEditTool();
	}
}
