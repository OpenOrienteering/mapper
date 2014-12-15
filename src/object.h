/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013, 2014 Kai Pastor
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
#include "core/map_coord.h"
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

/**
 * Abstract base class which combines coordinates and a symbol to form an object
 * (in a map, or inside a point symbol as one of its elements).
 * 
 * Every object must have a symbol. If the symbol is not known, one of the
 * "undefined" symbols from the Map class can be used.
 * 
 * From the object's data, a call to update() will generate the object's "output",
 * that is a set of renderables and the calculation of the object's extent (bounding box).
 * The renderables can then be inserted into a map where they are used to display the object.
 */
class Object
{
friend class OCAD8FileImport;
friend class XMLImportExport;
public:
	/** Enumeration of possible object types. */
	enum Type
	{
		/**
		 * A single coordinate, no further coordinates can be added.
		 * For point symbols only.
		 */
		Point = 0,
		
		/**
		 * A dynamic list of coordinates.
		 * For line, area and combined symbols.
		 */
		Path = 1,
		
		/**
		 * Either one or two coordinates, for single-anchor or box text.
		 * For text symbols only.
		 */
		Text = 4
	};
	
	/** Creates an empty object with the given type,
	 *  optionally also assigning a symbol. */
	Object(Type type, const Symbol* symbol = NULL);
	
	/** Destructs the object. */
	virtual ~Object();
	
	/** Creates an identical copy of the object. */
	virtual Object* duplicate() const = 0;
	
	/**
	 * Checks for equality with another object. If compare_symbol is set,
	 * also the symbols are compared for having the same properties.
	 */
	bool equals(const Object* other, bool compare_symbol) const;
	
	/** Assignment, replaces this object's content with that of the other. */
	virtual Object& operator= (const Object& other);
	
	/** Returns the object type determined by the subclass */
	inline Type getType() const {return type;}
	
	/** Convenience cast to PointObject with type checking */
	PointObject* asPoint();
	/** Convenience cast to PointObject with type checking */
	const PointObject* asPoint() const;
	/** Convenience cast to PathObject with type checking */
	PathObject* asPath();
	/** Convenience cast to PathObject with type checking */
	const PathObject* asPath() const;
	/** Convenience cast to TextObject with type checking */
	TextObject* asText();
	/** Convenience cast to TextObject with type checking */
	const TextObject* asText() const;
	
	/** Loads the object in the old "native" file format from the given file. */
	void load(QIODevice* file, int version, Map* map);
	
	/** Saves the object in xml format to the given stream. */
	void save(QXmlStreamWriter& xml) const;
	/**
	 * Loads the object in xml format from the given stream.
	 * @param xml The stream to load the object from, must be at the correct tag.
	 * @param map The map in which the object will be inserted. Is assigned to
	 *     the object's map pointer and may be NULL.
	 * @param symbol_dict A dictionary mapping symbol IDs to symbol pointers.
	 * @param symbol If set, this symbol will be assigned to the object,
	 *     if NULL, the symbol will be read from the stream.
	 */
	static Object* load(QXmlStreamReader& xml, Map* map, const SymbolDictionary& symbol_dict, const Symbol* symbol = 0);
	
	/**
	 * Checks if the output_dirty flag is set and if yes,
	 * regenerates output and extent; returns true if output was previously dirty.
	 * @param force If set to true, recalculates the output and extent even
	 *     if the dirty flag is not set.
	 * @param insert_new_renderables If the object has a map pointer and this
	 *     flag is set, it will insert newly generated renderables into the map.
	 */
	bool update(bool force = false, bool insert_new_renderables = true) const;
	
	/** Moves the whole object
	 * @param dx X offset in native map coordinates.
	 * @param dy Y offset in native map coordinates.
	 */
	void move(qint64 dx, qint64 dy);
	
	/** Scales all coordinates, with the given scaling center */
	void scale(MapCoordF center, double factor);
	
	/** Scales all coordinates, with the center (0, 0).
	 * @param factor_x horizontal scaling factor
	 * @param factor_y vertical scaling factor
	 */
	void scale(double factor_x, double factor_y);
	
	/** Rotates the whole object around the center point.
	 *  The angle must be given in radians. */
	void rotateAround(MapCoordF center, double angle);
	
	/** Rotates the whole object around the center (0, 0).
	 *  The angle must be given in radians. */
	void rotate(double angle);
	
	/**
	 * Checks if the given coord, with the given tolerance, is on this object;
	 * with extended_selection, the coord is on point objects always
	 * if it is whithin their extent, otherwise it has to be close to
	 * their midpoint. Returns a Symbol::Type which specifies on which
	 * symbol type the coord is
	 * (important for combined symbols which can have areas and lines).
	 */
	int isPointOnObject(MapCoordF coord, float tolerance, bool treat_areas_as_paths, bool extended_selection) const;
	
	/** Checks if a path point (excluding curve control points)
	 * is included in the given box */
	bool intersectsBox(QRectF box) const;
	
	/** Takes ownership of the renderables */
	void takeRenderables();
	
	/** Deletes the renderables (and extent), undoing update() */
	void clearRenderables();
	
	/** Returns the renderables, read-only */
	inline const ObjectRenderables& renderables() const {return output;}
	
	// Getters / Setters
	
	/**
	 * Returns the raw MapCoordVector of the object.
	 * It's layout and interpretation depends on the object type.
	 */
	inline const MapCoordVector& getRawCoordinateVector() const {return coords;}
	
	/** Sets the object output's dirty state. */
	inline void setOutputDirty(bool dirty = true) {output_dirty = dirty;}
	/** Returns if the object's output must be regenerated. */
	inline bool isOutputDirty() const {return output_dirty;}
	
	/**
	 * Changes the object's symbol, returns if successful.
	 * 
	 * Some conversions are impossible, for example point to line. Normally,
	 * this method checks if the types of the old and the new symbol are
	 * compatible. If the old symbol pointer is no longer valid, you can
	 * use no_checks to disable this.
	 */
	bool setSymbol(const Symbol* new_symbol, bool no_checks);
	/** Returns the object's symbol. */
	inline const Symbol* getSymbol() const {return symbol;}
	
	/** NOTE: The extent is only valid after update() has been called! */
	inline const QRectF& getExtent() const {return extent;}
	
	/** Sets the object's map pointer. May be NULL if the object is not in a map. */
	inline void setMap(Map* map) {this->map = map;}
	/** Returns the object's map pointer. */
	inline Map* getMap() const {return map;}
	
	/** Constructs an object of the given type with the given symbol. */
	static Object* getObjectForType(Type type, const Symbol* symbol = NULL);
	
	
	/** Defines a type which maps keys to values, to be used for tagging objects. */
	typedef QHash<QString, QString> Tags;
	
	/** Returns a const reference to the object's tags. */
	const Tags& tags() const;
	
	/** Replaces the object's tags. */
	void setTags(const Tags& tags);
	
	/** Returns the value of the given tag key. */
	QString getTag(const QString& key) const;
	
	/** Sets the given tag key to the value. */
	void setTag(const QString& key, const QString& value);
	
	/** Removes the given tag key and its value. */
	void removeTag(const QString& key);
	
	
	/**
	 * @brief Extends a rectangle to enclose all of the object's control points.
	 */
	void includeControlPointsRect(QRectF& rect) const;
	
protected:
	Type type;
	const Symbol* symbol;
	MapCoordVector coords;
	Map* map;
	Tags object_tags;
	
	mutable bool output_dirty;        // does the output have to be re-generated because of changes?
	mutable QRectF extent;            // only valid after calling update()
	mutable ObjectRenderables output; // only valid after calling update()
};

/**
 * Object type which can be used for line, area and combined symbols.
 * Has a dynamic number of coordinates.
 * 
 * The coordinates are divided into one or multiple PathParts. A PathPart
 * is ended by a coordinate with the "hole point" flag. For all types of
 * flags which can be set, see the MapCoord documentation.
 */
class PathObject : public Object
{
public:
	/**
	 * Helper struct with information about parts of paths.
	 * A part is a path segment which is separated from other parts by
	 * a hole point at its end.
	 */
	struct PathPart
	{
		/** Index of first coordinate of this part in the coords vector */
		int start_index;
		/** Index of last coordinate of this part in the coords vector */
		int end_index;
		/** Index of first coordinate of this part in the PathCoord vector */
		int path_coord_start_index;
		/** Index of last coordinate of this part in the PathCoord vector */
		int path_coord_end_index;
		/** Pointer to path part containing this part */
		PathObject* path;
		
		/** Returns the number of coordinates which make up this part.
		 *  See also calcNumRegularPoints(). */
		inline int getNumCoords() const {return end_index - start_index + 1;}
		
		/** Calculates the number of points in this part,
		 *  excluding close points and curve handles. */
		int calcNumRegularPoints() const;
		
		/**
		 * Returns if the path part is closed. Objects with area symbols must
		 * always be closed.
		 * 
		 * For closed parts, the last coordinate is at the same position as the
		 * first coordinate of the part and has the "close point" flag set.
		 * These coords will move together when moved by the user, appearing as
		 * just one coordinate.
		 * 
		 * Parts can be closed and opened with setClosed() or connectEnds().
		 */
		inline bool isClosed() const
		{
			Q_ASSERT(end_index >= 0 && end_index < (int)path->coords.size());
			return path->coords[end_index].isClosePoint();
		}
		
		/**
		 * Closes or opens the sub-path.
		 * 
		 * If closed == true and may_use_existing_close_point == false,
		 * a new point is added as closing point even if its coordinates
		 * are identical to the existing last point. Else, the last point
		 * may be reused.
		 */
		void setClosed(bool closed, bool may_use_existing_close_point = false);
		
		/** Like setClosed(true), but merges start and end point at their center */
		void connectEnds();
		
		/** Returns the length of the part. */
		double getLength() const;
		
		/** Calculates the area of this part. */
		double calculateArea() const;
	};
	
	/** Returned by calcAllIntersectionsWith(). */
	struct Intersection
	{
		/** Coordinate of the intersection */
		MapCoordF coord;
		/** Part index of intersection */
		int part_index;
		/** Length of path until this intersection point */
		double length;
		/** Part index of intersection in other path */
		int other_part_index;
		/** Length of other path until this intersection point */
		double other_length;
		
		/** Compares the length attribute. */
		inline bool operator< (const Intersection& b) const {return length < b.length;}
		/** Fuzzy equality check. */
		inline bool operator== (const Intersection& b) const
		{
			// NOTE: coord is not compared, as the intersection is defined by the other params already.
			const double epsilon = 1e-10;
			return part_index == b.part_index &&
				qAbs(length - b.length) <= epsilon &&
				other_part_index == b.other_part_index &&
				qAbs(other_length - b.other_length) <= epsilon;
		}
		
		/**
		 * Creates an Intersection at the position specified by factors a and b
		 * between the a0/a1 and b0/b1 PathCoords in the given parts.
		 */
		static Intersection makeIntersectionAt(double a, double b, const PathCoord& a0, const PathCoord& a1, const PathCoord& b0, const PathCoord& b1, int part_index, int other_part_index);
	};
	
	/** std::vector of Intersection with the ability to sort them and remove duplicates. */
	class Intersections : public std::vector<Intersection>
	{
	public:
		/** Sorts the intersections and removes duplicates. */
		void clean();
	};
	
	
	/** Constructs a PathObject, optionally assigning a symbol. */
	PathObject(const Symbol* symbol = NULL);
	
	/** Constructs a PathObject, assigning initial coords and optionally the map pointer. */
	PathObject(const Symbol* symbol, const MapCoordVector& coords, Map* map = 0);
	
	/** Creates a duplicate of the path. Use asPath() on the result. */
	virtual Object* duplicate() const;
	
	/** Creates a new PathObject which contains a duplicate of only one part of this object. */
	PathObject* duplicatePart(int part_index) const;
	
	/** Replaces this object's contents by those of the other. */
	virtual Object& operator=(const Object& other);
	
	// Coordinate access methods
	
	/** Returns the number of coordinates, including curve handles and close points. */
	inline int getCoordinateCount() const {return (int)coords.size();}
	/** Returns the i-th coordinate. */
	inline MapCoord& getCoordinate(int pos) {Q_ASSERT(pos >= 0 && (std::size_t)pos < coords.size()); return coords[pos];}
	/** Returns the i-th coordinate. */
	inline const MapCoord& getCoordinate(int pos) const {Q_ASSERT(pos >= 0 && (std::size_t)pos < coords.size()); return coords[pos];}
	
	/** Replaces the i-th coordinate with c. */
	void setCoordinate(int pos, MapCoord c);
	
	/** Adds the coordinate at the given index. */
	void addCoordinate(int pos, MapCoord c);
	
	/** Adds the coordinate at the end, optionally starting a new part.
	 *  If starting a new part, make sure that the last coord of the old part
	 *  has the hole point flag! */
	void addCoordinate(MapCoord c, bool start_new_part = false);
	
	/**
	 * Deletes a coordinate from the path.
	 * @param pos Index of the coordinate to delete.
	 * @param adjust_other_coords If set and the deleted coordinate was joining
	 *     two bezier curves, adapts the adjacent curves with a strategy defined
	 *     by delete_bezier_point_action. adjust_other_coords does not work
	 *     when deleting bezier curve handles!
	 * @param delete_bezier_point_action Must be an enum value from
	 *     Settings::DeleteBezierPointAction if adjust_other_coords is set.
	 */
	void deleteCoordinate(int pos, bool adjust_other_coords, int delete_bezier_point_action = -1);
	
	/** Deletes all coordinates of the object. */
	void clearCoordinates();
	
	/** Returns a coordinate with shifted index,
	 *  see shiftedCoordIndex() for details. */
	const MapCoord& shiftedCoord(int base_index, int offset, const PathPart& part) const;
	
	/** Returns a coordinate with shifted index,
	 *  see shiftedCoordIndex() for details. */
	MapCoord& shiftedCoord(int base_index, int offset, const PathPart& part);
	
	/**
	 * Calculates a shifted coordinate index, correctly handling wrap-around
	 * for closed parts. Returns -1 if going over an open path end or after
	 * looping around once in a closed path.
	 * @param base_index The base index from which to start.
	 * @param offset Offset from the base index.
	 * @param part Reference to part in which the base_index is.
	 */
	int shiftedCoordIndex(int base_index, int offset, const PathPart& part) const;
	
	/** Finds the path part containing the given coord index. */
	const PathPart& findPartForIndex(int coords_index) const;
	
	/** Finds the path part containing the given coord index. */
	int findPartIndexForIndex(int coords_index) const;
	
	/** Checks if the coord with given index is a curve handle. */
	bool isCurveHandle(int coord_index) const;
	
	
	/** Returns the number of path parts in this object */
	inline int getNumParts() const {return (int)parts.size();}
	
	/** Returns the i-th path part. */
	inline PathPart& getPart(int index) {return parts[index];}
	
	/** Returns the i-th path part. */
	inline const PathPart& getPart(int index) const {return parts[index];}
	
	/** Returns if the first path part (with index 0) is closed. */
	inline bool isFirstPartClosed() const {return (getNumParts() > 0) ? parts[0].isClosed() : false;}
	
	/** Deletes the i-th path part. */
	void deletePart(int part_index);
	
	/**
	 * Adjusts the start/end index attributes of all affected parts after
	 * the part with the given index has changed its size by change.
	 * This includes the changed part.
	 */
	void partSizeChanged(int part_index, int change);
	
	
	/**
	 * Returns the PathCoord vector, giving linearized information about the
	 * path's shape. This is only valid if the object is not dirty!
	 */
	inline const PathCoordVector& getPathCoordinateVector() const {return path_coords;}
	
	/** Clears the PathCoord vector. */
	inline void clearPathCoordinates() {path_coords.clear();}
	
	
	// Pattern methods
	
	/**
	 * Returns the rotation of the object pattern. Only has an effect in
	 * combination with a symbol interpreting this value.
	 */
	inline float getPatternRotation() const {return pattern_rotation;}
	
	/**
	 * Sets the rotation of the object pattern. Only has an effect in
	 * combination with a symbol interpreting this value.
	 */
	inline void setPatternRotation(float rotation) {pattern_rotation = rotation; output_dirty = true;}
	
	/**
	 * Returns the origin of the object pattern. Only has an effect in
	 * combination with a symbol interpreting this value.
	 */
	inline MapCoord getPatternOrigin() const {return pattern_origin;}
	
	/**
	 * Sets the origin of the object pattern. Only has an effect in
	 * combination with a symbol interpreting this value.
	 */
	inline void setPatternOrigin(const MapCoord& origin) {pattern_origin = origin; output_dirty = true;}
	
	
	// Operations
	
	/**
	 * Calculates the closest point on the path to the given coordinate,
	 * returns the squared distance of these points and PathCoord information
	 * for the point on the path.
	 * 
	 * This does not need to be an existing path coordinate. This method is
	 * usually called to find the position on the path the user clicked on.
	 * part_index can be set to a valid part index to constrain searching
	 * to this specific path part.
	 */
	void calcClosestPointOnPath(MapCoordF coord, float& out_distance_sq,
								PathCoord& out_path_coord, int part_index = -1) const;
	
	/**
	 * Calculates the closest control point coordinate to the given coordiante,
	 * returns the squared distance of these points and the index of the control point.
	 */
	void calcClosestCoordinate(MapCoordF coord, float& out_distance_sq, int& out_index) const;
	
	/**
	 * Splits the segment beginning at the coordinate with the given index
	 * with the given bezier curve parameter or split ratio.
	 * Returns the index of the added point.
	 */
	int subdivide(int index, float param);
	
	/**
	 * Returns if connectIfClose() would change something with the given parameters
	 */
	bool canBeConnected(const PathObject* other, double connect_threshold_sq) const;
	
	/**
	 * Returns if the objects were connected (if so, you can delete the other object).
	 * If one of the paths has to be reversed, it is done for the "other" path.
	 * Otherwise, the "other" path is not changed.
	 */
	bool connectIfClose(PathObject* other, double connect_threshold_sq);
	
	/**
	 * Connects the given parts, optionally merging the end coordinates at the
	 * center position, and copying over the coordindates from other.
	 */
	void connectPathParts(int part_index, PathObject* other,
						  int other_part_index, bool prepend, bool merge_ends = true);
	
	/**
	 * Splits the path into up to two parts at the given position.
	 */
	void splitAt(const PathCoord& split_pos, Object*& out1, Object*& out2);
	
	/**
	 * Replaces the path with a range of it starting and ending at the given lengths.
	 */
	void changePathBounds(int part_index, double start_len, double end_len);
	
	/** Appends (copies) the coordinates of other to this path. */
	void appendPath(PathObject* other);
	
	/** Appends (copies) the coordinates of a specific part
	 *  of the other path to this path. */
	void appendPathPart(PathObject* other, int part_index);
	
	/**
	 * Reverses the object's coordinates, resulting in switching
	 * the start / end / mid / dash symbol direction for line symbols.
	 */
	void reverse();
	
	/** Like reverse(), but only for the given part. */
	void reversePart(int part_index);
	
	/** Ensures that all parts are closed. Useful for objects with area-only symbols. */
	void closeAllParts();
	
	/**
	 * Creates a new path object containing the given coordinate range.
	 * The range must be within one part, and end may be smaller than start
	 * if the path is closed to wrap around.
	 */
	PathObject* extractCoordsWithinPart(int start, int end) const;
	
	/**
	 * Converts all polygonal sections in this path to splines.
	 * If at least one section is converted, returns true and
	 * returns an undo duplicate if the corresponding pointer is set.
	 */
	bool convertToCurves(PathObject** undo_duplicate = NULL);
	
	/**
	 * Converts the given range of coordinates to a spline by inserting handles.
	 * The range must consist of only polygonal segments before.
	 */
	void convertRangeToCurves(int part_number, int start_index, int end_index);
	
	/**
	 * Tries to remove points while retaining the path shape as much as possible.
	 * If at least one point is changed, returns true and
	 * returns an undo duplicate if the corresponding pointer is set.
	 */
	bool simplify(PathObject** undo_duplicate, float threshold);
	
	/** See Object::isPointOnObject() */
	int isPointOnPath(MapCoordF coord, float tolerance,
					  bool treat_areas_as_paths, bool extended_selection) const;
	
	/**
	 * Returns true if the given coordinate is inside the area
	 * defined by this object, which must be closed.
	 */
	bool isPointInsideArea(MapCoordF coord) const;
	
	/** Calculates the average distance (of the first part) to another path */
	float calcAverageDistanceTo(PathObject* other) const;
	
	/** Calculates the maximum distance (of the first part) to another path */
	float calcMaximumDistanceTo(PathObject* other) const;
	
	/**
	 * Calculates and adds all intersections with the other path to out.
	 * Note: intersections are not sorted and may contain duplicates!
	 * To clean them up, call clean() on the Intersections object after adding
	 * all intersections with objects you are interested in.
	 */
	void calcAllIntersectionsWith(PathObject* other, Intersections& out) const;
	
	/** Called by Object::update() */
	void updatePathCoords(MapCoordVectorF& float_coords) const;
	
	/** Called by Object::load() */
	void recalculateParts();
	
protected:
	/**
	 * Advances the cur_path_coord and current_index variables until they are
	 * at the path coord segment containing the cumulative length cur_length.
	 * While doing so, adds all coords it passes to the out_flags and out_coords.
	 * TODO: put this into a PathCoordIterator, internally! This is far too complex.
	 * 
	 * @param flags Coordinate flags of the object to iterate over.
	 * @param coords Coordinate positions of the object to iterate over.
	 * @param path_coords PathCoords of the object to iterate over.
	 * @param cur_path_coord Current path coord index, will be updated.
	 * @param current_index Current MapCoord index, will be updated.
	 * @param cur_length Target cumulative path length (TODO: rename)
	 * @param enforce_wrap Enforce wrapping around a closed part once.
	 * @param start_bezier_index Index of first point of bezier curve, if this
	 *     bezier curve has been split to determine the start position which
	 *     must already be in out_flags and out_coords. -1 if this start
	 *     position was not determined by a curve split.
	 * @param out_flags Passed flags will be appended here.
	 * @param out_coords Passed coords will be appended here.
	 * @param o3 If start_bezier_index is valid, pass in the first handle of
	 *     the second part of the split bezier curve.
	 * @param o4 If start_bezier_index is valid, pass in the second handle of
	 *     the second part of the split bezier curve.
	 *     This and o3 needs to be added to out_... if passing over a coordinate.
	 *     If not passing over a coordinate, instead the curve made up by
	 *     the start coordinate, o3, o4 and the next path coordinate needs
	 *     to be split again and added to the output.
	 */
	bool advanceCoordinateRangeTo(
		const MapCoordVector& flags,
		const MapCoordVectorF& coords,
		const PathCoordVector& path_coords,
		int& cur_path_coord,
		int& current_index,
		float cur_length,
		bool enforce_wrap,
		int start_bezier_index,
		MapCoordVector& out_flags,
		MapCoordVectorF& out_coords,
		const MapCoordF& o3,
		const MapCoordF& o4
	) const;
	
	/**
	 * Calculates the factors which should be applied to the length of the
	 * remaining bezier curve handle vectors when deleting a point joining
	 * two bezier curves to try to retain the original curves' shape.
	 * 
	 * This is a simple version, the result should be optimized with
	 * calcBezierPointDeletionRetainingShapeOptimization().
	 * 
	 * p0, p1, p2, q0 make up the first original curve,
	 * q0, q1, q2, q3 make up the second original curve.
	 * out_pfactor is set to the factor to apply to the vector (p1 - p0),
	 * out_qfactor is set to the factor to apply to the vector (q2 - q3),
	 */
	void calcBezierPointDeletionRetainingShapeFactors(
		MapCoord p0,
		MapCoord p1,
		MapCoord p2,
		MapCoord q0,
		MapCoord q1,
		MapCoord q2,
		MapCoord q3,
		double& out_pfactor,
		double& out_qfactor
	) const;
	
	/**
	 * Uses nonlinear optimization to improve the first result obtained by
	 * calcBezierPointDeletionRetainingShapeFactors().
	 */
	void calcBezierPointDeletionRetainingShapeOptimization(
		MapCoord p0,
		MapCoord p1,
		MapCoord p2,
		MapCoord q0,
		MapCoord q1,
		MapCoord q2,
		MapCoord q3,
		double& out_pfactor,
		double& out_qfactor
	) const;
	
	/**
	 * Is used internally by calcBezierPointDeletionRetainingShapeOptimization()
	 * to calculate the current cost. Evaluates the distance between p0 ... p3
	 * and the reference path.
	 */
	float calcBezierPointDeletionRetainingShapeCost(
		MapCoord p0,
		MapCoordF p1,
		MapCoordF p2,
		MapCoord p3,
		PathObject* reference
	) const;
	
	/**
	 * Sets coord as the point which closes a part: sets the correct flags
	 * on it and replaces the coord at the given index by it.
	 * TODO: make separate methods? Setting coords exists already.
	 */
	void setClosingPoint(int index, MapCoord coord);
	
	
	/**
	 * Rotation angle of the object pattern. Only used if the object
	 * has a symbol which interprets this value.
	 */
	float pattern_rotation;
	
	/**
	 * Origin shift of the object pattern. Only used if the object
	 * has a symbol which interprets this value.
	 */
	MapCoord pattern_origin;
	
private:
	/** Path parts list */
	mutable std::vector<PathPart> parts;
	
	/** Linearized shape information, only valid after calling update()! */
	mutable PathCoordVector path_coords;
};

/**
 * Object type which can only be used for point symbols,
 * and is also the only object which can be used with them.
 * 
 * Has exactly one coordinate, and additionally a rotation parameter.
 */
class PointObject : public Object
{
public:
	/** Constructs a PointObject, optionally assigning the symbol. */
	PointObject(const Symbol* symbol = NULL);
	
	/** Creates a duplicate of the point, use asPoint() on the result. */
	virtual Object* duplicate() const;
	
	/** Replaces the content of this object by that of anothe. */
	virtual Object& operator=(const Object& other);
	
	
	/** Sets the point's position to a new position given in native map coordinates. */
	void setPosition(qint64 x, qint64 y);
	
	/** Changes the point's position. */
	void setPosition(MapCoordF coord);
	
	/** Returns the point's position in native map coordinates. */
	void getPosition(qint64& x, qint64& y) const;
	
	/** Returns the point's position as MapCoordF. */
	MapCoordF getCoordF() const;
	
	/** Returns the point's coordinate. */
	MapCoord getCoord() const;
	
	
	/**
	 * Sets the point object's rotation (in radians).
	 * This may only be done if the object's symbol is rotatable!
	 */
	void setRotation(float new_rotation);
	
	/**
	 * Returns the point object's rotation (in radians). This is only used
	 * if the object has a symbol which interprets this value.
	 */
	inline float getRotation() const {return rotation;}
	
	
private:
	/** The object's rotation (in radians). */
	float rotation;
};


//### Object inline code ###

inline
const Object::Tags& Object::tags() const
{
	return object_tags;
}

inline
QString Object::getTag(const QString& key) const
{
	return object_tags.value(key);
}


#endif
