/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012, 2013, 2014 Kai Pastor
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


#ifndef _OPENORIENTEERING_PATH_COORD_H_
#define _OPENORIENTEERING_PATH_COORD_H_

#include "core/map_coord.h"

class PathCoord;
typedef std::vector<PathCoord> PathCoordVector;

/**
 * Paths that may consist of polygonal segments and curves are processed
 * into PathCoordVectors, approximating the path only with polygonal segments
 * and containing information about lengths etc.
 * 
 * A PathCoord represents the end point of one polygonal segment.
 * The first PathCoord for a part is generated for its start coordinate,
 * then only PathCoords at the end of segments (or inside bezier curves) follow.
 * So there is a PathCoord for each corner in the path.
 */
class PathCoord
{
public:
	/** Position of the PathCoord */
	MapCoordF pos;
	
	/** Cumulative length since path part start*/
	float clen;
	
	/** Index into the MapCoordVector(F) to the first coordinate of
	 *  the segment which contains this PathCoord */
	int index;
	
	/** Value of the curve / line parameter for this position */
	float param;
	
	
	/**
	 * Global position error threshold for approximating
	 * bezier curves with straight segments.
	 * TODO: make configurable
	 */
	static const float bezier_error;
	
	/**
	 * Global maximum length of generated PathCoord segments for curves.
	 * 
	 * This ensures that even for flat curves, segments will be generated regularly.
	 * This is important because while a curve may be flat, the mapping from the
	 * curve parameter to real position is not linear, which would result in problems.
	 * This is counteracted by generating many segments.
	 */
	static const float bezier_segment_maxlen;
	
	
	/**
	 * Calculates path coords for the given flags and coords.
	 * The position information from the flags is unused.
	 * @param flags Flags of the object.
	 * @param coords Coordinates of the object.
	 * @param path_coords The calculated path coords will be appended here.
	 */
	static void calculatePathCoords(
		const MapCoordVector& flags,
		const MapCoordVectorF& coords,
		PathCoordVector* path_coords
	);
	
	/**
	 * While processing path parts sequentially, this advances to the next part and
	 * returns its path coords in path_coords.
	 * TODO: extend documentation
	 */
	static bool getNextPathPart(
		const MapCoordVector& flags,
		const MapCoordVectorF& coords,
		int& part_start,
		int& part_end,
		PathCoordVector* path_coords,
		bool break_at_dash_points,
		bool append_path_coords
	);
	
	/**
	 * Creates a path coord for the coord with given index.
	 * @param path_coords Path coord vector of the object.
	 * @param index Index of normal MapCoord for which to create the PathCoord.
	 */
	static PathCoord findPathCoordForCoordinate(
		const PathCoordVector* path_coords,
		int index
	);
	
	/**
	 * Calculates a position at a given length along path.
	 * NOTE: works only for a single path part so far.
	 * @param flags Flags of the object.
	 * @param coords Coordinates of the object.
	 * @param path_coords Path coords of the object.
	 * @param length The desired length.
	 * @param path_coord_search_start If you know that the position is definitely
	 *     not before a certain path coord index, you can pass in the
	 *     search start index here for efficiency. Else, pass an integer starting with 0.
	 * @param out_pos The calculated position will be returned here.
	 * @param out_right_vector A vector pointing perpendicularly to the path's
	 *     direction to the right of the path will be returned here, if set.
	 *     Pass NULL to ignore.
	 */
	static void calculatePositionAt(
		const MapCoordVector& flags,
		const MapCoordVectorF& coords,
		const PathCoordVector& path_coords,
		float length,
		int& path_coord_search_start,
		MapCoordF* out_pos,
		MapCoordF* out_right_vector
	);
	
	/**
	 * Calculates a vector pointing perpendicularly to the path's
	 * direction to the right of the path at the given MapCoord index.
	 * NOTE: works only for a single path part so far.
	 * @param flags Flags of the object.
	 * @param coords Coordinates of the object.
	 * @param path_closed If the path is closed.
	 * @param i Index of the coordinate to calculate the vector for.
	 * @param scaling If set, a scaling factor is returned here which can
	 *     be used to scale symbols placed at the requested position. It
	 *     is 1 for smooth spots, and higher for sharp corners, such that
	 *     bordering lines offset from the main path by a distance scaled with
	 *     this factor will be in a constant distance from the path along
	 *     straigt segments. Pass NULL to ignore.
	 *     TODO: return angle here and calculate scaling factor in another function?
	 */
	static MapCoordF calculateRightVector(const MapCoordVector& flags, const MapCoordVectorF& coords, bool path_closed, int i, float* scaling);
	
	/**
	 * Calculates the path tangent at the given MapCoord index.
	 * Takes care of cases where successive points are at equal positions.
	 * @param coords Coordinates of the object.
	 * @param i Index of the coordinate where to query the tangent.
	 * @param backward If false, returns the forward tangent, if true, returns
	 *     the backward tangent. Makes a difference at sharp corners.
	 * @param ok Is set to true if the tangent can be found correctly, false if
	 *     failing to find the tangent.
	 */
	static MapCoordF calculateTangent(const MapCoordVector& coords, int i, bool backward, bool& ok);
	
	/** Version of calculateTangent() for MapCoordVectorF. */
	static MapCoordF calculateTangent(const MapCoordVectorF& coords, int i, bool backward, bool& ok);
	
	/**
	 * Similar to calculateTangent().
	 * TODO: remove this? This cannot cope with closed parts correctly, only closed paths.
	 */
	static MapCoordF calculateIncomingTangent(const MapCoordVectorF& coords, bool path_closed, int i, bool& ok);
	/**
	 * Similar to calculateTangent().
	 * TODO: remove this? This cannot cope with closed parts correctly, only closed paths.
	 */
	static MapCoordF calculateOutgoingTangent(const MapCoordVectorF& coords, bool path_closed, int i, bool& ok);
	
	/**
	 * Splits the cubic bezier curve made up by points c0 ... c3 at the
	 * curve parameter p and returns the new intermediate points in o0 ... o4.
	 */
	static void splitBezierCurve(
		MapCoordF c0,
		MapCoordF c1,
		MapCoordF c2,
		MapCoordF c3,
		float p,
		MapCoordF& o0,
		MapCoordF& o1,
		MapCoordF& o2,
		MapCoordF& o3,
		MapCoordF& o4
	);
	
private:
	/** Recursive approximation of a bezier curve by polygonal segments */
	static void curveToPathCoordRec(
		MapCoordF c0,
		MapCoordF c1,
		MapCoordF c2,
		MapCoordF c3,
		int coord_index,
		float max_error,
		float max_segment_len,
		PathCoordVector* path_coords,
		float p0,
		float p1
	);
	
	/** Recursive approximation of a bezier curve by polygonal segments, recursion start */
	static void curveToPathCoord(
		MapCoordF c0,
		MapCoordF c1,
		MapCoordF c2,
		MapCoordF c3,
		int coord_index,
		float max_error,
		float max_segment_len,
		PathCoordVector* path_coords
	);
};

#endif
