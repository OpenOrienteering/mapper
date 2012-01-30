/*
 *    Copyright 2012 Thomas Schöps
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


#ifndef _OPENORIENTEERING_SYMBOL_COMBINED_H_
#define _OPENORIENTEERING_SYMBOL_COMBINED_H_

#include <QGroupBox>

#include "symbol.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
QT_END_NAMESPACE

class SymbolSettingDialog;

/// Symbol which can combine other line and area symbols
class CombinedSymbol : public Symbol
{
friend class CombinedSymbolSettings;
friend class PointSymbolEditorWidget;
public:
	CombinedSymbol();
	virtual ~CombinedSymbol();
    virtual Symbol* duplicate();
	
	virtual void createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, bool path_closed, RenderableVector& output);
	virtual void colorDeleted(Map* map, int pos, MapColor* color);
    virtual bool containsColor(MapColor* color);
	bool containsSymbol(Symbol* symbol);
	
	virtual bool loadFinished(Map* map);
	
protected:
	virtual void saveImpl(QFile* file, Map* map);
	virtual bool loadImpl(QFile* file, Map* map);
	
	std::vector<Symbol*> parts;
};

class CombinedSymbolSettings : public QGroupBox
{
Q_OBJECT
public:
	CombinedSymbolSettings(CombinedSymbol* symbol, CombinedSymbol* in_map_symbol, Map* map, SymbolSettingDialog* parent);
    virtual ~CombinedSymbolSettings();
	
	static const int max_count;	// maximum number of symbols in a combined symbol
	
protected slots:
	void numberChanged(int index);
	void symbolChanged(int index);
	
private:
	CombinedSymbol* symbol;
	SymbolSettingDialog* dialog;
	
	QComboBox* number_edit;
	QLabel** symbol_labels;
	SymbolDropDown** symbol_edits;
	bool react_to_changes;
};

#endif
