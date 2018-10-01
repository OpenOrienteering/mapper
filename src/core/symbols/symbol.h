/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2018 Kai Pastor
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

#include <array>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QFlags>
#include <QHash>
#include <QImage>
#include <QMetaType>
#include <QRgb>
#include <QString>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {

class AreaSymbol;
class CombinedSymbol;
class LineSymbol;
class Map;
class MapColor;
class MapColorMap;
class Object;
class ObjectRenderables;
class PathObject;
class PathPartVector;
class PointSymbol;
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
public:
	/** 
	 * Enumeration of all possible symbol types.
	 * 
	 * Values are chosen to be able to be used as bits in a bitmask.
	 */
	enum Type
	{
		Point    = 1,
		Line     = 2,
		Area     = 4,
		Text     = 8,
		Combined = 16,
		
		NoSymbol = 0,
		AllSymbols = Point | Line | Area | Text | Combined
	};
	Q_DECLARE_FLAGS(TypeCombination, Type)
	
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
	
	
	explicit Symbol(Type type) noexcept;
	virtual ~Symbol();
	
protected:
	explicit Symbol(const Symbol& proto);
	virtual Symbol* duplicate() const = 0;
	
public:
	/**
	 * Duplicates a symbol.
	 * 
	 * This template function mitigates the incompatibility of std::unique_ptr
	 * with covariant return types when duplicating instances of the
	 * polymorphic type Symbol.
	 * 
	 * For convenient use outside of (child class) method implementatations,
	 * there is a free template function duplicate(const Derived& d).
	 */
	template<class S>
	static std::unique_ptr<S> duplicate(const S& s)
	{
		return std::unique_ptr<S>(static_cast<S*>(static_cast<const Symbol&>(s).duplicate()));
	}
	
	Symbol& operator=(const Symbol&) = delete;
	Symbol& operator=(Symbol&&) = delete;
	
	
	virtual bool validate() const;
	
	
	/**
	 * Checks for equality to the other symbol.
	 * 
	 * This function does not check the equality of the state
	 * (protected/hidden). Use stateEquals(Symbol*) for this comparison.
	 * 
	 * @param other The symbol to compare with.
	 * @param case_sensitivity Comparison mode for strings, e.g. symbol names.
	 */
	bool equals(const Symbol* other, Qt::CaseSensitivity case_sensitivity = Qt::CaseSensitive) const;
	
	/**
	 * Checks protected/hidden state for equality to the other symbol.
	 */
	bool stateEquals(const Symbol* other) const;
	
	
	/**
	 * Returns the type of the symbol.
	 */
	Type getType() const { return type; }
	
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
	
	/**
	 * Returns the combined bitmask of all symbol types this symbol contains.
	 */
	virtual TypeCombination getContainedTypes() const;
	
	/**
	 * Checks if the symbol can be applied to the given object.
	 */
	bool isTypeCompatibleTo(const Object* object) const;
	
	
	/**
	 * Returns if the symbol numbers are exactly equal.
	 */
	bool numberEquals(const Symbol* other) const;
	
	/**
	 * Returns if the symbol numbers are equal, ignoring trailing zeros.
	 */
	bool numberEqualsRelaxed(const Symbol* other) const;
	
	
	/**
	 * Saves the symbol in xml format.
	 * 
	 * This function invokes saveImpl(...) which may be overridden by child classes.
	 * 
	 * @param xml Stream to save to.
	 * @param map Reference to the map containing the symbol.
	 */
	void save(QXmlStreamWriter& xml, const Map& map) const;
	
	/**
	 * Load the symbol in xml format.
	 * 
	 * This function invokes loadImpl(...) which may be overridden by child classes.
	 * It does not add the symbol to the map.
	 * 
	 * @param xml Stream to load from.
	 * @param map Reference to the map which will eventually contain the symbol.
	 * @param symbol_dict Dictionary mapping symbol IDs to symbols.
	 */
	static std::unique_ptr<Symbol> load(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict);
	
	/**
	 * Called when loading the map is finished.
	 * 
	 * This event handler can be overridden in order to do tasks that need to
	 * access other symbols or map objects.
	 */
	virtual bool loadingFinishedEvent(Map* map);
	
	
	/**
	 * Creates renderables for a generic object.
	 * 
	 * This will create the renderables according to the object's properties
	 * and the given coordinates.
	 *  
	 * Implementations must use the coordinates (coords) instead of the object's
	 * coordinates.
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
	 * Called when a color is removed from the map.
	 * 
	 * Symbols need to remove all references to the given color when this event
	 * occurs.
	 */
	virtual void colorDeletedEvent(const MapColor* color) = 0;
	
	/**
	 * Returns if the given color is used by this symbol.
	 */
	virtual bool containsColor(const MapColor* color) const = 0;
	
	/**
	 * Returns the dominant color of this symbol.
	 * 
	 * If it is not possible to efficiently determine this color exactly,
	 * an appropriate heuristic should be used.
	 */
	virtual const MapColor* guessDominantColor() const = 0;
	
	/**
	 * Replaces colors used by this symbol.
	 */
	virtual void replaceColors(const MapColorMap& color_map) = 0;
	
	
	/**
	 * Called when a symbol was changed, replaced, or removed.
	 * 
	 * Symbol need top update or remove references to the given old_symbol.
	 * If new_symbol is nullptr, the symbol is about to be deleted.
	 * 
	 * Returns true if this symbol contained the deleted symbol.
	 */
	virtual bool symbolChangedEvent(const Symbol* old_symbol, const Symbol* new_symbol);
	
	/**
	 * Returns true if the given symbol is referenced by this symbol.
	 * 
	 * A symbol does not contain itself, so it must return true when the given
	 * symbol is identical to the symbol this function is being called for.
	 */
	virtual bool containsSymbol(const Symbol* symbol) const;
	
	
	/**
	 * Scales the symbol.
	 */
	virtual void scale(double factor) = 0;
	
	
	/**
	 * Returns the custom symbol icon. 
	 */
	QImage getCustomIcon() const { return custom_icon; }
	
	/**
	 * Sets a custom symbol icon.
	 * 
	 * The custom icon takes precedence over the generated one when custom icon
	 * display is enabled.
	 * Like the generated icon, it is not part of the symbol state which is
	 * compared by the `equals` functions.
	 * However, it is copied when duplicating an icon.
	 * 
	 * To clear the custom icon, this function can be called with a null QImage.
	 */
	void setCustomIcon(const QImage& image);
	
	/**
	 * Returns the symbol's icon.
	 * 
	 * This function returns (a scaled version of) the custom symbol icon if
	 * it is set and custom icon display is enabled, or a generated one.
	 * The icon is cached, making repeated calls cheap.
	 */
	QImage getIcon(const Map* map) const;
	
	/**
	 * Creates a symbol icon with the given side length (pixels).
	 * 
	 * If the zoom parameter is zero, the map's symbolIconZoom() is used.
	 * At a zoom of 1.0 (100%), a square symbol of 1 mm side length would fill
	 * 50% of the icon width and height. Larger symbols may be scaled down.
	 */
	QImage createIcon(const Map& map, int side_length, bool antialiasing = true, qreal zoom = 0) const;
	
	/**
	 * Clear the symbol's cached icon.
	 * 
	 * Call this function after changes to the symbol definition, custom icon,
	 * or general size/zoom/visibility settings.
	 * The cached icon will be recreated the next time getIcon() gets called.
	 */
	void resetIcon();
	
	/**
	 * Returns the dimension which shall considered when scaling the icon.
	 */
	virtual qreal dimensionForIcon() const;
	
	/**
	 * Returns the largest extent of all primitive lines which are part of the symbol.
	 * 
	 * Effectively, this is the half line width.
	 */
	virtual qreal calculateLargestLineExtent() const;
	
	
	// Getters / Setters
	
	/**
	 * Returns the symbol name.
	 */
	const QString& getName() const { return name; }
	
	/**
	 * Returns the symbol name with all HTML markup stripped.
	 */
	QString getPlainTextName() const;
	
	/**
	 * Sets the symbol name.
	 */
	void setName(const QString& new_name) { name = new_name; }
	
	
	/**
	 * Returns the symbol number as string
	 */
	QString getNumberAsString() const;
	
	/**
	 * Returns the i-th component of the symbol number as int.
	 */
	int getNumberComponent(int i) const { Q_ASSERT(i >= 0 && i < int(number_components)); return number[std::size_t(i)]; }
	
	/**
	 * Sets the i-th component of the symbol number.
	 */
	void setNumberComponent(int i, int new_number) { Q_ASSERT(i >= 0 && i < int(number_components)); number[std::size_t(i)] = new_number; }
	
	
	/**
	 * Returns the symbol description.
	 */
	const QString& getDescription() const { return description; }
	
	/**
	 * Sets the symbol description.
	 */
	void setDescription(const QString& new_description) { description = new_description; }
	
	
	/**
	 * Returns if this is a helper symbol (which is not printed in the final map).
	 */
	bool isHelperSymbol() const { return is_helper_symbol; }
	
	/**
	 * Sets if this is a helper symbol, see isHelperSymbol().
	 */
	void setIsHelperSymbol(bool value) { is_helper_symbol = value; }
	
	
	/**
	 * Returns if this symbol is hidden.
	 */
	bool isHidden() const { return is_hidden; }
	
	/**
	 * Sets the hidden state of this symbol.
	 */
	void setHidden(bool value) { is_hidden = value; }
	
	
	/**
	 * Returns if this symbol is protected.
	 * 
	 * Objects with a protected symbol cannot be selected or edited.
	 */
	bool isProtected() const { return is_protected; }
	
	/**
	 * Sets the protected state of this symbol.
	 */
	void setProtected(bool value) { is_protected = value; }
	
	
	/**
	 * Creates a properties widget for the symbol.
	 */
	virtual SymbolPropertiesWidget* createPropertiesWidget(SymbolSettingDialog* dialog) = 0;
	
	
	// Static
	
	/** Returns a newly created symbol of the given type */
	static std::unique_ptr<Symbol> makeSymbolForType(Type type);
	
	/**
	 * Returns if the symbol types can be applied to the same object types
	 */
	static bool areTypesCompatible(Type a, Type b);
	
	/**
	 * Returns a bitmask of all types which can be applied to
	 * the same objects as the given type.
	 */
	static TypeCombination getCompatibleTypes(Type type);

	
	/**
	 * Compares two symbols by number.
	 * 
	 * @return True if the number of s1 is less than the number of s2.
	 */
	static bool lessByNumber(const Symbol* s1, const Symbol* s2);
	
	/**
	 * Compares two symbols by the dominant colors' priorities.
	 * 
	 * @return True if s1's dominant color's priority is lower than s2's dominant color's priority.
	 */
	static bool lessByColorPriority(const Symbol* s1, const Symbol* s2);
	
	/**
	 * A functor for comparing symbols by dominant colors.
	 * 
	 * Other than lessByColorPriority(), this comparison uses the lowest
	 * priority which exists for a particular RGB color in the map, i.e.
	 * it groups colors by RGB values (and orders the result by priority).
	 * 
	 * All map colors are preprocessed in the constructor. Thus the functor
	 * becomes invalid as soon as colors are changed.
	 */
	struct lessByColor
	{
		/**
		 * Constructs the functor for the given map.
		 */
		lessByColor(const Map* map);
		
		/**
		 * Operator which compares two symbols by dominant colors.
		 * 
		 * \return True if s1's dominant color exists with lower prority then s2's dominant color.
		 */
		bool operator() (const Symbol* s1, const Symbol* s2) const;
		
	private:
		/**
		 * RGB values ordered by color priority.
		 */
		std::vector<QRgb> colors;
	};
	
	
	/**
	 * Number of components of symbol numbers.
	 */
	constexpr static auto number_components = 3u;
	
	
protected:
	/**
	 * Must be overridden to save type-specific symbol properties.
	 * 
	 * The map pointer can be used to get persistent indices to any pointers on map data.
	 */
	virtual void saveImpl(QXmlStreamWriter& xml, const Map& map) const = 0;
	
	/**
	 * Must be overridden to load type-specific symbol properties.
	 * 
	 * \see saveImpl()
	 * 
	 * Returns false if the current xml tag does not belong to the symbol and
	 * should be skipped, true if the element has been read completely.
	 */
	virtual bool loadImpl(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict) = 0;
	
	/**
	 * Must be overridden to compare specific attributes.
	 */
	virtual bool equalsImpl(const Symbol* other, Qt::CaseSensitivity case_sensitivity) const = 0;
	
	
private:
	mutable QImage icon;  ///< Cached symbol icon
	QImage custom_icon;   ///< Custom symbol icon
	QString name;
	QString description;
	std::array<int, number_components> number;
	Type type;
	bool is_helper_symbol;    /// \see isHelperSymbol()
	bool is_hidden;           /// \see isHidden()
	bool is_protected;        /// \see isProtected()
};



/**
 * Duplicates a symbol.
 * 
 * This free template function mitigates the incompatibility of std::unique_ptr
 * with covariant return types when duplicating instances of the
 * polymorphic type Symbol.
 * 
 * Synopsis:
 * 
 *    AreaSymbol s;
 *    auto s1 = duplicate(s);          // unique_ptr<AreaSymbol>
 *    auto s2 = duplicate<Symbol>(s);  // unique_ptr<Symbol>
 *    std::unique_ptr<Symbol> s3 = std::move(s1);
 */
template<class S>
typename std::enable_if<std::is_base_of<Symbol, S>::value, std::unique_ptr<S>>::type
/*std::unique_ptr<S>*/ duplicate(const S& s)
{
	return Symbol::duplicate<S>(s);
}


}  // namespace OpenOrienteering


Q_DECLARE_METATYPE(const OpenOrienteering::Symbol*)

Q_DECLARE_OPERATORS_FOR_FLAGS(OpenOrienteering::Symbol::TypeCombination)
Q_DECLARE_OPERATORS_FOR_FLAGS(OpenOrienteering::Symbol::RenderableOptions)


#endif
