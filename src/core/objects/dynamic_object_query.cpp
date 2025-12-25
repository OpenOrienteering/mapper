/*
 *    Copyright 2025 Matthias Kühlewein
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

#include "core/map.h"
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
	QStringList symbol_types = {QLatin1String("Point"), QLatin1String("Line"), QLatin1String("Area"), QLatin1String("Text"), QLatin1String("Combined")};
	int operation;
	
	if (element.startsWith(QLatin1String("==")))
		operation = 0;
	else if (element.startsWith(QLatin1String("!=")))
		operation = 1;
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
		result = operation ? (1<<index != symbol_type) : (1<<index == symbol_type);
		return true;
	}
	return false;
}

}  // namespace


namespace OpenOrienteering {

DynamicObjectQueryManager::DynamicObjectQueryManager()
{
	// nothing else
}

DynamicObjectQuery::~DynamicObjectQuery()= default;

DynamicObjectQuery* DynamicObjectQueryManager::parse(QStringRef token_text, QStringRef token_attributes_text)
{
	if (AreaObjectQuery::getKeyword() == token_text)
	{
		return new AreaObjectQuery(token_attributes_text);
	}
	else if (LineObjectQuery::getKeyword() == token_text)
	{
		return new LineObjectQuery(token_attributes_text);
	}
	else if (SymbolQuery::getKeyword() == token_text)
	{
		return new SymbolQuery(token_attributes_text);
	}
/*
	else if (ObjectQuery::getKeyword() == token_text)
	{
		return new ObjectQuery(token_attributes_text);
	}
*/
	return nullptr;		// 	we should not get here
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
/*
		case DynamicObjectQuery::ObjectQuery:
			return static_cast<const ObjectQuery*>(dynamic_query)->performQuery(object);
*/
		default:
			return false;	// we should not get here
	}
	
	Q_UNREACHABLE();
}


/* Bsp. und Ideen:
 * AREA(ISTOOSMALL)
 * AREA(REAL;<= 20.5;OR;>= 30.2)
 * SYMBOL(ISUNDEFINED) bzw. SYMBOL(TYPE;== UNDEFINED)
 * SYMBOL(TYPE;== AREA)
 * SYMBOL(TYPE;== ISHELPER)
 * SYMBOL(ID;== 102.1)
 * */
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
	bool symbol_type_result;
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
		else if (symbol_property == 0 && EvaluateSymbolType(element, (symbol ? symbol->getType() : 0), symbol_type_result))
		{
			result = and_or_operation ? (result || symbol_type_result) : (result && symbol_type_result);
			and_or_operation = 0;	// default after first operation is AND operation
		}
		//else if (symbol_property == 1 && ....)
		else	// unknown element or failure in EvaluateSymbolType()
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

/*
ObjectQuery::ObjectQuery(QStringRef token_attributes_text)
: DynamicObjectQuery(DynamicObjectQuery::ObjectQuery)
{
	parseTokenAttributes(token_attributes_text);
	valid = performQuery(nullptr);	// dry run
}
*/

DynamicObjectQuery::DynamicObjectQuery(Type type) noexcept
: type { type }
{
	// nothing else
}

void DynamicObjectQuery::parseTokenAttributes(QStringRef token_attributes_text)
{
	attributes = token_attributes_text.toString().split(QLatin1Char(';'), Qt::SkipEmptyParts);
	for (auto& item : attributes)
		item = item.trimmed();
}


}  // namespace OpenOrienteering
