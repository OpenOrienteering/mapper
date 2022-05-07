/*
 *    Copyright 2016 Mitchell Krome
 *    Copyright 2017-2022 Kai Pastor
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

#include <algorithm>
#include <iterator>
#include <new>
#include <utility>
#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QChar>
#include <QLatin1Char>
#include <QLatin1String>
#include <QString>
#include <QVarLengthArray>

#include "core/map.h"
#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "core/symbols/symbol.h"
#include "util/key_value_container.h"


// ### Local utilities ###

namespace {

static QChar special_chars[9] = {
    QLatin1Char('"'),
    QLatin1Char(' '),
    QLatin1Char('\t'),
    QLatin1Char('('),
    QLatin1Char(')'),
    QLatin1Char('='),
    QLatin1Char('!'),
    QLatin1Char('~'),
    QLatin1Char('\\')
};

QString toEscaped(QString string)
{
	for (int i = 0; i < string.length(); ++i)
	{
		const auto c = string.at(i);
		if (c == QLatin1Char('\\') || c == QLatin1Char('"'))
		{
			string.insert(i, QLatin1Char('\\'));
			++i;
		}
	}
	return string;
}

QString fromEscaped(QString string)
{
	for (int i = 0; i < string.length(); ++i)
	{
		const auto c = string.at(i);
		if (c == QLatin1Char('\\'))
		{
			string.remove(i, 1);
		}
	}
	return string;
}

QString keyToString(const QString &key)
{
	using std::begin;
	using std::end;
	if (std::find_first_of(begin(key), end(key), begin(special_chars), end(special_chars)) == end(key)
	    && key != QLatin1String("AND")
	    && key != QLatin1String("OR")
	    && key != QLatin1String("SYMBOL"))
		return key;
	else
		return QLatin1Char('"') + toEscaped(key) + QLatin1Char('"');
}


/**
 * A stack for tracking object query parsing state.
 */
class ObjectQueryNesting
{
	using ObjectQuery = OpenOrienteering::ObjectQuery;
	
	struct Element
	{
		ObjectQuery* expression;
		int depth;
	};
	QVarLengthArray<Element, 8> stack;  ///< (Maybe incomplete) parsing states.
	
public:
	int depth = 0;  ///< The explicit depth of nesting (number of parens).
	
	/// Returns true if elements can be popped at the current nesting.
	bool canPop() const noexcept
	{
		return !stack.empty() && stack.back().depth == depth;
	}
	
	/// Returns true if elements of the given type can be popped at the current nesting.
	bool canPop(ObjectQuery::Operator op) const noexcept {
		return canPop() && stack.back().expression->getOperator() == op;
	}
	
	/// Pushes an expression and the current depth to the stack.
	void push(ObjectQuery* expression)
	{
		stack.push_back(Element{expression, depth});
	}
	
	/// Removes an expression from the stack, and returns it.
	ObjectQuery* pop() noexcept {
		auto* expression = stack.back().expression;
		stack.pop_back();
		return expression;
	}
};


/**
 * Create a cheap placeholder ObjectQuery.
 * 
 * During object query parsing, operands right of operators are unknown
 * at construction time. However, ObjectQuery does not allowed to construct
 * logical query instances with invalid operands. This function creates valid
 * placeholders which can be replaced with an invalid operand in a second step.
 */
OpenOrienteering::ObjectQuery placeholder()
{
	return { static_cast<const OpenOrienteering::Symbol*>(nullptr) };
}

}  // namespace



namespace OpenOrienteering {

// ### ObjectQuery::LogicalOperands ###

ObjectQuery::LogicalOperands::LogicalOperands(const ObjectQuery::LogicalOperands& proto)
: first(proto.first ? std::make_unique<ObjectQuery>(*proto.first) : std::make_unique<ObjectQuery>())
, second(proto.second ? std::make_unique<ObjectQuery>(*proto.second) : std::make_unique<ObjectQuery>())
{
	// nothing else
}


ObjectQuery::LogicalOperands::~LogicalOperands() = default;


ObjectQuery::LogicalOperands& ObjectQuery::LogicalOperands::operator=(const ObjectQuery::LogicalOperands& proto)
{
	if (&proto == this)
		return *this;
	
	if (proto.first)
		first = std::make_unique<ObjectQuery>(*proto.first);
	if (proto.second)
		second = std::make_unique<ObjectQuery>(*proto.second);
	return *this;
}



// ### ObjectQuery::StringOperands ###

ObjectQuery::StringOperands::~StringOperands() = default;



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
		; // nothing
	}
	else if (op < 16)
	{
		new (&subqueries) LogicalOperands(query.subqueries);
	}
	else if (op < 32)
	{
		new (&tags) StringOperands(query.tags);
	}
	else if (op == ObjectQuery::OperatorSymbol)
	{
		symbol = query.symbol;
	}
}


ObjectQuery::ObjectQuery(ObjectQuery&& proto) noexcept
: op { ObjectQuery::OperatorInvalid }
{
	consume(std::move(proto));
}


ObjectQuery& ObjectQuery::operator=(const ObjectQuery& proto) noexcept
{
	if (&proto == this)
		return *this;
	
	reset();
	consume(ObjectQuery{proto});
	return *this;
}


ObjectQuery& ObjectQuery::operator=(ObjectQuery&& proto) noexcept
{
	if (&proto == this)
		return *this;
	
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


ObjectQuery::ObjectQuery(ObjectQuery::Operator op, const QString& value)
: op { op }
, tags { {}, value }
{
	// Can't have an empty key (but can have empty value)
	// Must be a key/value operator
	Q_ASSERT(op >= 19);
	Q_ASSERT(op <= 20);
	if (op < 19 || op > 20
	    || value.length() == 0)
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
	Q_ASSERT(op <= 3);
	if (op < 1 || op > 3
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
	Q_ASSERT(op <= 3);
	if (op < 1 || op > 3
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


ObjectQuery::ObjectQuery(const Symbol* symbol) noexcept
: op { ObjectQuery::OperatorSymbol }
, symbol { symbol }
{
	// nothing else
}


// static
ObjectQuery ObjectQuery::negation(ObjectQuery query) noexcept
{
	return { placeholder(), OperatorNot, std::move(query) };
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
	case OperatorSearch:
		//: Very short label
		return tr("Search");
	case OperatorObjectText:
		//: Very short label
		return tr("Text");
		
	case OperatorAnd:
		//: Very short label
		return tr("and");
	case OperatorOr:
		//: Very short label
		return tr("or");
	case OperatorNot:
		//: Very short label
		return tr("not");
		
	case OperatorSymbol:
		//: Very short label
		return tr("Symbol");
		
	case OperatorInvalid:
		//: Very short label
		return tr("invalid");
	}
	
	Q_UNREACHABLE();
}



bool ObjectQuery::operator()(const Object* object) const
{
	switch(op)
	{
	case OperatorIs:
		return [](auto const& container,  auto const& tags) {
			auto const it = container.find(tags.key);
			return it != container.end() && it->value == tags.value;
		} (object->tags(), tags);
	case OperatorIsNot:
		// If the object does have the tag, not is true
		return [](auto const& container,  auto const& tags) {
			auto const it = container.find(tags.key);
			return it == container.end() || it->value != tags.value;
		} (object->tags(), tags);
	case OperatorContains:
		return [](auto const& container,  auto const& tags) {
			auto const it = container.find(tags.key);
			return it != container.end() && it->value.contains(tags.value);
		} (object->tags(), tags);
	case OperatorSearch:
		if (object->getSymbol() && object->getSymbol()->getName().contains(tags.value, Qt::CaseInsensitive))
			return true;
		for (auto const& current : object->tags())
		{
			if (current.key.contains(tags.value, Qt::CaseInsensitive)
			    || current.value.contains(tags.value, Qt::CaseInsensitive))
				return true;
		}
		return false;
	case OperatorObjectText:
		if (object->getType() == Object::Text)
		    return static_cast<const TextObject*>(object)->getText().contains(tags.value, Qt::CaseInsensitive);
		return false;
		
	case OperatorAnd:
		return (*subqueries.first)(object) && (*subqueries.second)(object);
	case OperatorOr:
		return (*subqueries.first)(object) || (*subqueries.second)(object);
	case OperatorNot:
		return !(*subqueries.second)(object);
		
	case OperatorSymbol:
		return object->getSymbol() == symbol;
		
	case OperatorInvalid:
		return false;
	}
	
	Q_UNREACHABLE();
}



const ObjectQuery::LogicalOperands* ObjectQuery::logicalOperands() const
{
	const LogicalOperands* result = nullptr;
	if (op >= 1 && op <= 3)
	{
		result = &subqueries;
	}
	else
	{
		Q_ASSERT(op >= 1);
		Q_ASSERT(op <= 3);
	}
	return result;
}


const ObjectQuery::StringOperands* ObjectQuery::tagOperands() const
{
	const StringOperands* result = nullptr;
	if (op >= 16 && op <= 20)
	{
		result = &tags;
	}
	else
	{
		Q_ASSERT(op >= 16);
		Q_ASSERT(op <= 20);
	}
	return result;
}


const Symbol* ObjectQuery::symbolOperand() const
{
	const Symbol* result = nullptr;
	if (op == ObjectQuery::OperatorSymbol)
	{
		result = symbol;
	}
	else
	{
		Q_ASSERT(op == ObjectQuery::OperatorSymbol);
	}
	return result;
}



QString ObjectQuery::toString() const
{
	auto ret = QString{};
	switch (op)
	{
	case OperatorIs:
		ret = keyToString(tags.key) + QLatin1String(" = \"") + toEscaped(tags.value) + QLatin1Char('"');
		break;
	case OperatorIsNot:
		ret = keyToString(tags.key) + QLatin1String(" != \"") + toEscaped(tags.value) + QLatin1Char('"');
		break;
	case OperatorContains:
		ret = keyToString(tags.key) + QLatin1String(" ~= \"") + toEscaped(tags.value) + QLatin1Char('"');
		break;
	case OperatorSearch:
	case OperatorObjectText:
		ret = QLatin1Char('"') + toEscaped(tags.value) + QLatin1Char('"');
		break;
		
	case OperatorAnd:
		if (subqueries.first->getOperator() == OperatorOr)
			ret = QLatin1Char('(') + subqueries.first->toString() + QLatin1Char(')');
		else
			ret = subqueries.first->toString();
		ret += QLatin1String(" AND ");
		if (subqueries.second->getOperator() == OperatorOr)
			ret += QLatin1Char('(') + subqueries.second->toString() + QLatin1Char(')');
		else
			ret += subqueries.second->toString();
		break;
	case OperatorOr:
		ret = subqueries.first->toString() + QLatin1String(" OR ") + subqueries.second->toString();
		break;
	case OperatorNot:
		ret = QLatin1String("NOT ") + subqueries.second->toString();
		break;
		
	case OperatorSymbol:
		ret = QLatin1String("SYMBOL \"") + (symbol ? symbol->getNumberAsString() : QString{}) + QLatin1Char('\"');
		break;
		
	case OperatorInvalid:
		// Default empty string is sufficient
		break;
	}
	
	return ret;
}



void ObjectQuery::reset()
{
	if (op == ObjectQuery::OperatorInvalid)
	{
		; // nothing
	}
	else if (op < 16)
	{
		subqueries.~LogicalOperands();
		op = ObjectQuery::OperatorInvalid;
	}
	else if (op < 32)
	{
		tags.~StringOperands();
		op = ObjectQuery::OperatorInvalid;
	}
	else if (op == ObjectQuery::OperatorSymbol)
	{
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
		new (&subqueries) ObjectQuery::LogicalOperands(std::move(other.subqueries));
		other.subqueries.~LogicalOperands();
	}
	else if (op < 32)
	{
		new (&tags) ObjectQuery::StringOperands(std::move(other.tags));
		other.tags.~StringOperands();
	}
	else if (op == ObjectQuery::OperatorSymbol)
	{
		symbol = other.symbol;
	}
	other.op = ObjectQuery::OperatorInvalid;
}



bool operator==(const ObjectQuery::StringOperands& lhs, const ObjectQuery::StringOperands& rhs)
{
	return lhs.key == rhs.key && lhs.value == rhs.value;
}

bool operator==(const ObjectQuery& lhs, const ObjectQuery& rhs)
{
	if (lhs.op != rhs.op)
		return false;
	
	switch(lhs.op)
	{
	case ObjectQuery::OperatorIs:
	case ObjectQuery::OperatorIsNot:
	case ObjectQuery::OperatorContains:
	case ObjectQuery::OperatorSearch:
	case ObjectQuery::OperatorObjectText:
		return lhs.tags == rhs.tags;
		
	case ObjectQuery::OperatorAnd:
	case ObjectQuery::OperatorOr:
		return *lhs.subqueries.first == *rhs.subqueries.first
		       && *lhs.subqueries.second == *rhs.subqueries.second;
	case ObjectQuery::OperatorNot:
		return *lhs.subqueries.second == *rhs.subqueries.second;
		
	case ObjectQuery::OperatorSymbol:
		return lhs.symbol == rhs.symbol;
		
	case ObjectQuery::OperatorInvalid:
		return false;
	}
	
	Q_UNREACHABLE();
}



// ### ObjectQueryParser ###

void ObjectQueryParser::setMap(const Map* map)
{
	this->map = map;
}


ObjectQuery ObjectQueryParser::parse(const QString& text)
{
	auto result = ObjectQuery{};
	
	input = {&text, 0, text.size()};
	pos = 0;
	ObjectQueryNesting nested_expressions;
	auto* current = &result;
	
	getToken();
	while (token != TokenNothing)
	{
		if ((token == TokenWord || token == TokenString) && !*current)
		{
			// Parse a <TestTerm>.
			auto key = tokenAsString();
			getToken();
			if (token == TokenTextOperator)
			{
				auto op = token_text;
				getToken();
				if (token == TokenWord || token == TokenString)
				{
					auto value = tokenAsString();
					switch (op.at(0).toLatin1())
					{
					case '=':
						*current = { key, ObjectQuery::OperatorIs, value };
						break;
					case '!':
						*current = { key, ObjectQuery::OperatorIsNot, value };
						break;
					case '~':
						*current = { key, ObjectQuery::OperatorContains, value };
						break;
					default:
						Q_UNREACHABLE();
					}
					getToken();
				}
				else
				{
					op = {};
					break;
				}
			}
			else
			{
				*current = { ObjectQuery::OperatorSearch, key };
			}
			// Finish parsing of "NOT NOT NOT x", making the top-most NOT the current expression.
			while (nested_expressions.canPop(ObjectQuery::OperatorNot))
				current = nested_expressions.pop();
			// After a <TestTerm>, finish parsing of the right side of AND expression.
			while (nested_expressions.canPop(ObjectQuery::OperatorAnd))
				current = nested_expressions.pop();
		}
		else if (token == TokenSymbol && !*current)
		{
			// Start parsing right side of SYMBOL expression.
			getToken();
			if (token == TokenWord || token == TokenString)
			{
				auto const key = tokenAsString();
				auto* symbol = findSymbol(key);
				if (symbol || key.isEmpty())
					*current = ObjectQuery{symbol};
				getToken();
			}
			else
			{
				result = {};
				break;
			}
		}
		else if (token == TokenNot && !*current)
		{
			// Start parsing right side of NOT expression.
			*current = ObjectQuery::negation(placeholder());
			nested_expressions.push(current);
			current = current->logicalOperands()->second.get();
			*current = {};
			getToken();
		}
		else if (token == TokenAnd && *current)
		{
			// Start parsing right side of AND expression.
			*current = ObjectQuery{std::move(*current), ObjectQuery::OperatorAnd, placeholder()};
			nested_expressions.push(current);
			current = current->logicalOperands()->second.get();
			*current = {};
			getToken();
		}
		else if (token == TokenOr && *current)
		{
			// Terminate parsing of the current expression, which becomes the
			// left side of an OR expression, and start parsing the right side.
			while (nested_expressions.canPop())
				current = nested_expressions.pop();
			*current = ObjectQuery{std::move(*current), ObjectQuery::OperatorOr, placeholder()};
			nested_expressions.push(current);
			current = current->logicalOperands()->second.get();
			*current = {};
			getToken();
		}
		else if (token == TokenLeftParen && !*current)
		{
			// Start parsing of a new <ObjectQuery> (within pair of parens).
			nested_expressions.push(current);
			++nested_expressions.depth;
			getToken();
		}
		else if (token == TokenRightParen && *current && nested_expressions.depth > 0)
		{
			// End of <ParenExpression>.
			// Finish parsing of the current <ObjectQuery> (within pair of parens),
			// and make it the current expression.
			while (nested_expressions.canPop())
				nested_expressions.pop();
			--nested_expressions.depth;
			if (nested_expressions.canPop())
			{
				current = nested_expressions.pop();
				// After a <ParenExpression>, finish parsing a "NOT NOT NOT (x)" expression.
				while (nested_expressions.canPop(ObjectQuery::OperatorNot))
					current = nested_expressions.pop();
				// Now finish parsing of the right side of AND expression.
				while (nested_expressions.canPop(ObjectQuery::OperatorAnd))
					current = nested_expressions.pop();
			}
			else
			{
				current = {};
			}
			getToken();
		}
		else
		{
			// Invalid input
			result = {};
			break;
		}
	}
	
	if (result && (!*current || nested_expressions.depth > 0))
	{
		result = {};
	}
	
	return result;
}


int ObjectQueryParser::errorPos() const
{
	return token_start;
}


void ObjectQueryParser::getToken()
{
	token = TokenNothing;
	QChar current;
	while (true)
	{
		if (pos >= input.length())
			return;
		current = input.at(pos);
		if (current != QLatin1Char(' ') && current != QLatin1Char('\t'))
			break;
		++pos;
	}
	
	token = TokenUnknown;
	token_start = pos;
	if (current == QLatin1Char('"'))
	{
		for (++pos; pos < input.length(); ++pos)
		{
			current = input.at(pos);
			if (current == QLatin1Char('"'))
			{
				token = TokenString;
				token_text = input.mid(token_start + 1, pos - token_start - 1);
				++pos;
				break;
			}
			else if (current == QLatin1Char('\\'))
			{
				if (pos < input.length())
					++pos;
			}
		}
	}
	else if (current == QLatin1Char('('))
	{
		token = TokenLeftParen;
		token_text = input.mid(token_start, 1);
		++pos;
	}
	else if (current == QLatin1Char(')'))
	{
		token = TokenRightParen;
		token_text = input.mid(token_start, 1);
		++pos;
	}
	else if (current == QLatin1Char('='))
	{
		token = TokenTextOperator;
		token_text = input.mid(token_start, 1);
		++pos;
	}
	else if ((current == QLatin1Char('!') || current == QLatin1Char('~'))
	         && pos+1 < input.length()
	         && input.at(pos+1) == QLatin1Char('='))
	{
		token = TokenTextOperator;
		token_text = input.mid(token_start, 2);
		pos += 2;
	}
	else
	{
		for (++pos; pos < input.length(); ++pos)
		{
			current = input.at(pos);
			if (current == QLatin1Char('"')
			    || current == QLatin1Char(' ')
			    || current == QLatin1Char('\t')
			    || current == QLatin1Char('(')
			    || current == QLatin1Char(')')
			    || current == QLatin1Char('='))
			{
				break;
			}
			else if ((current == QLatin1Char('!') || current == QLatin1Char('~'))
			         && pos+1 < input.length()
			         && input.at(pos+1) == QLatin1Char('='))
			{
				break;
			}
		}
		token_text = input.mid(token_start, pos - token_start);
		if (token_text == QLatin1String("OR"))
			token = TokenOr;
		else if (token_text == QLatin1String("AND"))
			token = TokenAnd;
		else if (token_text == QLatin1String("NOT"))
			token = TokenNot;
		else if (token_text == QLatin1String("SYMBOL"))
			token = TokenSymbol;
		else
			token = TokenWord;
	}
}


QString ObjectQueryParser::tokenAsString() const
{
	QString string = token_text.toString();
	if (token == TokenString)
		string = fromEscaped(string);
	return string;
}


const Symbol* ObjectQueryParser::findSymbol(const QString& key) const
{
	auto const num_symbols = map ? map->getNumSymbols() : 0;
	for (int k = 0; k < num_symbols; ++k)
	{
		auto* symbol = map->getSymbol(k);
		if (symbol->getNumberAsString() == key)
			return symbol;
	}
	return nullptr;
}


}  // namespace OpenOrienteering
