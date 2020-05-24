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


#ifndef OPENORIENTEERING_SYMBOL_RULE_SET_H
#define OPENORIENTEERING_SYMBOL_RULE_SET_H

#include <functional>
#include <vector>

#include <QFlags>

#include "core/objects/object_query.h"

class QTextStream;

namespace OpenOrienteering {

class Map;
class Object;
class Symbol;


/**
 * This struct defines a single symbol assignment rule.
 * 
 * Unless the type is NoAssignment, a rule specifies a new symbol for objects
 * which match a particular query.
 */
struct SymbolRule
{
	// Intentionally no user-defined constructors, destructor, assignment.
	// Default ones are fine.
	
	enum RuleType
	{
		NoAssignment = 0,
		ManualAssignment,
		AutomaticAssignment,
		DefinedAssignment,
	};
	
	ObjectQuery   query;
	const Symbol* symbol;
	RuleType      type;
	
};


/**
 * An utility for assigning symbols to objects based on queries.
 * 
 * When the rule set is applied on a map or object, the rules are tried in
 * reverse order. After the first matching rule with a valid assigned symbol,
 * no further attempts are made for an object. This means lists using object
 * queries must have more specific rules following more general rules.
 */
class SymbolRuleSet : public std::vector<SymbolRule>
{
public:
	// Intentionally no user-defined constructors, destructor, assignment.
	// Default ones are fine.
	
	/**
	 * Creates a list which has a NoAssignment rule for every original symbol
	 * that the predicate returns true for.
	 * 
	 * The argument to the predicate is the symbol number.
	 * 
	 * The created list is sorted by formatted symbol number.
	 */
	static SymbolRuleSet forMatchingSymbols(const Map& map, const std::function<bool (int)>& predicate);
	
	/** 
	 * Creates a list which has a NoAssignment rule for every original symbol.
	 * 
	 * This function can be used to easily initialize variables of type
	 * SymbolRuleSet despite the absence of specialized constructors.
	 * 
	 * \see SymbolRuleSet::forMatchingSymbols
	 */
	static SymbolRuleSet forAllSymbols(const Map& map);
	
	/**
	 * Creates a list which has a NoAssignment rule for every used original symbol.
	 * 
	 * This function can be used to easily initialize variables of type
	 * SymbolRuleSet despite the absence of specialized constructors.
	 * 
	 * \see SymbolRuleSet::forMatchingSymbols
	 */
	static SymbolRuleSet forUsedSymbols(const Map& map);
	
	/**
	 * Remove all NoAssignment items.
	 */
	SymbolRuleSet& squeeze();
	
	/**
	 * Returns a copy which has all NoAssignment items removed.
	 */
	SymbolRuleSet squeezed() const;
	
	/**
	 * Modes for merging rule sets
	 */
	enum MergeMode
	{
		UpdateOnly,       ///< Update existing rules only.
		UpdateAndAppend,  ///< Update existing rules, and append new rules.
	};
	
	/**
	 * Merges a second rule set into this one.
	 */
	void merge(SymbolRuleSet&& other, MergeMode mode);
	
	/** 
	 * Sorts the items by original symbol number and name if possible.
	 * 
	 * The symbol number and name are only available for queries with operator
	 * ObjectQuery::OperatorSymbol. Other rules are moved to the end.
	 */
	void sortByQuerySymbol();
	
	/** 
	 * Sorts the items by tag query key and value if possible.
	 * 
	 * The tag key and value are only available for queries with operator
	 * ObjectQuery::OperatorIs, ObjectQuery::OperatorIsNot, and
	 * ObjectQuery::OperatorIs. Other rules are moved to the end.
	 */
	void sortByQueryKeyAndValue();
	
	
	/**
	 * Sets the assigned symbols to match the original symbol name if possible.
	 * 
	 * The symbol name is only available for queries with operator
	 * ObjectQuery::OperatorSymbol.
	 * 
	 * The symbols' types must be compatible.
	 */
	void matchQuerySymbolName(const Map& other_map);
	
	/**
	 * Sets the assigned symbols to match the original symbol number if possible.
	 * 
	 * The symbol number is only available for queries with operator
	 * ObjectQuery::OperatorSymbol.
	 * 
	 * The symbols' types must be compatible.
	 */
	void matchQuerySymbolNumber(const Map& other_map);
	
	
	/**
	 * Loads rules from a cross reference table (CRT) stream.
	 * 
	 * Each line in a CRT file takes the form:
	 * 
	 * REPLACEMENT    pattern
	 * 
	 * where REPLACEMENT is the formatted number of the replacement symbol.
	 * 
	 * If the replacement symbol number exists, the import creates a new entry
	 * of type DefinedReplacement with the given replacement symbol. Otherwise,
	 * the replacement symbool is set to nullptr, and the type is set to
	 * NoReplacement. No other validation is performed.
	 * 
	 * The query is set to operand ObjectQuery::OperatorSearch and the tag value
	 * is set to the given pattern.
	 */
	static SymbolRuleSet loadCrt(QTextStream& stream, const Map& replacement_map);
	
	/**
	 * Writes rules to a cross reference table (CRT) stream.
	 * 
	 * Entries of type NoAssignment are skipped.
	 * 
	 * \see loadCrt
	 */
	void writeCrt(QTextStream& stream) const;
	
	
	/**
	 * Converts plain search patterns into symbol patterns where possible.
	 */
	void recognizeSymbolPatterns(Map const& symbol_set);
	
	/**
	 * Finds symbols that are matched by multiple symbol number patterns.
	 * 
	 * Returns an arbitrary affected symbol, or nullptr if no such symbol exists.
	 */
	Symbol const* findDuplicateSymbolPattern() const;
	
	
	/**
	 * Applies the matching rules to the object.
	 * 
	 * This operator can be used with Map::applyOnAllObjects etc.
	 * 
	 * Normally, you don't want to call this unless the new symbols are already
	 * part of the object's map. Note that for efficiency, this should be
	 * called on a squeezed() map.
	 * 
	 * \see apply
	 */
	void operator()(Object* object) const;
	
	
	/**
	 * Options for importing of new colors and symbols in to a map.
	 * 
	 * The default value (no option selected) describes the minimal behaviour:
	 * symbols are imported only as needed, in their present state, and
	 * all existing symbols and colors are left untouched.
	 * 
	 * Note that if multiple sources symbols are mapped to the same replacement
	 * symbol, the result of `PreserveSymbolState` is that the target symbol
	 * gets the state of a random source symbol which is mapped to this target.
	 */
	enum Option
	{
		ImportAllSymbols    = 0x01,
		PreserveSymbolState = 0x02,
		RemoveUnusedSymbols = 0x04,
		RemoveUnusedColors  = 0x08,
	};
	Q_DECLARE_FLAGS(Options, Option)
	
	/**
	 * Adds colors and symbols from the symbol map to the object map,
	 * and applies the rules and options.
	 * 
	 * This function will create a copy of the rule set. When the rule set is no
	 * longer needed after this call, the copy can be avoided by calling apply()
	 * on an rvalue (e.g. the result of std::move()) instead.
	 */
	void apply(Map& object_map, const Map& symbol_set, Options options = {}) const &;
	
	/**
	 * This deleted signature blocks accidental passing of default Options, {},
	 * as second of two arguments.
	 */
	void apply(Map& object_map, Options options) const = delete;
	
	/**
	 * Adds colors and symbols from the symbol map to the object map,
	 * and applies the rules and options.
	 */
	void apply(Map& object_map, const Map& symbol_set, Options options) &&;
	
};


}  // namespace OpenOrienteering


Q_DECLARE_OPERATORS_FOR_FLAGS(OpenOrienteering::SymbolRuleSet::Options)


#endif // OPENORIENTEERING_SYMBOL_RULE_SET_H
