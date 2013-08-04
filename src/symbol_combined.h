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


#ifndef _OPENORIENTEERING_SYMBOL_COMBINED_H_
#define _OPENORIENTEERING_SYMBOL_COMBINED_H_

#include <QGroupBox>

#include "symbol.h"
#include "symbol_properties_widget.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QSpinBox;
class QPushButton;
class QXmlStreamReader;
class QXmlStreamWriter;
QT_END_NAMESPACE

class SymbolSettingDialog;

/**
 * Symbol which can combine other line and area symbols,
 * creating renderables for each of them.
 * 
 * To use, set the number of parts with setNumParts() and set the indivdual part
 * pointers with setPart(). Parts can be private, i.e. the CombinedSymbol owns
 * the part symbol and it is not entered in the map as an individual symbol.
 */
class CombinedSymbol : public Symbol
{
friend class CombinedSymbolSettings;
friend class PointSymbolEditorWidget;
friend class OCAD8FileImport;
public:
	CombinedSymbol();
	virtual ~CombinedSymbol();
	virtual Symbol* duplicate(const MapColorMap* color_map = NULL) const;
	
	virtual void createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output);
	virtual void colorDeleted(const MapColor* color);
	virtual bool containsColor(const MapColor* color) const;
	const MapColor* getDominantColorGuess() const;
	virtual bool symbolChanged(Symbol* old_symbol, Symbol* new_symbol);
	virtual bool containsSymbol(const Symbol* symbol) const;
	virtual void scale(double factor);
	virtual Type getContainedTypes() const;
	
	virtual bool loadFinished(Map* map);
	
    virtual float calculateLargestLineExtent(Map* map);
	
	// Getters / Setter
	inline int getNumParts() const {return (int)parts.size();}
	inline void setNumParts(int num) {parts.resize(num, NULL); private_parts.resize(num, false);}
	
	inline Symbol* getPart(int i) const {return parts[i];}
	void setPart(int i, Symbol* symbol, bool is_private);
	
	inline bool isPartPrivate(int i) const {return private_parts[i];}
	inline void setPartPrivate(int i, bool set_private) {private_parts[i] = set_private;}
	
	virtual SymbolPropertiesWidget* createPropertiesWidget(SymbolSettingDialog* dialog);
	
protected:
	virtual bool loadImpl(QIODevice* file, int version, Map* map);
	virtual void saveImpl(QXmlStreamWriter& xml, const Map& map) const;
	virtual bool loadImpl(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict);
	virtual bool equalsImpl(Symbol* other, Qt::CaseSensitivity case_sensitivity);
	
	std::vector<Symbol*> parts;
	std::vector<bool> private_parts;
	std::vector<int> temp_part_indices;	// temporary vector of the indices of the 'parts' symbols, used just for loading
};

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
