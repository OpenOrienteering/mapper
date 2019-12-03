/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2017-2019 Kai Pastor
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


#ifndef OPENORIENTEERING_SYMBOL_REPLACEMENT_H
#define OPENORIENTEERING_SYMBOL_REPLACEMENT_H

#include <memory>

#include <QString>

#include "core/objects/symbol_rule_set.h"

class QWidget;

namespace OpenOrienteering {

class Map;


/**
 * A generalized interactive symbol replacement operation.
 * 
 * This class models symbol replacement operations with new symbol sets and/or
 * CRT files, composing additional dialog steps around SymbolReplacementDialog.
 * All interactive features take a QWidget pointer which is used as a parent
 * for dialog windows.
 */
class SymbolReplacement
{
public:
	/**
	 * Constructs a symbol replacement within a single map, or for a to-be-loaded symbol set.
	 */
	SymbolReplacement(Map& object_map) noexcept;
	
	/**
	 * Constructs a symbol replacement with a map of objects and a target symbol set.
	 */
	SymbolReplacement(Map& object_map, const Map& symbol_set) noexcept;
	
	
	/**
	 * Returns the map containing the objects to be edited.
	 */
	Map& objectMap() noexcept { return object_map; }
	
	/**
	 * Returns the map containing the symbols to be assigned.
	 */
	const Map& symbolSet() const noexcept { return symbol_set; }
	
	
	/**
	 * Returns the suggested filepath for a CRT file matching the symbol set IDs.
	 */
	QString crtFileById() const;
	
	/**
	 * Returns the suggested filepath for a CRT file side by side to the filepath given as hint.
	 */
	QString crtFileSideBySide(const QString& hint) const;
	
	
	/**
	 * Lets the user choose a symbol set file and then perform symbol set replacement.
	 */
	bool withSymbolSetFileDialog(QWidget* parent);
	
	/**
	 * Lets the user perform symbol set replacement.
	 */
	bool withNewSymbolSet(QWidget* parent);
	
	/**
	 * Lets the user choose a CRT file and then perform symbol replacement.
	 */
	bool withCrtFileDialog(QWidget* parent);
	
	/**
	 * Tries to automatically detect a CRT file and then perform symbol replacement.
	 */
	bool withAutoCrtFile(QWidget* parent, const QString& hint);
	
	/**
	 * Tries to read a given CRT file and then perform symbol replacement.
	 */
	bool withCrtFile(QWidget* parent, const QString& crt_file);
	
	/**
	 * Loads the given CRT file and returns the symbol rule set.
	 * 
	 * This function turns search patterns matching symbol code numbers in the
	 * object map into real symbol search patterns. In addition, it verifies
	 * that there are no duplicate symbol search patterns.
	 * 
	 * On errors, messages are displayed to the user, and an empty symbol set
	 * is returned.
	 */
	SymbolRuleSet loadCrtFile(QWidget* parent, const QString& filepath) const;
	
protected:
	std::unique_ptr<Map> getOpenSymbolSet(QWidget* parent) const;
	
private:
	Map& object_map;
	Map const& symbol_set;
};


}  // namespace OpenOrienteering

#endif
