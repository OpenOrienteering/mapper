/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2015 Kai Pastor
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


#ifndef OPENORIENTEERING_VIRTUAL_PATH_H
#define OPENORIENTEERING_VIRTUAL_PATH_H

#include <memory>
#include <utility>
#include <vector>

#include <QRectF>

#include "core/map_coord.h"
#include "core/path_coord.h"
#include "core/virtual_coord_vector.h"

// IWYU pragma: no_forward_declare QRectF

namespace OpenOrienteering {

class PathCoordVector : public std::vector<PathCoord>
{
private:
	friend class SplitPathCoord;
	friend class VirtualPath;
	
	VirtualCoordVector virtual_coords;
	
public:
	PathCoordVector(const MapCoordVector& coords);
	
	PathCoordVector(const MapCoordVector& flags, const MapCoordVectorF& coords);
	
	PathCoordVector(const VirtualCoordVector& coords);
	
private:
	PathCoordVector(const PathCoordVector&) = default;
	
	PathCoordVector(PathCoordVector&&) = default;
	
	
	PathCoordVector& operator=(const PathCoordVector&) = default;
	
	PathCoordVector& operator=(PathCoordVector&&) = default;
	
	
	const VirtualFlagsVector& flags() const;
	
	const VirtualCoordVector& coords() const;
	
	bool isClosed() const;
	
	
public:
	/**
	 * Updates the path coords from the flags/coords, starting at first.
	 * 
	 * \return The index after the last element of this part.
	 */
	VirtualCoordVector::size_type update(VirtualCoordVector::size_type first);
	
	
	/**
	 * Finds the index of the next dash point after first, or returns size()-1.
	 * 
	 * \todo Consider return a SplitPathCoord (cf. actual usage).
	 */
	size_type findNextDashPoint(PathCoordVector::size_type first) const;
	
	
	/**
	 * Finds the path coordinate index with or just before the given length.
	 */
	size_type lowerBound(
	    PathCoord::length_type length,
	    size_type first,
	    size_type last
	) const;
	
	/**
	 * Finds the path coordinate index with or just after the given length.
	 */
	size_type upperBound(
	    PathCoord::length_type length,
	    size_type first,
	    size_type last
	) const;
	
	
	/**
	 * Returns the length of the path.
	 */
	PathCoord::length_type length() const;
	
	/**
	 * Calculates the area of this part.
	 */
	double calculateArea() const;
	
	QRectF calculateExtent() const;
	
	bool intersectsBox(const QRectF& box) const;
	
	bool isPointInside(const MapCoordF& coord) const;
	
private:
	/**
	 * Recursive approximation of a bezier curve by polygonal segments.
	 */
	void curveToPathCoord(
		MapCoordF c0,
		MapCoordF c1,
		MapCoordF c2,
		MapCoordF c3,
		MapCoordVector::size_type edge_start,
		float p0,
		float p1
	);
};



/**
 * A VirtualPath class represents a single path out of a sequence of coords and flags.
 * 
 * It provides a PathCoordVector which is is a polyline approximation of the path
 * (i.e. no curves) and provides metadata such as length for each point of the path.
 */
class VirtualPath
{
public:
	/** A reaonably sized unsigned integer type for coord vector sizes and indexes. */
	using size_type = VirtualCoordVector::size_type;
	
	/** The underlying coordinates and flags. */
	VirtualCoordVector coords;
	
	/** The derived flattened coordinates and meta data. */
	PathCoordVector path_coords;
	
	/** Index of first coordinate of this path in the coords. */
	size_type first_index;
	
	/** Index of the last coordinate of this part in the coords. */
	size_type last_index;
	
	
	explicit VirtualPath(const MapCoordVector& coords);
	
	VirtualPath(const MapCoordVector& coords, size_type first, size_type last);
	
	explicit VirtualPath(const VirtualCoordVector& coords);
	
	VirtualPath(const VirtualCoordVector& coords, size_type first, size_type last);
	
	VirtualPath(const MapCoordVector& flags, const MapCoordVectorF& coords);
	
	VirtualPath(const MapCoordVector& flags, const MapCoordVectorF& coords, size_type first, size_type last);
	
	VirtualPath(const VirtualPath&) = default;
	
	VirtualPath(VirtualPath&&) = default;
	
protected:
	VirtualPath& operator=(const VirtualPath&) = default;
	
	VirtualPath& operator=(VirtualPath&&) = default;
	
public:
	/**
	 * Returns true if there are no nodes in this path.
	 */
	bool empty() const;
	
	/**
	 * Returns the number of coordinates in this path.
	 * 
	 * \see countRegularNodes()
	 */
	size_type size() const;
	
	/** 
	 * Calculates the number of regular nodes in this path.
	 * 
	 * Regular nodes exclude close points and curve handles.
	 */
	size_type countRegularNodes() const;
	
	
	/**
	 * Returns true if the path is closed.
	 * 
	 * For closed paths, the last coordinate is at the same position as the
	 * first coordinate of the path,and has the "close point" flag set.
	 * These coords will move together when moved by the user, appearing as
	 * just one coordinate.
	 * 
	 * Parts of PathObjects can be closed and opened with PathPart::setClosed()
	 * or PathPart::connectEnds().
	 * 
	 * Objects with area symbols must always be closed.
	 */
	bool isClosed() const;
	
	
	/**
	 * Returns the length of the path.
	 */
	PathCoord::length_type length() const;
	
	/**
	 * Calculates the area of this part.
	 */
	double calculateArea() const;
	
	QRectF calculateExtent() const;
	
	bool intersectsBox(const QRectF& box) const;
	
	bool isPointInside(const MapCoordF& coord) const;
	
	PathCoord findClosestPointTo(
	        MapCoordF coord,
	        float& distance_squared,
	        float distance_bound_squared,
	        size_type start_index,
	        size_type end_index) const;
	
	
	/**
	 * Determines the index of the previous regular coordinate.
	 * 
	 * Regular coordinates exclude bezier control points and close points.
	 * 
	 * @param base_index The index of a regular coordinate from which to start.
	 */
	size_type prevCoordIndex(size_type base_index) const;
	
	/**
	 * Determines the index of the next regular coordinate.
	 * 
	 * Regular coordinates exclude bezier control points and close points.
	 * 
	 * @param base_index The index of a regular coordinate from which to start.
	 */
	size_type nextCoordIndex(size_type base_index) const;
	
	
	MapCoordF calculateTangent(size_type i) const;
	
	std::pair<MapCoordF, double> calculateTangentScaling(size_type i) const;
	
	/**
	 * Calculates the path tangent at the given MapCoord index.
	 * 
	 * Takes care of cases where successive points are at equal positions.
	 * 
	 * @param i Index of the coordinate where to query the tangent.
	 * @param backward If false, returns the forward tangent, if true, returns
	 *     the backward tangent. Makes a difference at sharp corners.
	 * @param ok Is set to true if the tangent can be found correctly, false if
	 *     failing to find the tangent.
	 */
	MapCoordF calculateTangent(
		size_type i,
		bool backward,
		bool& ok
	) const;
	
	/**
	 * Similar to calculateTangent().
	 */
	MapCoordF calculateIncomingTangent(
		size_type i,
		bool& ok
	) const;
	
	/**
	 * Similar to calculateTangent().
	 */
	MapCoordF calculateOutgoingTangent(
		size_type i,
		bool& ok
	) const;
	
	
	void copy(
	    const SplitPathCoord& first,
	    const SplitPathCoord& last,
	    MapCoordVector& out_coords
	) const;
	
	void copy(
	    const SplitPathCoord& first,
	    const SplitPathCoord& last,
	    MapCoordVector& out_flags,
	    MapCoordVectorF& out_coords
	) const;
	
	
	void copyLengths(
	    const SplitPathCoord& first,
	    const SplitPathCoord& last,
	    std::vector<PathCoord::length_type>& out_lengths
	) const;
	
};



// ### PathCoordVector inline code ###

inline
const VirtualFlagsVector& PathCoordVector::flags() const
{
	return virtual_coords.flags;
}

inline
const VirtualCoordVector& PathCoordVector::coords() const
{
	return virtual_coords;
}

inline
PathCoordVector::size_type PathCoordVector::lowerBound(
	PathCoord::length_type length,
    PathCoordVector::size_type first,
    PathCoordVector::size_type last ) const
{
	auto index = first;
	while (index != last)
	{
		++index;
		if ((*this)[index].clen < length)
		{
			--index;
			break;
		}
	}
	return index;
}

inline
PathCoordVector::size_type PathCoordVector::upperBound(
    PathCoord::length_type length,
    PathCoordVector::size_type first,
    PathCoordVector::size_type last ) const
{
	auto index = first;
	while (index != last && (*this)[index].clen < length)
	{
		++index;
	}
	return index;
}



// ### VirtualPath inline code ###

inline
bool VirtualPath::empty() const
{
	return last_index+1 == first_index;
}

inline
VirtualPath::size_type VirtualPath::size() const
{
	return last_index - first_index + 1;
}

inline
PathCoord::length_type VirtualPath::length() const
{
	return path_coords.length();
}

inline
double VirtualPath::calculateArea() const
{
	return path_coords.calculateArea();
}

inline
QRectF VirtualPath::calculateExtent() const
{
	return path_coords.calculateExtent();
}

inline
bool VirtualPath::intersectsBox(const QRectF& box) const
{
	return path_coords.intersectsBox(box);
}

inline
bool VirtualPath::isPointInside(const MapCoordF& coord) const
{
	return path_coords.isPointInside(coord);
}


}  // namespace OpenOrienteering

#endif
