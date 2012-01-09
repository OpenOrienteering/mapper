/*
 *    Copyright 2011 Thomas Schöps
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

#include "symbol.h"

class AreaSymbol : public Symbol
{
friend class PointSymbolSettings;
friend class PointSymbolEditorWidget;
public:
	AreaSymbol();
	virtual ~AreaSymbol();
	
	virtual void createRenderables(Object* object, const MapCoordVectorF& coords, RenderableVector& output);
	virtual void colorDeleted(int pos, MapColor* color);
	
	// Getters
	inline MapColor* getColor() const {return color;}
	
protected:
	virtual void saveImpl(QFile* file, Map* map);
	virtual void loadImpl(QFile* file, Map* map);
	
	MapColor* color;
};

#endif
