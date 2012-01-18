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


#ifndef _OPENORIENTEERING_SYMBOL_AREA_H_
#define _OPENORIENTEERING_SYMBOL_AREA_H_

#include <QGroupBox>

#include "symbol.h"

class ColorDropDown;
class SymbolSettingDialog;

class AreaSymbol : public Symbol
{
friend class AreaSymbolSettings;
friend class PointSymbolEditorWidget;
public:
	AreaSymbol();
	virtual ~AreaSymbol();
    virtual Symbol* duplicate();
	
	virtual void createRenderables(Object* object, const MapCoordVectorF& coords, RenderableVector& output);
	virtual void colorDeleted(Map* map, int pos, MapColor* color);
    virtual bool containsColor(MapColor* color);
	
	// Getters
	inline MapColor* getColor() const {return color;}
	
protected:
	virtual void saveImpl(QFile* file, Map* map);
	virtual bool loadImpl(QFile* file, Map* map);
	
	MapColor* color;
};

class AreaSymbolSettings : public QGroupBox
{
Q_OBJECT
public:
	AreaSymbolSettings(AreaSymbol* symbol, Map* map, SymbolSettingDialog* parent);
	
protected slots:
	void colorChanged();
	
private:
	AreaSymbol* symbol;
	SymbolSettingDialog* dialog;
	
	ColorDropDown* color_edit;
};

#endif
