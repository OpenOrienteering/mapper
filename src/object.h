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


#ifndef _OPENORIENTEERING_OBJECT_H_
#define _OPENORIENTEERING_OBJECT_H_

#include <vector>

#include <QRectF>
#include <QHash>

#include "file_format.h"
#include "map_coord.h"
#include "path_coord.h"
#include "renderable.h"
#include "symbol.h"

QT_BEGIN_NAMESPACE
class QIODevice;
class QXmlStreamReader;
class QXmlStreamWriter;
QT_END_NAMESPACE

class Map;
class PointObject;
class PathObject;
class TextObject;

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
		Text = 4
	};
	
	/// Creates an empty object
	Object(Type type, Symbol* symbol = NULL);
	virtual ~Object();
	virtual Object* duplicate() = 0;
	bool equals(Object* other, bool compare_symbol);
	virtual Object& operator= (const Object& other);
	
	/// Returns the object type determined by the subclass
	inline Type getType() const {return type;}
	// Convenience casts with type checking
	PointObject* asPoint();
	const PointObject* asPoint() const;
	PathObject* asPath();
	const PathObject* asPath() const;
	TextObject* asText();
	const TextObject* asText() const;
	
	void save(QIODevice* file);
	void load(QIODevice* file, int version, Map* map);
	void save(QXmlStreamWriter& xml) const;
	static Object* load(QXmlStreamReader& xml, Map* map, const SymbolDictionary& symbol_dict, Symbol* symbol = 0) throw (FileFormatException);
	
	/// Checks if the output_dirty flag is set and if yes, regenerates output and extent; returns true if output was previously dirty.
	/// Use force == true to force a redraw
	bool update(bool force = false, bool insert_new_renderables = true);
	
	/// Moves the whole object
	void move(qint64 dx, qint64 dy);
	
	/// Scales all coordinates, with the given scaling center
	void scale(MapCoordF center, double factor);
	
	/// Rotates the whole object. The angle must be given in radians.
	void rotateAround(MapCoordF center, double angle);
	
	/// Checks if the given coord, with the given tolerance, is on this object; with extended_selection, the coord is on point objects always if it is whithin their extent,
	/// otherwise it has to be close to their midpoint. Returns a Symbol::Type which specifies on which symbol type the coord is
	/// (important for combined symbols which can have areas and lines).
	int isPointOnObject(MapCoordF coord, float tolerance, bool treat_areas_as_paths, bool extended_selection);
	
	/// Checks if a path point (excluding curve control points) is included in the given box
	bool intersectsBox(QRectF box);
	
	/// Take ownership of the renderables
	void takeRenderables();
	
	/// Delete the renderables (and extent), undoing update()
	void clearRenderables();
	
	/// The renderables, read-only
	inline const ObjectRenderables& renderables() const {return output;}
	
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

	void setTag(const QString& key, const QString& value) {this->tags[key] = value;}
	QString getTag(const QString& key) const {return this->tags[key];}
	bool hasTag(const QString& key) const {return this->tags.contains(key);}
	QHash<QString, QString> getTags() const {return tags;}
	void setTags(const QHash<QString, QString>& tags) {this->tags = tags;}
	
protected:
	Type type;
	Symbol* symbol;
	MapCoordVector coords;
	Map* map;
	QHash<QString, QString> tags;
	
	bool output_dirty;				// does the output have to be re-generated because of changes?
	QRectF extent;					// only valid after calling update()
	ObjectRenderables output;		// only valid after calling update()
};

/// Object type which can be used for line, area and combined symbols
class PathObject : public Object
{
public:
	/// Every PathPart has a type taken from this enum which affects how they can be edited: paths can be edited freely, the other types retain their shape.
	/// TODO: currently, all parts are implicitly assumed to be of type Path
	enum PartType
	{
		Path = 0,
		Circle = 1,
		Rect = 2
	};
	
	/// Helper struct with information about parts of paths. A part is a path segment which is separated from other parts by hole points.
	struct PathPart
	{
		int start_index;
		int end_index;
		int path_coord_start_index;
		int path_coord_end_index;
		PathObject* path;
		
		inline int getNumCoords() const {return end_index - start_index + 1;}
		inline bool isClosed() const {Q_ASSERT(end_index >= 0 && end_index < (int)path->coords.size()); return path->coords[end_index].isClosePoint();}
		/// Closes or opens the sub-path.
		/// If closed == true and may_use_existing_close_point == false, a new point is added as closing point even if its coordinates are identical to the existing last point.
		void setClosed(bool closed, bool may_use_existing_close_point = false);
		/// like setClosed(true), but merges start and end point at their center
		void connectEnds();	
		/// Calculates the number of points, excluding close points and curve handles
		int calcNumRegularPoints();
		
		/// This returns the *cumulative length* at the part end!
		/// TODO: rename method
		double getLength();
		double calculateArea();
	};
	
	/// Returned by calcAllIntersectionsWith().
	struct Intersection
	{
		/// Coordinate of the intersection
		MapCoordF coord;
		/// Part index of intersection
		int part_index;
		/// Length of path until this intersection point
		double length;
		/// Part index of intersection in other path
		int other_part_index;
		/// Length of other path until this intersection point
		double other_length;
		
		inline bool operator< (const Intersection& b) const {return length < b.length;}
		inline bool operator== (const Intersection& b) const
		{
			// NOTE: coord is not compared, as the intersection is defined by the other params already.
			const double epsilon = 1e-10;
			return part_index == b.part_index &&
				qAbs(length - b.length) <= epsilon &&
				other_part_index == b.other_part_index &&
				qAbs(other_length - b.other_length) <= epsilon;
		}
		
		static Intersection makeIntersectionAt(double a, double b, const PathCoord& a0, const PathCoord& a1, const PathCoord& b0, const PathCoord& b1, int part_index, int other_part_index);
	};
	class Intersections : public std::vector<Intersection>
	{
	public:
		/// Sorts the intersections and removes duplicates.
		void clean();
	};
	
	
	PathObject(Symbol* symbol = NULL);
	PathObject(Symbol* symbol, const MapCoordVector& coords, Map* map = 0);
	virtual Object* duplicate();
	PathObject* duplicatePart(int part_index);
	virtual Object& operator=(const Object& other);
	
	// Coordinate access methods
	
	inline int getCoordinateCount() const {return (int)coords.size();}
	inline MapCoord& getCoordinate(int pos) {Q_ASSERT(pos >= 0 && pos < (int)coords.size()); return coords[pos];}
	void setCoordinate(int pos, MapCoord c);
	void addCoordinate(int pos, MapCoord c);
	void addCoordinate(MapCoord c, bool start_new_part = false);
	/// NOTE: adjust_other_coords does not work if deleting bezier curve handles!
	///       delete_bezier_point_action must be an enum value from Settings::DeleteBezierPointAction if
	///          deleting points from bezier splines with adjust_other_coords == true.
	void deleteCoordinate(int pos, bool adjust_other_coords, int delete_bezier_point_action = -1);
	void clearCoordinates();
	
	MapCoord& shiftedCoord(int base_index, int offset, PathPart& part);
	int shiftedCoordIndex(int base_index, int offset, PathPart& part); // Returns the base_index shifted by offset, correctly handling holes in areas and closed paths. Returns -1 if the index is invalid (happens if going over a path end or after looping around once in a closed path)
	PathPart& findPartForIndex(int coords_index);
	int findPartIndexForIndex(int coords_index);
	bool isCurveHandle(int coord_index);
	
	inline int getNumParts() const {return (int)parts.size();}
	inline PathPart& getPart(int index) {return parts[index];}
	inline bool isFirstPartClosed() const {return (getNumParts() > 0) ? parts[0].isClosed() : false;}
	void deletePart(int part_index);
	void partSizeChanged(int part_index, int change); // changes the parts[] information about start en
	
	inline const PathCoordVector& getPathCoordinateVector() const {return path_coords;}
	inline void clearPathCoordinates() {path_coords.clear();}
	
	// Pattern methods
	
	inline float getPatternRotation() const {return pattern_rotation;}
	inline void setPatternRotation(float rotation) {pattern_rotation = rotation; output_dirty = true;}
	inline MapCoord getPatternOrigin() const {return pattern_origin;}
	inline void setPatternOrigin(const MapCoord& origin) {pattern_origin = origin; output_dirty = true;}
	
	// Operations
	
	/// Calculates the closest point on the path to the given coordinate, returns the squared distance of these points and PathCoord information for the point on the path.
	/// This does not have to be an existing path coordinate. This method is usually called to find the position on the path the user clicked on.
	/// part_index can be set to a valid part index to constrain searching to this specific path part.
	void calcClosestPointOnPath(MapCoordF coord, float& out_distance_sq, PathCoord& out_path_coord, int part_index = -1);
	/// Calculates the closest control point coordinate to the given coordiante, returns the squared distance of these points and the index of the control point.
	void calcClosestCoordinate(MapCoordF coord, float& out_distance_sq, int& out_index);
	/// Splits the segment beginning at the coordinate with the given index with the given bezier curve parameter or split ratio. Returns the index of the added point.
	int subdivide(int index, float param);
	/// Returns if connectIfClose() would do something with the given parameters
	bool canBeConnected(PathObject* other, double connect_threshold_sq);
	/// Returns if the objects were connected (if so, you can delete the other object). If one of the paths has to be reversed, it is done for the "other" path. Otherwise, the "other" path is not changed.
	bool connectIfClose(PathObject* other, double connect_threshold_sq);
	/// Connects the given parts, merging the end coordinates at the center position and copying over the coordindates from other.
	void connectPathParts(int part_index, PathObject* other, int other_part_index, bool prepend);
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
	/// Ensures that all parts are closed. Useful for objects with area-only symbols.
	void closeAllParts();
	/// Creates a new path object containing the given coordinate range.
	PathObject* extractCoords(int start, int end);
	/// Converts all polygonal sections in this path to splines.
	/// If at least one section is converted, returns true and
	/// returns an undo duplicate if the corresponding pointer is set.
	bool convertToCurves(PathObject** undo_duplicate = NULL);
	/// Converts the given range of coordinates to a spline by inserting handles.
	/// The range must consist of only polygonal segments before.
	void convertRangeToCurves(int part_number, int start_index, int end_index);
	/// Tries to remove points while retaining the path shape as much as possible.
	/// If at least one point is changed, returns true and
	/// returns an undo duplicate if the corresponding pointer is set.
	bool simplify(PathObject** undo_duplicate = NULL);
	/// See Object::isPointOnObject()
	int isPointOnPath(MapCoordF coord, float tolerance, bool treat_areas_as_paths, bool extended_selection);
	/// Returns true if the given coordinate is inside the area defined by this object, which must be closed.
	bool isPointInsideArea(MapCoordF coord);
	/// Calculates the average distance (of the first part) to another path
	float calcAverageDistanceTo(PathObject* other);
	/// Calculates the maximum distance (of the first part) to another path
	float calcMaximumDistanceTo(PathObject* other);
	/// Calculates and adds all intersections with the other path to out.
	/// Note: intersections are not sorted and may contain duplicates!
	/// To clean them up, call clean() on the Intersections object after adding
	/// all intersections with objects you are interested in.
	void calcAllIntersectionsWith(PathObject* other, Intersections& out);
	
	/// Called by Object::update()
	void updatePathCoords(MapCoordVectorF& float_coords);
	/// Called by Object::load()
	void recalculateParts();
	
protected:
	bool advanceCoordinateRangeTo(const MapCoordVector& flags, const MapCoordVectorF& coords, const PathCoordVector& path_coords, int& cur_path_coord, int& current_index, float cur_length,
								  bool enforce_wrap, int start_bezier_index, MapCoordVector& out_flags, MapCoordVectorF& out_coords, const MapCoordF& o3, const MapCoordF& o4);
	void calcBezierPointDeletionRetainingShapeFactors(MapCoord p0, MapCoord p1, MapCoord p2, MapCoord q0, MapCoord q1, MapCoord q2, MapCoord q3, double& out_pfactor, double& out_qfactor);
	void calcBezierPointDeletionRetainingShapeOptimization(MapCoord p0, MapCoord p1, MapCoord p2, MapCoord q0, MapCoord q1, MapCoord q2, MapCoord q3, double& out_pfactor, double& out_qfactor);
	float calcBezierPointDeletionRetainingShapeCost(MapCoord p0, MapCoordF p1, MapCoordF p2, MapCoord p3, PathObject* reference);
	
	/// Sets coord as the point which closes a subpath (the normal path or a hole in it).
	void setClosingPoint(int index, MapCoord coord);
	
	float pattern_rotation;
	MapCoord pattern_origin;
	
	std::vector<PathPart> parts;
	PathCoordVector path_coords;	// only valid after calling update()
};

/// Object type which can only be used for point symbols, and is also the only object which can be used with them
class PointObject : public Object
{
public:
	PointObject(Symbol* symbol = NULL);
	virtual Object* duplicate();
	virtual Object& operator=(const Object& other);
	
	void setPosition(qint64 x, qint64 y);
	void setPosition(MapCoordF coord);
	void getPosition(qint64& x, qint64& y) const;
	MapCoordF getCoordF() const;
	MapCoord getCoord() const;
	
	void setRotation(float new_rotation);
	inline float getRotation() const {return rotation;}
	
private:
	float rotation;	// 0 to 2*M_PI
};

#endif
