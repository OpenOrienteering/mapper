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
friend class XMLImportExport;
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
    inline Type getType() const {return type;}
	
	void save(QFile* file);
	void load(QFile* file, int version, Map* map);
	
	/// Checks if the output_dirty flag is set and if yes, regenerates output and extent; returns true if output was previously dirty.
	/// Use force == true to force a redraw
	bool update(bool force = false, bool remove_old_renderables = true);
	
	/// Moves the whole object
	void move(qint64 dx, qint64 dy);
	
	/// Scales all coordinates, with the origin as scaling center
	void scale(double factor);
	
	/// Rotates the whole object
	void rotateAround(MapCoordF center, double angle);
	
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
	inline const MapCoordVector& getRawCoordinateVector() const {return coords;}
	
	inline void setOutputDirty(bool dirty = true) {output_dirty = dirty;}
	inline bool isOutputDirty() const {return output_dirty;}
	
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
	Map* map;
	
	bool output_dirty;				// does the output have to be re-generated because of changes?
	RenderableVector output;		// only valid after calling update()
	QRectF extent;					// only valid after calling update()
};

/// Object type which can be used for line, area and combined symbols
class PathObject : public Object
{
public:
	/// Helper struct with information about parts of paths. A part is a path segment which is separated from other parts by hole points.
	struct PathPart
	{
		int start_index;
		int end_index;
		int path_coord_start_index;
		int path_coord_end_index;
		PathObject* path;
		
		inline int getNumCoords() const {return end_index - start_index + 1;}
		inline bool isClosed() const {assert(end_index < (int)path->coords.size()); return path->coords[end_index].isClosePoint();}
		/// Closes or opens the sub-path
		void setClosed(bool closed);
		/// like setClosed(true), but merges start and end point at their center
		void connectEnds();	
		/// Calculates the number of points, excluding close points and curve handles
		int calcNumRegularPoints();
	};
	
	PathObject(Symbol* symbol = NULL);
	PathObject(Symbol* symbol, const MapCoordVector& coords, Map* map = 0);
    virtual Object* duplicate();
	PathObject* duplicateFirstPart();
	
	// Coordinate access methods
	inline int getCoordinateCount() const {return (int)coords.size();}
	
	inline MapCoord& getCoordinate(int pos) {return coords[pos];}
	void setCoordinate(int pos, MapCoord c);
	void addCoordinate(int pos, MapCoord c);
	void addCoordinate(MapCoord c);
	void deleteCoordinate(int pos, bool adjust_other_coords);	// adjust_other_coords does not work if deleting bezier curve handles!
	
	MapCoord& shiftedCoord(int base_index, int offset, PathPart& part);
	int shiftedCoordIndex(int base_index, int offset, PathPart& part); // Returns the base_index shifted by offset, correctly handling holes in areas and closed paths. Returns -1 if the index is invalid (happens if going over a path end or after looping around once in a closed path)
	PathPart& findPartForIndex(int coords_index);
	int findPartIndexForIndex(int coords_index);
	
	inline int getNumParts() const {return (int)parts.size();}
	inline PathPart& getPart(int index) {return parts[index];}
	inline bool isFirstPartClosed() const {return (getNumParts() > 0) ? parts[0].isClosed() : false;}
	void deletePart(int part_index);
	void partSizeChanged(int part_index, int change); // changes the parts[] information about start en
	
	inline const PathCoordVector& getPathCoordinateVector() const {return path_coords;}
	
	// Operations
	
	/// Calculates the closest point on the path to the given coordinate, returns the squared distance of these points and PathCoord information for the point on the path.
	/// This does not have to be an existing path coordinate. This method is usually called to find the position on the path the user clicked on.
	void calcClosestPointOnPath(MapCoordF coord, float& out_distance_sq, PathCoord& out_path_coord);
	/// Splits the segment beginning at the coordinate with the given index with the given bezier curve parameter or split ratio. Returns the index of the added point.
	int subdivide(int index, float param);
	/// Returns if connectIfClose() would do something with the given parameters
	bool canBeConnected(PathObject* other, double connect_threshold_sq);
	/// Returns if the objects were connected (if so, you can delete the other object). If one of the paths has to be reversed, it is done for the "other" path. Otherwise, the "other" path is not changed.
	bool connectIfClose(PathObject* other, double connect_threshold_sq);
	/// Splits the path into up to two parts at the given position
	void splitAt(const PathCoord& split_pos, Object*& out1, Object*& out2);
	/// Replaces the path with a range of it starting and ending at the given lengths
	void changePathBounds(int part_index, double start_len, double end_len);
	/// Appends (copies) the coordinates of other to this path
	void appendPath(PathObject* other);
	/// Appends (copies) the coordinates of a specific part of the other path to this path
	void appendPathPart(PathObject* other, int part_index);
	/// Reverses the object's coordinates, resulting in switching the dash direction for line symbols
	void reverse();
	/// Like reverse(), but only for the given part
	void reversePart(int part_index);
	/// See Object::isPointOnObject()
	int isPointOnPath(MapCoordF coord, float tolerance);
	
	/// Called by Object::update()
	void updatePathCoords(MapCoordVectorF& float_coords);
	/// Called by Object::load()
	void recalculateParts();
	
protected:
	void connectPathParts(int part_index, PathObject* other, int other_part_index, bool prepend);
	void advanceCoordinateRangeTo(const MapCoordVector& flags, const MapCoordVectorF& coords, const PathCoordVector& path_coords, int& cur_path_coord, int& current_index, float cur_length,
								  bool enforce_wrap, int start_bezier_index, MapCoordVector& out_flags, MapCoordVectorF& out_coords, const MapCoordF& o3, const MapCoordF& o4);
	/// Sets coord as the point which closes a subpath (the normal path or a hole in it).
	void setClosingPoint(int index, MapCoord coord);
	
	std::vector<PathPart> parts;
	PathCoordVector path_coords;	// only valid after calling update()
};

// TODO: circle, ellise and rectangle objects as subclasses of PathObject

/// Object type which can only be used for point symbols, and is also the only object which can be used with them
class PointObject : public Object
{
public:
	PointObject(Symbol* symbol = NULL);
    virtual Object* duplicate();
	
	void setPosition(qint64 x, qint64 y);
	void setPosition(MapCoordF coord);
	void getPosition(qint64& x, qint64& y) const;
	MapCoordF getCoordF() const;
	
	void setRotation(float new_rotation);
	inline float getRotation() const {return rotation;}
	
private:
	float rotation;	// 0 to 2*M_PI
};

#endif
