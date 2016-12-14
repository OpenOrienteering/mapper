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


/**
 * Utility to match objects based on tag values.
 */
class ObjectQuery
{
public:
	enum Operator {
		// Operators 1 .. 15 operate on other queries
		OperatorAnd      = 1,  ///< And-chains two object queries
		OperatorOr       = 2,  ///< Or-chains two object queries
		
		// Operators 16 .. 18 operate on object tags
		OperatorIs       = 16, ///< Tests an existing tag for equality with the given value
		OperatorIsNot    = 17, ///< Tests an existing tag for inequality with the given value
		OperatorContains = 18, ///< Tests an existing tag for containing the given value
		
		OperatorInvalid  = 0   ///< Marks an invalid query
	};
	
	/**
	 * Constructs a query for a key and value.
	 */
	ObjectQuery(const QString& key, Operator op, const QString& value);
	
	/**
	 * Constructs a query which connects two sub-queries.
	 */
	ObjectQuery(std::unique_ptr<ObjectQuery> left, Operator op, std::unique_ptr<ObjectQuery> right);
	
	
	/**
	 * Returns the underlying operator.
	 */
	Operator getOperator() const;
	
	
	/**
	 * Evaluates this query on the given object and returns whether it matches.
	 */
	bool operator()(const Object* object) const;
	
	
	/**
	 * Select the objects in the current part which match to this query.
	 */
	void selectMatchingObjects(Map* map, MapEditorController* controller) const;
	
	
private:
	Operator op;
	
	// For compare ops
	QString key_arg;
	QString value_arg;
	
	// For logical ops
	std::unique_ptr<ObjectQuery> left_arg;
	std::unique_ptr<ObjectQuery> right_arg;
};

Q_DECLARE_METATYPE(ObjectQuery::Operator)

#endif
