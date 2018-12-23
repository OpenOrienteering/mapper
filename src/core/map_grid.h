/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014, 2015, 2017, 2018 Kai Pastor
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


#ifndef OPENORIENTEERING_MAP_GRID_H
#define OPENORIENTEERING_MAP_GRID_H

#include <QRgb>

class QIODevice;
class QPainter;
class QRectF;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {

class Map;
class MapCoordF;


/**
 * Class for displaying a grid on a map.
 * 
 * Each map has an instance of this class which can be retrieved with map->getGrid().
 * The grid's visibility is defined per MapView.
 * 
 * Grid lines are thin. They are either drawn using a cosmetic pen (usually on
 * screen) or with 0.1 mm (usually on paper).
 */
class MapGrid
{
public:
	/** Options for aligning the grid with different north concepts. */
	enum Alignment
	{
		MagneticNorth = 0,
		GridNorth     = 1,
		TrueNorth     = 2
	};
	
	/** Different units for specifying the grid interval. */
	enum Unit
	{
		MillimetersOnMap = 0,
		MetersInTerrain  = 1
	};
	
	/** Different display modes for map grids. */
	enum DisplayMode
	{
		AllLines        = 0,
		HorizontalLines = 1,
		VerticalLines   = 2
	};
	
	/** Creates a new map grid with default settings. */
	MapGrid();
	
	/** Loads the grid in the old "native" format from the given file. */
	const MapGrid& load(QIODevice* file, int version);
	
	/** Saves the grid in xml format to the given stream. */
	void save(QXmlStreamWriter& xml) const;
	
	/** Loads the grid in xml format from the given stream. */
	const MapGrid& load(QXmlStreamReader& xml);
	
	/**
	 * Draws the map grid.
	 * 
	 * @param painter The QPainter used for drawing.
	 * @param bounding_box Bounding box of the area to draw the grid for, in
	 *     map coordinates.
	 * @param map Map to draw the grid for.
	 * @param scale_adjustment If zero, uses a cosmetic pen (one pixel wide),
	 *        otherwise this is the divisor used to create a 0.1 mm wide pen.
	 */
	void draw(QPainter* painter, const QRectF& bounding_box, Map* map, qreal scale_adjustment = 0) const;
	void draw(QPainter* painter, const QRectF& bounding_box, Map* map, bool) const = delete;
	
	/**
	 * Calculates the "final" parameters with the following properties:
	 * - spacings and offsets are in millimeters on the map
	 * - rotation is relative to the vector (1, 0) and counterclockwise
	 */
	void calculateFinalParameters(
		double& final_horz_spacing, double& final_vert_spacing,
		double& final_horz_offset, double& final_vert_offset,
		double& final_rotation, Map* map
	) const;
	
	/** Returns the grid point which is closest to the given position. */
	MapCoordF getClosestPointOnGrid(MapCoordF position, Map* map) const;
	
	// Getters / Setters
	
	inline bool isSnappingEnabled() const {return snapping_enabled;}
	inline void setSnappingEnabled(bool enable) {snapping_enabled = enable;}
	inline QRgb getColor() const {return color;}
	inline void setColor(QRgb color) {this->color = color;}
	
	/**
	 * Returns true if the grid is not opaque.
	 */
	bool hasAlpha() const;
	
	inline DisplayMode getDisplayMode() const {return display;}
	inline void setDisplayMode(DisplayMode mode) {display = mode;}
	
	inline Alignment getAlignment() const {return alignment;}
	inline void setAlignment(Alignment alignment) {this->alignment = alignment;}
	inline double getAdditionalRotation() const {return additional_rotation;}
	inline void setAdditionalRotation(double rotation) {additional_rotation = rotation;}
	
	inline Unit getUnit() const {return unit;}
	inline void setUnit(Unit unit) {this->unit = unit;}
	inline double getHorizontalSpacing() const {return horz_spacing;}
	inline void setHorizontalSpacing(double spacing) {horz_spacing = spacing;}
	inline double getVerticalSpacing() const {return vert_spacing;}
	inline void setVerticalSpacing(double spacing) {vert_spacing = spacing;}
	inline double getHorizontalOffset() const {return horz_offset;}
	inline void setHorizontalOffset(double offset) {horz_offset = offset;}
	inline double getVerticalOffset() const {return vert_offset;}
	inline void setVerticalOffset(double offset) {vert_offset = offset;}
	
private:
	bool snapping_enabled;
	QRgb color;
	DisplayMode display;
	
	Alignment alignment;
	double additional_rotation;
	
	Unit unit;
	double horz_spacing;
	double vert_spacing;
	double horz_offset;
	double vert_offset;
	
	friend bool operator==(const MapGrid& lhs, const MapGrid& rhs);
};

/**
 * Compares two map grid objects.
 * 
 * @return true if the objects are equal, false otherwise
 */
bool operator==(const MapGrid& lhs, const MapGrid& rhs);

/**
 * Compares two map grid objects for inequality.
 * 
 * @return true if the objects are not equal, false otherwise
 */
bool operator!=(const MapGrid& lhs, const MapGrid& rhs);


}  // namespace OpenOrienteering

#endif
