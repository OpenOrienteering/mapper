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

QT_BEGIN_NAMESPACE
class QFile;
QT_END_NAMESPACE

class Symbol;
class Map;

/// Base class which combines coordinates and a symbol to form an object (in a map or point symbol).
class Object
{
public:
	typedef std::vector<Renderable*> RenderaleVector;
	
	enum Type
	{
		Point = 0,	// A single coordinate, no futher coordinates can be added
		Path = 1,	// A dynamic list of coordinates
		Circle = 2,
		Rectangle = 3,
		Text = 4
	};
	
	/// Creates an empty object
	Object(Map* map, Type type, Symbol* symbol = NULL);
	virtual ~Object();
	virtual Object* duplicate() = 0;
	
	/// Returns the object type determined by the subclass
	inline Type getType() {return type;}
	
	void save(QFile* file);
	void load(QFile* file);
	
	/// Checks if the output_dirty flag is set and if yes, regenerates output and extent; returns true if output was previously dirty.
	/// Use force == true to force a redraw
	bool update(bool force = false);
	
	/// Returns the coordinate vector
	inline const MapCoordVector& getCoordinateVector() const {return coords;}
	inline int getCoordinateCount() const {return (int)coords.size();}
	inline MapCoord getCoordinate(int pos) const {return coords[pos];}
	
	// Methods to traverse the output
	inline RenderableVector::const_iterator beginRenderables() const {return output.begin();}
	inline RenderableVector::const_iterator endRenderables() const {return output.end();}
	
	// Getters / Setters
	inline void setOutputDirty(bool dirty = true) {output_dirty = dirty;}
	inline bool isOutputDirty() const {return output_dirty;}
	
	inline void setPathClosed(bool value = true) {path_closed = value;}
	inline bool isPathClosed() const {return path_closed;}
	
	/// Changes the object's symbol, returns if successful. Some conversions are impossible, for example point to line.
	/// Normally, this method checks if the types of the old and the new symbol are compatible. If the old symbol pointer
	/// is no longer valid, you can use no_checks to disable this.
	bool setSymbol(Symbol* new_symbol, bool no_checks);
	inline Symbol* getSymbol() const {return symbol;}
	
	/// NOTE: The extent is only valid after update() has been called!
	inline const QRectF& getExtent() const {return extent;}
	
	inline Map* getMap() const {return map;}
	
	static Object* getObjectForType(Type type, Map* map, Symbol* symbol = NULL);
	
protected:
	/// Removes all output of the object. It will be re-generated the next time update() is called
	void clearOutput();
	
	Type type;
	Symbol* symbol;
	MapCoordVector coords;
	bool path_closed;		// does the coordinates represent a closed path (return to first coord after the last?)   TODO: Move this to PathObject?!
	Map* map;
	
	bool output_dirty;		// does the output have to be re-generated because of changes?
	RenderableVector output;
	QRectF extent;		// only valid after calling update()
};

/// Object type which can be used for line, area and combined symbols
class PathObject : public Object
{
public:
	PathObject(Map* map, Symbol* symbol = NULL);
    virtual Object* duplicate();
	
	inline void setCoordinate(int pos, MapCoord c) {coords[pos] = c;}
	inline void addCoordinate(int pos, MapCoord c) {coords.insert(coords.begin() + pos, c);}
	inline void addCoordinate(MapCoord c) {coords.push_back(c);}
	inline void deleteCoordinate(int pos) {coords.erase(coords.begin() + pos);}
};

/// Object type which can only be used for point symbols, and is also the only object which can be used with them
class PointObject : public Object
{
public:
	PointObject(Map* map, MapCoord position, Symbol* symbol = NULL);
    virtual Object* duplicate();
	
	void setPosition(MapCoord position);
	MapCoord getPosition();
	
	void setRotation(float new_rotation);
	inline float getRotation() const {return rotation;}
	
private:
	float rotation;	// 0 to 2*M_PI
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
	
	TextObject(Map* map, Symbol* symbol = NULL);
	virtual Object* duplicate();
	
	inline bool hasSingleAnchor() const {return coords.size() == 1;}
	void setAnchorPosition(MapCoord position);
	MapCoord getAnchorPosition();	// or midpoint if a box is used
	void setBox(MapCoord midpoint, double width, double height);
	inline double getBoxWidth() const {assert(!hasSingleAnchor()); return coords[1].xd();}
	inline double getBoxHeight() const {assert(!hasSingleAnchor()); return coords[1].yd();}
	
	void setText(const QString& text);
	inline const QString& getText() const {return text;}
	
	void setHorizontalAlignment(HorizontalAlignment h_align);
	inline HorizontalAlignment getHorizontalAlignment() const {return h_align;}
	
	void setVerticalAlignment(VerticalAlignment v_align);
	inline VerticalAlignment getVerticalAlignment() const {return v_align;}
	
	void setRotation(float new_rotation);
	inline float getRotation() const {return rotation;}
	
private:
	QString text;
	HorizontalAlignment h_align;
	VerticalAlignment v_align;
	float rotation;	// 0 to 2*M_PI
};

#endif
