/*
 *    Copyright 2017-2020 Kai Pastor
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


#include "symbol_rule_set.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include <QtGlobal>
#include <QChar>
#include <QHash>
#include <QLatin1Char>
#include <QLatin1String>
#include <QString>
#include <QTextStream>

#include "core/map.h"
#include "core/objects/object.h"
#include "core/symbols/symbol.h"
#include "undo/undo_manager.h"


namespace OpenOrienteering {

// The SymbolRule member "query" may throw on copying.
Q_STATIC_ASSERT(std::is_nothrow_constructible<SymbolRule>::value);
Q_STATIC_ASSERT(std::is_nothrow_default_constructible<SymbolRule>::value);
Q_STATIC_ASSERT(std::is_copy_constructible<SymbolRule>::value);
Q_STATIC_ASSERT(std::is_nothrow_move_constructible<SymbolRule>::value);
Q_STATIC_ASSERT(std::is_nothrow_destructible<SymbolRule>::value);
Q_STATIC_ASSERT(std::is_copy_assignable<SymbolRule>::value);
Q_STATIC_ASSERT(std::is_nothrow_move_assignable<SymbolRule>::value);

// Analogously for SymbolRuleSet
Q_STATIC_ASSERT(std::is_nothrow_constructible<SymbolRuleSet>::value);
Q_STATIC_ASSERT(std::is_nothrow_default_constructible<SymbolRuleSet>::value);
Q_STATIC_ASSERT(std::is_copy_constructible<SymbolRuleSet>::value);
Q_STATIC_ASSERT(std::is_nothrow_move_constructible<SymbolRuleSet>::value);
Q_STATIC_ASSERT(std::is_nothrow_destructible<SymbolRuleSet>::value);
Q_STATIC_ASSERT(std::is_copy_assignable<SymbolRuleSet>::value);
Q_STATIC_ASSERT(std::is_nothrow_move_assignable<SymbolRuleSet>::value);


// static
SymbolRuleSet SymbolRuleSet::forMatchingSymbols(const Map& map, const std::function<bool (int)>& predicate)
{
	SymbolRuleSet list;
	list.reserve(std::size_t(map.getNumSymbols()));
	for (int i = 0; i < map.getNumSymbols(); ++i)
	{
		if (!predicate(i))
			continue;
		auto original = map.getSymbol(i);
		list.push_back({{original}, nullptr, SymbolRule::NoAssignment});
	}
	list.sortByQueryKeyAndValue();
	return list;
}

// static
SymbolRuleSet SymbolRuleSet::forAllSymbols(const Map& map)
{
	return SymbolRuleSet::forMatchingSymbols(map, [](int /*i*/) { return true; });
}

// static
SymbolRuleSet SymbolRuleSet::forUsedSymbols(const Map& map)
{
	std::vector<bool> symbols_in_use(std::size_t(map.getNumSymbols()));
	map.determineSymbolsInUse(symbols_in_use);
	return SymbolRuleSet::forMatchingSymbols(map, [&symbols_in_use](int i) { return symbols_in_use[std::size_t(i)]; });
}


SymbolRuleSet& SymbolRuleSet::squeeze()
{
	auto is_NoAssignment = [](auto const& item) {
		return item.type == SymbolRule::NoAssignment;
	};
	erase(std::remove_if(begin(), end(), is_NoAssignment), end());
	return *this;
}


SymbolRuleSet SymbolRuleSet::squeezed() const
{
	return SymbolRuleSet(*this).squeeze();
}


void SymbolRuleSet::merge(SymbolRuleSet&& other, MergeMode mode)
{
	for (auto& item : other)
	{
		for (auto& current : *this)
		{
			if (item.query == current.query)
			{
				if (item.type != SymbolRule::NoAssignment)
				{
					current.symbol = item.symbol;
					current.type = item.type;
					item = {};
				}
				break;
			}
		}
	}
	
	if (mode == UpdateAndAppend)
	{
		for (auto&& item : other)
		{
			if (item.query.getOperator() != ObjectQuery::OperatorInvalid)
			{
				emplace_back(std::move(item));
			}
		}
	}
}



void SymbolRuleSet::sortByQuerySymbol()
{
	auto last = std::remove_if(begin(), end(), [](auto item) {
		return item.query.getOperator() != ObjectQuery::OperatorSymbol
		       || !item.query.symbolOperand();
	});
	std::sort(begin(), last, [](auto lhs, auto rhs) {
		auto lhs_op = lhs.query.symbolOperand();
		auto rhs_op = rhs.query.symbolOperand();
		return (lhs_op->getNumberAsString() < rhs_op->getNumberAsString()
		        && lhs_op->getName() < rhs_op->getName());
	});
}


void SymbolRuleSet::sortByQueryKeyAndValue()
{
	auto last = std::remove_if(begin(), end(), [](auto item) {
		const auto op = item.query.getOperator();
		return (op != ObjectQuery::OperatorIs
		        && op != ObjectQuery::OperatorIsNot
		        && op != ObjectQuery::OperatorContains)
		       || !item.query.tagOperands();
	});
	std::sort(begin(), last, [](auto lhs, auto rhs) {
		auto lhs_op = lhs.query.tagOperands();
		auto rhs_op = rhs.query.tagOperands();
		return (lhs_op->key < rhs_op->key)
		       && (lhs_op->value < rhs_op->value);
	});
}



void SymbolRuleSet::matchQuerySymbolName(const Map& other_map)
{
	auto findMatchingSymbol = [&other_map](const Symbol* original)->const Symbol*
	{
		for (int k = 0; k < other_map.getNumSymbols(); ++k)
		{
			auto other_symbol = other_map.getSymbol(k);
			if (original->getName() == other_symbol->getName()
			    && Symbol::areTypesCompatible(original->getType(), other_symbol->getType()))
			{
				return other_symbol;
			}
		}
		return nullptr;
	};
	
	for (auto& item : *this)
	{
		if (item.query.getOperator() != ObjectQuery::OperatorSymbol)
			continue;
		
		auto original = item.query.symbolOperand();
		if (!original)
			continue;
		
		auto candidate = findMatchingSymbol(original);
		if (!candidate)
		{
			candidate = findMatchingSymbol(original);
		}
		if (candidate != item.symbol)
		{
			if (candidate)
			{
				item.symbol = candidate;
				item.type = SymbolRule::AutomaticAssignment;
			}
			else 
			{
				item.type = SymbolRule::NoAssignment;
			}
		}
	}
}


void SymbolRuleSet::matchQuerySymbolNumber(const Map& other_map)
{
	auto findMatchingSymbol = [&other_map](const Symbol* original, auto compare)->const Symbol*
	{
		for (int k = 0; k < other_map.getNumSymbols(); ++k)
		{
			auto other_symbol = other_map.getSymbol(k);
			if ((original->*compare)(other_symbol)
				&& Symbol::areTypesCompatible(original->getType(), other_symbol->getType()))
			{
				return other_symbol;
			}
		}
		return nullptr;
	};
	
	for (auto& item : *this)
	{
		if (item.query.getOperator() != ObjectQuery::OperatorSymbol)
			continue;
		
		auto original = item.query.symbolOperand();
		if (!original)
			continue;
		
		auto candidate = findMatchingSymbol(original, &Symbol::numberEquals);
		if (!candidate)
		{
			candidate = findMatchingSymbol(original, &Symbol::numberEqualsRelaxed);
		}
		if (candidate != item.symbol)
		{
			if (candidate)
			{
				item.symbol = candidate;
				item.type = SymbolRule::AutomaticAssignment;
			}
			else 
			{
				item.type = SymbolRule::NoAssignment;
			}
		}
	}
}



// static
SymbolRuleSet SymbolRuleSet::loadCrt(QTextStream& stream, const Map& replacement_map)
{
	auto list = SymbolRuleSet{};
	stream.setIntegerBase(10); // No autodectection; 001 is 1.
	while (!stream.atEnd())
	{
		QString replacement_key;
		stream >> replacement_key;
		if (stream.status() == QTextStream::ReadPastEnd)
		{
			stream.resetStatus();
			break;
		}
		auto pattern = stream.readLine().trimmed();
		if (stream.status() == QTextStream::Ok
		    && !replacement_key.startsWith(QLatin1Char{'#'}))
		{
			auto parsed_query = ObjectQueryParser().parse(pattern);
			if (!parsed_query)
				parsed_query = {ObjectQuery::OperatorSearch, pattern};
			list.push_back({std::move(parsed_query), nullptr, SymbolRule::NoAssignment});
			for (int k = 0; k < replacement_map.getNumSymbols(); ++k)
			{
				auto symbol = replacement_map.getSymbol(k);
				if (symbol->getNumberAsString() == replacement_key)
				{
					list.back().symbol = symbol;
					list.back().type = SymbolRule::DefinedAssignment;
					break;
				}
			}
		}
	}
	return list;
}


void SymbolRuleSet::writeCrt(QTextStream& stream) const
{
	for (const auto& item : *this)
	{
		if (item.type != SymbolRule::NoAssignment
		    && item.symbol)
		{
			const auto& query = item.query;
			auto second_field = QString{};
			switch (query.getOperator())
			{
			case ObjectQuery::OperatorSearch:
				if (query.tagOperands())
					second_field = query.tagOperands()->value;
				break;
				
			case ObjectQuery::OperatorSymbol:
				if (query.symbolOperand())
					second_field = query.symbolOperand()->getNumberAsString();
				break;
				
			case ObjectQuery::OperatorIs:
				if (query.tagOperands()
				    && query.tagOperands()->key == QLatin1String("Layer"))
				{
					second_field = query.tagOperands()->value;
					break;
				}
				Q_FALLTHROUGH();
			case ObjectQuery::OperatorIsNot:
			case ObjectQuery::OperatorContains:
				if (query.tagOperands())
					second_field = query.tagOperands()->key + QLatin1Char(' ')
					               + query.labelFor(query.getOperator()) + QLatin1String(" \"")
					               + query.tagOperands()->value + QLatin1Char('"');
				break;
				
			default:
				qDebug("Unsupported query in SymbolRuleSet::writeCrt");
			}

			if (!second_field.isEmpty())
			{
				auto first_field = item.symbol->getNumberAsString();
				auto whitespace = QString(qMax(1, 10-first_field.length()), QLatin1Char{' '});
				stream <<  first_field << whitespace << second_field << endl;
			}
		}
	}
}



void SymbolRuleSet::recognizeSymbolPatterns(const Map& symbol_set)
{
	for (auto& item : *this)
	{
		if (item.type == SymbolRule::NoAssignment
		    || item.query.getOperator() != ObjectQuery::OperatorSearch)
			continue;
		Q_ASSERT(item.symbol);
		
		auto operands = item.query.tagOperands();
		if (!operands || operands->value.isEmpty())
			continue;
		
		// Find original symbol number matching the pattern
		for (int i = 0; i < symbol_set.getNumSymbols(); ++i)
		{
			auto symbol = symbol_set.getSymbol(i);
			if (symbol->getNumberAsString() == operands->value)
			{
				if (Symbol::areTypesCompatible(symbol->getType(), item.symbol->getType()))
					item.query = { symbol };
				break;
			}
		}
	}
}



const Symbol* SymbolRuleSet::findDuplicateSymbolPattern() const
{
	for (auto const& item : *this)
	{
		if (item.query.getOperator() != ObjectQuery::OperatorSymbol)
			continue;
		
		auto has_conflict = [&item](const auto& other)->bool {
			return &other != &item
			       && other.type != SymbolRule::NoAssignment
			       && item.query == other.query
			       && item.symbol != other.symbol;
		};
		using std::begin; using std::end;
		if (std::any_of(begin(*this), end(*this), has_conflict))
		{
			return item.symbol;
		}
	}
	return nullptr;
}



void SymbolRuleSet::operator()(Object* object) const
{
	auto const match = std::find_if(rbegin(), rend(), [object](auto const& item) {
		return item.symbol && item.query(object);
	});
	if (match != rend())
		object->setSymbol(match->symbol, false);
}


void SymbolRuleSet::apply(Map& object_map, const Map& symbol_set, Options options) const &
{
	SymbolRuleSet(*this).apply(object_map, symbol_set, options);
}

void SymbolRuleSet::apply(Map& object_map, const Map& symbol_set, Options options) &&
{
	squeeze();
	
	std::unordered_set<const Symbol*> old_symbols;
	if (options.testFlag(RemoveUnusedSymbols))
	{
		for (int i = 0; i < object_map.getNumSymbols(); ++i)
			old_symbols.insert(object_map.getSymbol(i));
	}
	
	// Import new symbols if needed
	if (&object_map != &symbol_set)
	{
		auto symbol_filter = std::vector<bool>(std::size_t(symbol_set.getNumSymbols()), true);
		if (!options.testFlag(ImportAllSymbols))
		{
			// Import only symbols which are chosen as replacement symbols
			auto item = symbol_filter.begin();
			for (int i = 0; i < symbol_set.getNumSymbols(); ++i)
			{
				auto symbol = symbol_set.getSymbol(i);
				*item = std::any_of(begin(), end(), [symbol](auto item) {
					return item.symbol == symbol;
				});
				++item;
			}
		}
		
		auto symbol_mapping = object_map.importMap(symbol_set, Map::MinimalSymbolImport, &symbol_filter, -1, false);
		
		auto preserve_state = options.testFlag(PreserveSymbolState);
		for (auto& item : *this)
		{
			if (!symbol_mapping.contains(item.symbol))
				continue; // unused symbol
			
			auto replacement = symbol_mapping[item.symbol];
			if (item.query.getOperator() == ObjectQuery::OperatorSymbol
			    && preserve_state)
			{
				auto original = item.query.symbolOperand();
				Q_ASSERT(original);
				replacement->setHidden(original->isHidden());
				replacement->setProtected(original->isProtected());
			}
			item.symbol = replacement;
		}
	}
	
	// Change symbols for all objects
	object_map.applyOnAllObjects(std::cref(*this));
	
	// Delete unused old symbols
	if (!old_symbols.empty())
	{
		std::vector<bool> symbols_in_use;
		object_map.determineSymbolsInUse(symbols_in_use);
		
		for (int i = object_map.getNumSymbols() - 1; i >= 0; --i)
		{
			auto symbol = object_map.getSymbol(i);
			if (!symbols_in_use[std::size_t(i)]
			    && old_symbols.find(symbol) != old_symbols.end())
			{
				object_map.deleteSymbol(i);
			}
		}
	}
	
	// Delete unused colors
	if (options.testFlag(RemoveUnusedColors))
	{
		std::vector<bool> all_symbols;
		all_symbols.assign(std::size_t(object_map.getNumSymbols()), true);
		std::vector<bool> colors_in_use;
		object_map.determineColorsInUse(all_symbols, colors_in_use);
		if (colors_in_use.empty())
			colors_in_use.assign(std::size_t(object_map.getNumColors()), false);
		
		for (int i = object_map.getNumColors() - 1; i >= 0; --i)
		{
			if (!colors_in_use[std::size_t(i)])
				object_map.deleteColor(i);
		}
	}
	
	// Finish
	object_map.updateAllObjects();
	object_map.setObjectsDirty();
	object_map.setSymbolsDirty();
	object_map.undoManager().clear();
}


}  // namespace OpenOrienteering
