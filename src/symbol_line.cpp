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


#include "symbol_line.h"

#include <QtGui>
#include <QFile>

#include "object.h"
#include "map_color.h"
#include "util.h"
#include "symbol_setting_dialog.h"
#include "symbol_point_editor.h"
#include "symbol_point.h"
#include "symbol_area.h"
#include "qbezier_p.h"

LineSymbol::LineSymbol() : Symbol(Symbol::Line)
{
	line_width = 0;
	color = NULL;
	minimum_length = 0;
	cap_style = FlatCap;
	join_style = MiterJoin;
	pointed_cap_length = 1000;
	
	start_symbol = NULL;
	mid_symbol = NULL;
	end_symbol = NULL;
	dash_symbol = NULL;
	
	dashed = false;
	
	segment_length = 4000;
	end_length = 0;
	show_at_least_one_symbol = true;
	minimum_mid_symbol_count = 0;
	minimum_mid_symbol_count_when_closed = 0;

	dash_length = 4000;
	break_length = 1000;
	dashes_in_group = 1;
	in_group_break_length = 500;
	half_outer_dashes = false;
	mid_symbols_per_spot = 1;
	mid_symbol_distance = 0;
	
	// Border lines
	have_border_lines = false;
	border_color = NULL;
	border_width = 0;
	border_shift = 0;
	dashed_border = false;
	border_dash_length = 2 * 1000;
	border_break_length = 1 * 1000;
}
LineSymbol::~LineSymbol()
{
	delete start_symbol;
	delete mid_symbol;
	delete end_symbol;
	delete dash_symbol;
}

Symbol* LineSymbol::duplicate() const
{
	LineSymbol* new_line = new LineSymbol();
	new_line->duplicateImplCommon(this);
	new_line->line_width = line_width;
	new_line->color = color;
	new_line->minimum_length = minimum_length;
	new_line->cap_style = cap_style;
	new_line->join_style = join_style;
	new_line->pointed_cap_length = pointed_cap_length;
	new_line->start_symbol = start_symbol ? reinterpret_cast<PointSymbol*>(start_symbol->duplicate()) : NULL;
	new_line->mid_symbol = mid_symbol ? reinterpret_cast<PointSymbol*>(mid_symbol->duplicate()) : NULL;
	new_line->end_symbol = end_symbol ? reinterpret_cast<PointSymbol*>(end_symbol->duplicate()) : NULL;
	new_line->dash_symbol = dash_symbol ? reinterpret_cast<PointSymbol*>(dash_symbol->duplicate()) : NULL;
	new_line->dashed = dashed;
	new_line->segment_length = segment_length;
	new_line->end_length = end_length;
	new_line->show_at_least_one_symbol = show_at_least_one_symbol;
	new_line->minimum_mid_symbol_count = minimum_mid_symbol_count;
	new_line->minimum_mid_symbol_count_when_closed = minimum_mid_symbol_count_when_closed;
	new_line->dash_length = dash_length;
	new_line->break_length = break_length;
	new_line->dashes_in_group = dashes_in_group;
	new_line->in_group_break_length = in_group_break_length;
	new_line->half_outer_dashes = half_outer_dashes;
	new_line->mid_symbols_per_spot = mid_symbols_per_spot;
	new_line->mid_symbol_distance = mid_symbol_distance;
	new_line->have_border_lines = have_border_lines;
	new_line->border_color = border_color;
	new_line->border_width = border_width;
	new_line->border_shift = border_shift;
	new_line->dashed_border = dashed_border;
	new_line->border_dash_length = border_dash_length;
	new_line->border_break_length = border_break_length;
	return new_line;
}

void LineSymbol::createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, RenderableVector& output)
{
	PathObject* path = reinterpret_cast<PathObject*>(object);
	int num_parts = path->getNumParts();
	if (num_parts == 1)
		createRenderables(path->getPart(0).isClosed(), flags, coords, (PathCoordVector*)&path->getPathCoordinateVector(), output);	
	else if (num_parts > 1)
	{
		// TODO: optimize, remove the copying
		MapCoordVector part_flags;
		MapCoordVectorF part_coords;
		PathCoordVector part_path_coords;
		for (int i = 0; i < num_parts; ++i)
		{
			PathObject::PathPart& part = path->getPart(i);
			part_flags.assign(flags.begin() + part.start_index, flags.begin() + (part.end_index + 1));
			part_coords.assign(coords.begin() + part.start_index, coords.begin() + (part.end_index + 1));
			part_path_coords.assign(path->getPathCoordinateVector().begin() + part.path_coord_start_index,
									 path->getPathCoordinateVector().begin() + (part.path_coord_end_index + 1));
			createRenderables(part.isClosed(), part_flags, part_coords, &part_path_coords, output);	
		}
	}
}

void LineSymbol::createRenderables(bool path_closed, const MapCoordVector& flags, const MapCoordVectorF& coords, PathCoordVector* path_coords, RenderableVector& output)
{
	// TODO: Optimization: Use pre-supplied path_coords in more places
	
	if (coords.size() < 2)
		return;
	
	PathCoordVector local_path_coords;
	if (!path_coords)
	{
		PathCoord::calculatePathCoords(flags, coords, &local_path_coords);
		path_coords = &local_path_coords;
	}
	
	// Start or end symbol?
	if (start_symbol)
	{
		bool ok;
		MapCoordF tangent = PathCoord::calculateTangent(coords, 0, false, ok);
		if (ok)
		{
			// NOTE: misuse of the point symbol
			PointObject point_object(start_symbol);
			point_object.setRotation(atan2(-tangent.getY(), tangent.getX()));
			start_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), coords, output);
		}
	}
	if (end_symbol)
	{
		size_t last = coords.size() - 1;
		
		bool ok;
		MapCoordF tangent = PathCoord::calculateTangent(coords, last, true, ok);
		if (ok)
		{
			PointObject point_object(end_symbol);
			point_object.setRotation(atan2(-tangent.getY(), tangent.getX()));
			MapCoordVectorF end_coord;
			end_coord.push_back(coords[last]);
			end_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), end_coord, output);
		}
	}
	
	// Dash symbols?
	if (dash_symbol && !dash_symbol->isEmpty())
		createDashSymbolRenderables(path_closed, flags, coords, output);
	
	// The line itself
	MapCoordVector processed_flags;
	MapCoordVectorF processed_coords;
	bool create_border = have_border_lines && border_width > 0 && border_color != NULL && !(border_dash_length == 0 && dashed_border);
	if (!dashed)
	{
		// Base line?
		if (line_width > 0)
		{
			if (color && (cap_style != PointedCap || pointed_cap_length == 0) && !have_border_lines)
				output.push_back(new LineRenderable(this, coords, flags, *path_coords, path_closed));
			else if (have_border_lines || (cap_style == PointedCap && pointed_cap_length > 0))
			{
				int part_start = 0;
				int part_end = 0;
				PathCoordVector line_coords;
				while (PathCoord::getNextPathPart(flags, coords, part_start, part_end, &line_coords, false, false))
				{
					int cur_line_coord = 1;
					bool has_start = !(part_start == 0 && path_closed);
					bool has_end = !(part_end == (int)flags.size() - 1 && path_closed);
					
					processContinuousLine(path_closed, flags, coords, line_coords, 0, line_coords[line_coords.size() - 1].clen,
										  has_start, has_end, cur_line_coord, processed_flags, processed_coords, true, false, output);
				}
			}
		}
		
		// Symbols?
		if (mid_symbol && !mid_symbol->isEmpty() && segment_length > 0)
			createDottedRenderables(path_closed, flags, coords, output);
	}
	else
	{
		// Dashed lines
		if (dash_length > 0)
			processDashedLine(path_closed, flags, coords, processed_flags, processed_coords, output);
		else
			return;	// wrong parameter
	}
	
	assert(processed_coords.size() != 1);
	if (processed_coords.size() >= 2)
	{
		// Create main line renderable
		if (color)
			output.push_back(new LineRenderable(this, processed_coords, processed_flags, *path_coords, path_closed));
		
		// Border lines?
		if (create_border)
			createBorderLines(processed_flags, processed_coords, path_closed, output);
	}
}

void LineSymbol::createBorderLines(const MapCoordVector& flags, const MapCoordVectorF& coords, bool path_closed, RenderableVector& output)
{
	LineSymbol border_symbol;
	border_symbol.line_width = border_width;
	border_symbol.color = border_color;
	
	MapCoordVector border_flags;
	MapCoordVectorF border_coords;
	
	float shift = 0.001f * 0.5f * line_width + 0.001f * border_shift;
	
	if (dashed_border)
	{
		MapCoordVector dashed_flags;
		MapCoordVectorF dashed_coords;
		
		border_symbol.dashed = true;
		border_symbol.dash_length = border_dash_length;
		border_symbol.break_length = border_break_length;
		border_symbol.processDashedLine(path_closed, flags, coords, dashed_flags, dashed_coords, output);
		border_symbol.dashed = false;	// important, otherwise more dashes might be added by createRenderables()!
		
		shiftCoordinates(dashed_flags, dashed_coords, path_closed, shift, border_flags, border_coords);
		border_symbol.createRenderables(path_closed, border_flags, border_coords, NULL, output);
		shiftCoordinates(dashed_flags, dashed_coords, path_closed, -shift, border_flags, border_coords);
		border_symbol.createRenderables(path_closed, border_flags, border_coords, NULL, output);
	}
	else
	{
		shiftCoordinates(flags, coords, path_closed, shift, border_flags, border_coords);
		border_symbol.createRenderables(path_closed, border_flags, border_coords, NULL, output);
		shiftCoordinates(flags, coords, path_closed, -shift, border_flags, border_coords);
		border_symbol.createRenderables(path_closed, border_flags, border_coords, NULL, output);
	}
}

void LineSymbol::shiftCoordinates(const MapCoordVector& flags, const MapCoordVectorF& coords, bool path_closed, float shift, MapCoordVector& out_flags, MapCoordVectorF& out_coords)
{
	const float curve_threshold = 0.03f;	// TODO: decrease for export/print?
	const int MAX_OFFSET = 16;
	QBezierCopy offsetCurves[MAX_OFFSET];
	
	int size = flags.size();
	out_flags.clear();
	out_coords.clear();
	out_flags.reserve(size);
	out_coords.reserve(size);
	
	MapCoord no_flags;
	
	for (int i = 0; i < size; ++i)
	{
		float scaling;
		MapCoordF right_vector = PathCoord::calculateRightVector(flags, coords, path_closed, i, &scaling);
		float radius = scaling * shift;
		
		if (flags[i].isCurveStart())
		{
			// Use QBezierCopy code to shift the curve, but set start and end point manually to get the correct end points (because of line joins)
			// TODO: it may be necessary to remove some of the generated curves in the case an outer point is moved inwards
			QBezierCopy bezier;
			
			bool reverse = (-shift < 0);
			if (!reverse)
				bezier = QBezierCopy::fromPoints(coords[i].toQPointF(), coords[i+1].toQPointF(), coords[i+2].toQPointF(), coords[(i+3) % size].toQPointF());
			else
				bezier = QBezierCopy::fromPoints(coords[(i+3) % size].toQPointF(), coords[i+2].toQPointF(), coords[i+1].toQPointF(), coords[i].toQPointF());
			
			int count = bezier.shifted(offsetCurves, MAX_OFFSET, qAbs(shift), curve_threshold);
			if (count)
			{
				out_flags.push_back(no_flags);
				out_coords.push_back(MapCoordF(coords[i].getX() + radius * right_vector.getX(), coords[i].getY() + radius * right_vector.getY()));
				
				if (reverse)
				{
					for (int i = count - 1; i >= 0; --i)
					{
						out_flags[out_flags.size() - 1].setCurveStart(true);
						
						out_flags.push_back(no_flags);
						out_coords.push_back(MapCoordF(offsetCurves[i].pt3().x(), offsetCurves[i].pt3().y()));
						
						out_flags.push_back(no_flags);
						out_coords.push_back(MapCoordF(offsetCurves[i].pt2().x(), offsetCurves[i].pt2().y()));
						
						if (i > 0)
						{
							out_flags.push_back(no_flags);
							out_coords.push_back(MapCoordF(offsetCurves[i].pt1().x(), offsetCurves[i].pt1().y()));
						}
					}
				}
				else
				{
					for (int i = 0; i < count; ++i)
					{
						out_flags[out_flags.size() - 1].setCurveStart(true);
						
						out_flags.push_back(no_flags);
						out_coords.push_back(MapCoordF(offsetCurves[i].pt2().x(), offsetCurves[i].pt2().y()));
						
						out_flags.push_back(no_flags);
						out_coords.push_back(MapCoordF(offsetCurves[i].pt3().x(), offsetCurves[i].pt3().y()));
						
						if (i < count - 1)
						{
							out_flags.push_back(no_flags);
							out_coords.push_back(MapCoordF(offsetCurves[i].pt4().x(), offsetCurves[i].pt4().y()));
						}
					}
				}
				
				/*float end_scaling;
				MapCoordF end_right_vector = PathCoord::calculateRightVector(flags, coords, path_closed, i+3, &end_scaling);
				float end_radius = end_scaling * shift;
				out_flags.push_back(no_flags);
				out_coords.push_back(MapCoordF(coords[i+3].getX() + end_radius * end_right_vector.getX(), coords[i].getY() + end_radius * end_right_vector.getY()));*/
			}
			
			i += 2;
		}
		else
		{
			out_flags.push_back(flags[i]);
			out_coords.push_back(MapCoordF(coords[i].getX() + radius * right_vector.getX(), coords[i].getY() + radius * right_vector.getY()));
		}
	}
}

void LineSymbol::processContinuousLine(bool path_closed, const MapCoordVector& flags, const MapCoordVectorF& coords, const PathCoordVector& line_coords,
									   float start, float end, bool has_start, bool has_end, int& cur_line_coord,
									   MapCoordVector& processed_flags, MapCoordVectorF& processed_coords, bool include_first_point, bool set_mid_symbols, RenderableVector& output)
{
	float pointed_cap_length_f = (cap_style == PointedCap) ? (0.001f * pointed_cap_length) : 0;
	float line_length = end - start;
	
	bool create_line = true;
	int num_caps = 2 - (has_end ? 0 : 1) - (has_start ? 0 : 1);
	float adapted_cap_length = pointed_cap_length_f;
	if (num_caps > 0 && line_length / num_caps <= adapted_cap_length)
	{
		create_line = false;
		adapted_cap_length = line_length / num_caps;
	}
	bool has_start_cap = has_start && adapted_cap_length > 0;
	bool has_end_cap = has_end && adapted_cap_length > 0;
	
	if (has_start_cap)
	{
		// Create pointed line cap start
		createPointedLineCap(flags, coords, line_coords, start, start + adapted_cap_length, cur_line_coord, false, output);
	}
	
	if (create_line)
	{
		// Add line to processed_coords/flags
		calculateCoordinatesForRange(flags, coords, line_coords, start + (has_start_cap ? adapted_cap_length : 0), end - (has_end_cap ? adapted_cap_length : 0), cur_line_coord,
									 include_first_point, processed_flags, processed_coords, NULL, set_mid_symbols, output);
	}
	
	if (has_end_cap)
	{
		// Create pointed line cap end
		createPointedLineCap(flags, coords, line_coords, end - adapted_cap_length, end, cur_line_coord, true, output);
	}
	if (has_end && !processed_flags.empty())
		processed_flags[processed_flags.size() - 1].setHolePoint(true);
}

void LineSymbol::createPointedLineCap(const MapCoordVector& flags, const MapCoordVectorF& coords, const PathCoordVector& line_coords,
									  float start, float end, int& cur_line_coord, bool is_end, RenderableVector& output)
{
	AreaSymbol area_symbol;
	area_symbol.setColor(color);
	
	float line_half_width = 0.001f * 0.5f * line_width;
	float cap_length = 0.001f * pointed_cap_length;
	float tan_angle = line_half_width / cap_length;
	
	MapCoordVector cap_flags;
	MapCoordVectorF cap_middle_coords;
	std::vector<float> cap_lengths;
	
	calculateCoordinatesForRange(flags, coords, line_coords, start, end, cur_line_coord, true, cap_flags, cap_middle_coords, &cap_lengths, false, output);
	
	// Calculate coordinates on the left and right side of the line
	int cap_size = (int)cap_middle_coords.size();
	MapCoordVectorF cap_coords;
	MapCoordVectorF cap_coords_left;
	float sign = is_end ? (-1) : 1;
	for (int i = 0; i < cap_size; ++i)
	{
		float dist_from_start = is_end ? (end - cap_lengths[i]) : (cap_lengths[i] - start);
		float factor = dist_from_start / cap_length;
		assert(factor >= -0.001f && factor <= 1.01f);
		
		MapCoordF right_vector;
		float scaling;
		int orig_start_index = line_coords[cur_line_coord].index;
		int orig_end_index = line_coords[cur_line_coord].index + (flags[line_coords[cur_line_coord].index].isCurveStart() ? 3 : 1);
		if (orig_end_index >= (int)coords.size())
			orig_end_index = 0;
		if (cap_middle_coords[i].lengthToSquared(coords[orig_start_index]) < 0.001f*0.001f)
			right_vector = PathCoord::calculateRightVector(flags, coords, false, orig_start_index, &scaling);
		else if (cap_middle_coords[i].lengthToSquared(coords[orig_end_index]) < 0.001f*0.001f)
			right_vector = PathCoord::calculateRightVector(flags, coords, false, orig_end_index, &scaling);
		else
			right_vector = PathCoord::calculateRightVector(cap_flags, cap_middle_coords, false, i, &scaling);
		assert(right_vector.lengthSquared() < 1.01f);
		float radius = factor * scaling * line_half_width;
		if (radius > 2 * line_half_width)
			radius = 2 * line_half_width;
		else if (radius < 0)
			radius = 0;
		
		cap_flags[i].setHolePoint(false);
		cap_coords.push_back(MapCoordF(cap_middle_coords[i].getX() + radius * right_vector.getX(), cap_middle_coords[i].getY() + radius * right_vector.getY()));
		cap_coords_left.push_back(MapCoordF(cap_middle_coords[i].getX() - radius * right_vector.getX(), cap_middle_coords[i].getY() - radius * right_vector.getY()));
		
		// Create small overlap to avoid visual glitches with the on-screen display (should not matter for printing, could probably turn it off for this)
		if ((is_end && i == 0) || (!is_end && i == cap_size - 1))
		{
			const float overlap_factor = 0.005f;
			
			bool ok;
			MapCoordF tangent = PathCoord::calculateTangent(cap_middle_coords, i, !is_end, ok);
			if (ok)
			{
				tangent.normalize();
				tangent = MapCoordF(tangent.getX() * overlap_factor * sign, tangent.getY() * overlap_factor * sign);
				
				MapCoordF& coord = cap_coords[cap_coords.size() - 1];
				coord = MapCoordF(coord.getX() + tangent.getX(), coord.getY() + tangent.getY());
				
				MapCoordF& coord2 = cap_coords_left[cap_coords_left.size() - 1];
				coord2 = MapCoordF(coord2.getX() + tangent.getX(), coord2.getY() + tangent.getY());
			}
		}
		
		// Control points for bezier curves
		if (i >= 3 && cap_flags[i-3].isCurveStart())
		{
			cap_coords.push_back(cap_coords[cap_coords.size() - 1]);
			cap_coords_left.push_back(cap_coords_left[cap_coords_left.size() - 1]);
			
			MapCoordF tangent = MapCoordF(cap_middle_coords[i].getX() - cap_middle_coords[i-1].getX(), cap_middle_coords[i].getY() - cap_middle_coords[i-1].getY());
			assert(tangent.lengthSquared() < 999*999);
			float right_scale = tangent.length() * tan_angle * sign;
			cap_coords[i-1] = MapCoordF(cap_coords[i].getX() - tangent.getX() - right_vector.getX() * right_scale, cap_coords[i].getY() - tangent.getY() - right_vector.getY() * right_scale);
			cap_coords_left[i-1] = MapCoordF(cap_coords_left[i].getX() - tangent.getX() + right_vector.getX() * right_scale, cap_coords_left[i].getY() - tangent.getY() + right_vector.getY() * right_scale);
		}
		if (cap_flags[i].isCurveStart())
		{
			// TODO: Tangent scaling depending on curvature? Adaptive subdivision of the curves?
			MapCoordF tangent = MapCoordF(cap_middle_coords[i+1].getX() - cap_middle_coords[i].getX(), cap_middle_coords[i+1].getY() - cap_middle_coords[i].getY());
			assert(tangent.lengthSquared() < 999*999);
			float right_scale = tangent.length() * tan_angle * sign;
			cap_coords.push_back(MapCoordF(cap_coords[i].getX() + tangent.getX() + right_vector.getX() * right_scale, cap_coords[i].getY() + tangent.getY() + right_vector.getY() * right_scale));
			cap_coords_left.push_back(MapCoordF(cap_coords_left[i].getX() + tangent.getX() - right_vector.getX() * right_scale, cap_coords_left[i].getY() + tangent.getY() - right_vector.getY() * right_scale));
			i += 2;
		}
		else
		{
			if (i < cap_size - 1 && cap_lengths[i] == cap_lengths[i+1])
				++i;
		}
	}
	
	// Concatenate left and right side coordinates
	cap_flags.reserve(2 * cap_flags.size());
	cap_coords.reserve(2 * cap_coords.size());
	
	MapCoord curve_start;
	curve_start.setCurveStart(true);
	MapCoord no_flag;
	for (int i = (int)cap_coords_left.size() - (is_end ? 2 : 1); i >= 0; --i)
	{
		if (i >= 3 && cap_flags[i - 3].isCurveStart())
		{
			cap_flags.push_back(curve_start);
			cap_flags.push_back(no_flag);
			cap_flags.push_back(no_flag);
			cap_coords.push_back(cap_coords_left[i]);
			cap_coords.push_back(cap_coords_left[i-1]);
			cap_coords.push_back(cap_coords_left[i-2]);
			i -= 2;
		}
		else if (is_end && i >= 2 && i == (int)cap_coords_left.size() - 2 && cap_flags[i - 2].isCurveStart())
		{
			cap_flags[cap_flags.size() - 1].setCurveStart(true);
			cap_flags.push_back(no_flag);
			cap_flags.push_back(no_flag);
			cap_coords.push_back(cap_coords_left[i]);
			cap_coords.push_back(cap_coords_left[i-1]);
			i -= 1;
		}
		else
		{
			cap_flags.push_back(no_flag);
			cap_coords.push_back(cap_coords_left[i]);
		}
	}
	
	// Add renderable
	assert(cap_coords.size() >= 3 && cap_coords.size() == cap_flags.size());
	output.push_back(new AreaRenderable(&area_symbol, cap_coords, cap_flags, NULL));
}

void LineSymbol::processDashedLine(bool path_closed, const MapCoordVector& flags, const MapCoordVectorF& coords, MapCoordVector& out_flags, MapCoordVectorF& out_coords, RenderableVector& output)
{
	int size = (int)coords.size();
	
	PointObject point_object(mid_symbol);
	MapCoordVectorF point_coord;
	point_coord.push_back(MapCoordF(0, 0));
	MapCoordF right_vector;
	
	float dash_length_f = 0.001f * dash_length;
	float break_length_f = 0.001f * break_length;
	float in_group_break_length_f = 0.001f * in_group_break_length;
	
	int part_start = 0;
	int part_end = 0;
	PathCoordVector line_coords;
	out_flags.reserve(4 * coords.size());
	out_coords.reserve(4 * coords.size());
	
	//bool dash_point_before = false;
	float cur_length = 0;
	float old_length = 0;	// length from line part(s) before dash point(s) which is not accounted for yet
	int first_line_coord = 0;
	int cur_line_coord = 1;
	while (PathCoord::getNextPathPart(flags, coords, part_start, part_end, &line_coords, true, true))
	{
		if (part_start > 0 && flags[part_start-1].isHolePoint())
		{
			++first_line_coord;
			cur_line_coord = first_line_coord + 1;
			while (line_coords[cur_line_coord].clen == 0)
				++cur_line_coord;	// TODO: Debug where double identical line coords with clen == 0 can come from? Happened with pointed line ends + dashed border lines in a closed path
			cur_length = line_coords[first_line_coord].clen;
		}
		int line_coords_size = (int)line_coords.size();
		
		bool starts_with_dashpoint = (part_start > 0 && flags[part_start].isDashPoint());
		bool ends_with_dashpoint = (part_end < size - 1 && flags[part_end].isDashPoint());
		bool half_first_dash = (part_start == 0 && (half_outer_dashes || path_closed)) || (starts_with_dashpoint && dashes_in_group == 1);
		bool half_last_dash = (part_end == size - 1 && (half_outer_dashes || path_closed)) || (ends_with_dashpoint && dashes_in_group == 1);
		
		double length = line_coords[line_coords_size - 1].clen - line_coords[first_line_coord].clen;
		
		float num_dashgroups_f = (length + break_length_f - (half_first_dash ? 0.5f*dash_length_f : 0) - (half_last_dash ? 0.5f*dash_length_f : 0)) /
		(break_length_f + dashes_in_group * dash_length_f + (dashes_in_group-1) * in_group_break_length_f);
		num_dashgroups_f += (half_first_dash ? 1 : 0) + (half_last_dash ? 1 : 0);
		int lower_dashgroup_count = qRound(floor(num_dashgroups_f));
		
		float switch_deviation = 0.2f * ((dashes_in_group*dash_length_f + break_length_f + (dashes_in_group-1)*in_group_break_length_f)) / dashes_in_group;
		float minimum_optimum_length = (2*dashes_in_group*dash_length_f + break_length_f + 2*(dashes_in_group-1)*in_group_break_length_f);
		float minimum_optimum_num_dashes = 2*dashes_in_group - (half_first_dash ? 0.5f : 0) - (half_last_dash ? 0.5f : 0);
		if (length <= 0.0 || (lower_dashgroup_count <= 1 && length < minimum_optimum_length - minimum_optimum_num_dashes * switch_deviation))
		{
			// Line part too short for dashes, use just one continuous line for it
			if (!ends_with_dashpoint)
			{
				processContinuousLine(path_closed, flags, coords, line_coords, cur_length, cur_length + length + old_length,
									  (!path_closed && out_flags.empty()) || (!out_flags.empty() && out_flags[out_flags.size() - 1].isHolePoint()), !(path_closed && part_end == size - 1),
									  cur_line_coord, out_flags, out_coords, true, old_length == 0 && length >= dash_length_f - switch_deviation, output);
				cur_length += length + old_length;
				old_length = 0;
			}
			else
				old_length += length;
		}
		else
		{
			int higher_dashgroup_count = qRound(ceil(num_dashgroups_f));
			float lower_dashgroup_deviation = (length - (lower_dashgroup_count*dashes_in_group*dash_length_f + (lower_dashgroup_count-1)*break_length_f + lower_dashgroup_count*(dashes_in_group-1)*in_group_break_length_f)) / (lower_dashgroup_count*dashes_in_group);
			float higher_dashgroup_deviation = (-1) * (length - (higher_dashgroup_count*dashes_in_group*dash_length_f + (higher_dashgroup_count-1)*break_length_f + higher_dashgroup_count*(dashes_in_group-1)*in_group_break_length_f)) / (higher_dashgroup_count*dashes_in_group);
			if (!half_first_dash && !half_last_dash)
				assert(lower_dashgroup_deviation >= 0 && higher_dashgroup_deviation >= 0); // TODO; seems to fail as long as halving first/last dashes affects the outermost dash only
			int num_dashgroups = (lower_dashgroup_deviation > higher_dashgroup_deviation) ? higher_dashgroup_count : lower_dashgroup_count;
			assert(num_dashgroups >= 2);
			
			int num_half_dashes = 2*num_dashgroups*dashes_in_group - (half_first_dash ? 1 : 0) - (half_last_dash ? 1 : 0);
			float adapted_dash_length = (length - (num_dashgroups-1)*break_length_f - num_dashgroups*(dashes_in_group-1)*in_group_break_length_f) / (0.5f*num_half_dashes);
			
			for (int dashgroup = 0; dashgroup < num_dashgroups; ++dashgroup)
			{
				for (int dash = 0; dash < dashes_in_group; ++dash)
				{
					bool is_first_dash = dashgroup == 0 && dash == 0;
					bool is_half_dash = (is_first_dash && half_first_dash) || (dashgroup == num_dashgroups-1 && dash == dashes_in_group-1 && half_last_dash);
					float cur_dash_length = is_half_dash ? adapted_dash_length / 2 : adapted_dash_length;
					
					// Process immediately if this is not the last dash before a dash point
					if (!(ends_with_dashpoint && dash == dashes_in_group - 1 && dashgroup == num_dashgroups - 1))
					{
						// The dash has an end if it is not the last dash in a closed path
						bool has_end = !(dash == dashes_in_group - 1 && dashgroup == num_dashgroups - 1 && path_closed && part_end == size - 1);
						
						processContinuousLine(path_closed, flags, coords, line_coords, cur_length, cur_length + old_length + cur_dash_length,
											  (!path_closed && out_flags.empty()) || (!out_flags.empty() && out_flags[out_flags.size() - 1].isHolePoint()), has_end,
											  cur_line_coord, out_flags, out_coords, true, !is_half_dash, output);
						cur_length += old_length + cur_dash_length;
						old_length = 0;
						//dash_point_before = false;
						
						if (dash < dashes_in_group - 1)
							cur_length += in_group_break_length_f;
					}
					else
						old_length += cur_dash_length;
				}
				
				if (dashgroup < num_dashgroups - 1)
					cur_length += break_length_f;
			}
		}
		
		if (ends_with_dashpoint && dashes_in_group == 1 && mid_symbol && !mid_symbol->isEmpty())
		{
			// Place a mid symbol on the dash point
			MapCoordF right = PathCoord::calculateRightVector(flags, coords, path_closed, part_end, NULL);
			point_object.setRotation(atan2(right.getX(), right.getY()));
			point_coord[0] = coords[part_end];
			mid_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), point_coord, output);
		}
		
		cur_length = line_coords[line_coords_size - 1].clen - old_length;
		
		//dash_point_before = ends_with_dashpoint;
		first_line_coord = line_coords_size - 1;
	}
}

void LineSymbol::createDashSymbolRenderables(bool path_closed, const MapCoordVector& flags, const MapCoordVectorF& coords, RenderableVector& output)
{
	PointObject point_object(dash_symbol);
	MapCoordVectorF point_coord;
	point_coord.push_back(MapCoordF(0, 0));
	MapCoordF right_vector;
	
	int size = (int)coords.size();
	for (int i = 0; i < size; ++i)
	{
		if (!flags[i].isDashPoint())
			continue;
		
		float scaling;
		MapCoordF right = PathCoord::calculateRightVector(flags, coords, path_closed, i, &scaling);
		
		point_object.setRotation(atan2(right.getX(), right.getY()));
		point_coord[0] = coords[i];
		dash_symbol->createRenderablesScaled(&point_object, point_object.getRawCoordinateVector(), point_coord, output, scaling);
	}
}

void LineSymbol::createDottedRenderables(bool path_closed, const MapCoordVector& flags, const MapCoordVectorF& coords, RenderableVector& output)
{
	PointObject point_object(mid_symbol);
	MapCoordVectorF point_coord;
	point_coord.push_back(MapCoordF(0, 0));
	MapCoordF right_vector;
	
	float segment_length_f = 0.001f * segment_length;
	float end_length_f = 0.001f * end_length;
	float mid_symbol_distance_f = 0.001f * mid_symbol_distance;
	float mid_symbols_length = (mid_symbols_per_spot - 1)*mid_symbol_distance_f;
	bool is_first_part = true;
	int part_start = 0;
	int part_end = 0;
	PathCoordVector line_coords;
	while (PathCoord::getNextPathPart(flags, coords, part_start, part_end, &line_coords, true, false))
	{
		if (is_first_part)
		{
			if (end_length == 0)
			{
				// Insert point at start coordinate
				right_vector = PathCoord::calculateRightVector(flags, coords, path_closed, part_start, NULL);
				point_object.setRotation(atan2(right_vector.getX(), right_vector.getY()));
				point_coord[0] = coords[part_start];
				mid_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), point_coord, output);
			}
			
			is_first_part = false;
		}
		
		double length = line_coords[line_coords.size() - 1].clen;
		
		double segmented_length = length;
		if (end_length > 0)
			segmented_length = qMax(0.0, length - 2 * end_length_f);
		segmented_length -= (mid_symbols_per_spot - 1) * mid_symbol_distance_f;
		
		int lower_segment_count = qMax((end_length == 0) ? 1 : 0, qRound(floor(segmented_length / (segment_length_f + (mid_symbols_per_spot - 1) * mid_symbol_distance_f))));
		int higher_segment_count = qMax((end_length == 0) ? 1 : 0, qRound(ceil(segmented_length / (segment_length_f + (mid_symbols_per_spot - 1) * mid_symbol_distance_f))));

		int line_coord_search_start = 0;
		if (end_length > 0)
		{
			if (length <= (mid_symbols_per_spot - 1) * mid_symbol_distance_f)
			{
				if (show_at_least_one_symbol)
				{
					// Insert point at start coordinate
					right_vector = PathCoord::calculateRightVector(flags, coords, path_closed, part_start, NULL);
					point_object.setRotation(atan2(right_vector.getX(), right_vector.getY()));
					point_coord[0] = coords[part_start];
					mid_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), point_coord, output);
					
					// Insert point at end coordinate
					right_vector = PathCoord::calculateRightVector(flags, coords, path_closed, part_end, NULL);
					point_object.setRotation(atan2(right_vector.getX(), right_vector.getY()));
					point_coord[0] = coords[part_end];
					mid_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), point_coord, output);
				}
			}
			else
			{
				double lower_abs_deviation = qAbs(length - lower_segment_count * segment_length_f - (lower_segment_count+1)*mid_symbols_length - 2 * end_length_f);
				double higher_abs_deviation = qAbs(length - higher_segment_count * segment_length_f - (higher_segment_count+1)*mid_symbols_length - 2 * end_length_f);
				int segment_count = (lower_abs_deviation >= higher_abs_deviation) ? higher_segment_count : lower_segment_count;
				
				double deviation = (lower_abs_deviation >= higher_abs_deviation) ? (-1 * higher_abs_deviation) : (lower_abs_deviation);
				double ideal_length = segment_count * segment_length_f + 2 * end_length_f;
				double adapted_end_length = end_length_f + deviation * (end_length_f / ideal_length);
				double adapted_segment_length = segment_length_f + deviation * (segment_length_f / ideal_length);
				assert(qAbs(2*adapted_end_length + segment_count*adapted_segment_length + (segment_count + 1)*mid_symbols_length - length) < 0.001f);
				
				if (adapted_segment_length >= 0 && (show_at_least_one_symbol || higher_segment_count > 0 || length > 2*end_length_f - 0.5 * (2*end_length_f+segment_length_f+2*mid_symbols_length - (2*end_length_f+mid_symbols_length))))
				{
					for (int i = 0; i < segment_count + 1; ++i)
					{
						for (int s = 0; s < mid_symbols_per_spot; ++s)
						{
							double position = adapted_end_length + s * mid_symbol_distance_f + i * (adapted_segment_length + mid_symbols_length);
							PathCoord::calculatePositionAt(flags, coords, line_coords, position, line_coord_search_start, &point_coord[0], &right_vector);
							point_object.setRotation(atan2(right_vector.getX(), right_vector.getY()));
							mid_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), point_coord, output);
						}
					}
				}
			}
		}
		else
		{
			if (length > mid_symbols_length)
			{
				double lower_segment_deviation = qAbs(length - lower_segment_count * segment_length_f - (lower_segment_count+1)*mid_symbols_length) / lower_segment_count;
				double higher_segment_deviation = qAbs(length - higher_segment_count * segment_length_f - (higher_segment_count+1)*mid_symbols_length) / higher_segment_count;
				int segment_count = (lower_segment_deviation > higher_segment_deviation) ? higher_segment_count : lower_segment_count;
				double adapted_segment_length = (length - (segment_count+1)*mid_symbols_length) / segment_count + mid_symbols_length;
				assert(qAbs(segment_count * adapted_segment_length + mid_symbols_length) - length < 0.001f);
				
				if (adapted_segment_length >= mid_symbols_length)
				{
					for (int i = 0; i <= segment_count; ++i)
					{
						for (int s = 0; s < mid_symbols_per_spot; ++s)
						{
							// The outermost symbols are handled outside this loop
							if (i == 0 && s == 0)
								continue;
							if (i == segment_count && s == mid_symbols_per_spot - 1)
								break;
							
							PathCoord::calculatePositionAt(flags, coords, line_coords, s * mid_symbol_distance_f + i * adapted_segment_length, line_coord_search_start, &point_coord[0], &right_vector);
							point_object.setRotation(atan2(right_vector.getX(), right_vector.getY()));
							mid_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), point_coord, output);
						}
					}
				}
			}
		}
		
		if (end_length == 0)
		{
			// Insert point at end coordinate
			right_vector = PathCoord::calculateRightVector(flags, coords, path_closed, part_end, NULL);
			point_object.setRotation(atan2(right_vector.getX(), right_vector.getY()));
			point_coord[0] = coords[part_end];
			mid_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), point_coord, output);
		}
		
		if (flags[part_end].isHolePoint())
			is_first_part = true;
	}
}

void LineSymbol::calculateCoordinatesForRange(const MapCoordVector& flags, const MapCoordVectorF& coords, const PathCoordVector& line_coords,
										float start, float end, int& cur_line_coord, bool include_start_coord, MapCoordVector& out_flags, MapCoordVectorF& out_coords,
										std::vector<float>* out_lengths, bool set_mid_symbols, RenderableVector& output)
{
	assert(cur_line_coord > 0);
	int line_coords_size = (int)line_coords.size();
	while (cur_line_coord < line_coords_size - 1 && line_coords[cur_line_coord].clen < start)
		++cur_line_coord;
	
	// Start position
	int start_bezier_index = -1;		// if the range starts at a bezier curve, this is the curve's index, otherwise -1
	float start_bezier_split_param = 0;	// the parameter value where the split of the curve for the range start was made
	MapCoordF o3, o4;					// temporary bezier control points
	if (flags[line_coords[cur_line_coord].index].isCurveStart())
	{
		int index = line_coords[cur_line_coord].index;
		float factor = (start - line_coords[cur_line_coord-1].clen) / qMax(1e-7f, (line_coords[cur_line_coord].clen - line_coords[cur_line_coord-1].clen));
		assert(factor >= -0.01f && factor <= 1.01f);
		if (factor > 1)
			factor = 1;
		else if (factor < 0)
			factor = 0;
		float prev_param = (line_coords[cur_line_coord-1].index == line_coords[cur_line_coord].index) ? line_coords[cur_line_coord-1].param : 0;
		assert(prev_param <= line_coords[cur_line_coord].param);
		float p = prev_param + (line_coords[cur_line_coord].param - prev_param) * factor;
		assert(p >= 0 && p <= 1);
		
		MapCoordF o0, o1;
		if (!include_start_coord)
		{
			MapCoordF o2;
			PathCoord::splitBezierCurve(coords[index], coords[index+1], coords[index+2], (index < (int)flags.size() - 3) ? coords[index+3] : coords[0], p, o0, o1, o2, o3, o4);
		}
		else
		{
			out_coords.push_back(MapCoordF(0, 0));
			PathCoord::splitBezierCurve(coords[index], coords[index+1], coords[index+2], (index < (int)flags.size() - 3) ? coords[index+3] : coords[0], p, o0, o1, out_coords[out_coords.size() - 1], o3, o4);
			MapCoord flag(0, 0);
			flag.setCurveStart(true);
			out_flags.push_back(flag);
			if (out_lengths)
				out_lengths->push_back(start);
		}
		
		start_bezier_split_param = p;
		start_bezier_index = index;
	}
	else if (include_start_coord)
	{
		out_coords.push_back(MapCoordF(0, 0));
		PathCoord::calculatePositionAt(flags, coords, line_coords, start, cur_line_coord, &out_coords[out_coords.size() - 1], NULL);
		out_flags.push_back(MapCoord(0, 0));
		if (out_lengths)
			out_lengths->push_back(start);
	}
	
	int current_index = line_coords[cur_line_coord].index;
	
	// Middle position
	int num_mid_symbols = qMax(1, mid_symbols_per_spot);
	if (set_mid_symbols && mid_symbol && !mid_symbol->isEmpty() && (num_mid_symbols-1) * 0.001f * mid_symbol_distance <= end - start)
	{
		PointObject point_object(mid_symbol);
		MapCoordVectorF point_coord;
		point_coord.push_back(MapCoordF(0, 0));
		MapCoordF right_vector;
		
		float mid_position = (end + start) / 2 - ((num_mid_symbols-1) * 0.001f * mid_symbol_distance) / 2;
		advanceCoordinateRangeTo(flags, coords, line_coords, cur_line_coord, current_index, mid_position, start_bezier_index, out_flags, out_coords, out_lengths, o3, o4);
		
		for (int i = 0; i < num_mid_symbols; ++i)
		{
			PathCoord::calculatePositionAt(flags, coords, line_coords, mid_position, cur_line_coord, &point_coord[0], &right_vector);
			
			point_object.setRotation(atan2(right_vector.getX(), right_vector.getY()));
			mid_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), point_coord, output);
			
			mid_position += 0.001f * mid_symbol_distance;
			if (i < num_mid_symbols - 1)
				advanceCoordinateRangeTo(flags, coords, line_coords, cur_line_coord, current_index, mid_position, start_bezier_index, out_flags, out_coords, out_lengths, o3, o4);
		}
	}
	
	// End position
	advanceCoordinateRangeTo(flags, coords, line_coords, cur_line_coord, current_index, end, start_bezier_index, out_flags, out_coords, out_lengths, o3, o4);
	
	out_coords.push_back(MapCoordF(0, 0));
	if (flags[current_index].isCurveStart())
	{
		int index = line_coords[cur_line_coord].index;
		float factor = (end - line_coords[cur_line_coord-1].clen) / qMax(1e-7f, (line_coords[cur_line_coord].clen - line_coords[cur_line_coord-1].clen));
		assert(factor >= -0.01f && factor <= 1.01f);
		if (factor > 1)
			factor = 1;
		else if (factor < 0)
			factor = 0;
		float prev_param = (line_coords[cur_line_coord-1].index == line_coords[cur_line_coord].index) ? line_coords[cur_line_coord-1].param : 0;
		assert(prev_param <= line_coords[cur_line_coord].param);
		float p = prev_param + (line_coords[cur_line_coord].param - prev_param) * factor;
		assert(p >= 0 && p <= 1);
		
		out_flags.push_back(MapCoord(0, 0));
		out_coords.push_back(MapCoordF(0, 0));
		out_flags.push_back(MapCoord(0, 0));
		out_coords.push_back(MapCoordF(0, 0));
		if (out_lengths)
		{
			out_lengths->push_back(-1);
			out_lengths->push_back(-1);
		}
		MapCoordF unused, unused2;
		
		out_flags.push_back(MapCoord(0, 0));
		if (start_bezier_index == current_index)
		{
			// The dash end is in the same curve as the start, need to make a second split with the correct parameter
			p = (p - start_bezier_split_param) / (1 - start_bezier_split_param);
			assert(p >= 0 && p <= 1);
			
			PathCoord::splitBezierCurve(out_coords[out_coords.size() - 4], o3, o4, (index < (int)flags.size() - 3) ? coords[index+3] : coords[0],
										p, out_coords[out_coords.size() - 3], out_coords[out_coords.size() - 2],
										out_coords[out_coords.size() - 1], unused, unused2);
		}
		else
		{
			PathCoord::splitBezierCurve(coords[index], coords[index+1], coords[index+2], (index < (int)flags.size() - 3) ? coords[index+3] : coords[0],
										p, out_coords[out_coords.size() - 3], out_coords[out_coords.size() - 2],
										out_coords[out_coords.size() - 1], unused, unused2);
		}
		if (out_lengths)
			out_lengths->push_back(end);
	}
	else
	{
		out_flags.push_back(MapCoord(0, 0));
		PathCoord::calculatePositionAt(flags, coords, line_coords, end, cur_line_coord, &out_coords[out_coords.size() - 1], NULL);
		if (out_lengths)
			out_lengths->push_back(end);
	}
}

void LineSymbol::advanceCoordinateRangeTo(const MapCoordVector& flags, const MapCoordVectorF& coords, const PathCoordVector& line_coords, int& cur_line_coord, int& current_index, float cur_length,
									 int start_bezier_index, MapCoordVector& out_flags, MapCoordVectorF& out_coords, std::vector<float>* out_lengths, const MapCoordF& o3, const MapCoordF& o4)
{
	int line_coords_size = (int)line_coords.size();
	while (cur_line_coord < line_coords_size - 1 && line_coords[cur_line_coord].clen < cur_length)
	{
		++cur_line_coord;
		if (line_coords[cur_line_coord].index != current_index)
		{
			if (current_index == start_bezier_index)
			{
				out_flags.push_back(MapCoord(0, 0));
				out_coords.push_back(o3);
				if (out_lengths)
					out_lengths->push_back(-1);
				out_flags.push_back(MapCoord(0, 0));
				out_coords.push_back(o4);
				if (out_lengths)
					out_lengths->push_back(-1);
				out_flags.push_back(flags[current_index + 3]);
				out_coords.push_back(coords[current_index + 3]);
				if (out_lengths)
					out_lengths->push_back(line_coords[cur_line_coord-1].clen);
				
				current_index += 3;
				assert(current_index == line_coords[cur_line_coord].index);
			}
			else
			{
				assert((!flags[current_index].isCurveStart() && current_index + 1 == line_coords[cur_line_coord].index) ||
				       (flags[current_index].isCurveStart() && current_index + 3 == line_coords[cur_line_coord].index) ||
				       (flags[current_index+1].isHolePoint() && current_index + 2 == line_coords[cur_line_coord].index));
				do
				{
					++current_index;
					out_flags.push_back(flags[current_index]);
					out_coords.push_back(coords[current_index]);
					if (out_lengths)
						out_lengths->push_back(line_coords[cur_line_coord-1].clen);
				} while (current_index < line_coords[cur_line_coord].index);
			}
		}
	}	
}

void LineSymbol::colorDeleted(Map* map, int pos, MapColor* color)
{
	bool have_changes = false;
	if (mid_symbol && mid_symbol->containsColor(color))
	{
		mid_symbol->colorDeleted(map, pos, color);
		have_changes = true;
	}
	if (start_symbol && start_symbol->containsColor(color))
	{
		start_symbol->colorDeleted(map, pos, color);
		have_changes = true;
	}
	if (end_symbol && end_symbol->containsColor(color))
	{
		end_symbol->colorDeleted(map, pos, color);
		have_changes = true;
	}
	if (dash_symbol && dash_symbol->containsColor(color))
	{
		dash_symbol->colorDeleted(map, pos, color);
		have_changes = true;
	}
    if (color == this->color)
	{
		this->color = NULL;
		have_changes = true;
	}
	if (have_changes)
		getIcon(map, true);
}

bool LineSymbol::containsColor(MapColor* color)
{
	if (color == this->color)
		return true;
	if (mid_symbol && mid_symbol->containsColor(color))
		return true;
	if (start_symbol && start_symbol->containsColor(color))
		return true;
	if (end_symbol && end_symbol->containsColor(color))
		return true;
	if (dash_symbol && dash_symbol->containsColor(color))
		return true;
	return false;
}

void LineSymbol::scale(double factor)
{
	line_width = qRound(factor * line_width);
	
	minimum_length = qRound(factor * minimum_length);
	pointed_cap_length = qRound(factor * pointed_cap_length);
	
	if (start_symbol)
		start_symbol->scale(factor);
	if (mid_symbol)
		mid_symbol->scale(factor);
	if (end_symbol)
		end_symbol->scale(factor);
	if (dash_symbol)
		dash_symbol->scale(factor);

	segment_length = qRound(factor * segment_length);
	end_length = qRound(factor * end_length);
	minimum_mid_symbol_count = qRound(factor * minimum_mid_symbol_count);
	minimum_mid_symbol_count_when_closed = qRound(factor * minimum_mid_symbol_count_when_closed);

	dash_length = qRound(factor * dash_length);
	break_length = qRound(factor * break_length);
	in_group_break_length = qRound(factor * in_group_break_length);
	mid_symbol_distance = qRound(factor * mid_symbol_distance);

	border_width = qRound(factor * border_width);
	border_shift = qRound(factor * border_shift);
	border_dash_length = qRound(factor * border_dash_length);
	border_break_length = qRound(factor * border_break_length);
}

void LineSymbol::ensurePointSymbols(const QString& start_name, const QString& mid_name, const QString& end_name, const QString& dash_name)
{
	if (!start_symbol)
	{
		start_symbol = new PointSymbol();
		start_symbol->setRotatable(true);
	}
	start_symbol->setName(start_name);
	if (!mid_symbol)
	{
		mid_symbol = new PointSymbol();
		mid_symbol->setRotatable(true);
	}
	mid_symbol->setName(mid_name);
	if (!end_symbol)
	{
		end_symbol = new PointSymbol();
		end_symbol->setRotatable(true);
	}
	end_symbol->setName(end_name);
	if (!dash_symbol)
	{
		dash_symbol = new PointSymbol();
		dash_symbol->setRotatable(true);
	}
	dash_symbol->setName(dash_name);
}

void LineSymbol::cleanupPointSymbols()
{
	if (start_symbol != NULL && start_symbol->isEmpty())
	{
		delete start_symbol;
		start_symbol = NULL;
	}
	if (mid_symbol != NULL && mid_symbol->isEmpty())
	{
		delete mid_symbol;
		mid_symbol = NULL;
	}
	if (end_symbol != NULL && end_symbol->isEmpty())
	{
		delete end_symbol;
		end_symbol = NULL;
	}
	if (dash_symbol != NULL && dash_symbol->isEmpty())
	{
		delete dash_symbol;
		dash_symbol = NULL;
	}
}

void LineSymbol::saveImpl(QFile* file, Map* map)
{
	file->write((const char*)&line_width, sizeof(int));
	int temp = map->findColorIndex(color);
	file->write((const char*)&temp, sizeof(int));
	file->write((const char*)&minimum_length, sizeof(int));
	temp = (int)cap_style;
	file->write((const char*)&temp, sizeof(int));
	temp = (int)join_style;
	file->write((const char*)&temp, sizeof(int));
	file->write((const char*)&pointed_cap_length, sizeof(int));
	bool have_symbol = start_symbol != NULL;
	file->write((const char*)&have_symbol, sizeof(bool));
	if (have_symbol)
		start_symbol->save(file, map);
	have_symbol = mid_symbol != NULL;
	file->write((const char*)&have_symbol, sizeof(bool));
	if (have_symbol)
		mid_symbol->save(file, map);
	have_symbol = end_symbol != NULL;
	file->write((const char*)&have_symbol, sizeof(bool));
	if (have_symbol)
		end_symbol->save(file, map);
	have_symbol = dash_symbol != NULL;
	file->write((const char*)&have_symbol, sizeof(bool));
	if (have_symbol)
		dash_symbol->save(file, map);
	file->write((const char*)&dashed, sizeof(bool));
	file->write((const char*)&segment_length, sizeof(int));
	file->write((const char*)&end_length, sizeof(int));
	file->write((const char*)&show_at_least_one_symbol, sizeof(bool));
	file->write((const char*)&minimum_mid_symbol_count, sizeof(int));
	file->write((const char*)&minimum_mid_symbol_count_when_closed, sizeof(int));
	file->write((const char*)&dash_length, sizeof(int));
	file->write((const char*)&break_length, sizeof(int));
	file->write((const char*)&dashes_in_group, sizeof(int));
	file->write((const char*)&in_group_break_length, sizeof(int));
	file->write((const char*)&half_outer_dashes, sizeof(bool));
	file->write((const char*)&mid_symbols_per_spot, sizeof(int));
	file->write((const char*)&mid_symbol_distance, sizeof(int));
	file->write((const char*)&have_border_lines, sizeof(bool));
	temp = map->findColorIndex(border_color);
	file->write((const char*)&temp, sizeof(int));
	file->write((const char*)&border_width, sizeof(int));
	file->write((const char*)&border_shift, sizeof(int));
	file->write((const char*)&dashed_border, sizeof(bool));
	file->write((const char*)&border_dash_length, sizeof(int));
	file->write((const char*)&border_break_length, sizeof(int));
}

bool LineSymbol::loadImpl(QFile* file, int version, Map* map)
{
	file->read((char*)&line_width, sizeof(int));
	int temp;
	file->read((char*)&temp, sizeof(int));
	color = (temp >= 0) ? map->getColor(temp) : NULL;
	if (version >= 2)
		file->read((char*)&minimum_length, sizeof(int));
	file->read((char*)&temp, sizeof(int));
	cap_style = (CapStyle)temp;
	file->read((char*)&temp, sizeof(int));
	join_style = (JoinStyle)temp;
	file->read((char*)&pointed_cap_length, sizeof(int));
	bool have_symbol;
	file->read((char*)&have_symbol, sizeof(bool));
	if (have_symbol)
	{
		start_symbol = new PointSymbol();
		if (!start_symbol->load(file, version, map))
			return false;
	}
	file->read((char*)&have_symbol, sizeof(bool));
	if (have_symbol)
	{
		mid_symbol = new PointSymbol();
		if (!mid_symbol->load(file, version, map))
			return false;
	}
	file->read((char*)&have_symbol, sizeof(bool));
	if (have_symbol)
	{
		end_symbol = new PointSymbol();
		if (!end_symbol->load(file, version, map))
			return false;
	}
	file->read((char*)&have_symbol, sizeof(bool));
	if (have_symbol)
	{
		dash_symbol = new PointSymbol();
		if (!dash_symbol->load(file, version, map))
			return false;
	}
	file->read((char*)&dashed, sizeof(bool));
	file->read((char*)&segment_length, sizeof(int));
	if (version >= 1)
		file->read((char*)&end_length, sizeof(int));
	if (version >= 5)
		file->read((char*)&show_at_least_one_symbol, sizeof(bool));
	if (version >= 2)
	{
		file->read((char*)&minimum_mid_symbol_count, sizeof(int));
		file->read((char*)&minimum_mid_symbol_count_when_closed, sizeof(int));	
	}
	file->read((char*)&dash_length, sizeof(int));
	file->read((char*)&break_length, sizeof(int));
	file->read((char*)&dashes_in_group, sizeof(int));
	file->read((char*)&in_group_break_length, sizeof(int));
	file->read((char*)&half_outer_dashes, sizeof(bool));
	file->read((char*)&mid_symbols_per_spot, sizeof(int));
	file->read((char*)&mid_symbol_distance, sizeof(int));
	file->read((char*)&have_border_lines, sizeof(bool));
	file->read((char*)&temp, sizeof(int));
	border_color = (temp >= 0) ? map->getColor(temp) : NULL;
	file->read((char*)&border_width, sizeof(int));
	file->read((char*)&border_shift, sizeof(int));
	file->read((char*)&dashed_border, sizeof(bool));
	file->read((char*)&border_dash_length, sizeof(int));
	file->read((char*)&border_break_length, sizeof(int));
	return true;
}

SymbolPropertiesWidget* LineSymbol::createPropertiesWidget(SymbolSettingDialog* dialog)
{
	return new LineSymbolSettings(this, dialog);
}


// ### LineSymbolSettings ###

LineSymbolSettings::LineSymbolSettings(LineSymbol* symbol, SymbolSettingDialog* dialog)
: SymbolPropertiesWidget(symbol, dialog), symbol(symbol), dialog(dialog)
{
	Map* map = dialog->getPreviewMap();
	
	symbol->ensurePointSymbols(tr("Start symbol"), tr("Mid symbol"), tr("End symbol"), tr("Dash symbol"));
	
	QWidget* line_tab = new QWidget();
	QGridLayout* layout = new QGridLayout();
	layout->setColumnStretch(1, 1);
	line_tab->setLayout(layout);
	
	QLabel* width_label = new QLabel(tr("Line width:"));
	width_edit = new QLineEdit(QString::number(0.001f * symbol->getLineWidth()));
	width_edit->setValidator(new DoubleValidator(0, 999999, width_edit));
	
	QLabel* color_label = new QLabel(tr("Line color:"));
	color_edit = new ColorDropDown(map, symbol->getColor());
	
	int row = 0, col = 0;
	layout->addWidget(width_label, row, col++);
	layout->addWidget(width_edit,  row, col);
	
	row++; col = 0;
	layout->addWidget(color_label, row, col++);
	layout->addWidget(color_edit,  row, col, 1, -1);
	
	
	QLabel* minimum_length_label = new QLabel(tr("Minimum line length:"));
	minimum_length_edit = new QLineEdit(QString::number(0.001f * symbol->minimum_length));
	minimum_length_edit->setValidator(new DoubleValidator(0, 999999, minimum_length_edit));
	
	QLabel* line_cap_label = new QLabel(tr("Line cap:"));
	line_cap_combo = new QComboBox();
	line_cap_combo->addItem(tr("flat"), QVariant(LineSymbol::FlatCap));
	line_cap_combo->addItem(tr("round"), QVariant(LineSymbol::RoundCap));
	line_cap_combo->addItem(tr("square"), QVariant(LineSymbol::SquareCap));
	line_cap_combo->addItem(tr("pointed"), QVariant(LineSymbol::PointedCap));
	line_cap_combo->setCurrentIndex(line_cap_combo->findData(symbol->cap_style));
	
	pointed_cap_length_label = new QLabel(tr("Cap length:"));
	pointed_cap_length_edit = new QLineEdit(QString::number(0.001 * symbol->pointed_cap_length));
	pointed_cap_length_edit->setValidator(new DoubleValidator(0, 999999, pointed_cap_length_edit));
	
	QLabel* line_join_label = new QLabel(tr("Line join:"));
	line_join_combo = new QComboBox();
	line_join_combo->addItem(tr("miter"), QVariant(LineSymbol::MiterJoin));
	line_join_combo->addItem(tr("round"), QVariant(LineSymbol::RoundJoin));
	line_join_combo->addItem(tr("bevel"), QVariant(LineSymbol::BevelJoin));
	line_join_combo->setCurrentIndex(line_join_combo->findData(symbol->join_style));
	
	dashed_check = new QCheckBox(tr("Line is dashed"));
	dashed_check->setChecked(symbol->dashed);
	
	border_check = new QCheckBox(tr("Enable border lines"));
	border_check->setChecked(symbol->have_border_lines);
	
	line_settings_list 
	  << minimum_length_label << minimum_length_edit
	  << line_cap_label << line_cap_combo 
	  << line_join_label << line_join_combo
	  << pointed_cap_length_label << pointed_cap_length_edit
	  << dashed_check
	  << border_check;
	
	row++; col = 0;
	layout->addWidget(minimum_length_label, row, col++);
	layout->addWidget(minimum_length_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(line_cap_label, row, col++);
	layout->addWidget(line_cap_combo, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(pointed_cap_length_label, row, col++);
	layout->addWidget(pointed_cap_length_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(line_join_label, row, col++);
	layout->addWidget(line_join_combo, row, col, 1, -1);
	
	row++; col = 0;
	layout->addWidget(new QWidget(), row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(new QLabel(QString("<b>%1</b>").arg(tr("Dashed line"))), row, col++, 1, -1);
	row++; col = 0;
	layout->addWidget(dashed_check, row, col, 1, -1);
	
	QLabel* dash_length_label = new QLabel(tr("Dash length:"));
	dash_length_edit = new QLineEdit(QString::number(0.001 * symbol->dash_length));
	dash_length_edit->setValidator(new DoubleValidator(0, 999999, dash_length_edit));
	
	QLabel* break_length_label = new QLabel(tr("Break length:"));
	break_length_edit = new QLineEdit(QString::number(0.001 * symbol->break_length));
	break_length_edit->setValidator(new DoubleValidator(0, 999999, break_length_edit));
	
	QLabel* dash_group_label = new QLabel(tr("Dashes grouped together:"));
	dash_group_combo = new QComboBox();
	dash_group_combo->addItem(tr("none"), QVariant(1));
	dash_group_combo->addItem(tr("2"), QVariant(2));
	dash_group_combo->addItem(tr("3"), QVariant(3));
	dash_group_combo->addItem(tr("4"), QVariant(4));
	dash_group_combo->setCurrentIndex(dash_group_combo->findData(QVariant(symbol->dashes_in_group)));
	
	in_group_break_length_label = new QLabel(tr("In-group break length:"));
	in_group_break_length_edit = new QLineEdit(QString::number(0.001 * symbol->in_group_break_length));
	in_group_break_length_edit->setValidator(new DoubleValidator(0, 999999, in_group_break_length_edit));
	
	half_outer_dashes_check = new QCheckBox(tr("Half length of first and last dash"));
	half_outer_dashes_check->setChecked(symbol->half_outer_dashes);
	
	dashed_widget_list
	  << dash_length_label << dash_length_edit
	  << break_length_label << break_length_edit
	  << dash_group_label << dash_group_combo
	  << in_group_break_length_label << in_group_break_length_edit
	  << half_outer_dashes_check;
	
	row++; col = 0;
	layout->addWidget(dash_length_label, row, col++);
	layout->addWidget(dash_length_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(break_length_label, row, col++);
	layout->addWidget(break_length_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(dash_group_label, row, col++);
	layout->addWidget(dash_group_combo, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(in_group_break_length_label, row, col++);
	layout->addWidget(in_group_break_length_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(half_outer_dashes_check, row, col, 1, -1);
	
	
	row++; col = 0;
	layout->addWidget(new QWidget(), row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(new QLabel(QString("<b>%1</b>").arg(tr("Mid symbols"))), row, col, 1, -1);
	
	
	QLabel* mid_symbol_per_spot_label = new QLabel(tr("Mid symbols per spot:"));
	mid_symbol_per_spot_edit = new QLineEdit(QString::number(symbol->mid_symbols_per_spot));
	mid_symbol_per_spot_edit->setValidator(new QIntValidator(1, 99, mid_symbol_per_spot_edit));
	
	mid_symbol_distance_label = new QLabel(tr("Mid symbol distance:"));
	mid_symbol_distance_edit = new QLineEdit(QString::number(0.001 * symbol->mid_symbol_distance));
	mid_symbol_distance_edit->setValidator(new DoubleValidator(0, 999999, mid_symbol_distance_edit));
	
	mid_symbol_widget_list
	  << mid_symbol_per_spot_label << mid_symbol_per_spot_edit
	  << mid_symbol_distance_label << mid_symbol_distance_edit;
	
	row++; col = 0;
	layout->addWidget(mid_symbol_per_spot_label, row, col++);
	layout->addWidget(mid_symbol_per_spot_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(mid_symbol_distance_label, row, col++);
	layout->addWidget(mid_symbol_distance_edit, row, col, 1, -1);
	
	
	QLabel* segment_length_label = new QLabel(tr("Segment length:"));
	segment_length_edit = new QLineEdit(QString::number(0.001 * symbol->segment_length));
	segment_length_edit->setValidator(new DoubleValidator(0, 999999, segment_length_edit));
	
	QLabel* end_length_label = new QLabel(tr("End length:"));
	end_length_edit = new QLineEdit(QString::number(0.001 * symbol->end_length));
	end_length_edit->setValidator(new DoubleValidator(0, 999999, end_length_edit));
	
	show_at_least_one_symbol_check = new QCheckBox(tr("Show at least one mid symbol"));
	show_at_least_one_symbol_check->setChecked(symbol->show_at_least_one_symbol);
	
	QLabel* minimum_mid_symbol_count_label = new QLabel(tr("Minimum mid symbol count:"));
	minimum_mid_symbol_count_edit = new QLineEdit(QString::number(0.001f * symbol->minimum_mid_symbol_count));
	minimum_mid_symbol_count_edit->setValidator(new DoubleValidator(0, 999999, minimum_mid_symbol_count_edit));
	
	QLabel* minimum_mid_symbol_count_when_closed_label = new QLabel(tr("Minimum mid symbol count when closed:"));
	minimum_mid_symbol_count_when_closed_edit = new QLineEdit(QString::number(0.001f * symbol->minimum_mid_symbol_count_when_closed));
	minimum_mid_symbol_count_when_closed_edit->setValidator(new DoubleValidator(0, 999999, minimum_mid_symbol_count_when_closed_edit));
	
	undashed_widget_list
	  << segment_length_label << segment_length_edit
	  << end_length_label << end_length_edit
	  << show_at_least_one_symbol_check
	  << minimum_mid_symbol_count_label << minimum_mid_symbol_count_edit
	  << minimum_mid_symbol_count_when_closed_label << minimum_mid_symbol_count_when_closed_edit;
	
	row++; col = 0;
	layout->addWidget(segment_length_label, row, col++);
	layout->addWidget(segment_length_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(end_length_label, row, col++);
	layout->addWidget(end_length_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(show_at_least_one_symbol_check, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(minimum_mid_symbol_count_label, row, col++);
	layout->addWidget(minimum_mid_symbol_count_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(minimum_mid_symbol_count_when_closed_label, row, col++);
	layout->addWidget(minimum_mid_symbol_count_when_closed_edit, row, col, 1, -1);
	
	
	
	row++; col = 0;
	layout->addWidget(new QWidget(), row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(new QLabel(QString("<b>%1</b>").arg(tr("Border"))), row, col++, 1, -1);
	row++; col = 0;
	layout->addWidget(border_check, row, col, 1, -1);
	
	QLabel* border_width_label = new QLabel(tr("Border width:"));
	border_width_edit = new QLineEdit(QString::number(0.001f * symbol->border_width));
	border_width_edit->setValidator(new DoubleValidator(0, 999999, border_width_edit));
	
	QLabel* border_color_label = new QLabel(tr("Border color:"));
	border_color_edit = new ColorDropDown(map, symbol->border_color);
	
	QLabel* border_shift_label = new QLabel(tr("Border shift:"));
	border_shift_edit = new QLineEdit(QString::number(0.001f * symbol->border_shift));
	border_shift_edit->setValidator(new DoubleValidator(-999999, 999999, border_shift_edit));
	
	border_dashed_check = new QCheckBox(tr("Border is dashed"));
	border_dashed_check->setChecked(symbol->dashed_border);
	
	border_widget_list
	  << border_width_label << border_width_edit
	  << border_color_label << border_color_edit
	  << border_shift_label << border_shift_edit
	  << border_dashed_check;
	
	row++; col = 0;
	layout->addWidget(border_width_label, row, col++);
	layout->addWidget(border_width_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(border_color_label, row, col++);
	layout->addWidget(border_color_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(border_shift_label, row, col++);
	layout->addWidget(border_shift_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(border_dashed_check, row, col, 1, -1);
	
	QLabel* border_dash_length_label = new QLabel(tr("Border dash length:"));
	border_dash_length_edit = new QLineEdit(QString::number(0.001f * symbol->border_dash_length));
	border_dash_length_edit->setValidator(new DoubleValidator(0, 999999, border_dash_length_edit));
	
	QLabel* border_break_length_label = new QLabel(tr("Border break length:"));
	border_break_length_edit = new QLineEdit(QString::number(0.001f * symbol->border_break_length));
	border_break_length_edit->setValidator(new DoubleValidator(0, 999999, border_break_length_edit));
	
	border_dash_widget_list
	  << border_dash_length_label << border_dash_length_edit
	  << border_break_length_label << border_break_length_edit;
	
	row++; col = 0;
	layout->addWidget(border_dash_length_label, row, col++);
	layout->addWidget(border_dash_length_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(border_break_length_label, row, col++);
	layout->addWidget(border_break_length_edit, row, col, 1, -1);
	
	row++;
	layout->setRowStretch(row, 1);
	
	updateWidgets();
	
	QSize min_size = line_tab->size();
	min_size.setHeight(0);
	
	scroll_area = new QScrollArea();
	scroll_area->setWidget(line_tab);
	scroll_area->setWidgetResizable(true);
	scroll_area->setFrameShape(QFrame::NoFrame);
	scroll_area->setMinimumSize(min_size);
	addPropertiesGroup(tr("Line settings"), scroll_area);
	
	PointSymbolEditorWidget* point_symbol_editor = 0;
	MapEditorController* controller = dialog->getPreviewController();
	
	QList<PointSymbol*> point_symbols;
	point_symbols << symbol->getStartSymbol() << symbol->getMidSymbol() << symbol->getEndSymbol() << symbol->getDashSymbol();
	Q_FOREACH(PointSymbol* point_symbol, point_symbols)
	{
		point_symbol_editor = new PointSymbolEditorWidget(controller, point_symbol, 16);
		addPropertiesGroup(point_symbol->getName(), point_symbol_editor);
		connect(point_symbol_editor, SIGNAL(symbolEdited()), this, SLOT(pointSymbolEdited()));
	}
	
	connect(width_edit, SIGNAL(textEdited(QString)), this, SLOT(widthChanged(QString)));
	connect(color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(colorChanged()));
	connect(minimum_length_edit, SIGNAL(textEdited(QString)), this, SLOT(minimumDimensionsEdited(QString)));
	connect(line_cap_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(lineCapChanged(int)));
	connect(line_join_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(lineJoinChanged(int)));
	connect(pointed_cap_length_edit, SIGNAL(textEdited(QString)), this, SLOT(pointedLineCapLengthChanged(QString)));
	connect(dashed_check, SIGNAL(clicked(bool)), this, SLOT(dashedChanged(bool)));
	connect(segment_length_edit, SIGNAL(textEdited(QString)), this, SLOT(segmentLengthChanged(QString)));
	connect(end_length_edit, SIGNAL(textEdited(QString)), this, SLOT(endLengthChanged(QString)));
	connect(show_at_least_one_symbol_check, SIGNAL(clicked(bool)), this, SLOT(showAtLeastOneSymbolChanged(bool)));
	connect(minimum_mid_symbol_count_edit, SIGNAL(textEdited(QString)), this, SLOT(minimumDimensionsEdited(QString)));
	connect(minimum_mid_symbol_count_when_closed_edit, SIGNAL(textEdited(QString)), this, SLOT(minimumDimensionsEdited(QString)));
	connect(dash_length_edit, SIGNAL(textEdited(QString)), this, SLOT(dashLengthChanged(QString)));
	connect(break_length_edit, SIGNAL(textEdited(QString)), this, SLOT(breakLengthChanged(QString)));
	connect(dash_group_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(dashGroupsChanged(int)));
	connect(in_group_break_length_edit, SIGNAL(textEdited(QString)), this, SLOT(inGroupBreakLengthChanged(QString)));
	connect(half_outer_dashes_check, SIGNAL(clicked(bool)), this, SLOT(halfOuterDashesChanged(bool)));
	connect(mid_symbol_per_spot_edit, SIGNAL(textEdited(QString)), this, SLOT(midSymbolsPerDashChanged(QString)));
	connect(mid_symbol_distance_edit, SIGNAL(textEdited(QString)), this, SLOT(midSymbolDistanceChanged(QString)));
	connect(border_check, SIGNAL(clicked(bool)), this, SLOT(borderCheckClicked(bool)));
	connect(border_width_edit, SIGNAL(textEdited(QString)), this, SLOT(borderWidthEdited(QString)));
	connect(border_color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(borderColorChanged()));
	connect(border_shift_edit, SIGNAL(textEdited(QString)), this, SLOT(borderShiftChanged(QString)));
	connect(border_dashed_check, SIGNAL(clicked(bool)), this, SLOT(borderDashedClicked(bool)));
	connect(border_dash_length_edit, SIGNAL(textEdited(QString)), this, SLOT(borderDashesChanged(QString)));
	connect(border_break_length_edit, SIGNAL(textEdited(QString)), this, SLOT(borderDashesChanged(QString)));
}

LineSymbolSettings::~LineSymbolSettings()
{
	symbol->cleanupPointSymbols();
}

void LineSymbolSettings::pointSymbolEdited()
{
	dialog->updatePreview();
	updateWidgets();
}

void LineSymbolSettings::widthChanged(QString text)
{
	symbol->line_width = qRound(1000 * text.toFloat());
	dialog->updatePreview();
	updateWidgets();
}

void LineSymbolSettings::colorChanged()
{
	symbol->color = color_edit->color();
	dialog->updatePreview();
	updateWidgets();
}

void LineSymbolSettings::minimumDimensionsEdited(QString text)
{
	symbol->minimum_length = qRound(1000 * minimum_length_edit->text().toFloat());
	symbol->minimum_mid_symbol_count = qRound(1000 * minimum_mid_symbol_count_edit->text().toFloat());
	symbol->minimum_mid_symbol_count_when_closed = qRound(1000 * minimum_mid_symbol_count_when_closed_edit->text().toFloat());
}

void LineSymbolSettings::lineCapChanged(int index)
{
	symbol->cap_style = (LineSymbol::CapStyle)line_cap_combo->itemData(index).toInt();
	dialog->updatePreview();
	updateWidgets();
}

void LineSymbolSettings::lineJoinChanged(int index)
{
	symbol->join_style = (LineSymbol::JoinStyle)line_join_combo->itemData(index).toInt();
	dialog->updatePreview();
}

void LineSymbolSettings::pointedLineCapLengthChanged(QString text)
{
	symbol->pointed_cap_length = qRound(1000 * text.toFloat());
	dialog->updatePreview();
}

void LineSymbolSettings::dashedChanged(bool checked)
{
	symbol->dashed = checked;
	dialog->updatePreview();
	updateWidgets();
	if (checked)
		ensureWidgetVisible(half_outer_dashes_check);
}

void LineSymbolSettings::segmentLengthChanged(QString text)
{
	symbol->segment_length = qRound(1000 * text.toFloat());
	dialog->updatePreview();
}

void LineSymbolSettings::endLengthChanged(QString text)
{
	symbol->end_length = qRound(1000 * text.toFloat());
	dialog->updatePreview();
	updateWidgets();
}

void LineSymbolSettings::showAtLeastOneSymbolChanged(bool checked)
{
	symbol->show_at_least_one_symbol = checked;
	dialog->updatePreview();
}

void LineSymbolSettings::dashLengthChanged(QString text)
{
	symbol->dash_length = qRound(1000 * text.toFloat());
	dialog->updatePreview();
}

void LineSymbolSettings::breakLengthChanged(QString text)
{
	symbol->break_length = qRound(1000 * text.toFloat());
	dialog->updatePreview();
}

void LineSymbolSettings::dashGroupsChanged(int index)
{
	symbol->dashes_in_group = dash_group_combo->itemData(index).toInt();
	if (symbol->dashes_in_group > 1)
	{
		symbol->half_outer_dashes = false;
		half_outer_dashes_check->setChecked(false);
	}
	dialog->updatePreview();
	updateWidgets();
}

void LineSymbolSettings::inGroupBreakLengthChanged(QString text)
{
	symbol->in_group_break_length = qRound(1000 * text.toFloat());
	dialog->updatePreview();
}

void LineSymbolSettings::halfOuterDashesChanged(bool checked)
{
	symbol->half_outer_dashes = checked;
	dialog->updatePreview();
}

void LineSymbolSettings::midSymbolsPerDashChanged(QString text)
{
	symbol->mid_symbols_per_spot = qMax(1, text.toInt());
	dialog->updatePreview();
	updateWidgets();
}

void LineSymbolSettings::midSymbolDistanceChanged(QString text)
{
	symbol->mid_symbol_distance = qRound(1000 * text.toFloat());
	dialog->updatePreview();
}

void LineSymbolSettings::borderCheckClicked(bool checked)
{
	symbol->have_border_lines = checked;
	dialog->updatePreview();
	updateWidgets();
	if (checked)
		ensureWidgetVisible(border_break_length_edit);
}

void LineSymbolSettings::borderWidthEdited(QString text)
{
	symbol->border_width = qRound(1000 * text.toFloat());
	dialog->updatePreview();
}

void LineSymbolSettings::borderColorChanged()
{
	symbol->border_color = border_color_edit->color();
	dialog->updatePreview();
}

void LineSymbolSettings::borderShiftChanged(QString text)
{
	symbol->border_shift = qRound(1000 * text.toFloat());
	dialog->updatePreview();
}

void LineSymbolSettings::borderDashedClicked(bool checked)
{
	symbol->dashed_border = checked;
	dialog->updatePreview();
	updateWidgets();
	if (checked)
		ensureWidgetVisible(border_break_length_edit);
}

void LineSymbolSettings::borderDashesChanged(QString text)
{
	symbol->border_dash_length = qRound(1000 * border_dash_length_edit->text().toFloat());
	symbol->border_break_length = qRound(1000 * border_break_length_edit->text().toFloat());
	dialog->updatePreview();
}

void LineSymbolSettings::ensureWidgetVisible(QWidget* widget)
{
	widget_to_ensure_visible = widget;
	QTimer::singleShot(0, this, SLOT(ensureWidgetVisible()));
}

void LineSymbolSettings::ensureWidgetVisible()
{
	scroll_area->ensureWidgetVisible(widget_to_ensure_visible, 5, 5);
}

void LineSymbolSettings::updateWidgets()
{
	const bool symbol_active = symbol->line_width > 0;
	color_edit->setEnabled(symbol_active);
	
	const bool line_active = symbol_active && symbol->color != NULL;
	Q_FOREACH(QWidget* line_settings_widget, line_settings_list)
	{
		line_settings_widget->setEnabled(line_active);
	}
	if (line_active && symbol->cap_style != LineSymbol::PointedCap)
	{
		pointed_cap_length_label->setEnabled(false);
		pointed_cap_length_edit->setEnabled(false);
	}
	
	const bool line_dashed = symbol->dashed;
	if (line_dashed)
	{
		Q_FOREACH(QWidget* undashed_widget, undashed_widget_list)
		{
			undashed_widget->setVisible(false);
		}
		Q_FOREACH(QWidget* dashed_widget, dashed_widget_list)
		{
			dashed_widget->setVisible(true);
			dashed_widget->setEnabled(line_active);
		}
		in_group_break_length_label->setEnabled(line_active && symbol->dashes_in_group > 1);
		in_group_break_length_edit->setEnabled(line_active && symbol->dashes_in_group > 1);
		half_outer_dashes_check->setEnabled(line_active && symbol->dashes_in_group  == 1);
	}
	else
	{
		Q_FOREACH(QWidget* undashed_widget, undashed_widget_list)
		{
			undashed_widget->setVisible(true);
			undashed_widget->setEnabled(line_active && !symbol->mid_symbol->isEmpty());
		}
		show_at_least_one_symbol_check->setEnabled(show_at_least_one_symbol_check->isEnabled() && symbol->end_length > 0);
		Q_FOREACH(QWidget* dashed_widget, dashed_widget_list)
		{
			dashed_widget->setVisible(false);
		}
	}
	
	Q_FOREACH(QWidget* mid_symbol_widget, mid_symbol_widget_list)
	{
		mid_symbol_widget->setEnabled(!symbol->mid_symbol->isEmpty());
	}
	mid_symbol_distance_label->setEnabled(mid_symbol_distance_label->isEnabled() && symbol->mid_symbols_per_spot > 1);
	mid_symbol_distance_edit->setEnabled(mid_symbol_distance_edit->isEnabled() && symbol->mid_symbols_per_spot > 1);
	
	const bool border_active = symbol_active && symbol->have_border_lines;
	Q_FOREACH(QWidget* border_widget, border_widget_list)
	{
		border_widget->setVisible(border_active);
		border_widget->setEnabled(border_active);
	}
	Q_FOREACH(QWidget* border_dash_widget, border_dash_widget_list)
	{
		border_dash_widget->setVisible(border_active);
		border_dash_widget->setEnabled(border_active && symbol->dashed_border);
	}
}

#include "symbol_line.moc"
