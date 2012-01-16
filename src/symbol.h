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


#ifndef _OPENORIENTEERING_SYMBOL_H_
#define _OPENORIENTEERING_SYMBOL_H_

#include <assert.h>

#include <QString>
#include <QObject>

#include "map.h"
#include "renderable.h"

QT_BEGIN_NAMESPACE
class QFile;
QT_END_NAMESPACE

class Map;
class Object;

/// Base class for map symbols.
/// Provides among other things a symbol number consisting of multiple parts, e.g. "2.4.12". Parts which are not set are assigned the value -1.
class Symbol
{
public:
	enum Type
	{
		Point = 0,
		Line = 1,
		Area = 2,
		Text = 3,
		Combined = 4,
		
		NoSymbol = -1
	};
	
	/// Constructs an empty symbol
	Symbol(Type type);
	virtual ~Symbol();
	virtual Symbol* duplicate() = 0;
	
	/// Returns the type of the symbol.
	inline Type getType() {return type;}
	
	/// Saving and loading
	void save(QFile* file, Map* map);
	bool load(QFile* file, Map* map);
	
	/// Creates renderables to display one specific instance of this symbol defined by the given object and coordinates
	/// (NOTE: do not use the object's coordinates as the given coordinates can be an updated, transformed version of them!)
	virtual void createRenderables(Object* object, const MapCoordVectorF& coords, RenderableVector& output) = 0;
	
	/// Called by the map in which the symbol is to notify it of a color being deleted (pointer becomes invalid, indices change)
	virtual void colorDeleted(Map* map, int pos, MapColor* color) {}
	
	/// Must return if the given color is used by this symbol
	virtual bool containsColor(MapColor* color) = 0;
	
	/// Returns the symbol's icon, creates it if it was not created yet. update == true forces an update of the icon.
	QImage* getIcon(Map* map, bool update = false);
	
	// Getters / Setters
	inline const QString& getName() const {return name;}
	inline void setName(const QString& new_name) {name = new_name;}
	
	QString getNumberAsString();
	inline int getNumberComponent(int i) const {assert(i >= 0 && i < number_components); return number[i];}
	inline void setNumberComponent(int i, int new_number) {assert(i >= 0 && i < number_components); number[i] = new_number;}
	
	inline const QString& getDescription() const {return description;}
	inline void setDescription(const QString& new_description) {description = new_description;}
	
	inline bool isHelperSymbol() const {return is_helper_symbol;}
	inline void setIsHelperSymbol(bool value) {is_helper_symbol = value;}
	
	// Static
	static Symbol* getSymbolForType(Type type);
	
	static const int number_components = 3;
	static const int icon_size = 32;
	
protected:
	/// Must be overridden to save type-specific symbol properties. The map pointer can be used to get persistent indices to any pointers on map data
	virtual void saveImpl(QFile* file, Map* map) = 0;
	/// Must be overridden to load type-specific symbol properties. See saveImpl()
	virtual bool loadImpl(QFile* file, Map* map) = 0;
	
	/// Duplicates properties which are common for all symbols from other to this object
	void duplicateImplCommon(Symbol* other);
	
	Type type;
	QString name;
	int number[number_components];
	QString description;
	bool is_helper_symbol;
	QImage* icon;
};

#endif
