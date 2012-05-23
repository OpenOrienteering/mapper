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
#include "symbol_properties_widget.h"

class QComboBox;
class QLabel;

class SymbolSettingDialog;

/// Symbol which can combine other line and area symbols
class CombinedSymbol : public Symbol
{
friend class CombinedSymbolSettings;
friend class PointSymbolEditorWidget;
friend class OCAD8FileImport;
public:
	CombinedSymbol();
	virtual ~CombinedSymbol();
	virtual Symbol* duplicate() const;
	
	virtual void createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, RenderableVector& output);
	virtual void colorDeleted(Map* map, int pos, MapColor* color);
	virtual bool containsColor(MapColor* color);
	virtual bool symbolChanged(Symbol* old_symbol, Symbol* new_symbol);
	bool containsSymbol(const Symbol* symbol) const;
	virtual void scale(double factor);
	virtual Type getContainedTypes();
	
	virtual bool loadFinished(Map* map);
	
	// Getters / Setter
	inline int getNumParts() const {return (int)parts.size();}
	inline void setNumParts(int num) {parts.resize(num);}
	
	inline Symbol* getPart(int i) const {return parts[i];}
	inline void setPart(int i, Symbol* symbol) {parts[i] = symbol;}
	
	virtual SymbolPropertiesWidget* createPropertiesWidget(SymbolSettingDialog* dialog);
	
protected:
	virtual void saveImpl(QFile* file, Map* map);
	virtual bool loadImpl(QFile* file, int version, Map* map);
	
	std::vector<Symbol*> parts;
	std::vector<int> temp_part_indices;	// temporary vector of the indices of the 'parts' symbols, used just for loading
};

class CombinedSymbolSettings : public SymbolPropertiesWidget
{
Q_OBJECT
public:
	CombinedSymbolSettings(CombinedSymbol* symbol, SymbolSettingDialog* dialog);
	virtual ~CombinedSymbolSettings();
	
	static const int max_count;	// maximum number of symbols in a combined symbol
	
protected slots:
	void numberChanged(int index);
	void symbolChanged(int index);
	
private:
	CombinedSymbol* symbol;
//	SymbolSettingDialog* dialog;
	
	QComboBox* number_edit;
	QLabel** symbol_labels;
	SymbolDropDown** symbol_edits;
	bool react_to_changes;
};

#endif
