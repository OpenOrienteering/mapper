/*
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

#include "dynamic_object_query.h"

#include <algorithm>
#include <functional>

#include <Qt>
#include <QLatin1Char>
#include <QLatin1String>

#include "core/map.h"
#include "core/map_part.h"
#include "core/objects/object.h"
#include "core/symbols/symbol.h"

namespace {

struct CompOpStruct {
	const QString op;
	const std::function<bool (double, double)> fn;
};

static const CompOpStruct numerical_compare_operations[6] = {
	{ QLatin1String("<="), [](double op1, double op2) { return op1 <= op2; } },
	{ QLatin1String(">="), [](double op1, double op2) { return op1 >= op2; } },
	{ QLatin1String("=="), [](double op1, double op2) { return op1 == op2; } },
	{ QLatin1String("!="), [](double op1, double op2) { return op1 != op2; } },
	{ QLatin1String("<"),  [](double op1, double op2) { return op1 < op2; } },
	{ QLatin1String(">"),  [](double op1, double op2) { return op1 > op2; } },
};

bool NumericalComparison(bool compare, QString element, double op1 = 0.0)
{
	for (const auto& comp : numerical_compare_operations)
	{
		if (element.startsWith(comp.op))
		{
			const auto valuestr = element.remove(comp.op).trimmed();
			bool ok;
			const double op2 = valuestr.toDouble(&ok);
			if (!compare)
				return ok;
			return (comp.fn)(op1, op2);
		}
	}
	return false;
}

bool EvaluateAndOrOperation(const QString& element, int& and_or_operation)
{
	if (element == QLatin1String("AND"))
		and_or_operation = 0;
	else if (element == QLatin1String("OR"))
		and_or_operation = 1;
	else
		return false;
	return true;
}

bool EvaluatePaperReal(const QString& element, int& type)
{
	if (element == QLatin1String("PAPER"))
		type = 0;
	else if (element == QLatin1String("REAL"))
		type = 1;
	else
		return false;
	return true;
}

bool EvaluateSymbolType(QString element, int symbol_type, bool& result)
{
	const QStringList symbol_types = {QLatin1String("Point"), QLatin1String("Line"), QLatin1String("Area"), QLatin1String("Text"), QLatin1String("Combined")};
	int operation;
	
	if (element.startsWith(QLatin1String("==")))
		operation = 1;
	else if (element.startsWith(QLatin1String("!=")))
		operation = 0;
	else
		return false;
	
	const auto valuestr = element.remove(0, 2).trimmed();
	//auto index = symbol_types.indexOf(valuestr, 0, Qt::CaseInsensitive);	// Qt 6.7
	auto match = std::find_if(symbol_types.begin(), symbol_types.end(), [&valuestr](const auto& item) {
		return !item.compare(valuestr, Qt::CaseInsensitive);
	});
	if (match != symbol_types.end())
	{
		auto index = match - symbol_types.begin();
		result = operation ? (1<<index == symbol_type) : (1<<index != symbol_type);
		return true;
	}
	return false;
}

bool EvaluateSymbolId(QString element, QString symbol_id, bool& result)
{
	int operation;
	
	if (element.startsWith(QLatin1String("==")))
		operation = 1;
	else if (element.startsWith(QLatin1String("!=")))
		operation = 0;
	else
		return false;
	
	const auto symbol_id_elements = symbol_id.split(QLatin1Char('.'));
	const auto valuestr = element.remove(0, 2).trimmed();
	if (valuestr.isEmpty())
		return false;
	const auto value_elements = valuestr.split(QLatin1Char('.'));
	if (value_elements.size() > 3)
		return false;

	for (auto i = 0; i < value_elements.size(); ++i)
	{
		if (i >= symbol_id_elements.size())
		{
			result = !(bool)operation;
			return true;
		}
		if (value_elements.at(i) == QLatin1Char('*'))
		{
			result = (bool)operation;
			return true;
		}
		if (value_elements.at(i) != symbol_id_elements.at(i))
		{
			result = !(bool)operation;
			return true;
		}
	}
	result = (bool)operation;
	return true;
}

}  // namespace


namespace OpenOrienteering {

const QStringList DynamicObjectQueryManager::keywords = {QLatin1String{"LINE"}, QLatin1String{"AREA"}, QLatin1String{"SYMBOL"}, QLatin1String{"OBJECT"}};

DynamicObjectQueryManager::DynamicObjectQueryManager()
{
	// nothing else
}

DynamicObjectQuery::~DynamicObjectQuery()= default;

DynamicObjectQuery* DynamicObjectQueryManager::parse(QStringRef token_text, QStringRef token_attributes_text)
{
	if (token_text == keywords[DynamicObjectQuery::AreaObjectQuery])
	{
		return new AreaObjectQuery(token_attributes_text);
	}
	else if (token_text == keywords[DynamicObjectQuery::LineObjectQuery])
	{
		return new LineObjectQuery(token_attributes_text);
	}
	else if (token_text == keywords[DynamicObjectQuery::SymbolQuery])
	{
		return new SymbolQuery(token_attributes_text);
	}
	else if (token_text == keywords[DynamicObjectQuery::GeneralObjectQuery])
	{
		return new GeneralObjectQuery(token_attributes_text);
	}
	return nullptr;
}

bool DynamicObjectQueryManager::performDynamicQuery(const Object* object, const DynamicObjectQuery* dynamic_query)
{
	switch (dynamic_query->getType())
	{
		case DynamicObjectQuery::AreaObjectQuery:
			if (object->getType() == Object::Path)
			{
				const auto& path_object = static_cast<const PathObject*>(object);
				const auto symbol = path_object->getSymbol();
				if (symbol && symbol->getContainedTypes() & Symbol::Area)
					return static_cast<const AreaObjectQuery*>(dynamic_query)->performQuery(path_object);
			}
			return false;
		case DynamicObjectQuery::LineObjectQuery:
			if (object->getType() == Object::Path)
			{
				const auto& path_object = static_cast<const PathObject*>(object);
				return static_cast<const LineObjectQuery*>(dynamic_query)->performQuery(path_object);
			}
			return false;
		case DynamicObjectQuery::SymbolQuery:
			{
				const auto symbol = object->getSymbol();
				const auto map = object->getMap();
				return symbol && map ? static_cast<const SymbolQuery*>(dynamic_query)->performQuery(map, symbol) : false;
			}
		case DynamicObjectQuery::GeneralObjectQuery:
			{
				const auto map = object->getMap();
				return map ? static_cast<const GeneralObjectQuery*>(dynamic_query)->performQuery(map, object) : false;
			}
		default:
			return false;	// we should not get here
	}
	
	Q_UNREACHABLE();
}

const QStringList DynamicObjectQueryManager::getContextKeywords(const QString& text, int position, bool& append)
{
	int keyword_found = -1;
	int i = position - 1;
	for ( ; i > 3; --i)	// consider shortest keyword
	{
		if (text.at(i) == QLatin1Char(')'))
		{
			break;
		}
		else if (text.at(i) == QLatin1Char('('))
		{
			for (auto j = 0; j < keywords.size(); ++j)
			{
				auto match = text.lastIndexOf(keywords[j], i-1);
				if (match != -1 && match == i - keywords[j].length())
				{
					keyword_found = j;
					break;
				}
			}
		}
	}
	
	if (keyword_found == -1)
	{
		QStringList context_keywords;
		for (auto keyword : keywords)
			context_keywords.append(keyword.append(QLatin1String("()")));
		append = true;
		return context_keywords;
	}
	
	auto parameters = text.mid(i, position - i).trimmed();
	
	switch (keyword_found)
	{
		case DynamicObjectQuery::LineObjectQuery:
		case DynamicObjectQuery::AreaObjectQuery:
			for (const auto& comp : numerical_compare_operations)
			{
				if (parameters.endsWith(comp.op))
					return QStringList();
			}
			{
				QStringList keywords = {keyword_found == DynamicObjectQuery::LineObjectQuery ? QLatin1String("ISTOOSHORT;") : QLatin1String("ISTOOSMALL;")};
				keywords += QString(QLatin1String("PAPER; REAL; AND; OR;")).split(QLatin1Char(' '));
				for (const auto& comp : numerical_compare_operations)
					keywords += comp.op;
				return keywords;
			}
			
		case DynamicObjectQuery::SymbolQuery:
			if (parameters.endsWith(QLatin1String("==")) || parameters.endsWith(QLatin1String("!=")))
			{
				const auto find_type_keyword = parameters.lastIndexOf(QLatin1String("TYPE"));
				const auto find_id_keyword = parameters.lastIndexOf(QLatin1String("ID"));
				if (find_type_keyword >= find_id_keyword)	// if both keywords are not found the '=' part is needed for the -1 values
				{
					return QString(QLatin1String("Point; Line; Area; Text; Combined;")).split(QLatin1Char(' '));
				}
				return QStringList();
			}
			return QString(QLatin1String("ISUNDEFINED; TYPE; ID; == != AND; OR;")).split(QLatin1Char(' '));
			
		case DynamicObjectQuery::GeneralObjectQuery:
			return QStringList{QLatin1String("IGNORESYMBOL;"), QLatin1String("ISDUPLICATE;")};
			
		default:
			return QStringList();	// we should not get here
	}
	
	Q_UNREACHABLE();
}


AreaObjectQuery::AreaObjectQuery(QStringRef token_attributes_text)
: DynamicObjectQuery(DynamicObjectQuery::AreaObjectQuery)
{
	parseTokenAttributes(token_attributes_text);
	valid = performQuery(nullptr);	// dry run
}

bool AreaObjectQuery::performQuery(const PathObject* path_object) const
{
	int area_type = 0;	// 0 = PAPER, 1 = REAL
	int and_or_operation = 1;	// 0 = AND, 1 = OR
	double object_value;
	int object_value_status = -1;	// -1 = undefined, 0 = PAPER value, 1 = REAL value
	
	bool result = false;
	for (auto& element : attributes)
	{
		if (EvaluateAndOrOperation(element, and_or_operation))
			continue;
		else if (EvaluatePaperReal(element, area_type))
			continue;
		else if (element == QLatin1String("ISTOOSMALL"))
		{
			const bool is_too_small = path_object ? path_object->isAreaTooSmall() : false;
			result = and_or_operation ? (result || is_too_small) : (result && is_too_small);
			and_or_operation = 0;	// default after first operation is AND operation
		}
		else if (NumericalComparison(false, element))
		{
			if (object_value_status != area_type)
			{
				object_value = path_object ? (area_type ? path_object->calculateRealArea() : path_object->calculatePaperArea()) : 0.0;
				object_value_status = area_type;
			}
			const bool comparison_result = NumericalComparison(true, element, object_value);
			result = and_or_operation ? (result || comparison_result) : (result && comparison_result);
			and_or_operation = 0;	// default after first operation is AND operation
		}
		else	// unknown element or failure in NumericalComparison()
		{
			if (!path_object)
				return false;	// dry run failed
			break;
		}
	}
	if (path_object)
		return result;
	return true;	// dry run was successful
}


LineObjectQuery::LineObjectQuery(QStringRef token_attributes_text)
: DynamicObjectQuery(DynamicObjectQuery::LineObjectQuery)
{
	parseTokenAttributes(token_attributes_text);
	valid = performQuery(nullptr);	// dry run
}

bool LineObjectQuery::performQuery(const PathObject* path_object) const
{
	int line_type = 0;	// 0 = PAPER, 1 = REAL
	int and_or_operation = 1;	// 0 = AND, 1 = OR
	double object_value;
	int object_value_status = -1;	// -1 = undefined, 0 = PAPER value, 1 = REAL value
	
	bool result = false;
	for (auto& element : attributes)
	{
		if (EvaluateAndOrOperation(element, and_or_operation))
			continue;
		else if (EvaluatePaperReal(element, line_type))
			continue;
		else if (element == QLatin1String("ISTOOSHORT"))
		{
			const bool is_too_short = path_object ? path_object->isLineTooShort() : false;
			result = and_or_operation ? (result || is_too_short) : (result && is_too_short);
			and_or_operation = 0;	// default after first operation is AND operation
		}
		else if (NumericalComparison(false, element))
		{
			if (object_value_status != line_type)
			{
				object_value = path_object ? (line_type ? path_object->getRealLength() : path_object->getPaperLength()) : 0.0;
				object_value_status = line_type;
			}
			const bool comparison_result = NumericalComparison(true, element, object_value);
			result = and_or_operation ? (result || comparison_result) : (result && comparison_result);
			and_or_operation = 0;	// default after first operation is AND operation
		}
		else	// unknown element or failure in NumericalComparison()
		{
			if (!path_object)
				return false;	// dry run failed
			break;
		}
	}
	if (path_object)
		return result;
	return true;	// dry run was successful
}


SymbolQuery::SymbolQuery(QStringRef token_attributes_text)
: DynamicObjectQuery(DynamicObjectQuery::SymbolQuery)
{
	parseTokenAttributes(token_attributes_text);
	valid = performQuery(nullptr, nullptr);	// dry run
}


bool SymbolQuery::performQuery(const Map* map, const Symbol* symbol) const
{
	int and_or_operation = 1;	// 0 = AND, 1 = OR
	int symbol_property = 0;	// 0 = TYPE, 1 = ID
	
	bool result = false;
	bool eval_symbol_result;
	for (auto& element : attributes)
	{
		if (EvaluateAndOrOperation(element, and_or_operation))
			continue;
		else if (element == QLatin1String("TYPE"))
			symbol_property = 0;
		else if (element == QLatin1String("ID"))
			symbol_property = 1;
		else if (element == QLatin1String("ISUNDEFINED"))
		{
			const bool is_undefined = map && symbol ? (map->findSymbolIndex(symbol) < 0) : false;
			result = and_or_operation ? (result || is_undefined) : (result && is_undefined);
			and_or_operation = 0;	// default after first operation is AND operation
		}
		else if (symbol_property == 0 && EvaluateSymbolType(element, (symbol ? symbol->getType() : 0), eval_symbol_result))
		{
			result = and_or_operation ? (result || eval_symbol_result) : (result && eval_symbol_result);
			and_or_operation = 0;	// default after first operation is AND operation
		}
		else if (symbol_property == 1 && EvaluateSymbolId(element, (symbol ? symbol->getNumberAsString() : QLatin1String("1")), eval_symbol_result))
		{
			result = and_or_operation ? (result || eval_symbol_result) : (result && eval_symbol_result);
			and_or_operation = 0;	// default after first operation is AND operation
		}
		else	// unknown element or failure in EvaluateSymbolType() or EvaluateSymbolId()
		{
			if (!symbol)
				return false;	// dry run failed
			break;
		}
	}
	if (symbol)
		return result;
	return true;	// dry run was successful
}


GeneralObjectQuery::GeneralObjectQuery(QStringRef token_attributes_text)
: DynamicObjectQuery(DynamicObjectQuery::GeneralObjectQuery)
{
	parseTokenAttributes(token_attributes_text);
	valid = performQuery(nullptr, nullptr);	// dry run
}

bool GeneralObjectQuery::performQuery(const Map* map, const Object* object) const
{
	int and_or_operation = 1;	// 0 = AND, 1 = OR
	bool ignore_symbols = false;
	
	bool result = false;
	for (auto& element : attributes)
	{
		if (EvaluateAndOrOperation(element, and_or_operation))
			continue;
		else if (element == QLatin1String("IGNORESYMBOL"))
			ignore_symbols = true;
		else if (element == QLatin1String("ISDUPLICATE"))
		{
			if (!map || !object)
				return true;
			const bool isduplicate_result = map->getCurrentPart()->existsObject([object, ignore_symbols](auto const* o)
			                                { return object != o && object->equals(o, !ignore_symbols); }
			);
			result = and_or_operation ? (result || isduplicate_result) : (result && isduplicate_result);
			and_or_operation = 0;	// default after first operation is AND operation
		}
		else	// unknown element
		{
			if (!object)
				return false;	// dry run failed
			break;
		}
	}
	if (object)
		return result;
	return true;	// dry run was successful
}


DynamicObjectQuery::DynamicObjectQuery(Type type) noexcept
: type { type }
{
	// nothing else
}

void DynamicObjectQuery::parseTokenAttributes(QStringRef token_attributes_text)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	attributes = token_attributes_text.toString().split(QLatin1Char(';'), Qt::SkipEmptyParts);
#else
	attributes = token_attributes_text.toString().split(QLatin1Char(';'), QString::SkipEmptyParts);
#endif
	for (auto& item : attributes)
		item = item.trimmed();
}


}  // namespace OpenOrienteering
