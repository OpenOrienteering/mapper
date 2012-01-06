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
Q_OBJECT
friend class PointSymbolSettings;
public:
	/// Constructs an empty point symbol
	PointSymbol(Map* map);
	
    virtual Type getType() {return Symbol::Point;}
    
    virtual void createRenderables(Object* object, const MapCoordVectorF& coords, RenderableVector& output);
	
	// Getters / Setters
	inline bool isRotatable() const {return rotatable;}
	inline int getInnerRadius() const {return inner_radius;}
	inline MapColor* getInnerColor() const {return inner_color;}
	inline int getOuterWidth() const {return outer_width;}
	inline MapColor* getOuterColor() const {return outer_color;}
	
public slots:
	void colorDeleted(int pos, MapColor* color);
	
protected:
    virtual void saveImpl(QFile* file, Map* map);
    virtual void loadImpl(QFile* file, Map* map);
	
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
	void innerRadiusChanged(QString text);
	void innerColorChanged();
	void outerWidthChanged(QString text);
	void outerColorChanged();
	
private:
	PointSymbol* symbol;
	SymbolSettingDialog* dialog;
	
	QCheckBox* oriented_to_north_check;
	QLineEdit* inner_radius_edit;
	ColorDropDown* inner_color_edit;
	QLineEdit* outer_width_edit;
	ColorDropDown* outer_color_edit;
};

#endif
