/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2017 Kai Pastor
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


#ifndef OPENORIENTEERING_COMBINED_SYMBOL_SETTINGS_H
#define OPENORIENTEERING_COMBINED_SYMBOL_SETTINGS_H

#include <QObject>

#include "gui/symbols/symbol_properties_widget.h"

class QLabel;
class QPushButton;
class QSpinBox;

class CombinedSymbol;
class Symbol;
class SymbolDropDown;
class SymbolSettingDialog;

class CombinedSymbolSettings : public SymbolPropertiesWidget
{
Q_OBJECT
public:
	CombinedSymbolSettings(CombinedSymbol* symbol, SymbolSettingDialog* dialog);
	virtual ~CombinedSymbolSettings();
	
	void reset(Symbol* symbol);
	
	/**
	 * Updates the content and state of all input fields.
	 */
	void updateContents();
	
	static const int max_count;	// maximum number of symbols in a combined symbol
	
protected slots:
	void numberChanged(int value);
	void symbolChanged(int index);
	void editClicked(int index);
	
private:
	CombinedSymbol* symbol;
	
	QSpinBox* number_edit;
	QLabel** symbol_labels;
	SymbolDropDown** symbol_edits;
	QPushButton** edit_buttons;
};

#endif
