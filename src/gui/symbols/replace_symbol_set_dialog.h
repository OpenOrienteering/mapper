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


#ifndef OPENORIENTEERING_REPLACE_SYMBOL_SET_DIALOG_H
#define OPENORIENTEERING_REPLACE_SYMBOL_SET_DIALOG_H

#include <memory>
#include <vector>

#include <QObject>
#include <QDialog>
#include <QString>

class QCheckBox;
class QIODevice;
class QComboBox;
class QTableWidget;
class QWidget;

namespace OpenOrienteering {

class Map;
class SymbolDropDownDelegate;
class SymbolRuleSet;


/**
 * Dialog and tool for replacing the map's symbol set with another.
 * 
 * Lets the user choose options and possibly even choose the replacement
 * for every single symbol.
 */
class ReplaceSymbolSetDialog : public QDialog
{
Q_OBJECT
public:
	/**
	 * Lets the user select a symbol set file, and shows the dialog.
	 * 
	 * Returns true if the replacement has been finished, false if aborted.
	 */
	static bool showDialog(QWidget* parent, Map& object_map);
	
	/**
	 * Lets the user select a CRT file, and shows the dialog.
	 * 
	 * Returns true if the replacement has been finished, false if aborted.
	 */
	static bool showDialogForCRT(QWidget* parent, Map& object_map, const Map& symbol_set);
	
	/**
	 * Shows the dialog for a given object map, symbol map, and cross reference file.
	 * 
	 * Returns true if the replacement has been finished, false if aborted.
	 */
	static bool showDialogForCRT(QWidget* parent, Map& object_map, const Map& symbol_set, QIODevice& crt_file);
	
	/**
	 * Returns the suggested path (including name) for finding a CRT file
	 * for the given symbol set IDs.
	 */
	static QString discoverCrtFile(const QString& source_id, const QString& target_id);
	
private:
	enum Mode
	{
		ReplaceSymbolSet,  ///< Replace current current symbols
		AssignByPattern,   ///< Assign new symbols based on a pattern
	};
	
	ReplaceSymbolSetDialog(QWidget* parent, Map& object_map, const Map& symbol_set, SymbolRuleSet& replacements, Mode mode);
	~ReplaceSymbolSetDialog() override;
	
public:
	ReplaceSymbolSetDialog(const ReplaceSymbolSetDialog&) = delete;
	ReplaceSymbolSetDialog(ReplaceSymbolSetDialog&&) = delete;
	
	ReplaceSymbolSetDialog& operator=(const ReplaceSymbolSetDialog&) = delete;
	ReplaceSymbolSetDialog&& operator=(ReplaceSymbolSetDialog&&) = delete;
	
private:
	void showHelp();
	
	void matchByName();
	void matchByNumber();
	void resetReplacements();
	
	void openCrtFile();
	void openCrtFile(const QString& path);
	bool saveCrtFile();
	
	void done(int r) override;
	
	void updateMappingTable();
	void updateMappingFromTable();
	
	Map& object_map;
	const Map& symbol_set;
	SymbolRuleSet& replacements;
	
	QCheckBox* import_all_check = nullptr;
	QCheckBox* delete_unused_symbols_check = nullptr;
	QCheckBox* delete_unused_colors_check = nullptr;
	QCheckBox* preserve_symbol_states_check = nullptr;
	QComboBox* id_edit;
	QTableWidget* mapping_table;
	std::vector<std::unique_ptr<SymbolDropDownDelegate>> symbol_widget_delegates;
	
	Mode mode;
};


}  // namespace OpenOrienteering

#endif
