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

#ifndef OPENORIENTEERING_OBJECT_QUERY_H
#define OPENORIENTEERING_OBJECT_QUERY_H

#include <memory>

#include <QCoreApplication>
#include <QMetaType>
#include <QString>

class Map;
class MapEditorController;
class Object;


/**
 * Utility to match objects based on tag values.
 * 
 * This class can be used with value semantics. It can be move-constructed and
 * move-assigned in O(1). Normal copy-construction and assignment may involve
 * expensive copying of the expression tree.
 * 
 * OperatorAnd and OperatorOr evaluate the left argument first. When constructing
 * complex queries, left sub-query chains should be kept short in order to
 * benefit from short circuiting the evaluation of these logical operators after
 * evaluation of the left argument.
 */
class ObjectQuery
{
	Q_DECLARE_TR_FUNCTIONS(ObjectQuery)
	
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
	
	// Parameters for logical operations
	struct LogicalOperands
	{
		std::unique_ptr<ObjectQuery> first;
		std::unique_ptr<ObjectQuery> second;
		
		LogicalOperands() = default;
		LogicalOperands(const LogicalOperands& proto);
		LogicalOperands(LogicalOperands&&) = default;
		~LogicalOperands() = default;
		LogicalOperands& operator=(const LogicalOperands& proto);
		LogicalOperands& operator=(LogicalOperands&&) = default;
	 };
	
	// Parameters for operations on tags.
	struct TagOperands
	{
		QString key;
		QString value;
	};

	ObjectQuery() noexcept;
	ObjectQuery(const ObjectQuery& query);
	ObjectQuery(ObjectQuery&& proto) noexcept;
	ObjectQuery& operator=(const ObjectQuery& proto) noexcept;
	ObjectQuery& operator=(ObjectQuery&& proto) noexcept;
	
	~ObjectQuery();
	
	/**
	 * Returns true if the query is valid.
	 */
	operator bool() const noexcept { return op != OperatorInvalid; }
	
	/**
	 * Constructs a query for a key and value.
	 */
	ObjectQuery(const QString& key, Operator op, const QString& value);
	
	/**
	 * Constructs a query which connects two sub-queries.
	 * 
	 * The sub-queries are copied.
	 */
	ObjectQuery(const ObjectQuery& first, Operator op, const ObjectQuery& second);
	
	/**
	 * Constructs a query which connects two sub-queries.
	 */
	ObjectQuery(ObjectQuery&& first, Operator op, ObjectQuery&& second) noexcept;
	
	
	/**
	 * Returns the underlying operator.
	 */
	Operator getOperator() const noexcept { return op; }
	
	
	/**
	 * Returns a short label for the operator which can be used in the user interface.
	 */
	static QString labelFor(Operator op);
	
	
	/**
	 * Evaluates this query on the given object and returns whether it matches.
	 */
	bool operator()(const Object* object) const;
	
	
	/**
	 * Select the objects in the current part which match to this query.
	 */
	void selectMatchingObjects(Map* map, MapEditorController* controller) const;
	
	
	/**
	 * Returns the operands of logical query operations.
	 */
	const LogicalOperands* logicalOperands() const;
	
	/**
	 * Returns the operands of logical query operations.
	 */
	const TagOperands* tagOperands() const;
	
	
private:
	/**
	 * Resets the query to the OperatorInvalid state.
	 */
	void reset();
	
	/**
	 * Moves the other query's data to this one which must be in OperatorInvalid state.
	 * 
	 * Leaves the other object in OperatorInvalid state.
	 * In effect, this is a move assignment with a strong precondition.
	 */
	void consume(ObjectQuery&& other);
	
	Operator op;
	
	union
	{
		LogicalOperands subqueries;
		TagOperands     tags;
	};
	
};

Q_DECLARE_METATYPE(ObjectQuery::Operator)

#endif
