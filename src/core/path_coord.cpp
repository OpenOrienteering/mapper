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


#include "path_coord.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>

#include "core/map_coord.h"
#include "core/virtual_coord_vector.h"
#include "core/virtual_path.h"


namespace OpenOrienteering {

static_assert(std::is_nothrow_default_constructible<PathCoord>::value,
              "PathCoord must be nothrow default constructible.");
static_assert(std::is_nothrow_copy_constructible<PathCoord>::value,
              "PathCoord must be nothrow copy constructible.");
static_assert(std::is_nothrow_move_constructible<PathCoord>::value,
              "PathCoord must be nothrow move constructible.");
static_assert(std::is_nothrow_copy_assignable<PathCoord>::value,
              "PathCoord must be nothrow copy assignable.");
static_assert(std::is_nothrow_move_assignable<PathCoord>::value,
              "PathCoord must be nothrow move assignable.");
static_assert(std::is_nothrow_destructible<PathCoord>::value,
              "PathCoord must be nothrow destructible.");


// ### PathCoord ###

void PathCoord::splitBezierCurve(MapCoordF c0, MapCoordF c1, MapCoordF c2, MapCoordF c3, float p, MapCoordF& o0, MapCoordF& o1, MapCoordF& o2, MapCoordF& o3, MapCoordF& o4)
{
	if (p >= 1.0)
	{
		o0 = c1;
		o1 = c2;
		o2 = c3;
		o3 = c3;
		o4 = c3;
	}
	else if (p <= 0.0)
	{
		o0 = c0;
		o1 = c0;
		o2 = c0;
		o3 = c1;
		o4 = c2;
	}
	else
	{
		// The output variables may point to the same storage (if unused), so we 
		// must not access any output identifier after another one was assigned to.
		auto c12 = c1 + (c2 - c1) * p;
		o0 = c0 + (c1 - c0) * p;
		auto tmp_o1 = o0 + (c12 - o0) * p;
		o4 = c2 + (c3 - c2) * p;
		o3 = c12 + (o4 - c12) * p;
		o2 = tmp_o1 + (o3 - tmp_o1) * p;
		o1 = tmp_o1;
	}
}



// ### SplitPathCoord ###

MapCoordF SplitPathCoord::tangentVector() const
{
	auto& path_coords = *this->path_coords;
	auto& flags = path_coords.flags();
	auto& coords = path_coords.coords();
	auto path_closed = path_coords.isClosed();
	
	std::size_t index = path_coord_index;
	auto last_index = path_coords.size() - 1;
	
	/// Check distances, advance to a significant length, handle closed paths.
	MapCoordF next = curve_start[0];
	if (pos.distanceSquaredTo(next) >= PathCoord::tangentEpsilonSquared())
		goto next_found;

	if (is_curve_start)
	{
		next = curve_start[1];
		if (pos.distanceSquaredTo(next) >= PathCoord::tangentEpsilonSquared())
			goto next_found;
	}
	
	// Search along current edge
	if (index < last_index && param != 0.0f)
	{
		do
		{
			++index;
			next = path_coords[index].pos;
			if (pos.distanceSquaredTo(next) >= PathCoord::tangentEpsilonSquared())
				goto next_found;
		}
		while (path_coords[index].param != 0.0f);
	}
		
	// Attention, switching from PathCoordVector index to MapCoordVectorF index.
	index = path_coords[index].index;
	last_index = coords.size() - 1;
	
	// Search along control points
	while (index < last_index)
	{
		++index;
		next = coords[index];
		if (pos.distanceSquaredTo(next) >= PathCoord::tangentEpsilonSquared())
			goto next_found;
	}
	
	if (path_closed && path_coord_index > 0)
	{
		index = 0;
		last_index = path_coords[path_coord_index].index;
		while (index < last_index)
		{
			++index;
			next = coords[index];
			if (pos.distanceSquaredTo(next) >= PathCoord::tangentEpsilonSquared())
				goto next_found;
		}
		
		if (flags[index].isCurveStart())
		{
			next = coords[index+1];
			if (pos.distanceSquaredTo(next) >= PathCoord::tangentEpsilonSquared())
				goto next_found;
		}
		
		// Search along curve
		Q_ASSERT(index == path_coords[path_coord_index].index);
		
		// Attention, switching from MapCoordVectorF index to PathCoordVector index.
		auto pc = std::lower_bound(std::begin(path_coords), std::begin(path_coords)+path_coord_index, index, PathCoord::indexLessThanValue);
		index = std::distance(std::begin(path_coords), pc);
		last_index = path_coord_index;
		while (index < last_index)
		{
			if (pos.distanceSquaredTo(next) >= PathCoord::tangentEpsilonSquared())
				goto next_found;
			
			++index;
			next = path_coords[index].pos;
		}
	}
	
	// No coordinate is distant enough, so reset next.
	next = pos; 
	
next_found:
	next -= pos;
	next.normalize();
	
	
	MapCoordF prev = curve_end[1];
	if (pos.distanceSquaredTo(prev) >= PathCoord::tangentEpsilonSquared())
		goto prev_found;
	
	if (is_curve_end)
	{
		prev = curve_end[0];
		if (pos.distanceSquaredTo(prev) >= PathCoord::tangentEpsilonSquared())
			goto prev_found;
	}
	
	index = path_coord_index;
	
	// Search along curve
	if (param != 0.0f)
	{
		while (path_coords[index].param != 0.0f)
		{
			--index;
			prev = path_coords[index].pos;
			if (pos.distanceSquaredTo(prev) >= PathCoord::tangentEpsilonSquared())
				goto prev_found;
		}
	}
	
	// Attention, switching from PathCoordVector index to MapCoordVectorF index.
	index = path_coords[index].index;
	
	// Search along control points
	while (index > 0)
	{
		--index;
		prev = coords[index];
		if (pos.distanceSquaredTo(prev) >= PathCoord::tangentEpsilonSquared())
			goto prev_found;
	}
	
	last_index = path_coords.size() - 1;
	if (path_closed && path_coord_index != last_index)
	{
		index = path_coords[last_index].index;
		last_index = path_coords[path_coord_index].index + (flags[path_coord_index].isCurveStart() ? 3 : 1);
		while (index > last_index)
		{
			--index;
			prev = coords[index];
			if (pos.distanceSquaredTo(prev) >= PathCoord::tangentEpsilonSquared())
				goto prev_found;
		}
		
		// Search along curve
		Q_ASSERT(index > path_coords[path_coord_index].index);
		// Attention, switching from MapCoordVectorF index to PathCoordVector index.
		auto pc = std::upper_bound(std::begin(path_coords)+path_coord_index, std::end(path_coords)-1, index, PathCoord::valueLessThanIndex);
		index = std::distance(std::begin(path_coords), pc);
		last_index = path_coord_index + 1;
		while (index > last_index)
		{
			--index;
			prev = path_coords[index].pos;
			if (pos.distanceSquaredTo(prev) >= PathCoord::tangentEpsilonSquared())
				goto prev_found;
		}
	}
	prev = pos;
	
prev_found:
	prev -= pos;
	prev.normalize();
	
	next -= prev;
	return next;
}

// static
SplitPathCoord SplitPathCoord::begin(const PathCoordVector& path_coords)
{
	Q_ASSERT(!path_coords.empty());
	
	auto& flags  = path_coords.flags();
	auto& coords = path_coords.coords();
	Q_ASSERT(coords.size() == flags.size());
	
	auto first_index = path_coords.front().index;
	auto last_index  = path_coords.back().index;
	Q_ASSERT(last_index > first_index);
	
	SplitPathCoord split = { coords[first_index], first_index, 0.0f, path_coords.front().clen, &path_coords, 0, false, flags[first_index].isCurveStart(), {}, {} };
	if (flags[last_index].isClosePoint())
	{
		split.curve_end[1] = coords[last_index-1];
		if (last_index - first_index > 2 && flags[last_index - 3].isCurveStart())
		{
			split.is_curve_end = true;
			split.curve_end[0] = coords[last_index-2];
		}
	}
	else
	{
		split.curve_end[1] = split.pos;
	}
	
	split.curve_start[0] = coords[first_index+1];
	if (split.is_curve_start)
	{
		Q_ASSERT(first_index+2 <= last_index);
		split.curve_start[1] = coords[first_index+2];
	}
	
	return split;
}

// static
SplitPathCoord SplitPathCoord::end(const PathCoordVector& path_coords)
{
	Q_ASSERT(!path_coords.empty());
	
	auto& flags  = path_coords.flags();
	auto& coords = path_coords.coords();
	Q_ASSERT(coords.size() == flags.size());
	
	auto first_index = path_coords.front().index;
	auto last_index  = path_coords.back().index;
	Q_ASSERT(last_index > first_index);
	
	SplitPathCoord split = { coords[last_index], last_index, 0.0f, path_coords.back().clen, &path_coords, path_coords.size() - 1, false, false, {}, {} };
	if (flags[last_index].isClosePoint())
	{
		split.curve_start[0] = coords[first_index+1];
		if (flags[first_index].isCurveStart())
		{
			split.is_curve_start = true;
			split.curve_start[1] = coords[first_index+2];
		}
	}
	else
	{
		split.curve_start[0] = split.pos;
	}
	
	split.curve_end[1] = coords[last_index-1];
	if (last_index - first_index > 2 && flags[last_index-3].isCurveStart())
	{
		split.is_curve_end = true;
		split.curve_end[0] = coords[last_index-2];
	}
	
	return split;
}

// static
SplitPathCoord SplitPathCoord::at(
    const PathCoordVector& path_coords,
    std::vector<PathCoord>::size_type path_coord_index )
{
	Q_ASSERT(path_coord_index < path_coords.size());
	
	auto& flags  = path_coords.flags();
	auto& coords = path_coords.coords();
	Q_ASSERT(coords.size() > 1);
	Q_ASSERT(coords.size() == flags.size());
	
	auto index = path_coords[path_coord_index].index;
	SplitPathCoord split = { coords[index], index, path_coords[path_coord_index].param, path_coords[path_coord_index].clen, &path_coords, path_coord_index, false, flags[index].isCurveStart(), {}, {} };
	
	if (index+1 < coords.size())
	{
		split.curve_start[0] = coords[index+1];
		if (split.is_curve_start && index+2 < coords.size())
		{
			split.curve_start[1] = coords[index+2];
		}
		else
		{
			Q_ASSERT(!split.is_curve_start);
		}
	}
	else
	{
		Q_ASSERT(!split.is_curve_start);
		split.curve_start[0] = split.pos;
	}
	
	if (index >= 1)
	{
		split.curve_end[1] = coords[index-1];
		if (index >= 3 && flags[index-3].isCurveStart())
		{
			split.curve_end[0] = coords[index-2];
			split.is_curve_end = true;
		}
	}
	else
	{
		split.curve_end[1] = split.pos;
	}
	
	return split;
}

// static
SplitPathCoord SplitPathCoord::at(
        length_type length,
        const SplitPathCoord& first )
{
	const auto& path_coords = *first.path_coords;
	const auto& coords = path_coords.coords();
	const auto& flags  = path_coords.flags();
	
	SplitPathCoord split = first;
	if (length > first.clen)
	{
		split.path_coord_index = path_coords.upperBound(length, first.path_coord_index, first.path_coords->size()-1);
	}
	if (split.path_coord_index > first.path_coord_index)
	{
		// New path coordinate, really
		split.clen = length;
		
		const auto& current_coord = path_coords[split.path_coord_index];
		const auto& prev_coord    = path_coords[split.path_coord_index-1];
		const auto curve_length   = current_coord.clen - prev_coord.clen;
		
		split.is_curve_end   = flags[prev_coord.index].isCurveStart();
		split.is_curve_start = split.is_curve_end;
		
		auto factor = 1.0f;
		if (qFuzzyCompare(1.0f + length, 1.0f + current_coord.clen) ||
		    qFuzzyCompare(1.0f + curve_length, 1.0f) ||
		    length > current_coord.clen)
		{
			// Close match at current path coordinate,
			// or near-zero curve length,
			// or length exceeding path length.
			split.pos   = current_coord.pos;
			split.clen  = current_coord.clen;
			split.param = current_coord.param;
		}
		else
		{
			// Split between path coordinates.
			factor = qBound(0.0f, (length - prev_coord.clen) / curve_length, 1.0f);
			auto prev_param    = prev_coord.param;
			auto current_param = current_coord.param;
			if (current_param == 0.0f)
				current_param = 1.0f;
			split.param = prev_param + (current_param - prev_param) * factor;
			Q_ASSERT(split.param >= 0.0f && split.param <= 1.0f);
			if (split.param == 1.0f)
				split.param = 0.0f;
		}
		
		if (!split.is_curve_end)
		{
			// Straight
			split.pos = prev_coord.pos + qreal(factor) * (current_coord.pos - prev_coord.pos);
			
			if (current_coord.index > path_coords.front().index)
				split.curve_end[1] = coords[current_coord.index-1];
			// else
				// leave split.curve_end[1] as copied from first.
			
			split.curve_start[0] = current_coord.pos;
		}
		else if (split.param == 0.0f)
		{
			// At a node, after a curve
			split.pos          = current_coord.pos;
			
			if (prev_coord.index == first.index)
			{
				// Split in same curve as first
				split.curve_end[0] = first.curve_start[0];
				split.curve_end[1] = first.curve_start[1];
			}
			else
			{
				split.curve_end[0] = coords[prev_coord.index+1];
				split.curve_end[1] = coords[prev_coord.index+2];
			}
			
			// curve_start: handled later
		}
		else
		{
			// In curve
			Q_ASSERT(split.is_curve_start);
			Q_ASSERT(split.is_curve_end);
			
			auto edge_start = prev_coord.index;
			Q_ASSERT(edge_start+3 <= path_coords.back().index);
			
			if (prev_coord.index == first.index)
			{
				auto p = (split.param - first.param) / (1.0f - first.param);
				Q_ASSERT(p >= 0.0f);
				Q_ASSERT(p <= 1.0f);
				
				PathCoord::splitBezierCurve(first.pos, first.curve_start[0], first.curve_start[1], coords[edge_start+3],
				                            p,
				                            split.curve_end[0], split.curve_end[1], split.pos, split.curve_start[0], split.curve_start[1]);
			}
			else
			{
				PathCoord::splitBezierCurve(coords[edge_start], coords[edge_start+1], coords[edge_start+2], coords[edge_start+3],
				                            split.param,
				                            split.curve_end[0], split.curve_end[1], split.pos, split.curve_start[0], split.curve_start[1]);
			}
		}
		
		if (split.param == 0.0f)
		{
			// Handle curve_start for non-in-bezier splits.
			split.is_curve_start = flags[current_coord.index].isCurveStart();
			if (split.is_curve_start)
			{
				split.curve_start[0] = coords[current_coord.index+1];
				split.curve_start[1] = coords[current_coord.index+2];
			}
			else if (current_coord.index < path_coords.back().index)
			{
				split.curve_start[0] = coords[current_coord.index+1];
			}
			else
			{
				/// \todo Handle closed paths.
				split.curve_start[0] = current_coord.pos;
			}
		}
		else
		{
			--split.path_coord_index;
		}
		
		split.index = path_coords[split.path_coord_index].index;
	}
	
	return split;
}


// Not inline or constexpr, because it is meant to be used by function pointer.
bool PathCoord::indexLessThanValue(const PathCoord& coord, size_type value)
{
	return coord.index < value;
}

// Not inline or constexpr, because it is meant to be used by function pointer.
bool PathCoord::valueLessThanIndex(size_type value, const PathCoord& coord)
{
	return value < coord.index;
}


}  // namespace OpenOrienteering
