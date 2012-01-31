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


#ifndef _OPENORIENTEERING_SYMBOL_POINT_H_
#define _OPENORIENTEERING_SYMBOL_POINT_H_

#include <QGroupBox>

#include "symbol.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
QT_END_NAMESPACE

class Map;
class MapColor;
class SymbolSettingDialog;
class ColorDropDown;

class PointSymbol : public Symbol
{
friend class PointSymbolSettings;
friend class PointSymbolEditorWidget;
public:
	/// Constructs an empty point symbol
	PointSymbol();
    virtual ~PointSymbol();
    virtual Symbol* duplicate();
    
	virtual void createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, bool path_closed, RenderableVector& output);
	void createRenderablesScaled(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, bool path_closed, RenderableVector& output, float coord_scale);
	virtual void colorDeleted(Map* map, int pos, MapColor* color);
    virtual bool containsColor(MapColor* color);
    virtual void scale(double factor);
	
	// Contained objects and symbols (elements)
	int getNumElements() const;
	void addElement(int pos, Object* object, Symbol* symbol);
	Object* getElementObject(int pos);
	Symbol* getElementSymbol(int pos);
	void deleteElement(int pos);
	
	/// Returns true if the point contains no elements and does not create renderables itself
	bool isEmpty() const;
	
	// Getters / Setters
	inline bool isRotatable() const {return rotatable;}
	inline void setRotatable(bool enable) {rotatable = enable;}
	inline int getInnerRadius() const {return inner_radius;}
	inline MapColor* getInnerColor() const {return inner_color;}
	inline int getOuterWidth() const {return outer_width;}
	inline MapColor* getOuterColor() const {return outer_color;}
	
protected:
    virtual void saveImpl(QFile* file, Map* map);
    virtual bool loadImpl(QFile* file, int version, Map* map);
	
	std::vector<Object*> objects;
	std::vector<Symbol*> symbols;
	
	bool rotatable;
	int inner_radius;		// in 1/1000 mm
	MapColor* inner_color;
	int outer_width;		// in 1/1000 mm
	MapColor* outer_color;
};

class PointSymbolSettings : public QGroupBox
{
Q_OBJECT
public:
	PointSymbolSettings(PointSymbol* symbol, Map* map, SymbolSettingDialog* parent);
	
public slots:
	void orientedToNorthClicked(bool checked);
	
private:
	PointSymbol* symbol;
	SymbolSettingDialog* dialog;
	
	QCheckBox* oriented_to_north_check;
};

#endif
