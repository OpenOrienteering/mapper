/*
 *    Copyright 2015 Kai Pastor
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


#ifndef OPENORIENTEERING_VIRTUAL_COORD_VECTOR_H
#define OPENORIENTEERING_VIRTUAL_COORD_VECTOR_H

#include <QtGlobal>

#include "map_coord.h"

namespace OpenOrienteering {

/**
 * @brief The VirtualFlagsVector class provides read-only access to a MapCoordVector.
 * 
 * This is an utility class which is used as a public member in VirtualCoordVector.
 * Its public API provides read-only STL-style access to a map coordinates vector.
 */
class VirtualFlagsVector
{
	friend class VirtualCoordVector;
	
private:
	// The pointer to the vector coordinate of coordinates. Never nullptr.
	const MapCoordVector* coords;
	
	// To be used by VirtualCoordVector only
	VirtualFlagsVector(const VirtualFlagsVector&) = default;
	
	// To be used by VirtualCoordVector only
	VirtualFlagsVector& operator=(const VirtualFlagsVector&) = default;
	
public:
	/**
	 * A reaonably sized unsigned integer type for map coord vector sizes and indexes.
	 */
	using size_type = quint32;
	
	/**
	 * Constructs a new accessor for the given vector of MapCoord.
	 */
	explicit VirtualFlagsVector(const MapCoordVector& coords);
	
	
	/**
	 * Returns a direct reference to the underlying coordinates.
	 */
	const MapCoordVector& data() const;
	
	
	// STL-style API (incomplete)
	
	bool empty() const;
	
	size_type size() const;
	
	MapCoordVector::const_reference operator[](size_type index) const;
	
	MapCoordVector::const_reference back() const;
};



/**
 * @brief The VirtualCoordVector class provides read-only access to possible distinct coordinates and flags.
 * 
 * Some algorithms in Mapper need to be applied not only to original integer coordinate lists,
 * but sometimes also to transformed floating-point coordinates sharing the same flags.
 * This class provides uniform access to coordinates and flags in two cases:
 * 
 * (a) Coordinates and flags coming from the same MapCoordVector, and
 * 
 * (b) Coordinates coming from a MapCoordVectorF, and flags from a MapCoordVector.
 */
class VirtualCoordVector
{
public:
	/**
	 * A reaonably sized unsigned integer type for map coord vector sizes and indexes.
	 */
	using size_type = VirtualFlagsVector::size_type;
	
	/**
	 * An read-only accessor to the flags which provides an operator[](size_type).
	 */
	// Provides the coordinates in case (a).
	VirtualFlagsVector flags;
	
private:
	// A pointer to the MapCoordVectorF which provides the coordinates in case (b).
	const MapCoordVectorF* coords;
	
	// A pointer to the actual coordinate access implementation
	MapCoordF (VirtualCoordVector::*coords_access)(size_type index) const;
	
public:
	/**
	 * Creates another accessor for the data managed by the given prototype.
	 */
	VirtualCoordVector(const VirtualCoordVector& prototype) = default;
	
	/**
	 * Creates an accessor for the flags and coordinates in coords (case a).
	 */
	explicit VirtualCoordVector(const MapCoordVector& coords);
	
	/**
	 * Creates an accessor for the given flags and the coordinates in coords (case b).
	 */
	explicit VirtualCoordVector(const MapCoordVector& flags, const MapCoordVectorF& coords);
	
	
	/**
	 * Causes this accessor to serve the same data as rhs.
	 */
	VirtualCoordVector& operator=(const VirtualCoordVector& rhs) = default;
	
	
	// STL-style API (incomplete, and with some differences, such as returning by value)
	
	bool empty() const;
	
	size_type size() const;
	
	MapCoordF operator[](size_type index) const;
	
	MapCoordF back() const;
	
private:
	// Returns a coordinate from coords vector.
	MapCoordF fromMapCoordF(size_type index) const;
	
	// Returns a coordinate from the flags accessors.
	MapCoordF fromMapCoord(size_type index) const;
};



// ### VirtualFlagsVector inline code ###

inline
VirtualFlagsVector::VirtualFlagsVector(const MapCoordVector& coords)
 : coords { &coords }
{
	// nothing else
}

inline
const MapCoordVector& VirtualFlagsVector::data() const
{
	return *coords;
}

inline
bool VirtualFlagsVector::empty() const
{
	return coords->empty();
}

inline
VirtualFlagsVector::size_type VirtualFlagsVector::size() const
{
	return coords->size();
}

inline
MapCoordVector::const_reference VirtualFlagsVector::operator[](size_type index) const
{
	return (*coords)[index];
}

inline
MapCoordVector::const_reference VirtualFlagsVector::back() const
{
	return coords->back();
}



// ### VirtualCoordVector inline code ###

inline
VirtualCoordVector::VirtualCoordVector(const MapCoordVector& coords)
 : flags { coords }
 , coords { nullptr }
 , coords_access(&VirtualCoordVector::fromMapCoord)
{
	// nothing else
}

inline
VirtualCoordVector::VirtualCoordVector(const MapCoordVector& flags, const MapCoordVectorF& coords)
 : flags { flags }
 , coords { &coords }
 , coords_access(&VirtualCoordVector::fromMapCoordF)
{
	// nothing else
}

inline
bool VirtualCoordVector::empty() const
{
	return flags.empty();
}

inline
VirtualCoordVector::size_type VirtualCoordVector::size() const
{
	return flags.size();
}

inline
MapCoordF VirtualCoordVector::operator[](size_type index) const
{
	return (this->*coords_access)(index);
}

inline
MapCoordF VirtualCoordVector::back() const
{
	return (this->*coords_access)(size()-1);
}


}  // namespace OpenOrienteering

#endif
