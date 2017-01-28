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


#ifndef OPENORIENTEERING_SYMBOL_H
#define OPENORIENTEERING_SYMBOL_H

#include <QHash>
#include <QImage>
#include <QRgb>

#include "core/map_coord.h"
#include "fileformats/file_format.h"

QT_BEGIN_NAMESPACE
class QIODevice;
class QXmlStreamReader;
class QXmlStreamWriter;
QT_END_NAMESPACE

class AreaSymbol;
class CombinedSymbol;
class LineSymbol;
class Map;
class MapColor;
class MapColorMap;
class MapRenderables;
class Object;
class ObjectRenderables;
class PathObject;
class PathPartVector;
class PointSymbol;
class Renderable;
class Symbol;
class SymbolPropertiesWidget;
class SymbolSettingDialog;
class TextSymbol;
class VirtualCoordVector;

typedef QHash<QString, Symbol*> SymbolDictionary;


/**
 * Abstract base class for map symbols.
 * 
 * Provides among other things a symbol number consisting of multiple parts,
 * e.g. "2.4.12". Parts which are not set are assigned the value -1.
 */
class Symbol
{
friend class OCAD8FileImport;
friend class XMLImportExport;
public:
	/** Enumeration of all possible symbol types,
	 *  must be able to be used as bits in a bitmask. */
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
	
	/**
	 * RenderableOptions denominate variations in painting symbols.
	 */
	enum RenderableOption
	{
		RenderBaselines    = 1 << 0,   ///< Paint cosmetique contours and baselines
		RenderAreasHatched = 1 << 1,   ///< Paint hatching instead of opaque fill
		RenderNormal       = 0         ///< Paint normally
	};
	Q_DECLARE_FLAGS(RenderableOptions, RenderableOption)
	
	/** Constructs an empty symbol */
	Symbol(Type type);
	virtual ~Symbol();
	virtual Symbol* duplicate(const MapColorMap* color_map = NULL) const = 0;
	
	/**
	 * Checks for equality to the other symbol.
	 * @param other The symbol to compare with.
	 * @param case_sensitivity Comparison mode for strings, e.g. symbol names.
	 * @param compare_state If true, also compares symbol state (protected / hidden).
	 */
	bool equals(const Symbol* other, Qt::CaseSensitivity case_sensitivity = Qt::CaseSensitive, bool compare_state = false) const;
	
	
	/** Returns the type of the symbol */
	inline Type getType() const {return type;}
	
	// Convenience casts with type checking
	/** Case to PointSymbol with type checking */
	const PointSymbol* asPoint() const;
	/** Case to PointSymbol with type checking */
	PointSymbol* asPoint();
	/** Case to LineSymbol with type checking */
	const LineSymbol* asLine() const;
	/** Case to LineSymbol with type checking */
	LineSymbol* asLine();
	/** Case to AreaSymbol with type checking */
	const AreaSymbol* asArea() const;
	/** Case to AreaSymbol with type checking */
	AreaSymbol* asArea();
	/** Case to TextSymbol with type checking */
	const TextSymbol* asText() const;
	/** Case to TextSymbol with type checking */
	TextSymbol* asText();
	/** Case to CombinedSymbol with type checking */
	const CombinedSymbol* asCombined() const;
	/** Case to CombinedSymbol with type checking */
	CombinedSymbol* asCombined();
	
	/** Returns the or-ed together bitmask of all symbol types
	 *  this symbol contains */
	virtual Type getContainedTypes() const {return getType();}
	
	
	/**
	 * Checks if the symbol can be applied to the given object.
	 * TODO: refactor: use static areTypesCompatible() instead with the type of the object's symbol
	 */
	bool isTypeCompatibleTo(const Object* object) const;
	
	/** Returns if the symbol numbers are identical. */
	bool numberEquals(const Symbol* other, bool ignore_trailing_zeros) const;
	
	
	// Saving and loading
	
	/** Loads the symbol in the old "native" file format. */
	bool load(QIODevice* file, int version, Map* map);
	
	/**
	 * Saves the symbol in xml format.
	 * @param xml Stream to save to.
	 * @param map Reference to the map containing the symbol. Needed to find
	 *     symbol indices.
	 */
	void save(QXmlStreamWriter& xml, const Map& map) const;
	/**
	 * Load the symbol in xml format.
	 * @param xml Stream to load from.
	 * @param map Reference to the map containing the symbol.
	 * @param symbol_dict Dictionary mapping symbol IDs to symbol pointers.
	 */
	static Symbol* load(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict);
	
	/**
	 * Called after loading of the map is finished.
	 *  Can do tasks that need to reference other symbols or map objects.
	 */
	virtual bool loadFinished(Map* map);
	
	
	/**
	 * Creates renderables for a generic object.
	 * 
	 * This will create the renderables according to the object's properties
	 * and the given coordinates.
	 * 
	 * NOTE: methods which implement this should use the given coordinates
	 * instead of the object's coordinates, as those can be an updated,
	 * transformed version of the object's coords!
	 */
	virtual void createRenderables(
	        const Object *object,
	        const VirtualCoordVector &coords,
	        ObjectRenderables &output,
	        Symbol::RenderableOptions options) const = 0;
	
	/**
	 * Creates renderables for a path object.
	 * 
	 * This will create the renderables according to the object's properties
	 * and the coordinates given by the path_parts. This allows the immediate
	 * use of precalculated meta-information on paths.
	 * 
	 * \see createRenderables()
	 */
	virtual void createRenderables(
	        const PathObject* object,
	        const PathPartVector& path_parts,
	        ObjectRenderables &output,
	        Symbol::RenderableOptions options) const;
	
	/**
	 * Creates baseline renderables for a path object.
	 *
	 * Baseline renderables show the coordinate paths with minimum line width.
	 * 
	 * \see createRenderables()
	 */
	virtual void createBaselineRenderables(
	        const PathObject* object,
	        const PathPartVector& path_parts,
	        ObjectRenderables &output,
	        const MapColor* color) const;
	
	/**
	 * Called by the map in which the symbol is to notify it of a color being
	 * deleted (pointer becomes invalid, indices change).
	 */
	virtual void colorDeleted(const MapColor* color) = 0;
	
	/** Must return if the given color is used by this symbol. */
	virtual bool containsColor(const MapColor* color) const = 0;
	
	/**
	 * Returns the dominant color of this symbol, or a guess for this color
	 * in case it is impossible to determine it uniquely.
	 */
	virtual const MapColor* guessDominantColor() const = 0;
	
	/**
	 * Called by the map in which the symbol is to notify it of a symbol being
	 * changed (pointer becomes invalid).
	 * If new_symbol == NULL, the symbol is being deleted.
	 * Must return true if this symbol contained the deleted symbol.
	 */
	virtual bool symbolChanged(const Symbol* old_symbol, const Symbol* new_symbol);
	
	/**
	 * Must return if the given symbol is referenced by this symbol.
	 * Should NOT return true if the argument is itself.
	 */
	virtual bool containsSymbol(const Symbol* symbol) const;
	
	/** Scales the whole symbol */
	virtual void scale(double factor) = 0;
	
	/**
	 * Returns the symbol's icon, creates it if it was not created yet.
	 * update == true forces an update of the icon.
	 */
	QImage getIcon(const Map* map, bool update = false) const;
	
	/**
	 * Creates a new image with the given side length and draws the smybol icon onto it.
	 * Returns an image pointer which you must delete yourself when no longer needed.
	 */
	QImage createIcon(const Map* map, int side_length, bool antialiasing, int bottom_right_border = 0, float best_zoom = 2) const;
	
	/** Clear the symbol's icon. It will be recreated when it is needed. */
	void resetIcon() { icon = QImage(); }
	
	/**
	 * Returns the largest extent (half width) of all line symbols
	 * which may be included in this symbol.
	 * TODO: may fit into a subclass "PathSymbol"?
	 */
	virtual float calculateLargestLineExtent(Map* map) const;
	
	
	// Getters / Setters
	
	/** Returns the symbol name. */
	inline const QString& getName() const {return name;}
	/** Returns the symbol name after stripping all HTML. */
	QString getPlainTextName() const;
	/** Sets the symbol name. */
	inline void setName(const QString& new_name) {name = new_name;}
	
	/** Returns the symbol number as string. */
    QString getNumberAsString() const;
	/** Returns the i-th component of the symbol number as int. */
	inline int getNumberComponent(int i) const {Q_ASSERT(i >= 0 && i < number_components); return number[i];}
	/** Sets the i-th component of the symbol number. */
	inline void setNumberComponent(int i, int new_number) {Q_ASSERT(i >= 0 && i < number_components); number[i] = new_number;}
	
	/** Returns the symbol description. */
	inline const QString& getDescription() const {return description;}
	/** Sets the symbol description. */
	inline void setDescription(const QString& new_description) {description = new_description;}
	
	/** Returns if this is a helper symbol (which is not printed in the final map) */
	inline bool isHelperSymbol() const {return is_helper_symbol;}
	/** Sets if this is a helper symbol, see isHelperSymbol(). */
	inline void setIsHelperSymbol(bool value) {is_helper_symbol = value;}
	
	/** Returns if this symbol is hidden. */
	inline bool isHidden() const {return is_hidden;}
	/** Sets the hidden state of this symbol. */
	inline void setHidden(bool value) {is_hidden = value;}
	
	/** Returns if this symbol is protected, i.e. objects with this symbol
	 *  cannot be edited. */
	inline bool isProtected() const {return is_protected;}
	/** Sets the protected state of this symbol. */
	inline void setProtected(bool value) {is_protected = value;}
	
	/** Creates a properties widget for the symbol. */
	virtual SymbolPropertiesWidget* createPropertiesWidget(SymbolSettingDialog* dialog);
	
	
	// Static
	
	/** Returns a newly created symbol of the given type */
	static Symbol* getSymbolForType(Type type);
	
	/** Static read function; reads the type number, creates a symbol of
	 *  this type and loads it. Returns true if successful. */
	static bool loadSymbol(Symbol*& symbol, QIODevice* stream, int version, Map* map);
	
	/**
	 * Returns if the symbol types can be applied to the same object types
	 */
	static bool areTypesCompatible(Type a, Type b);
	
	/**
	 * Returns a bitmask of all types which can be applied to
	 * the same objects as the given type.
	 */
	static int getCompatibleTypes(Type type);

	
	/**
	 * @brief Compares two symbols by number.
	 * @return True if the number of s1 is less than the number of s2.
	 */
	static bool compareByNumber(const Symbol* s1, const Symbol* s2);
	
	/**
	 * @brief Compares two symbols by the dominant colors' priorities.
	 * @return True if s1's dominant color's priority is lower than s2's dominant color's priority.
	 */
	static bool compareByColorPriority(const Symbol* s1, const Symbol* s2);
	
	/**
	 * @brief Functor for comparing symbols by dominant colors.
	 * 
	 * Other than compareByColorPriority(), this comparison uses the lowest
	 * priority which exists for a particular color in the map. All map colors
	 * are preprocessed in the constructor. Thus the functor becomes invalid as
	 * soon as colors are changed.
	 */
	struct compareByColor
	{
		/**
		 * @brief Constructs the functor.
		 * @param map The map which defines all colors.
		 */
		compareByColor(Map* map);
		
		/**
		 * @brief Operator which compares two symbols by dominant colors.
		 * @return True if s1's dominant color exists with lower prority then s2's dominant color.
		 */
		bool operator() (const Symbol* s1, const Symbol* s2);
		
	private:
		/**
		 * @brief Maps color code to priority.
		 */
		QHash<QRgb, int> color_map;
	};

	/**
	 * Number of components of symbol numbers.
	 */
	static const int number_components = 3;
	
protected:
#ifndef NO_NATIVE_FILE_FORMAT
	/**
	 * Must be overridden to load type-specific symbol properties. See saveImpl()
	 */
	virtual bool loadImpl(QIODevice* file, int version, Map* map) = 0;
#endif
	
	/**
	 * Must be overridden to save type-specific symbol properties.
	 * The map pointer can be used to get persistent indices to any pointers on map data
	 */
	virtual void saveImpl(QXmlStreamWriter& xml, const Map& map) const = 0;
	
	/**
	 * Must be overridden to load type-specific symbol properties. See saveImpl().
	 * Return false if the current xml tag does not belong to the symbol and
	 * should be skipped, true if the element has been read completely.
	 */
	virtual bool loadImpl(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict) = 0;
	
	/**
	 * Must be overridden to compare symbol-specific attributes.
	 */
	virtual bool equalsImpl(const Symbol* other, Qt::CaseSensitivity case_sensitivity) const = 0;
	
	/**
	 * Duplicates properties which are common for all
	 * symbols from other to this object
	 */
	void duplicateImplCommon(const Symbol* other);
	
	
	/** The symbol type, determined by the subclass */
	Type type;
	/** Symbol name */
	QString name;
	/** Symbol number */
	int number[number_components];
	/** Symbol description */
	QString description;
	/** Helper symbol flag, see isHelperSymbol() */
	bool is_helper_symbol;
	/** Hidden flag, see isHidden() */
	mutable bool is_hidden;
	/** Protected flag, see isProtected() */
	bool is_protected;
	
private:
	/** Pointer to symbol icon, if generated */
	mutable QImage icon;
};

Q_DECLARE_METATYPE(const Symbol*)

Q_DECLARE_OPERATORS_FOR_FLAGS(Symbol::RenderableOptions)

#endif
