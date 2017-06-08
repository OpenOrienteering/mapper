/*
 *    Copyright 2016 Mitchell Krome
 *    Copyright 2017 Kai Pastor
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

#include "object.h"
#include "core/map.h"
#include "core/map_part.h"
#include "gui/map/map_editor.h"
#include "tools/tool.h"


// ### ObjectQuery::LogicalOperands ###

ObjectQuery::LogicalOperands::LogicalOperands(const ObjectQuery::LogicalOperands& proto)
: first(proto.first ? std::make_unique<ObjectQuery>(*proto.first) : std::make_unique<ObjectQuery>())
, second(proto.second ? std::make_unique<ObjectQuery>(*proto.second) : std::make_unique<ObjectQuery>())
{
	// nothing else
}


ObjectQuery::LogicalOperands& ObjectQuery::LogicalOperands::operator=(const ObjectQuery::LogicalOperands& proto)
{
	if (proto.first)
		first = std::make_unique<ObjectQuery>(*proto.first);
	if (proto.second)
		second = std::make_unique<ObjectQuery>(*proto.second);
	return *this;
}



// ### ObjectQuery ###

ObjectQuery::ObjectQuery() noexcept
: op { ObjectQuery::OperatorInvalid }
{
	// nothing else
}


ObjectQuery::ObjectQuery(const ObjectQuery& query)
: op { query.op }
{
	if (op == ObjectQuery::OperatorInvalid)
	{
		; // nothing else
	}
	else if (op < 16)
	{
		new (&subqueries) LogicalOperands(query.subqueries);
	}
	else
	{
		new (&tags) TagOperands(query.tags);
	}
}


ObjectQuery::ObjectQuery(ObjectQuery&& proto) noexcept
: op { ObjectQuery::OperatorInvalid }
{
	consume(std::move(proto));
}


ObjectQuery& ObjectQuery::operator=(const ObjectQuery& proto) noexcept
{
	reset();
	consume(ObjectQuery{proto});
	return *this;
}


ObjectQuery& ObjectQuery::operator=(ObjectQuery&& proto) noexcept
{
	reset();
	consume(std::move(proto));	
	return *this;
}


ObjectQuery::~ObjectQuery()
{
	reset();
}


ObjectQuery::ObjectQuery(const QString& key, ObjectQuery::Operator op, const QString& value)
: op { op }
, tags { key, value }
{
	// Can't have an empty key (but can have empty value)
	// Must be a key/value operator
	Q_ASSERT(op >= 16);
	Q_ASSERT(op <= 18);
	if (op < 16 || op > 18
	    || key.length() == 0)
	{
		reset();
	}
}


ObjectQuery::ObjectQuery(const ObjectQuery& first, ObjectQuery::Operator op, const ObjectQuery& second)
: op { op }
, subqueries {}
{
	// Both sub-queries must be valid.
	// Must be a logical operator
	Q_ASSERT(op >= 1);
	Q_ASSERT(op <= 2);
	if (op < 1 || op > 2
	    || !first || !second)
	{
		reset();
	}
	else
	{
		subqueries.first = std::make_unique<ObjectQuery>(first);
		subqueries.second = std::make_unique<ObjectQuery>(second);
	}
}


ObjectQuery::ObjectQuery(ObjectQuery&& first, ObjectQuery::Operator op, ObjectQuery&& second) noexcept
: op { op }
, subqueries {}
{
	// Both sub-queries must be valid.
	// Must be a logical operator
	Q_ASSERT(op >= 1);
	Q_ASSERT(op <= 2);
	if (op < 1 || op > 2
	    || !first || !second)
	{
		reset();
	}
	else
	{
		subqueries.first = std::make_unique<ObjectQuery>(std::move(first));
		subqueries.second = std::make_unique<ObjectQuery>(std::move(second));
	}
}



// static
QString ObjectQuery::labelFor(ObjectQuery::Operator op)
{
	switch (op)
	{
	case OperatorIs:
		//: Very short label
		return tr("is");
	case OperatorIsNot:
		//: Very short label
		return tr("is not");
	case OperatorContains:
		//: Very short label
		return tr("contains");
		
	case OperatorAnd:
		//: Very short label
		return tr("and");
	case OperatorOr:
		//: Very short label
		return tr("or");
		
	case OperatorInvalid:
		//: Very short label
		return tr("invalid");
	}
	
	Q_UNREACHABLE();
}



bool ObjectQuery::operator()(const Object* object) const
{
	const auto& object_tags = object->tags();

	switch(op)
	{
	case OperatorIs:
		return object_tags.contains(tags.key) && object_tags.value(tags.key) == tags.value;
	case OperatorIsNot:
		// If the object does have the tag, not is true
		return !object_tags.contains(tags.key) || object_tags.value(tags.key) != tags.value;
	case OperatorContains:
		return object_tags.contains(tags.key) && object_tags.value(tags.key).contains(tags.value);
		
	case OperatorAnd:
		return (*subqueries.first)(object) && (*subqueries.second)(object);
	case OperatorOr:
		return (*subqueries.first)(object) || (*subqueries.second)(object);
		
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



const ObjectQuery::LogicalOperands* ObjectQuery::logicalOperands() const
{
	const LogicalOperands* result = nullptr;
	if (op >= 1 && op <= 2)
	{
		result = &subqueries;
	}
	else
	{
		Q_ASSERT(op >= 1);
		Q_ASSERT(op <= 2);
	}
	return result;
}


const ObjectQuery::TagOperands* ObjectQuery::tagOperands() const
{
	const TagOperands* result = nullptr;
	if (op >= 16 && op <= 18)
	{
		result = &tags;
	}
	else
	{
		Q_ASSERT(op >= 16);
		Q_ASSERT(op <= 18);
	}
	return result;
}



void ObjectQuery::reset()
{
	if (op == ObjectQuery::OperatorInvalid)
	{
		; // nothing else
	}
	else if (op < 16)
	{
		subqueries.~LogicalOperands();
		op = ObjectQuery::OperatorInvalid;
	}
	else
	{
		tags.~TagOperands();
		op = ObjectQuery::OperatorInvalid;
	}
}


void ObjectQuery::consume(ObjectQuery&& other)
{
	Q_ASSERT(op == ObjectQuery::OperatorInvalid);
	op = other.op;
	if (op == ObjectQuery::OperatorInvalid)
	{
		; // nothing else
	}
	else if (op < 16)
	{
		new (&subqueries) ObjectQuery::LogicalOperands{std::move(other.subqueries)};
		other.subqueries.~LogicalOperands();
	}
	else
	{
		new (&tags) ObjectQuery::TagOperands{std::move(other.tags)};
		other.tags.~TagOperands();
	}
	other.op = ObjectQuery::OperatorInvalid;
}

