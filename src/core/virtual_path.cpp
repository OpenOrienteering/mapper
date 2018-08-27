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


#include "virtual_path.h"

#include "util/util.h"


namespace OpenOrienteering {

namespace
{

	/**
	 * Global position error threshold for approximating
	 * bezier curves with straight segments.
	 * TODO: make configurable
	 */
	const PathCoord::length_type bezier_error = 0.005;
	
	/**
	 * Global maximum length of generated PathCoord segments for curves.
	 * 
	 * This ensures that even for flat curves, segments will be generated regularly.
	 * This is important because while a curve may be flat, the mapping from the
	 * curve parameter to real position is not linear, which would result in problems.
	 * This is counteracted by generating many segments.
	 */
	const PathCoord::length_type bezier_segment_maxlen_squared = 1.0;
	
	
}  // namespace



// ### PathCoord ###

// static
PathCoord::length_type PathCoord::bezierError()
{
	return bezier_error;
}



// ### PathCoordVector ###

PathCoordVector::PathCoordVector(const MapCoordVector& coords)
    : virtual_coords(coords)
{
	// nothing else
}

PathCoordVector::PathCoordVector(const MapCoordVector& flags, const MapCoordVectorF& coords)
    : virtual_coords(flags, coords)
{
	// nothing else
}

PathCoordVector::PathCoordVector(const VirtualCoordVector& coords)
    : virtual_coords(coords)
{
	// nothing else
}

VirtualCoordVector::size_type PathCoordVector::update(VirtualCoordVector::size_type part_start)
{
	auto& flags = virtual_coords.flags;
	auto part_end = virtual_coords.size() - 1;
	if (part_start <= part_end)
	{	
		Q_ASSERT(virtual_coords.size() == flags.size());
		
		if (flags[part_start].isHolePoint())
		{
			auto pos = virtual_coords[0];
			qWarning("PathCoordVector at %g %g (mm) has an invalid hole at index %d.",
			         pos.x(), -pos.y(), part_start);
		}
		
		clear();
		if (empty() || (part_start > 0 && flags[part_start-1].isHolePoint()))
		{
			emplace_back(virtual_coords[part_start], part_start, 0.0, 0.0);
		}
		
		for (auto index = part_start + 1; index <= part_end; ++index)
		{
			if (flags[index-1].isCurveStart())
			{
				Q_ASSERT(index+2 <= part_end);
				
				// Add curve coordinates
				curveToPathCoord(virtual_coords[index-1], virtual_coords[index], virtual_coords[index+1], virtual_coords[index+2], index-1, 0, 1);
				index += 2;
			}
			
			// Add end point
			const PathCoord& prev = back();
			emplace_back(virtual_coords[index], index, 0.0, prev.clen + prev.pos.distanceTo(virtual_coords[index]));
			
			Q_ASSERT(back().index == index);
			
			if (index < part_end && flags[index].isHolePoint())
			{
				part_end = index;
			}
		}
	}
	return part_end;
}

bool PathCoordVector::isClosed() const
{
	return virtual_coords.flags[back().index].isClosePoint();
}

PathCoordVector::size_type PathCoordVector::findNextDashPoint(PathCoordVector::size_type first) const
{
	// Get behind the current point
	auto prev_index = (*this)[first].index;
	auto last = size() - 1;
	
	while (first != last && (*this)[first].index == prev_index)
	{
		++first;
	}
	
	// Find a dash point or hole point
	while (first != last)
	{
		const auto& current_flags = virtual_coords.flags[(*this)[first].index];
		if (current_flags.isDashPoint() || current_flags.isHolePoint())
			break;
		++first;
	}
	
	return first;
}

PathCoord::length_type PathCoordVector::length() const
{
	Q_ASSERT(!empty());
	return back().clen;
}

double PathCoordVector::calculateArea() const
{
	Q_ASSERT(!empty());
	
	auto area = 0.0;
	auto end_index = size() - 1;
	auto j = end_index;  // The last vertex is the 'previous' one to the first
	
	for (auto i = 0u; i <= end_index; ++i)
	{
		area += ((*this)[j].pos.x() + (*this)[i].pos.x()) * ((*this)[j].pos.y() - (*this)[i].pos.y()); 
		j = i;
	}
	return qAbs(area) / 2;
}

QRectF PathCoordVector::calculateExtent() const
{
	QRectF extent(0.0, 0.0, -1.0, 0.0);
	if (!empty())
	{
		auto pc = begin();
		extent = QRectF(pc->pos.x(), pc->pos.y(), 0.0001, 0.0001);
		
		auto last = end();
		for (++pc; pc != last; ++pc)
		{
			rectInclude(extent, pc->pos);
		}
		
		Q_ASSERT(extent.isValid());
	}
	return extent;
}

bool PathCoordVector::intersectsBox(const QRectF& box) const
{
	bool result = false;
	if (!empty())
	{
		auto last_pos = front().pos;
		result = std::any_of(begin()+1, end(), [&box, &last_pos](const PathCoord& pc)
		{
			auto pos = pc.pos;
			auto result = lineIntersectsRect(box, last_pos, pos); /// \todo Implement this here, used nowhere else
			last_pos = pos;
			return result;
		});
	}
	return result;
}

bool PathCoordVector::isPointInside(const MapCoordF& coord) const
{
	bool inside = false;
	if (size() > 2)
	{
		auto last_pos = back().pos;
		for(const auto& path_coord : *this)
		{
			auto pos = path_coord.pos;
			if ( ((pos.y() > coord.y()) != (last_pos.y() > coord.y())) &&
			     (coord.x() < (last_pos.x() - pos.x()) *
			      (coord.y() - pos.y()) / (last_pos.y() - pos.y()) + pos.x()) )
			{
				inside = !inside;
			}
			last_pos = pos;
		}
	}
	return inside;
}

void PathCoordVector::curveToPathCoord(
        MapCoordF c0,
        MapCoordF c1,
        MapCoordF c2,
        MapCoordF c3,
        MapCoordVector::size_type edge_start,
        float p0,
        float p1 )
{
	// Common
	auto p_half = (p0 + p1) * 0.5;
	MapCoordF c12((c1.x() + c2.x()) * 0.5, (c1.y() + c2.y()) * 0.5);
	
	auto inner_len_sq = c0.distanceSquaredTo(c3);
	auto outer_len    = [&]() { return c0.distanceTo(c1) + c1.distanceTo(c2) + c2.distanceTo(c3); };
	if (inner_len_sq <= bezier_segment_maxlen_squared && outer_len() - sqrt(inner_len_sq) <= bezier_error)
	{
		const PathCoord& prev = back();
		emplace_back(c12, edge_start, p_half, prev.clen + float(prev.pos.distanceTo(c12)));
	}
	else
	{
		// Split in two
		MapCoordF c01((c0.x() + c1.x()) * 0.5f, (c0.y() + c1.y()) * 0.5f);
		MapCoordF c23((c2.x() + c3.x()) * 0.5f, (c2.y() + c3.y()) * 0.5f);
		MapCoordF c012((c01.x() + c12.x()) * 0.5f, (c01.y() + c12.y()) * 0.5f);
		MapCoordF c123((c12.x() + c23.x()) * 0.5f, (c12.y() + c23.y()) * 0.5f);
		MapCoordF c0123((c012.x() + c123.x()) * 0.5f, (c012.y() + c123.y()) * 0.5f);
		
		curveToPathCoord(c0, c01, c012, c0123, edge_start, p0, p_half);
		curveToPathCoord(c0123, c123, c23, c3, edge_start, p_half, p1);
	}
}



//### VirtualPath ###

VirtualPath::VirtualPath(const MapCoordVector& coords)
 : VirtualPath(coords, 0, coords.size()-1)
{
	// nothing else
}

VirtualPath::VirtualPath(const MapCoordVector& coords, size_type first, size_type last)
 : coords(coords)
 , path_coords(coords)
 , first_index(first)
 , last_index(last)
{
	// nothing else
}

VirtualPath::VirtualPath(const VirtualCoordVector& coords)
 : VirtualPath(coords, 0, coords.size()-1)
{
	// nothing else
}

VirtualPath::VirtualPath(const VirtualCoordVector& coords, size_type first, size_type last)
 : coords(coords)
 , path_coords(coords)
 , first_index(first)
 , last_index(last)
{
	// nothing else
}

VirtualPath::VirtualPath(const MapCoordVector& flags, const MapCoordVectorF& coords)
 : VirtualPath(flags, coords, 0, coords.size()-1)
{
	// nothing else
}

VirtualPath::VirtualPath(const MapCoordVector& flags, const MapCoordVectorF& coords, size_type first, size_type last)
 : coords(flags, coords)
 , path_coords(flags, coords)
 , first_index(first)
 , last_index(last)
{
	// nothing else
}

VirtualPath::size_type VirtualPath::countRegularNodes() const
{
	size_type num_regular_points = 0;
	for (auto i = first_index; i <= last_index; ++i)
	{
		++num_regular_points;
		if (coords.flags[i].isCurveStart())
			i += 2;
	}
	
	if (num_regular_points && isClosed())
		--num_regular_points;
	
	return num_regular_points;
}

bool VirtualPath::isClosed() const
{
	Q_ASSERT(last_index < coords.size());
	Q_ASSERT(last_index >= first_index);
	return coords.flags[last_index].isClosePoint();
}

PathCoord VirtualPath::findClosestPointTo(
        MapCoordF coord,
        float& distance_squared,
        float distance_bound_squared,
        size_type start_index,
        size_type end_index ) const
{
	Q_ASSERT(!path_coords.empty());
	
	auto result = path_coords.front();
	
	// Find upper bound for distance.
	for (const auto& path_coord : path_coords)
	{
		if (path_coord.index > end_index)
			break;
		if (path_coord.index < start_index)
			continue;
		
		auto to_coord = coord - path_coord.pos;
		auto dist_sq = to_coord.lengthSquared();
		if (dist_sq < distance_bound_squared)
		{
			distance_bound_squared = dist_sq;
			result = path_coord;
		}
	}
	
	// Check between this coord and the next one.
	distance_squared = distance_bound_squared;
	auto last = end(path_coords)-1;
	for (auto pc = begin(path_coords); pc != last; ++pc)
	{
		if (pc->index > end_index)
			break;
		if (pc->index < start_index)
			continue;
		
		auto pos = pc->pos;
		auto next_pc = pc+1;
		auto next_pos = next_pc->pos;
		
		auto tangent = next_pos - pos;
		tangent.normalize();
		
		auto to_coord = coord - pos;
		float dist_along_line = MapCoordF::dotProduct(to_coord, tangent);
		if (dist_along_line <= 0)
		{
			if (to_coord.lengthSquared() < distance_squared)
			{
				distance_squared = to_coord.lengthSquared();
				result = *pc;
			}
			continue;
		}
		
		float line_length = next_pc->clen - pc->clen;
		if (dist_along_line >= line_length)
		{
			if (coord.distanceSquaredTo(next_pos) < distance_squared)
			{
				distance_squared = coord.distanceSquaredTo(next_pos);
				result = *next_pc;
			}
			continue;
		}
		
		auto right = tangent.perpRight();
		
		float dist_from_line = MapCoordF::dotProduct(right, to_coord);
		auto dist_from_line_sq = dist_from_line * dist_from_line;
		if (dist_from_line_sq < distance_squared)
		{
			distance_squared = dist_from_line_sq;
			result.clen = pc->clen + dist_along_line;
			result.index = pc->index;
			auto factor = dist_along_line / line_length;
			if (next_pc->index == pc->index)
				result.param = pc->param + (next_pc->param - pc->param) * factor;
			else
				result.param = pc->param + (1.0 - pc->param) * factor; /// \todo verify
			
			if (coords.flags[result.index].isCurveStart())
			{
				MapCoordF unused;
				PathCoord::splitBezierCurve(MapCoordF(coords.flags[result.index]), MapCoordF(coords.flags[result.index+1]),
											MapCoordF(coords.flags[result.index+2]), MapCoordF(coords.flags[result.index+3]),
											result.param, unused, unused, result.pos, unused, unused);
			}
			else
			{
				result.pos = pos + (next_pos - pos) * factor;
			}
		}
	}
	return result;
}

VirtualPath::size_type VirtualPath::prevCoordIndex(size_type base_index) const
{
	Q_ASSERT(base_index >= first_index);
	
	size_type result = first_index;
	if (Q_UNLIKELY(base_index > last_index))
	{
		result = last_index;
	}
	else if (base_index > first_index)
	{
		result = base_index - 1;
		for (auto i = 1; i <= 3; ++i)
		{
			auto alternative = base_index - i;
			if (alternative < first_index || alternative > last_index)
				break;
			
			if (coords.flags[alternative].isCurveStart())
				result = alternative;
		}
	}
	
	return result;
}

VirtualPath::size_type VirtualPath::nextCoordIndex(size_type base_index) const
{
	Q_ASSERT(base_index <= last_index);
	
	size_type result = base_index + 1;
	if (Q_UNLIKELY(result > last_index))
	{
		result = last_index;
	}
	
	if (Q_UNLIKELY(result <= first_index))
	{
		result = first_index;
	}
	else
	{
		for (auto i = 0; i < 2; ++i)
		{
			if (coords.flags[base_index].isCurveStart())
			{
				result = base_index + 3;
				break;
			}
			
			if (base_index == first_index)
				break;
			
			--base_index;
		}
	}
	
	return result;
}

MapCoordF VirtualPath::calculateTangent(size_type i) const
{
	bool ok_to_coord, ok_to_next;
	MapCoordF to_coord = calculateIncomingTangent(i, ok_to_coord);
	MapCoordF to_next = calculateOutgoingTangent(i, ok_to_next);
	
	if (!ok_to_next)
	{
		to_next = to_coord;
	}
	else if (ok_to_coord)
	{
		to_coord.normalize();
		to_next.normalize();
		to_next += to_coord;
	}
	
	return to_next;
}

std::pair<MapCoordF, double> VirtualPath::calculateTangentScaling(size_type i) const
{
	bool ok_to_coord, ok_to_next;
	MapCoordF to_coord = calculateIncomingTangent(i, ok_to_coord);
	MapCoordF to_next = calculateOutgoingTangent(i, ok_to_next);
	
	auto scaling = 1.0;
	if (!ok_to_next)
	{
		to_next = to_coord;
	}
	else if (ok_to_coord)
	{
		to_coord.normalize();
		to_next.normalize();
		if (to_next == -to_coord)
		{
			to_next = to_next.perpRight();
			scaling = std::numeric_limits<double>::infinity();
		}
		else
		{
			to_next += to_coord;
			to_next.normalize();
			scaling = 1.0/MapCoordF::dotProduct(to_next, to_coord);
		}
	}
	
	return std::make_pair(to_next, scaling);
}

MapCoordF VirtualPath::calculateTangent(
        size_type i,
        bool backward,
        bool& ok) const
{
	if (backward)
		return calculateIncomingTangent(i, ok);
	else
		return calculateOutgoingTangent(i, ok);
}

MapCoordF VirtualPath::calculateIncomingTangent(
        size_type i,
        bool& ok) const
{
	ok = true;
	MapCoordF tangent;
	
	for (auto k = i; k > first_index; ) /// \todo Implement unsigned
	{
		--k;
		tangent = coords[i] - coords[k];
		if (tangent.lengthSquared() > PathCoord::tangentEpsilonSquared())
			return tangent;
	}
	
	if (isClosed() && last_index > i+1)
	{
		// Continue from end
		for (auto k = last_index; k > i; --k)
		{
			tangent = coords[i] - coords[k];
			if (tangent.lengthSquared() > PathCoord::tangentEpsilonSquared())
				return tangent;
		}
	}
	
	ok = false;
	return tangent;
}

MapCoordF VirtualPath::calculateOutgoingTangent(
        size_type i,
        bool& ok) const
{
	ok = true;
	MapCoordF tangent;
	
	for (auto k = i+1; k <= last_index; ++k)
	{
		tangent = coords[k] - coords[i];
		if (tangent.lengthSquared() > PathCoord::tangentEpsilonSquared())
			return tangent;
	}
	
	if (isClosed())
	{
		// Continue from start
		for (auto k = first_index; k < i; ++k)
		{
			tangent = coords[k] - coords[i];
			if (tangent.lengthSquared() > PathCoord::tangentEpsilonSquared())
				return tangent;
		}
	}
	
	ok = false;
	return tangent;
}

void VirtualPath::copy(
        const SplitPathCoord& first,
        const SplitPathCoord& last,
        MapCoordVector& out_coords ) const
{
	Q_ASSERT(!empty());
	Q_ASSERT(first.path_coords == &path_coords);
	Q_ASSERT(last.path_coords == &path_coords);
	
	auto& flags  = coords.flags;
	
	// Handle first coordinate and its flags.
	if (out_coords.empty() ||
	    out_coords.back().isHolePoint() ||
	    !out_coords.back().isPositionEqualTo(MapCoord(first.pos)) )
	{
		out_coords.emplace_back(first.pos);
	}
	else
	{
		out_coords.back().setHolePoint(false);
		out_coords.back().setClosePoint(false);
	}
	
	if (first.index == last.index)
	{
		out_coords.back().setCurveStart(last.is_curve_end && last.param != first.param);
	}
	else
	{
		out_coords.back().setCurveStart(first.is_curve_start);
		
		auto stop_index = last.index;
		if (last.param == 0.0)
		{
			stop_index -= last.is_curve_end ? 3 : 1;
		}
		
		auto index = first.index + 1;
		if (first.is_curve_start)
		{
			out_coords.emplace_back(first.curve_start[0]);
			out_coords.emplace_back(first.curve_start[1]);
			index += 2;
		}
		
		for (; index <= stop_index; ++index)
		{
			out_coords.emplace_back(coords[index]);
			out_coords.back().setFlags(flags[index].flags());
		}
	}
		
	if (out_coords.back().isCurveStart())
	{
		Q_ASSERT(last.is_curve_end);
		out_coords.emplace_back(last.curve_end[0]);
		out_coords.emplace_back(last.curve_end[1]);
	}
	
	out_coords.emplace_back(last.pos);
	if (last.param == 0.0)
	{
		out_coords.back().setHolePoint(flags[last.index].isHolePoint());
		out_coords.back().setClosePoint(flags[last.index].isClosePoint());
	}
}

void VirtualPath::copy(
        const SplitPathCoord& first,
        const SplitPathCoord& last,
        MapCoordVector& out_flags,
        MapCoordVectorF& out_coords) const
{
	Q_ASSERT(!empty());
	Q_ASSERT(first.path_coords == &path_coords);
	Q_ASSERT(last.path_coords == &path_coords);
	
	Q_ASSERT(out_coords.size() == out_flags.size());
	
	auto& flags  = coords.flags;
	
	// Handle first coordinate and its flags.
	if (out_coords.empty() ||
	    out_flags.back().isHolePoint() ||
	    out_coords.back() != first.pos)
	{
		out_coords.emplace_back(first.pos);
		out_flags.emplace_back();
	}
	else
	{
		out_flags.back().setHolePoint(false);
		out_flags.back().setClosePoint(false);
	}
	
	if (first.index == last.index)
	{
		out_flags.back().setCurveStart(last.is_curve_end && last.param != first.param);
	}
	else 
	{
		out_flags.back().setCurveStart(first.is_curve_start);
		
		auto stop_index = last.index;
		if (last.param == 0.0)
		{
			stop_index -= last.is_curve_end ? 3 : 1;
		}
		
		auto index = first.index + 1;
		if (first.is_curve_start && index < stop_index)
		{
			out_flags.emplace_back();
			out_coords.emplace_back(first.curve_start[0]);
			out_flags.emplace_back();
			out_coords.emplace_back(first.curve_start[1]);
			index += 2;
		}
		
		for (; index <= stop_index; ++index)
		{
			out_flags.emplace_back(flags[index]);
			out_coords.emplace_back(coords[index]);
		}
	}
	
	if (out_flags.back().isCurveStart())
	{
		Q_ASSERT(last.is_curve_end);
		out_flags.emplace_back();
		out_coords.emplace_back(last.curve_end[0]);
		out_flags.emplace_back();
		out_coords.emplace_back(last.curve_end[1]);
	}
	
	out_flags.emplace_back();
	out_coords.emplace_back(last.pos);
	if (last.param == 0.0)
	{
		out_flags.back().setHolePoint(flags[last.index].isHolePoint());
		out_flags.back().setClosePoint(flags[last.index].isClosePoint());
	}
	
	Q_ASSERT(out_coords.size() == out_flags.size());
}

// This algorithm must (nearly) match VirtualPath::copy().
// (It will always copy the length of the starting point, but
// VirtualPath::copy() might skip this point in some cases.)
void VirtualPath::copyLengths(
        const SplitPathCoord& first,
        const SplitPathCoord& last,
        std::vector<PathCoord::length_type>& out_lengths ) const
{
	Q_ASSERT(!empty());
	Q_ASSERT(first.path_coords == &path_coords);
	Q_ASSERT(last.path_coords == &path_coords);
	
	auto& flags  = coords.flags;
	
	// Handle first coordinate
	out_lengths.emplace_back(first.clen);
	
	bool after_curve_start = false;
	
	if (first.index == last.index)
	{
		after_curve_start = last.is_curve_end && last.param != first.param;
	}
	else 
	{
		after_curve_start = first.is_curve_start;
		
		auto stop_index = last.index;
		if (last.param == 0.0)
		{
			stop_index -= last.is_curve_end ? 3 : 1;
		}
		
		auto index = first.index + 1;
		if (first.is_curve_start && index < stop_index)
		{
			after_curve_start = false;
			out_lengths.emplace_back(first.clen);
			out_lengths.emplace_back(first.clen);
			index += 2;
		}
		
		auto path_coord_index = first.path_coord_index;
		for (; index <= stop_index; ++index)
		{
			while (path_coords[path_coord_index].index < index)
				++path_coord_index;
			
			if (path_coords[path_coord_index].index == index)
			{
				// Normal node
				out_lengths.emplace_back(path_coords[path_coord_index].clen);
				after_curve_start = flags[index].isCurveStart();
			}
			else
			{
				// Control point
				out_lengths.emplace_back(out_lengths.back());
				after_curve_start = false;
			}
		}
	}
		
	if (after_curve_start)
	{
		auto length = out_lengths.back();
		out_lengths.emplace_back(length);
		out_lengths.emplace_back(length);
	}
	
	out_lengths.push_back(last.clen);
}


}  // namespace OpenOrienteering
