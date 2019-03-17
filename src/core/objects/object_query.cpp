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

#include <algorithm>
#include <iterator>
#include <new>

#include <Qt>
#include <QtGlobal>
#include <QChar>
#include <QHash>
#include <QLatin1Char>
#include <QLatin1String>
#include <QString>
#include <QVarLengthArray>

#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "core/symbols/symbol.h"


// ### Local utilites ###

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
			++i;
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

}



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


ObjectQuery::ObjectQuery(const Symbol* symbol) noexcept
: op { ObjectQuery::OperatorSymbol }
, symbol { symbol }
{
	// nothing else
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
	case OperatorSearch:
		if (object->getSymbol() && object->getSymbol()->getName().contains(tags.value, Qt::CaseInsensitive))
			return true;
		for (auto it = object_tags.begin(), last = object_tags.end(); it != last; ++it)
		{
			if (it.key().contains(tags.value, Qt::CaseInsensitive)
			    || it.value().contains(tags.value, Qt::CaseInsensitive))
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
		
	case OperatorSymbol:
		ret = QLatin1String("SYMBOL \"") + symbol->getNumberAsString() + QLatin1Char('\"');
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
		
	case ObjectQuery::OperatorSymbol:
		return lhs.symbol == rhs.symbol;
		
	case ObjectQuery::OperatorInvalid:
		return false;
	}
	
	Q_UNREACHABLE();
}


ObjectQuery ObjectQueryParser::parse(const QString& text)
{
	auto result = ObjectQuery{};
	
	input = {&text, 0, text.size()};
	pos = 0;
	auto paren_depth = 0;
	QVarLengthArray<ObjectQuery*, 8> nested_expressions;
	auto* current = &result;
	
	getToken();
	while (token != TokenNothing)
	{
		if ((token == TokenWord || token == TokenString) && !*current)
		{
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
		}
		else if (token == TokenAnd && *current)
		{
			if (!nested_expressions.isEmpty() && nested_expressions.back()->getOperator() == ObjectQuery::OperatorAnd)
			{
				nested_expressions.pop_back(); // replaced by current query
			}
			// Cannot construct logical ObjectQuery with invalid operand, but can assign later...
			auto tmp = ObjectQuery{std::move(*current), ObjectQuery::OperatorAnd, {ObjectQuery::OperatorSearch, text}};
			*current = std::move(tmp);
			nested_expressions.push_back(current);
			
			current = current->logicalOperands()->second.get();
			*current = {};
			
			getToken();
		}
		else if (token == TokenOr && *current)
		{
			if (!nested_expressions.isEmpty())
			{
				current = nested_expressions.back();
				nested_expressions.pop_back();
			}
			// Cannot construct logical ObjectQuery with invalid operand, but can assign later...
			auto query = ObjectQuery{std::move(*current), ObjectQuery::OperatorOr, {ObjectQuery::OperatorSearch, text}};
			*current = std::move(query);
			nested_expressions.push_back(current);
			
			current = current->logicalOperands()->second.get();
			*current = {};
			
			getToken();
		}
		else if (token == TokenLeftParen && !*current)
		{
			++paren_depth;
			nested_expressions.push_back(current);
			getToken();
		}
		else if (token == TokenRightParen && *current
		         && !nested_expressions.isEmpty()
		         && paren_depth > 0)
		{
			--paren_depth;
			current = nested_expressions.back();
			nested_expressions.pop_back();
			getToken();
		}
		else
		{
			// Invalid input
			result = {};
			break;
		}
	}
	
	if (result && (!*current || paren_depth > 0))
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
	         && pos < input.length()
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
			         && pos < input.length()
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


}  // namespace OpenOrienteering
