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


#ifndef _OPENORIENTEERING_SYMBOL_H_
#define _OPENORIENTEERING_SYMBOL_H_

#include <vector>

#include <QComboBox>
#include <QItemDelegate>

#include "map_coord.h"
#include "path_coord.h"
#include "file_format.h"

QT_BEGIN_NAMESPACE
class QIODevice;
class QXmlStreamReader;
class QXmlStreamWriter;
QT_END_NAMESPACE

class Map;
class MapColor;
class Object;
class CombinedSymbol;
class TextSymbol;
class AreaSymbol;
class LineSymbol;
class PointSymbol;
class SymbolPropertiesWidget;
class SymbolSettingDialog;
class Renderable;
class MapRenderables;
class ObjectRenderables;
typedef std::vector<Renderable*> RenderableVector;

class Symbol;
typedef QHash<QString, Symbol*> SymbolDictionary;

// From core/map_color.h
typedef QHash<const MapColor*, const MapColor*> MapColorMap;


/// Base class for map symbols.
/// Provides among other things a symbol number consisting of multiple parts, e.g. "2.4.12". Parts which are not set are assigned the value -1.
class Symbol
{
friend class OCAD8FileImport;
friend class XMLImportExport;
public:
	enum Type
	{
		Point = 1,
		Line = 2,
		Area = 4,
		Text = 8,
		Combined = 16,
		
		NoSymbol = 0,
		AllSymbols = Point | Line | Area | Text | Combined
	};
	
// 	typedef QHash<const MapColor*, const MapColor*> MapColorMap;
	
	/// Constructs an empty symbol
	Symbol(Type type);
	virtual ~Symbol();
	virtual Symbol* duplicate(const MapColorMap* color_map = NULL) const = 0;
	bool equals(Symbol* other, Qt::CaseSensitivity case_sensitivity = Qt::CaseSensitive, bool compare_state = false);
	
	/// Returns the type of the symbol
	inline Type getType() const {return type;}
	// Convenience casts with type checking
	const PointSymbol* asPoint() const;
	PointSymbol* asPoint();
	const LineSymbol* asLine() const;
	LineSymbol* asLine();
	const AreaSymbol* asArea() const;
	AreaSymbol* asArea();
	const TextSymbol* asText() const;
	TextSymbol* asText();
	const CombinedSymbol* asCombined() const;
	CombinedSymbol* asCombined();
	
	/// Returns the or-ed together bitmask of all symbol types this symbol contains
	virtual Type getContainedTypes() const {return getType();}
	
	/// Can the symbol be applied to the given object?
	/// TODO: refactor: use static areTypesCompatible() instead with the type of the object's symbol
	bool isTypeCompatibleTo(Object* object);
	
	/// Returns if the symbol numbers are identical.
	bool numberEquals(Symbol* other, bool ignore_trailing_zeros);
	
	/// Saving and loading
	void save(QIODevice* file, Map* map);
	bool load(QIODevice* file, int version, Map* map);
	void save(QXmlStreamWriter& xml, const Map& map) const;
	static Symbol* load(QXmlStreamReader& xml, Map& map, SymbolDictionary& symbol_dict) throw (FileFormatException);
	
	/// Called after loading of the map is finished. Can do tasks that need to reference other symbols or map objects.
	virtual bool loadFinished(Map* map) {return true;}
	
	/// Creates renderables to display one specific instance of this symbol defined by the given object and coordinates
	/// (NOTE: methods which implement this should use the given coordinates instead of the object's coordinates, as those can be an updated, transformed version of the object's coords!)
	virtual void createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output) = 0;
	
	/// Called by the map in which the symbol is to notify it of a color being deleted (pointer becomes invalid, indices change)
	virtual void colorDeleted(const MapColor* color) = 0;
	
	/// Must return if the given color is used by this symbol
	virtual bool containsColor(const MapColor* color) const = 0;
	
	/// Returns the dominant color of this symbol, or a guess for this color in case it is impossible to determine it uniquely
	virtual const MapColor* getDominantColorGuess() const = 0;
	
	/// Called by the map in which the symbol is to notify it of a symbol being changed (pointer becomes invalid).
	/// If new_symbol == NULL, the symbol is being deleted.
	/// Must return true if this symbol contained the deleted symbol.
	virtual bool symbolChanged(Symbol* old_symbol, Symbol* new_symbol) {return false;}
	
	/// Must return if the given symbol is referenced by this symbol.
	/// Should NOT return true if the argument is itself.
	virtual bool containsSymbol(const Symbol* symbol) const {return false;}
	
	/// Scales the whole symbol
	virtual void scale(double factor) = 0;
	
	/// Returns the symbol's icon, creates it if it was not created yet. update == true forces an update of the icon.
	QImage* getIcon(Map* map, bool update = false);
	
	/// Creates a new image with the given side length and draws the smybol icon onto it.
	/// Returns an image pointer which you must delete yourself when no longer needed.
	QImage* createIcon(Map* map, int side_length, bool antialiasing, int bottom_right_border = 0);
	
	/// Clear the symbol's icon. It will be recreated when it is needed.
	void resetIcon() { delete icon; icon = NULL; }
	
	/// Returns the largest extent (half width) of all line symbols
	/// which may be included in this symbol.
	/// TODO: may fit into a subclass "PathSymbol"?
	virtual float calculateLargestLineExtent(Map* map) {return 0;}
	
	// Getters / Setters
	inline const QString& getName() const {return name;}
	QString getPlainTextName() const;
	inline void setName(const QString& new_name) {name = new_name;}
	
    QString getNumberAsString() const;
	inline int getNumberComponent(int i) const {assert(i >= 0 && i < number_components); return number[i];}
	inline void setNumberComponent(int i, int new_number) {assert(i >= 0 && i < number_components); number[i] = new_number;}
	
	inline const QString& getDescription() const {return description;}
	inline void setDescription(const QString& new_description) {description = new_description;}
	
	inline bool isHelperSymbol() const {return is_helper_symbol;}
	inline void setIsHelperSymbol(bool value) {is_helper_symbol = value;}
	
	inline bool isHidden() const {return is_hidden;}
	inline void setHidden(bool value) {is_hidden = value;}
	
	inline bool isProtected() const {return is_protected;}
	inline void setProtected(bool value) {is_protected = value;}
	
	virtual SymbolPropertiesWidget* createPropertiesWidget(SymbolSettingDialog* dialog);
	
	// Static
	
	/// Returns a newly created symbol of the given type
	static Symbol* getSymbolForType(Type type);
	/// Static save function; saves the symbol and its type number
	static void saveSymbol(Symbol* symobl, QIODevice* stream, Map* map);
	/// Static read function; reads the type number, creates a symbol of this type and loads it. Returns true if successful.
	static bool loadSymbol(Symbol*& symbol, QIODevice* stream, int version, Map* map);
	/// Creates "baseline" renderables for an object - symbol combination. These only show the coordinate paths with minimum line width, and optionally a hatching pattern for areas.
	static void createBaselineRenderables(Object* object, Symbol* symbol, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output, bool hatch_areas);
	
	/// Returns if the symbol types can be applied to the same object types
	static bool areTypesCompatible(Type a, Type b);
	
	/// Returns a bitmask of all types which can be applied to the same objects as the given type
	static int getCompatibleTypes(Type type);
	
	static const int number_components = 3;
	static const int icon_size = 32;
	
protected:
	/// Must be overridden to save type-specific symbol properties. The map pointer can be used to get persistent indices to any pointers on map data
	virtual void saveImpl(QIODevice* file, Map* map) = 0;
	/// Must be overridden to load type-specific symbol properties. See saveImpl()
	virtual bool loadImpl(QIODevice* file, int version, Map* map) = 0;
	/// Must be overridden to save type-specific symbol properties. The map pointer can be used to get persistent indices to any pointers on map data
	virtual void saveImpl(QXmlStreamWriter& xml, const Map& map) const = 0;
	/// Must be overridden to load type-specific symbol properties. See saveImpl().
	/// Return false if the current xml tag does not belong to the symbol and should be skipped, true if the element has been read completely.
	virtual bool loadImpl(QXmlStreamReader& xml, Map& map, SymbolDictionary& symbol_dict) = 0;
	/// Must be overridden to compare symbol-specific attributes.
	virtual bool equalsImpl(Symbol* other, Qt::CaseSensitivity case_sensitivity) = 0;
	
	/// Duplicates properties which are common for all symbols from other to this object
	void duplicateImplCommon(const Symbol* other);
	
	Type type;
	QString name;
	int number[number_components];
	QString description;
	bool is_helper_symbol;
	bool is_hidden;
	bool is_protected;
	QImage* icon;
};

class SymbolDropDown : public QComboBox
{
Q_OBJECT
public:
	/// filter is a bitwise-or combination of the allowed Symbol::Type types.
	SymbolDropDown(Map* map, int filter, Symbol* initial_symbol = NULL, const Symbol* excluded_symbol = NULL, QWidget* parent = NULL);
	
	/// Returns the selected symbol or NULL if no symbol selected
	Symbol* symbol() const;
	
	/// Sets the selection to the given symbol
	void setSymbol(Symbol* symbol);
	
	/// Adds a custom text item below the topmost "- none -" which can be identified by the given id.
	void addCustomItem(const QString& text, int id);
	
	/// Returns the id of the current item if it is a custom item, or -1 otherwise
	int customID() const;
	
	/// Sets the selection to the custom item with the given id
	void setCustomItem(int id);
	
protected slots:
	// TODO: react to changes in the map (not important as long as that cannot happen as long as a SymbolDropDown is shown, which is the case currently)
	
private:
	int num_custom_items;
};

class SymbolDropDownDelegate : public QItemDelegate
{
Q_OBJECT
public:
	SymbolDropDownDelegate(int symbol_type_filter, QObject* parent = 0);
	
	virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	virtual void setEditorData(QWidget* editor, const QModelIndex& index) const;
	virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;
	virtual void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	
private slots:
	void emitCommitData();
	
private:
	int symbol_type_filter;
};

#endif
