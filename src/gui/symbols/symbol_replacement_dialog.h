/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2017-2019, 2025 Kai Pastor
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


#ifndef OPENORIENTEERING_SYMBOL_REPLACEMENT_DIALOG_H
#define OPENORIENTEERING_SYMBOL_REPLACEMENT_DIALOG_H

#include <memory>
#include <vector>

#include <QObject>
#include <QDialog>
#include <QString>

#include "core/objects/symbol_rule_set.h"

class QAction;
class QCheckBox;
class QComboBox;
class QTableWidget;
class QWidget;

namespace OpenOrienteering {

class Map;
class SymbolDropDownDelegate;


/**
 * Dialog for assigning replacement symbols for original symbols and patterns.
 */
class SymbolReplacementDialog : public QDialog
{
	Q_OBJECT
	
public:
	enum Mode
	{
		ReplaceSymbolSet,  ///< Replace all current symbols
		AssignByPattern,   ///< Assign new symbols based on patterns
	};
	
	SymbolReplacementDialog(QWidget* parent, Map& object_map, const Map& symbol_set, SymbolRuleSet& symbol_rules, Mode mode);
	~SymbolReplacementDialog() override;
	
	SymbolReplacementDialog(const SymbolReplacementDialog&) = delete;
	SymbolReplacementDialog(SymbolReplacementDialog&&) = delete;
	
	SymbolReplacementDialog& operator=(const SymbolReplacementDialog&) = delete;
	SymbolReplacementDialog&& operator=(SymbolReplacementDialog&&) = delete;
	
	SymbolRuleSet::Options replacementOptions() const;
	QString replacementId() const;
	
protected:
	void done(int r) override;
	
	void updateMappingTable();
	void updateMappingFromTable();
	
private slots:
	void showHelp();
	
	void matchByName();
	void matchByNumber();
	void resetReplacements();
	
	void openCrtFile();
	bool saveCrtFile();
	
private:
	Map& object_map;
	const Map& symbol_set;
	SymbolRuleSet& symbol_rules;
	
	QAction* match_by_name_action = nullptr;
	QAction* match_by_number_action = nullptr;
	QCheckBox* import_all_check = nullptr;
	QCheckBox* delete_unused_symbols_check = nullptr;
	QCheckBox* delete_unused_colors_check = nullptr;
	QCheckBox* preserve_symbol_states_check = nullptr;
	QComboBox* id_edit;
	QTableWidget* mapping_table = nullptr;
	std::vector<std::unique_ptr<SymbolDropDownDelegate>> symbol_widget_delegates;
	
	Mode mode;
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_SYMBOL_REPLACEMENT_DIALOG_H
