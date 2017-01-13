/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#ifndef _OPENORIENTEERING_SYMBOL_DIALOG_REPLACE_H_
#define _OPENORIENTEERING_SYMBOL_DIALOG_REPLACE_H_

#include <QHash>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QTableWidget;
QT_END_NAMESPACE

class Map;
class Symbol;
class SymbolDropDownDelegate;

/**
 * Dialog for replacing the map's symbol set with another.
 * Lets the user choose options and possibly even choose the replacement
 * for every single symbol.
 */
class ReplaceSymbolSetDialog : public QDialog
{
Q_OBJECT
public:
	/**
	 * Lets the user select a file to load the symbols from and shows the dialog.
	 * Returns true if the replacement has been finished, false if aborted.
	 */
	static bool showDialog(QWidget* parent, Map* map);
	
private slots:
	void matchByNumberClicked(bool checked);
	void showHelp();
	void apply();
	
private:
	ReplaceSymbolSetDialog(QWidget* parent, Map* map, Map* symbol_map);
    virtual ~ReplaceSymbolSetDialog();
	
	void calculateNumberMatchMapping();
	Symbol* findNumberMatch(Symbol* original, bool ignore_trailing_zeros);
	void updateMappingTable();
	void updateMappingFromTable();
	
	Map* map;
	Map* symbol_map;
	QHash<const Symbol*, const Symbol*> mapping;
	
	QCheckBox* import_all_check;
	QCheckBox* delete_unused_symbols_check;
	QCheckBox* delete_unused_colors_check;
	QCheckBox* preserve_symbol_states_check;
	QCheckBox* match_by_number_check;
	QTableWidget* mapping_table;
	std::vector<SymbolDropDownDelegate*> symbol_widget_delegates;
};

#endif
