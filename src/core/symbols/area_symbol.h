/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2019 Kai Pastor
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


#ifndef OPENORIENTEERING_AREA_SYMBOL_H
#define OPENORIENTEERING_AREA_SYMBOL_H

#include <cstddef>
#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QFlags>
#include <QRectF>
#include <QString>

#include "symbol.h"

class QRectF;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {

class AreaRenderable;
class LineSymbol;
class Map;
class MapColor;
class MapColorMap;
class MapCoord;
class MapCoordF;
class Object;
class ObjectRenderables;
class PathObject;
class PathPartVector;
class PointSymbol;
class SymbolPropertiesWidget;
class SymbolSettingDialog;
class VirtualCoordVector;


/**
 * Symbol for PathObjects where the enclosed area is filled with a solid color
 * and / or with one or more patterns.
 */
class AreaSymbol : public Symbol
{
friend class AreaSymbolSettings;
friend class PointSymbolEditorWidget;
public:
	/** Describes a fill pattern. */
	struct FillPattern
	{
		/** Types of fill patterns. */
		enum Type
		{
			/** Parallel lines pattern */
			LinePattern = 1,
			/** Point grid pattern */
			PointPattern = 2
		};
		
		/**
		 * Flags for pattern properties.
		 */
		enum Option
		{
			Default                      = 0x00,
			NoClippingIfCompletelyInside = 0x01,
			NoClippingIfCenterInside     = 0x02,
			NoClippingIfPartiallyInside  = 0x03,
			AlternativeToClipping        = 0x03, ///< Bitmask for NoClipping* options
			Rotatable                    = 0x10, ///< Pattern is rotatable per-object
		};
		Q_DECLARE_FLAGS(Options, Option)
		
		/** Type of the pattern */
		Type type;
		/** Basic properties of the pattern. */
		Options flags;
		/** Rotation angle in radians
		 * 
		 * \todo Switch to qreal when legacy native file format is dropped.
		 */
		qreal angle;
		/** Distance between parallel lines, as usual in 0.001mm */
		int line_spacing;
		/** Offset of the first line from the origin */
		int line_offset;
		
		// For type == LinePattern only:
		
		/** Line color */
		const MapColor* line_color;
		/** Line width */
		int line_width;
		
		// For type == PointPattern only:
		
		/** Offset of first point along parallel lines */
		int offset_along_line;
		/** Point distance along parallel lines */
		int point_distance;
		/** Contained point symbol */
		PointSymbol* point;
		
		/** Display name (transient) */
		QString name;
		
		
		/** Creates a default fill pattern */
		FillPattern() noexcept;
		
		/** Saves the pattern in xml format */
		void save(QXmlStreamWriter& file, const Map& map) const;
		/** Loads the pattern in xml format */
		void load(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict, int version);
		/**
		 * Checks if the pattern settings are equal to the other.
		 * TODO: should the transient name really be compared?!
		 */
		bool equals(const FillPattern& other, Qt::CaseSensitivity case_sensitivity) const;
		
		
		/**
		 * Returns true if the pattern is rotatable per object.
		 */
		bool rotatable() const;
		
		/**
		 * Controls whether the pattern is rotatable per object.
		 */
		void setRotatable(bool value);
		
		
		/**
		 * Returns the flags which control drawing at boundary.
		 */
		Options clipping() const;
		
		/**
		 * Sets the flags which control drawing at boundary.
		 */
		void setClipping(Options clipping);
		
		
		/**
		 * Removes the pattern's references to the deleted color.
		 */
		void colorDeleted(const MapColor* color);
		                  
		/**
		 * Tests if the pattern contains the given color.
		 */
		bool containsColor(const MapColor* color) const;
		
		/**
		 * Returns the patterns primary color.
		 */
		const MapColor* guessDominantColor() const;
		
		
		/**
		 * Creates renderables for this pattern to fill the area surrounded by the outline.
		 * @param outline A renderable giving the extent and outline.
		 * @param delta_rotation Rotation offset which is added to the pattern angle.
		 * @param pattern_origin Origin point for line / point placement.
		 * @param output Created renderables will be inserted here.
		 */
		void createRenderables(
			const AreaRenderable& outline,
			qreal delta_rotation,
			const MapCoord& pattern_origin,
			ObjectRenderables& output
		) const;
		
		/** Does the heavy-lifting in loops over lines. */
		template <int type>
		void createRenderables(
			const AreaRenderable& outline,
			qreal delta_rotation,
			const MapCoord& pattern_origin,
			const QRectF& point_extent,
			LineSymbol* line,
			qreal rotation,
			ObjectRenderables& output
		) const;
		
		/** Creates one line of renderables, called by createRenderables(). */
		template <int type>
		void createLine(
			MapCoordF first, MapCoordF second,
			qreal delta_offset,
			LineSymbol* line,
			qreal rotation,
			const AreaRenderable& outline,
			ObjectRenderables& output
		) const;
		
		/** Creates a single line of renderables for a PointPattern. */
		void createPointPatternLine(
			MapCoordF first, MapCoordF second,
			qreal delta_offset,
			qreal rotation,
			const AreaRenderable& outline,
			ObjectRenderables& output
		) const;
		
		
		/** Spatially scales the pattern settings by the given factor. */
		void scale(double factor);
		
		qreal dimensionForIcon() const;
		
	};
	
	AreaSymbol() noexcept;
	~AreaSymbol() override;
	
protected:
	explicit AreaSymbol(const AreaSymbol& proto);
	AreaSymbol* duplicate() const override;
	
public:
	void createRenderables(
	        const Object *object,
	        const VirtualCoordVector &coords,
	        ObjectRenderables &output,
	        Symbol::RenderableOptions options) const override;
	
	void createRenderables(
	        const PathObject* object,
	        const PathPartVector& path_parts,
	        ObjectRenderables &output,
	        Symbol::RenderableOptions options) const override;
	
	void createRenderablesNormal(
	        const PathObject* object,
	        const PathPartVector& path_parts,
	        ObjectRenderables& output) const;
	
	/**
	 * Creates area hatching renderables for a path object.
	 */
	void createHatchingRenderables(
	        const PathObject *object,
	        const PathPartVector& path_parts,
	        ObjectRenderables &output,
	        const MapColor* color) const;
	
	
	void colorDeletedEvent(const MapColor* color) override;
	bool containsColor(const MapColor* color) const override;
	const MapColor* guessDominantColor() const override;
	void replaceColors(const MapColorMap& color_map) override;
	void scale(double factor) override;
	
	qreal dimensionForIcon() const override;
	
	// Getters / Setters
	inline const MapColor* getColor() const {return color;}
	inline void setColor(const MapColor* color) {this->color = color;}
	inline int getMinimumArea() const {return minimum_area; }
	inline int getNumFillPatterns() const {return int(patterns.size());}
	inline void setNumFillPatterns(int count) {patterns.resize(std::size_t(count));}
	inline FillPattern& getFillPattern(int i) {return patterns[std::size_t(i)];}
	inline const FillPattern& getFillPattern(int i) const {return patterns[std::size_t(i)];}
	bool hasRotatableFillPattern() const;
	SymbolPropertiesWidget* createPropertiesWidget(SymbolSettingDialog* dialog) override;
	
protected:
	void saveImpl(QXmlStreamWriter& xml, const Map& map) const override;
	bool loadImpl(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict, int version) override;
	
	/**
	 * Compares AreaSymbol objects for equality.
	 * 
	 * Fill patterns are only compared in order.
	 */
	bool equalsImpl(const Symbol* other, Qt::CaseSensitivity case_sensitivity) const override;
	
	std::vector<FillPattern> patterns;
	const MapColor* color;
	int minimum_area;	// in mm^2 // FIXME: unit (factor) wrong
};



inline
bool AreaSymbol::FillPattern::rotatable() const
{
	return flags.testFlag(Option::Rotatable);
}

inline
AreaSymbol::FillPattern::Options AreaSymbol::FillPattern::clipping() const
{
	return flags & Option::AlternativeToClipping;
}


}  // namespace OpenOrienteering


Q_DECLARE_OPERATORS_FOR_FLAGS(OpenOrienteering::AreaSymbol::FillPattern::Options)


#endif
