/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2017 Kai Pastor
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


#ifndef OPENORIENTEERING_PATH_COORD_H
#define OPENORIENTEERING_PATH_COORD_H

#include <vector>

#include <QtGlobal>

#include "map_coord.h"

namespace OpenOrienteering {

class PathCoordVector;


/**
 * A PathCoord represents a node in a polygonal approximation of a path.
 * 
 * Complex paths which may consist of straight edges and curves are processed
 * into PathCoordVectors, approximating the path with straight edges only.
 * 
 * Apart from a point on this polygonal path, a PathCoord contains additional
 * information about that point:
 * - the index of the original point where the current edge started,
 * - the relative position on this edge, and
 * - the length since the start of the path.
 */
class PathCoord
{
	friend class PathCoordVector;
	
public:
	/** A reaonably sized unsigned integer type for map coord vector sizes and indexes. */
	using size_type = quint32;
	
	/** A reaonably precise float type for lengths and distances. */
	using length_type = float;
	
	/** A reaonably precise float type for relative position in the range [0, 1). */
	using param_type = float;
	
	
	/** Position. */
	MapCoordF pos;
	
	/** MapCoordVector(F) index of the start of the edge which this position belongs to. */
	size_type index;
	
	/** Relative location of this position on the MapCoordVector edge ([0.0, 1.0)). */
	param_type param;
	
	/** Cumulative length of the path since the start of the current part. */
	length_type clen;
	
	
	/** Default contructor. */
	constexpr PathCoord() noexcept;
	
	/** Copy constructor. */
	constexpr PathCoord(const PathCoord&) noexcept = default;
	
	/** Move constructor. */
	PathCoord(PathCoord&&) noexcept = default;
	
	/** Explicit construction with all member values. */
	constexpr PathCoord(const MapCoordF& pos, size_type index, param_type param, length_type clen) noexcept;
	
	
	/** Assignment operator. */
	PathCoord& operator=(const PathCoord&) noexcept = default;
	
	/** Move assignment operator. */
	PathCoord& operator=(PathCoord&&) noexcept = default;
	
	
	/**
	 * Global position error threshold for approximating bezier curves with straight segments.
	 * 
	 * @todo Make bezier error configurable
	 */
	static length_type bezierError();
	
	
	/**
	 * Returns true if the PathCoord's index is lower than value.
	 * 
	 * This function can be used for doing a binary search on a sorted container
	 * of PathCoords.
	 * 
	 * @see std::lower_bound
	 */
	static bool indexLessThanValue(const PathCoord& coord, size_type value);
	
	/**
	 * Returns true if the value is lower than the PathCoord's index.
	 * 
	 * This function can be used for doing a binary search on a sorted container
	 * of PathCoords.
	 * 
	 * @see std::upper_bound
	 */
	static bool valueLessThanIndex(size_type value, const PathCoord& coord);
	
	
	/**
	 * Splits a cubic bezier curve.
	 * 
	 * The curve made up by the points c0 ... c3 is split up at the relative
	 * position p (0..1). The new intermediate points (between c0 and c3) are
	 * returned in o0 ... o4.
	 * 
	 * If not all returned values are needed, it is possible to have a subset of
	 * o0..o4 point to the same memory.
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
	
	
	/**
	 * The minimum required (squared) distance of neighboring nodes which are to
	 * be considered for determining path tangents.
	 */
	static constexpr qreal tangentEpsilonSquared();
};



/**
 * An arbitrary position on a path.
 * 
 * A SplitPathCoord supports processing paths in connection with PathCoordVector.
 * It can represent an arbitrary position even between the elements of the
 * PathCoordVector. Other than PathCoord, it keeps a reference to the
 * PathCoordVector. It captures additional state such as adjusted curve parameters.
 * 
 * \see PathCoord
 */
class SplitPathCoord
{
public:
	/** A reaonably precise float type for lengths and distances. */
	using length_type = PathCoord::length_type;
	
	/** Position. */
	MapCoordF pos;
	
	/** Index of the cooresponding or preceding map coordinate and flags. */
	PathCoord::size_type index;
	
	/** Relative location of this position on the MapCoordVector edge ([0.0, 1.0)). */
	float param;
	
	/** Cumulative length of the path since the start of the current part. */
	length_type clen;
	
	/** The underlying vector path coordinates. */
	const PathCoordVector* path_coords;
	
	/** Index of the corresponding or preceding path_coord in a vector. */
	std::vector<PathCoord>::size_type path_coord_index;
	
	/** If true, a bezier edge ends at this split position. */
	bool is_curve_end;
	
	/** If true, a bezier edge starts at this split position. */
	bool is_curve_start;
	
	/** If a bezier edge ends here, this will hold the last control points.
	 * 
	 * Otherwise, curve_end[1] will hold the preceding coordinate,
	 * or the current coordinate for the start of open paths.
	 */
	MapCoordF curve_end[2];
	
	/** If a bezier edge starts here, this will hold the next control points.
	 * 
	 * Otherwise, curve_start[0] will hold the next coordinate,
	 * or the current coordinate for the end of open paths
	 */
	MapCoordF curve_start[2];
	
	
	/**
	 * Returns a vector which is a tangent to the path at this position.
	 */
	MapCoordF tangentVector() const;
	
	
	/**
	 * Returns a SplitPathCoord at the begin of the given path.
	 */
	static SplitPathCoord begin(const PathCoordVector& path_coords);
	
	/**
	 * Returns a SplitPathCoord at the end of the given path.
	 */
	static SplitPathCoord end(const PathCoordVector& path_coords);
	
	/**
	 * Returns a SplitPathCoord at the given PathCoordVector index.
	 */
	static SplitPathCoord at(
		const PathCoordVector& path_coords,
		std::vector<PathCoord>::size_type path_coord_index
	);
	
	/**
	 * Returns a SplitPathCoord at the given length.
	 * 
	 * The search for the position will start at first.
	 */
	static SplitPathCoord at(
		length_type length,
		const SplitPathCoord& first
	);
	
	/**
	 * Returns a SplitPathCoord at the given length.
	 */
	static SplitPathCoord at(
		const PathCoordVector& path_coords,
		length_type length
	);
};



// ### PathCoord inline code ###

constexpr PathCoord::PathCoord() noexcept
 : PathCoord( {}, 0, 0.0, 0.0)
{
	// Nothing else
}

constexpr PathCoord::PathCoord(const MapCoordF& pos, size_type index, param_type param, PathCoord::length_type clen) noexcept
 : pos { pos }
 , index { index }
 , param { param }
 , clen { clen }
{
	// Nothing else
}

constexpr qreal PathCoord::tangentEpsilonSquared()
{
	return 0.000625; // App. 0.025 mm distance
}



// ### SplitPathCoord inline code ###

// static
inline
SplitPathCoord SplitPathCoord::at(const PathCoordVector& path_coords, SplitPathCoord::length_type length)
{
	return at(length, begin(path_coords));
}


}  // namespace OpenOrienteering

#endif
