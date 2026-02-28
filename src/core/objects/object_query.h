/*
 *    Copyright 2016 Mitchell Krome
 *    Copyright 2017-2024 Kai Pastor
 *    Copyright 2026 Matthias Kühlewein
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

#include "core/objects/dynamic_object_query.h"

#include <QCoreApplication>
#include <QMetaType>
#include <QString>
#include <QStringRef>

namespace OpenOrienteering {

class Map;
class Object;
class Symbol;


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
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::ObjectQuery)
	
public:
	// Important: When adding operators update the Is... functions below
	enum Operator {
		// Operators 1 .. 15 operate on other queries
		OperatorAnd      = 1,  ///< And-chains two object queries
		OperatorOr       = 2,  ///< Or-chains two object queries
		OperatorNot      = 3,  ///< Negation of the sub-query
		
		// Operators 16 .. 18 operate on object tags and other strings
		OperatorIs       = 16, ///< Tests an existing tag for equality with the given value (case-sensitive)
		OperatorIsNot    = 17, ///< Tests an existing tag for inequality with the given value (case-sensitive)
		OperatorContains = 18, ///< Tests an existing tag for containing the given value (case-sensitive)
		
		OperatorSearch   = 19, ///< Tests if the symbol name, a tag key or a tag value contains the given value (case-insensitive)
		OperatorObjectText = 20, ///< Text object content (case-insensitive)
		
		// More operators, 32 ..
		OperatorSymbol   = 32, ///< Test the symbol for equality.
		OperatorDynamic  = 33, ///< Processing dynamic object properties
		
		OperatorInvalid  = 0   ///< Marks an invalid query
	};
	
	// Parameters for logical operations
	struct LogicalOperands
	{
		std::unique_ptr<ObjectQuery> first;
		std::unique_ptr<ObjectQuery> second;
		
		LogicalOperands() = default;
		explicit LogicalOperands(const LogicalOperands& proto); // maybe expensive copying
		LogicalOperands(LogicalOperands&&) = default;
		~LogicalOperands();
		LogicalOperands& operator=(const LogicalOperands& proto);
		LogicalOperands& operator=(LogicalOperands&&) = default;
	 };
	
	// Parameters for operations on tags.
	struct StringOperands
	{
		QString key;
		QString value;
		
		~StringOperands();
	};

	ObjectQuery() noexcept;
	explicit ObjectQuery(const ObjectQuery& query); // maybe expensive copying
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
	 * Constructs a query for a value.
	 * 
	 * Valid for OperatorSearch.
	 */
	ObjectQuery(Operator op, const QString& value);
	
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
	 * Constructs a query for a particular symbol.
	 */
	ObjectQuery(const Symbol* symbol) noexcept;
	
	/**
	 * Constructs a query for a dynamic object property.
	 */
	ObjectQuery(const DynamicObjectQuery* dynamic_query) noexcept;
	
	/**
	 * Returns a query which is the negation of the sub-query.
	 */
	static ObjectQuery negation(ObjectQuery query) noexcept;
	
	
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
	 * Returns the operands of logical query operations.
	 */
	const LogicalOperands* logicalOperands() const;
	
	/**
	 * Returns the operands of logical query operations.
	 */
	const StringOperands* tagOperands() const;
	
	/**
	 * Returns the operand of symbol operations.
	 */
	const Symbol* symbolOperand() const;
	
	
	/**
	 * Pretty print the query.
	 * 
	 * The output is meant to be formal language, for possible parsing.
	 */
	QString toString() const;
	
	
	friend bool operator==(const ObjectQuery& lhs, const ObjectQuery& rhs);
	
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
	
	bool IsLogicalOperator(Operator op) const { return op >= 1 && op <= 3; }
	bool IsTagOperator(Operator op) const { return op >= 16 && op <= 18; }
	bool IsValueOperator(Operator op) const { return op >= 19 && op <= 20; }
	bool IsStringOperator(Operator op) const { return op >= 16 && op <= 20; }
	
	using SymbolOperand = const Symbol*;
	
	Operator op;
	
	union
	{
		LogicalOperands subqueries;
		StringOperands  tags;
		SymbolOperand   symbol;
		const DynamicObjectQuery* dynamic_query;
	};
	
};

bool operator==(const ObjectQuery& lhs, const ObjectQuery& rhs);

bool operator!=(const ObjectQuery& lhs, const ObjectQuery& rhs);

bool operator==(const ObjectQuery::StringOperands& lhs, const ObjectQuery::StringOperands& rhs);

bool operator!=(const ObjectQuery::StringOperands& lhs, const ObjectQuery::StringOperands& rhs);



/**
 * Utility to construct object queries from text.
 * 
 * The text shall be in the formal language generated by ObjectQuery::toString():
 * 
 * <ObjectQuery>      ::=  <AndExpression>  |  <ObjectQuery> 'OR' <AndExpression>
 * <AndExpression>    ::=  <BoolExpression>  |  <AndExpression> 'AND' <BoolExpression>
 * <BoolExpression>   ::=  <ParenExpression>  |  <SymbolTerm>  |  <TestTerm>  |  'NOT' <BoolExpression>
 * <ParenExpression>  ::=  '(' <ObjectQuery> ')'
 * <SymbolTerm>       ::=  'SYMBOL' <Literal>
 * <TestTerm>         ::=  <IsTerm> | <IsNotTerm> | <ContainsTerm> | <SearchTerm>
 * <IsTerm>           ::=  <Literal> '=' <Literal>
 * <IsNotTerm>        ::=  <Literal> '!=' <Literal>
 * <ContainsTerm>     ::=  <Literal> '~=' <Literal>
 * <SearchTerm>       ::=  <Literal>
 * <Literal>          ::=  <Word> | '"' <String> '"'
 * <Word>             ::=  ...
 * <String>           ::=  ...
 */
class ObjectQueryParser
{
public:
	ObjectQueryParser() = default;
	explicit ObjectQueryParser(const Map* map);
	
	/**
	 * Sets the map for looking up symbols.
	 */
	void setMap(const Map* map);
	
	/**
	 * Returns an ObjectQuery for the given text.
	 * 
	 * The return query has ObjectQuery::OperatorInvalid in case of error.
	 */
	ObjectQuery parse(const QString& text);
	
	/**
	 * In case of error, returns the approximate position where parsing the
	 * text failed.
	 */
	int errorPos() const;
	
	enum TokenType
	{
		TokenUnknown = 0,
		TokenNothing,
		TokenString,
		TokenWord,
		TokenTextOperator,
		TokenSymbol,
		TokenOr,
		TokenAnd,
		TokenNot,
		TokenLeftParen,
		TokenRightParen,
		TokenDynamicQuery,
	};
	
private:
	void getToken();
	
	QString tokenAsString() const;
	
	const Symbol* findSymbol(const QString& key) const;
	
	DynamicObjectQueryManager dynamic_object_manager;
	const Map* map = nullptr;
	QStringRef input;
	QStringRef token_text;
	QStringRef token_attributes_text;
	TokenType token;
	DynamicObjectQuery* dynamic_token;
	int token_start = -1;
	int pos;
};



inline
bool operator!=(const ObjectQuery& lhs, const ObjectQuery& rhs)
{
	return !(lhs==rhs);
}

inline
bool operator!=(const ObjectQuery::StringOperands& lhs, const ObjectQuery::StringOperands& rhs)
{
	return !(lhs==rhs);
}


}  // namespace OpenOrienteering

Q_DECLARE_METATYPE(OpenOrienteering::ObjectQuery::Operator)


#endif // OPENORIENTEERING_OBJECT_QUERY_H
