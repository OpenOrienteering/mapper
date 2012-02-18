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


#ifndef _OPENORIENTEERING_OBJECT_H_
#define _OPENORIENTEERING_OBJECT_H_

#include <assert.h>

#include "map_coord.h"
#include "renderable.h"
#include "path_coord.h"

QT_BEGIN_NAMESPACE
class QFile;
QT_END_NAMESPACE

class Symbol;
class Map;

/// Base class which combines coordinates and a symbol to form an object (in a map or point symbol).
class Object
{
friend class OCAD8FileImport;
public:
	enum Type
	{
		Point = 0,	// A single coordinate, no futher coordinates can be added
		Path = 1,	// A dynamic list of coordinates
		Circle = 2,
		Rectangle = 3,
		Text = 4
	};
	
	/// Creates an empty object
	Object(Type type, Symbol* symbol = NULL);
	virtual ~Object();
	virtual Object* duplicate() = 0;
	
	/// Returns the object type determined by the subclass
	inline Type getType() {return type;}
	
	void save(QFile* file);
	void load(QFile* file, int version, Map* map);
	
	/// Checks if the output_dirty flag is set and if yes, regenerates output and extent; returns true if output was previously dirty.
	/// Use force == true to force a redraw
	bool update(bool force = false);
	
	/// Moves the whole object
	void move(qint64 dx, qint64 dy);
	
	/// Scales all coordinates, with the origin as scaling center
	void scale(double factor);
	
	/// Rotates the whole object
	void rotateAround(MapCoordF center, double angle);
	
	/// Reverses the object's coordinates, resulting in switching the dash direction for line symbols
	void reverse();
	
	/// Checks if the given coord, with the given tolerance, is on this object; with extended_selection, the coord is on point objects always if it is whithin their extent,
	/// otherwise it has to be close to their midpoint. Returns a Symbol::Type which specifies on which symbol type the coord is
	/// (important for combined symbols which can have areas and lines).
	int isPointOnObject(MapCoordF coord, float tolerance, bool extended_selection);
	
	/// Checks if a path point (excluding curve control points) is included in the given box
	bool isPathPointInBox(QRectF box);
	
	/// Take ownership of the renderables
	void takeRenderables();
	
	// Methods to traverse the output
	inline RenderableVector::const_iterator beginRenderables() const {return output.begin();}
	inline RenderableVector::const_iterator endRenderables() const {return output.end();}
	inline int getNumRenderables() const {return (int)output.size();}
	
	// Getters / Setters
	inline const MapCoordVector& getRawCoordinateVector() const {return coords;}	// NOTE: for closed paths, the last and first elements of this vector are equal
	inline int getCoordinateCount() const {return qMax(0, (int)coords.size() - (path_closed ? 1 : 0));}
	inline MapCoord getCoordinate(int pos) const {return coords[pos];}
	inline const PathCoordVector& getPathCoordinateVector() const {return path_coords;}
	
	inline void setOutputDirty(bool dirty = true) {output_dirty = dirty;}
	inline bool isOutputDirty() const {return output_dirty;}
	
	void setPathClosed(bool value = true);
	inline bool isPathClosed() const {return path_closed;}
	
	/// Changes the object's symbol, returns if successful. Some conversions are impossible, for example point to line.
	/// Normally, this method checks if the types of the old and the new symbol are compatible. If the old symbol pointer
	/// is no longer valid, you can use no_checks to disable this.
	bool setSymbol(Symbol* new_symbol, bool no_checks);
	inline Symbol* getSymbol() const {return symbol;}
	
	/// NOTE: The extent is only valid after update() has been called!
	inline const QRectF& getExtent() const {return extent;}
	
	inline void setMap(Map* map) {this->map = map;}
	inline Map* getMap() const {return map;}
	
	static Object* getObjectForType(Type type, Symbol* symbol = NULL);
	
protected:
	/// Removes all output of the object. It will be re-generated the next time update() is called
	void clearOutput();
	
	Type type;
	Symbol* symbol;
	MapCoordVector coords;
	bool path_closed;				// does the coordinates represent a closed path (return to first coord after the last?)   TODO: Move this to PathObject?!
	Map* map;
	
	bool output_dirty;				// does the output have to be re-generated because of changes?
	RenderableVector output;		// only valid after calling update()
	PathCoordVector path_coords;	// these are calculated for symbols containing line or area symbols, only valid after calling update()
	QRectF extent;					// only valid after calling update()
};

/// Object type which can be used for line, area and combined symbols
class PathObject : public Object
{
public:
	PathObject(Symbol* symbol = NULL);
    virtual Object* duplicate();
	
	int calcNumRegularPoints();
	void calcClosestPointOnPath(MapCoordF coord, float& out_distance_sq, PathCoord& out_path_coord);
	int subdivide(int index, float param);	// returns the index of the added point
	
	void setCoordinate(int pos, MapCoord c);
	void addCoordinate(int pos, MapCoord c);
	void addCoordinate(MapCoord c);
	void deleteCoordinate(int pos, bool adjust_other_coords);	// adjust_other_coords does not work if deleting bezier curve handles!
};

/// Object type which can only be used for point symbols, and is also the only object which can be used with them
class PointObject : public Object
{
public:
	PointObject(Symbol* symbol = NULL);
    virtual Object* duplicate();
	
	void setPosition(MapCoord position);
	MapCoord getPosition();
	
	void setRotation(float new_rotation);
	inline float getRotation() const {return rotation;}
	
private:
	float rotation;	// 0 to 2*M_PI
};

struct TextObjectLineInfo
{
	QString text;			// substring shown in this line
	int start_index;		// line start character index in original string
	int end_index;
	QRectF bounding_box;	// in text transformation
	double line_x;			// left endpoint of the baseline of this line of text in text coordinates
	double line_y;
	
	inline TextObjectLineInfo(const QString& text, int start_index, int end_index, const QRectF& bounding_box, double line_x, double line_y)
	 : text(text), start_index(start_index), end_index(end_index), bounding_box(bounding_box), line_x(line_x), line_y(line_y) {}
};

/// Object type which can only be used for text symbols.
/// Contains either 1 coordinate (single anchor point) or 2 coordinates (word wrap box: midpoint coordinate and width/height in second coordinate)
class TextObject : public Object
{
public:
	enum HorizontalAlignment
	{
		AlignLeft = 0,
		AlignHCenter = 1,
		AlignRight = 2
	};
	
	enum VerticalAlignment
	{
		AlignBaseline = 0,
		AlignTop = 1,
		AlignVCenter = 2,
		AlignBottom = 3
	};
	
	TextObject(Symbol* symbol = NULL);
	virtual Object* duplicate();
	
	inline bool hasSingleAnchor() const {return coords.size() == 1;}
	void setAnchorPosition(MapCoord position);
	MapCoord getAnchorPosition();	// or midpoint if a box is used
	void setBox(MapCoord midpoint, double width, double height);
	inline double getBoxWidth() const {assert(!hasSingleAnchor()); return coords[1].xd();}
	inline double getBoxHeight() const {assert(!hasSingleAnchor()); return coords[1].yd();}
	
	QTransform calcTextToMapTransform();
	QTransform calcMapToTextTransform();
	
	void setText(const QString& text);
	inline const QString& getText() const {return text;}
	
	void setHorizontalAlignment(HorizontalAlignment h_align);
	inline HorizontalAlignment getHorizontalAlignment() const {return h_align;}
	
	void setVerticalAlignment(VerticalAlignment v_align);
	inline VerticalAlignment getVerticalAlignment() const {return v_align;}
	
	void setRotation(float new_rotation);
	inline float getRotation() const {return rotation;}
	
	inline int getNumLineInfos() const {return (int)line_infos.size();}
	inline TextObjectLineInfo* getLineInfo(int i) {return &line_infos[i];}
	inline void clearLineInfos() {line_infos.clear();}
	inline void addLineInfo(TextObjectLineInfo line_info) {line_infos.push_back(line_info);}
	
	int calcTextPositionAt(MapCoordF coord, bool find_line_only);	// returns -1 if the coordinate is not at a text position
	int findLetterPosition(TextObjectLineInfo* line_info, QPointF point, const QFontMetricsF& metrics);
	TextObjectLineInfo* findLineInfoForIndex(int index);
	
private:
	QString text;
	HorizontalAlignment h_align;
	VerticalAlignment v_align;
	float rotation;	// 0 to 2*M_PI
	
	/// when renderables are generated for this object by a call to update(), this is filled with information about the generated lines
	std::vector<TextObjectLineInfo> line_infos;
};

#endif
