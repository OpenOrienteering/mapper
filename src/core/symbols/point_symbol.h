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


#ifndef OPENORIENTEERING_POINT_SYMBOL_H
#define OPENORIENTEERING_POINT_SYMBOL_H

#include <memory>
#include <vector>

#include <Qt>
#include <QtGlobal>

#include "symbol.h"

// IWYU pragma: no_include "core/objects/object.h"

class QPainterPath;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {

class Map;
class MapColor;
class MapColorMap;
class MapCoordF;
class ObjectRenderables;
class SymbolPropertiesWidget;
class SymbolSettingDialog;
class VirtualCoordVector;


/**
 * Symbol for PointObjects.
 * 
 * Apart from its own settings which are presented to the user as "[Midpoint
 * symbol]" in the point symbol editor, this class can contain a list of
 * elements together with a symbol for each element. All elements and the
 * PointObject's own settings make up the appearance of the point symbol.
 * The reason the own settings are left in is just efficiency, as for some
 * symbols like crop land, a really huge number of point objects may be generated.
 */
class PointSymbol : public Symbol
{
friend class PointSymbolSettings;
friend class PointSymbolEditorWidget;
friend class OCAD8FileImport;
friend class XMLImportExport;
public:
	/** Constructs an empty point symbol. */
	PointSymbol() noexcept;
	~PointSymbol() override;
	
protected:
	explicit PointSymbol(const PointSymbol& proto);
	PointSymbol* duplicate() const override;
	
public:
	bool validate() const override;
	
	void createRenderables(
	        const Object *object,
	        const VirtualCoordVector &coords,
	        ObjectRenderables &output,
	        RenderableOptions options ) const override;
	
	void createRenderablesScaled(const MapCoordF& coord, qreal rotation, ObjectRenderables& output, qreal coord_scale = 1) const;
	
	void createRenderablesIfCenterInside(const MapCoordF& point_coord, qreal rotation, const QPainterPath* outline, ObjectRenderables& output) const;
	void createPrimitivesIfCompletelyInside(const MapCoordF& point_coord, const QPainterPath* outline, ObjectRenderables& output) const;
	void createRenderablesIfCompletelyInside(const MapCoordF& point_coord, qreal rotation, const QPainterPath* outline, ObjectRenderables& output) const;
	void createPrimitivesIfPartiallyInside(const MapCoordF& point_coord, const QPainterPath* outline, ObjectRenderables& output) const;
	void createRenderablesIfPartiallyInside(const MapCoordF& point_coord, qreal rotation, const QPainterPath* outline, ObjectRenderables& output) const;
	
	void colorDeletedEvent(const MapColor* color) override;
	bool containsColor(const MapColor* color) const override;
	const MapColor* guessDominantColor() const override;
	void replaceColors(const MapColorMap& color_map) override;
	void scale(double factor) override;
	
	qreal dimensionForIcon() const override;
	
	// Contained objects and symbols (elements)
	
	/** Returns the number of contained elements. */
	int getNumElements() const;
	/** Adds a new element consisting of object and symbol at the given index. */
	void addElement(int pos, Object* object, Symbol* symbol);
	/** Returns the object of the i-th element. */
	Object* getElementObject(int pos);
	/** Returns the object of the i-th element. */
	const Object* getElementObject(int pos) const;
	/** Returns the symbol of the i-th element. */
	Symbol* getElementSymbol(int pos);
	/** Returns the symbol of the i-th element. */
	const Symbol* getElementSymbol(int pos) const;
	/** Deletes the i-th element. */
	void deleteElement(int pos);
	
	/**
	 * Returns true if the point contains no elements and does not create
	 * renderables itself. Useful to check if it can be deleted.
	 */
	bool isEmpty() const;
	
	/**
	 * Checks if the contained elements are rotationally symmetrical around
	 * the origin (this means, only point elements at (0,0) are allowed).
	 */
	bool isSymmetrical() const;
	
	// Getters / Setters
	inline bool isRotatable() const {return rotatable;}
	inline void setRotatable(bool enable) {rotatable = enable;}
	inline int getInnerRadius() const {return inner_radius;}
	inline void setInnerRadius(int value) {inner_radius = value;}
	inline const MapColor* getInnerColor() const {return inner_color;}
	inline void setInnerColor(const MapColor* color) {inner_color = color;}
	inline int getOuterWidth() const {return outer_width;}
	inline void setOuterWidth(int value) {outer_width = value;}
	inline const MapColor* getOuterColor() const {return outer_color;}
	inline void setOuterColor(const MapColor* color) {outer_color = color;}
	
	SymbolPropertiesWidget* createPropertiesWidget(SymbolSettingDialog* dialog) override;
	
	
protected:
	void saveImpl(QXmlStreamWriter& xml, const Map& map) const override;
	bool loadImpl(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict) override;
	bool equalsImpl(const Symbol* other, Qt::CaseSensitivity case_sensitivity) const override;
	
	
	/// \todo Expose elements more directly in PointSymbol API.
	struct Element
	{
		std::unique_ptr<Symbol> symbol;
		std::unique_ptr<Object> object;
	};
	std::vector<Element> elements;
	
	const MapColor* inner_color;
	const MapColor* outer_color;
	int inner_radius;		// in 1/1000 mm
	int outer_width;		// in 1/1000 mm
	bool rotatable;
};


}  // namespace OpenOrienteering

#endif
