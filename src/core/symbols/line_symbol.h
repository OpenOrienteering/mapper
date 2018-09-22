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


#ifndef OPENORIENTEERING_LINE_SYMBOL_H
#define OPENORIENTEERING_LINE_SYMBOL_H

#include <vector>  // IWYU pragma: keep

#include <Qt>
#include <QtGlobal>
#include <QString>

#include "core/map_coord.h"  // IWYU pragma: keep
#include "core/symbols/symbol.h"

class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {

class LineSymbol;
class Map;
class MapColor;
class MapColorMap;
class Object;
class ObjectRenderables;
class PathObject;
class PathPartVector;
class PointSymbol;
class SplitPathCoord;
class SymbolPropertiesWidget;
class SymbolSettingDialog;
class VirtualCoordVector;
class VirtualPath;

using MapCoordVector = std::vector<MapCoord>;
using MapCoordVectorF = std::vector<MapCoordF>;


/**
 * Settings for a line symbol's border.
 */
struct LineSymbolBorder
{
	const MapColor* color = nullptr;
	int width             = 0;
	int shift             = 0;
	int dash_length       = 2000;
	int break_length      = 1000;
	bool dashed           = false;
	
	// Default special member functions are fine.
	
	void save(QXmlStreamWriter& xml, const Map& map) const;
	bool load(QXmlStreamReader& xml, const Map& map);
	
	bool isVisible() const;
	void setupSymbol(LineSymbol& out) const;
	void scale(double factor);
};


bool operator==(const LineSymbolBorder &lhs, const LineSymbolBorder &rhs) noexcept;

inline bool operator!=(const LineSymbolBorder &lhs, const LineSymbolBorder &rhs) noexcept
{
	return !(lhs == rhs);
}



/** Symbol for PathObjects which displays a line along the path. */
class LineSymbol : public Symbol
{
friend class LineSymbolSettings;
friend class PointSymbolEditorWidget;
friend class OCAD8FileImport;
public:
	enum CapStyle
	{
		FlatCap = 0,
		RoundCap = 1,
		SquareCap = 2,
		PointedCap = 3
	};
	
	enum JoinStyle
	{
		BevelJoin = 0,
		MiterJoin = 1,
		RoundJoin = 2
	};
	
	/**
	 * Mid symbol placement on dashed lines.
	 */
	enum MidSymbolPlacement
	{
		CenterOfDash      = 0,  ///< Mid symbols on every dash
		CenterOfDashGroup = 1,  ///< Mid symbols on the center of a dash group
		CenterOfGap       = 2,  ///< Mid symbols on the main gap (i.e. not between dashes in a group)
		NoMidSymbols      = 99
	};
	
	/** Constructs an empty line symbol. */
	LineSymbol() noexcept;
	~LineSymbol() override;
	
protected:
	explicit LineSymbol(const LineSymbol& proto);
	LineSymbol* duplicate() const override;
	
public:
	bool validate() const override;
	
	void createRenderables(
	        const Object *object,
	        const VirtualCoordVector &coords,
	        ObjectRenderables &output,
	        Symbol::RenderableOptions options ) const override;
	
	void createRenderables(
	        const PathObject* object,
	        const PathPartVector& path_parts,
	        ObjectRenderables &output,
	        Symbol::RenderableOptions options ) const override;
	
	/**
	 * Creates the renderables for a single path (i.e. a single part).
	 * 
	 * For the single path part represented by VirtualPath, this function
	 * takes care of all renderables for the main line, borders, mid symbol
	 * and dash symbol being added to the output. It does not deal with
	 * start symbols and end symbols.
	 */
	void createSinglePathRenderables(const VirtualPath& path, bool path_closed, ObjectRenderables& output) const;
	
	
	void colorDeletedEvent(const MapColor* color) override;
	bool containsColor(const MapColor* color) const override;
	const MapColor* guessDominantColor() const override;
	void replaceColors(const MapColorMap& color_map) override;
	void scale(double factor) override;
	
	/**
	 * Creates empty point symbols with the given names for undefined subsymbols.
	 * 
	 * After calling this method, all subsymbols are defined, i.e. not nullptr.
	 * Call cleanupPointSymbols() later to remove the empty symbols.
	 */
    void ensurePointSymbols(
		const QString& start_name,
		const QString& mid_name,
		const QString& end_name,
		const QString& dash_name
	);
	
	/**
	 * Deletes unused point symbols and sets them to nullptr again.
	 * 
	 * See ensurePointSymbols().
	 */
	void cleanupPointSymbols();
	
	
	/**
	 * Returns the dimension which shall considered when scaling the icon.
	 */
	qreal dimensionForIcon() const override;
	
	/**
	 * Returns the largest extent (half width) of the components of this line.
	 */
	qreal calculateLargestLineExtent() const override;
	
	
	
	/**
	 * Returns the limit for miter joins in units of the line width.
	 * See the Qt docs for QPainter::setMiterJoin().
	 * TODO: Should that better be a line property?
	 * FIXME: shall be 0 for border lines.
	 */
	static constexpr qreal miterLimit() { return 1; }
	
	// Getters / Setters
	inline int getLineWidth() const {return line_width;}
	inline void setLineWidth(double width) {line_width = qRound(1000 * width);}
	inline const MapColor* getColor() const {return color;}
	inline void setColor(const MapColor* color) {this->color = color;}
	inline int getMinimumLength() const {return minimum_length;}
	inline void setMinimumLength(int length) {this->minimum_length = length;}
	inline CapStyle getCapStyle() const {return cap_style;}
	inline void setCapStyle(CapStyle style) {cap_style = style;}
	inline JoinStyle getJoinStyle() const {return join_style;}
	inline void setJoinStyle(JoinStyle style) {join_style = style;}
	
	int startOffset() const { return start_offset; }
	void setStartOffset(int value) { start_offset = value; }
	
	int endOffset() const { return end_offset; }
	void setEndOffset(int value) { end_offset = value; }
	
	inline bool isDashed() const {return dashed;}
	inline void setDashed(bool value) {dashed = value;}
	
	inline PointSymbol* getStartSymbol() const {return start_symbol;}
	void setStartSymbol(PointSymbol* symbol);
	inline PointSymbol* getMidSymbol() const {return mid_symbol;}
	void setMidSymbol(PointSymbol* symbol);
	inline PointSymbol* getEndSymbol() const {return end_symbol;}
	void setEndSymbol(PointSymbol* symbol);
	inline PointSymbol* getDashSymbol() const {return dash_symbol;}
	void setDashSymbol(PointSymbol* symbol);
	
	inline int getMidSymbolsPerSpot() const {return mid_symbols_per_spot;}
	inline void setMidSymbolsPerSpot(int value) {mid_symbols_per_spot = value;}
	inline int getMidSymbolDistance() const {return mid_symbol_distance;}
	inline void setMidSymbolDistance(int value) {mid_symbol_distance = value;}
	inline MidSymbolPlacement getMidSymbolPlacement() const { return mid_symbol_placement; }
	void setMidSymbolPlacement(MidSymbolPlacement placement);
	
	inline bool getSuppressDashSymbolAtLineEnds() const {return suppress_dash_symbol_at_ends;}
	inline void setSuppressDashSymbolAtLineEnds(bool value) {suppress_dash_symbol_at_ends = value;}
	bool getScaleDashSymbol() const { return scale_dash_symbol; }
	void setScaleDashSymbol(bool value) { scale_dash_symbol = value; }
	
	inline int getSegmentLength() const {return segment_length;}
	inline void setSegmentLength(int value) {segment_length = value;}
	inline int getEndLength() const {return end_length;}
	inline void setEndLength(int value) {end_length = value;}
	inline bool getShowAtLeastOneSymbol() const {return show_at_least_one_symbol;}
	inline void setShowAtLeastOneSymbol(bool value) {show_at_least_one_symbol = value;}
	inline int getMinimumMidSymbolCount() const {return minimum_mid_symbol_count;}
	inline void setMinimumMidSymbolCount(int value) {minimum_mid_symbol_count = value;}
	inline int getMinimumMidSymbolCountWhenClosed() const {return minimum_mid_symbol_count_when_closed;}
	inline void setMinimumMidSymbolCountWhenClosed(int value) {minimum_mid_symbol_count_when_closed = value;}
	
	inline int getDashLength() const {return dash_length;}
	inline void setDashLength(int value) {dash_length = value;}
	inline int getBreakLength() const {return break_length;}
	inline void setBreakLength(int value) {break_length = value;}
	inline int getDashesInGroup() const {return dashes_in_group;}
	inline void setDashesInGroup(int value) {dashes_in_group = value;}
	inline int getInGroupBreakLength() const {return in_group_break_length;}
	inline void setInGroupBreakLength(int value) {in_group_break_length = value;}
	inline bool getHalfOuterDashes() const {return half_outer_dashes;}
	inline void setHalfOuterDashes(bool value) {half_outer_dashes = value;}
	
	inline bool hasBorder() const {return have_border_lines;}
	inline void setHasBorder(bool value) {have_border_lines = value;}
	inline bool areBordersDifferent() const {return border != right_border;}
	
	inline LineSymbolBorder& getBorder() {return border;}
	inline const LineSymbolBorder& getBorder() const {return border;}
	inline LineSymbolBorder& getRightBorder() {return right_border;}
	inline const LineSymbolBorder& getRightBorder() const {return right_border;}
	
	SymbolPropertiesWidget* createPropertiesWidget(SymbolSettingDialog* dialog) override;
	
protected:
	void saveImpl(QXmlStreamWriter& xml, const Map& map) const override;
	bool loadImpl(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict) override;
	PointSymbol* loadPointSymbol(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict);
	bool equalsImpl(const Symbol* other, Qt::CaseSensitivity case_sensitivity) const override;
	
	/**
	 * Creates the border lines' renderables for a single path (i.e. a single part).
	 * 
	 * For the single path part represented by VirtualPath, this function takes
	 * care of all renderables for the border lines being added to the output.
	 */
	void createBorderLines(
	        const VirtualPath& path,
	        const SplitPathCoord& start,
	        const SplitPathCoord& end,
	        ObjectRenderables& output
	) const;
	
	void shiftCoordinates(
	        const VirtualPath& path,
	        double main_shift,
	        MapCoordVector& out_flags,
	        MapCoordVectorF& out_coords
	) const;
	
	/**
	 * Creates flags and coords for a continuous line segment, and 
	 * adds pointed line caps and mid symbol renderables to the output.
	 * 
	 * Note that this function does not create LineRenderables.
	 */
	void processContinuousLine(
	        const VirtualPath& path,
	        const SplitPathCoord& start,
	        const SplitPathCoord& end,
	        bool set_mid_symbols,
	        MapCoordVector& processed_flags,
	        MapCoordVectorF& processed_coords,
	        ObjectRenderables& output
	) const;
	
	void createPointedLineCap(
	        const VirtualPath& path,
	        const SplitPathCoord& start,
	        const SplitPathCoord& end,
	        bool is_end,
	        ObjectRenderables& output
	) const;
	
	/**
	 * Creates flags and coords for a single dashed path (i.e. a single part)
	 * which may consist of multiple sections separated by dash points,
	 * and adds pointed line caps and mid symbol renderables to the output.
	 * 
	 * Note that this function does not create LineRenderables.
	 */
	void processDashedLine(
	        const VirtualPath& path,
	        const SplitPathCoord& start,
	        const SplitPathCoord& end,
	        bool path_closed,
	        MapCoordVector& out_flags,
	        MapCoordVectorF& out_coords,
	        ObjectRenderables& output
	) const;
	
	/**
	 * Creates flags and coords for a single dashed path section, and adds
	 * pointed line caps and mid symbol renderables to the output.
	 * 
	 * This is the main function determining the layout of dash patterns.
	 * A path section is delimited by the part start, the part end, and/or
	 * by dash points.
	 * 
	 * Note that this function does not create LineRenderables.
	 * 
	 * \see LineSymbol::createMidSymbolRenderables
	 */
	SplitPathCoord createDashGroups(
	        const VirtualPath& path,
	        bool path_closed,
	        const SplitPathCoord& line_start,
	        const SplitPathCoord& start,
	        const SplitPathCoord& end,
	        bool is_part_start,
	        bool is_part_end,
	        MapCoordVector& out_flags,
	        MapCoordVectorF& out_coords,
	        ObjectRenderables& output
	) const;
	
	void createDashSymbolRenderables(
	        const VirtualPath& path,
	        bool path_closed,
	        ObjectRenderables& output
	) const;
	
	/**
	 * Adds just the mid symbol renderables for a single solid path
	 * (i.e. a single part) to the output.
	 * 
	 * This is the main function determining the layout of mid symbols
	 * when the path is solid, not dashed.
	 * 
	 * Note that this function does not create LineRenderables.
	 * 
	 * \see LineSymbol::createDashGroups
	 */
	void createMidSymbolRenderables(
	        const VirtualPath& path,
	        const SplitPathCoord& start,
	        const SplitPathCoord& end,
	        bool path_closed,
	        ObjectRenderables& output
	) const;
	
	/**
	 * Adds just the start and end symbol renderables to the output.
	 * 
	 * The start symbol is placed at the start of the first part,
	 * and the end symbols is placed at the end of the last part.
	 */
	void createStartEndSymbolRenderables(
	        const PathPartVector& path_parts,
	        ObjectRenderables& output
	) const;
	
	
	void replaceSymbol(PointSymbol*& old_symbol, PointSymbol* replace_with, const QString& name);
	
	// Members ordered for minimizing padding
	
	// Border line details
	LineSymbolBorder border;
	LineSymbolBorder right_border;
	
	// Point symbols
	PointSymbol* start_symbol;
	PointSymbol* mid_symbol;
	PointSymbol* end_symbol;
	PointSymbol* dash_symbol;
	
	// Base line
	const MapColor* color;
	int line_width;		// in 1/1000 mm
	int minimum_length;
	int start_offset;
	int end_offset;
	
	int mid_symbols_per_spot;
	int mid_symbol_distance;
	int minimum_mid_symbol_count;
	int minimum_mid_symbol_count_when_closed;
	
	// Not dashed
	int segment_length;
	int end_length;
	
	// Dashed
	int dash_length;
	int break_length;
	int dashes_in_group;
	int in_group_break_length;
	
	CapStyle cap_style;
	JoinStyle join_style;
	MidSymbolPlacement mid_symbol_placement;
	
	// Various flags
	bool dashed;
	bool half_outer_dashes;
	bool show_at_least_one_symbol;
	bool suppress_dash_symbol_at_ends;
	bool scale_dash_symbol;
	bool have_border_lines;
};


}  // namespace OpenOrienteering

#endif
