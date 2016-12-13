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

#ifndef OPENORIENTEERING_OBJECT_QUERY_H
#define OPENORIENTEERING_OBJECT_QUERY_H

#include <memory>

#include <QMetaType>
#include <QString>

class Map;
class MapEditorController;
class Object;

class ObjectQuery
{
public:
	enum Operation {
		IS_OP, // These operate on string
		CONTAINS_OP,
		NOT_OP,
		OR_OP, // These operate on other queries
		AND_OP,
		INVALID_OP // To signal something is invalid
	};

	ObjectQuery();
	ObjectQuery(const QString& key, Operation op, const QString& value);
	ObjectQuery(std::unique_ptr<ObjectQuery> left, Operation op, std::unique_ptr<ObjectQuery> right);

	/**
	 * Evaluates this query on object - returns whether the query is true or false
	 */
	bool operator()(Object* object) const;

	/**
	 * Select the objects in the current part which match to this query
	 */
	void selectMatchingObjects(Map* map, MapEditorController* controller) const;

	Operation getOp() const;
private:
	Operation op;

	// For compare ops
	QString key_arg;
	QString value_arg;

	// For logical ops
	std::unique_ptr<ObjectQuery> left_arg;
	std::unique_ptr<ObjectQuery> right_arg;
};

Q_DECLARE_METATYPE(ObjectQuery::Operation)

#endif
