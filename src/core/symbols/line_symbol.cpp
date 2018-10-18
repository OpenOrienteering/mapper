/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2018 Kai Pastor
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


#include "line_symbol.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <utility>

#include <QtNumeric>
#include <QCoreApplication>
#include <QLatin1String>
#include <QStringRef>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <private/qbezier_p.h>

#include "core/map.h"
#include "core/map_color.h"
#include "core/map_coord.h"
#include "core/path_coord.h"
#include "core/objects/object.h"
#include "core/renderables/renderable.h"
#include "core/renderables/renderable_implementation.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"
#include "core/virtual_coord_vector.h"
#include "core/virtual_path.h"
#include "util/xml_stream_util.h"

namespace OpenOrienteering {

using length_type = PathCoord::length_type;


// ### LineSymbolBorder ###

void LineSymbolBorder::save(QXmlStreamWriter& xml, const Map& map) const
{
	XmlElementWriter element(xml, QLatin1String("border"));
	element.writeAttribute(QLatin1String("color"), map.findColorIndex(color));
	element.writeAttribute(QLatin1String("width"), width);
	element.writeAttribute(QLatin1String("shift"), shift);
	if (dashed)
	{
		element.writeAttribute(QLatin1String("dashed"), true);
		element.writeAttribute(QLatin1String("dash_length"), dash_length);
		element.writeAttribute(QLatin1String("break_length"), break_length);
	}
}

bool LineSymbolBorder::load(QXmlStreamReader& xml, const Map& map)
{
	Q_ASSERT(xml.name() == QLatin1String("border"));
	XmlElementReader element(xml);
	color = map.getColor(element.attribute<int>(QLatin1String("color")));
	width = element.attribute<int>(QLatin1String("width"));
	shift = element.attribute<int>(QLatin1String("shift"));
	dashed = element.attribute<bool>(QLatin1String("dashed"));
	if (dashed)
	{
		dash_length = element.attribute<int>(QLatin1String("dash_length"));
		break_length = element.attribute<int>(QLatin1String("break_length"));
	}
	return !xml.error();
}


bool LineSymbolBorder::isVisible() const
{
	return width > 0 && color && !(dashed && dash_length == 0);
}

void LineSymbolBorder::setupSymbol(LineSymbol& out) const
{
	out.setLineWidth(0.001 * width);
	out.setColor(color);
	
	if (dashed)
	{
		out.setDashed(true);
		out.setDashLength(dash_length);
		out.setBreakLength(break_length);
	}
}

void LineSymbolBorder::scale(double factor)
{
	width = qRound(factor * width);
	shift = qRound(factor * shift);
	dash_length = qRound(factor * dash_length);
	break_length = qRound(factor * break_length);
}


bool operator==(const LineSymbolBorder& lhs, const LineSymbolBorder& rhs) noexcept
{
	return  ((!lhs.color && !rhs.color)
	         || (lhs.color && rhs.color && *lhs.color == *rhs.color))
	        && lhs.width == rhs.width
	        && lhs.shift == rhs.shift
	        && lhs.dashed == rhs.dashed
	        && (!lhs.dashed
	            || (lhs.dash_length == rhs.dash_length
	                && lhs.break_length == rhs.break_length));
}



// ### LineSymbol ###

LineSymbol::LineSymbol() noexcept
: Symbol ( Symbol::Line )
, start_symbol ( nullptr )
, mid_symbol ( nullptr )
, end_symbol ( nullptr )
, dash_symbol ( nullptr )
, color ( nullptr )
, line_width ( 0 )
, minimum_length ( 0 )
, start_offset ( 0 )
, end_offset ( 0 )
, mid_symbols_per_spot ( 1 )
, mid_symbol_distance ( 0 )
, minimum_mid_symbol_count ( 0 )
, minimum_mid_symbol_count_when_closed ( 0 )
, segment_length ( 4000 )
, end_length ( 0 )
, dash_length ( 4000 )
, break_length ( 1000 )
, dashes_in_group ( 1 )
, in_group_break_length ( 500 )
, cap_style ( FlatCap )
, join_style ( MiterJoin )
, mid_symbol_placement ( CenterOfDash )
, dashed ( false )
, half_outer_dashes ( false )
, show_at_least_one_symbol ( true )
, suppress_dash_symbol_at_ends ( false )
, scale_dash_symbol ( true )
, have_border_lines ( false )
{
	// nothing else
}


LineSymbol::LineSymbol(const LineSymbol& proto)
: Symbol ( proto )
, border ( proto.border )
, right_border ( proto.right_border )
, start_symbol ( proto.start_symbol ? Symbol::duplicate(*proto.start_symbol).release() : nullptr )
, mid_symbol ( proto.mid_symbol ? Symbol::duplicate(*proto.mid_symbol).release() : nullptr )
, end_symbol ( proto.end_symbol ? Symbol::duplicate(*proto.end_symbol).release() : nullptr )
, dash_symbol ( proto.dash_symbol ? Symbol::duplicate(*proto.dash_symbol).release() : nullptr )
, color ( proto.color )
, line_width ( proto.line_width )
, minimum_length ( proto.minimum_length )
, start_offset ( proto.start_offset )
, end_offset ( proto.end_offset )
, mid_symbols_per_spot ( proto.mid_symbols_per_spot )
, mid_symbol_distance ( proto.mid_symbol_distance )
, minimum_mid_symbol_count ( proto.minimum_mid_symbol_count )
, minimum_mid_symbol_count_when_closed ( proto.minimum_mid_symbol_count_when_closed )
, segment_length ( proto.segment_length )
, end_length ( proto.end_length )
, dash_length ( proto.dash_length )
, break_length ( proto.break_length )
, dashes_in_group ( proto.dashes_in_group )
, in_group_break_length ( proto.in_group_break_length )
, cap_style ( proto.cap_style )
, join_style ( proto.join_style )
, mid_symbol_placement ( proto.mid_symbol_placement )
, dashed ( proto.dashed )
, half_outer_dashes ( proto.half_outer_dashes )
, show_at_least_one_symbol ( proto.show_at_least_one_symbol )
, suppress_dash_symbol_at_ends ( proto.suppress_dash_symbol_at_ends )
, scale_dash_symbol ( proto.scale_dash_symbol )
, have_border_lines ( proto.have_border_lines )
{
	// nothing else
}


LineSymbol::~LineSymbol()
{
	delete start_symbol;
	delete mid_symbol;
	delete end_symbol;
	delete dash_symbol;
}



LineSymbol* LineSymbol::duplicate() const
{
	return new LineSymbol(*this);
}



bool LineSymbol::validate() const
{
	using std::begin;
	using std::end;
	using MemberSymbol = PointSymbol* LineSymbol::*;
	MemberSymbol members[4] = { &LineSymbol::start_symbol, &LineSymbol::mid_symbol, &LineSymbol::end_symbol, &LineSymbol::dash_symbol };
	return std::all_of(begin(members), end(members), [this](auto& member) {
		auto sub_symbol = this->*member;
		return !sub_symbol || !sub_symbol->isEmpty() || sub_symbol->validate();
	});
}



void LineSymbol::createRenderables(
        const Object* /*object*/,
        const VirtualCoordVector& coords,
        ObjectRenderables& output,
        RenderableOptions /*options*/) const
{
	const auto path_parts = PathPart::calculatePathParts(coords);
	createStartEndSymbolRenderables(path_parts, output);
	for (const auto& part : path_parts)
	{
		createSinglePathRenderables(part, part.isClosed(), output);
	}
}

void LineSymbol::createRenderables(
        const PathObject* object,
        const PathPartVector& path_parts,
        ObjectRenderables& output,
        RenderableOptions options ) const
{
	if (options.testFlag(Symbol::RenderBaselines))
	{
		createBaselineRenderables(object, path_parts, output, guessDominantColor());
	}
	else
	{
		createStartEndSymbolRenderables(path_parts, output);
		for (const auto& part : path_parts)
		{
			createSinglePathRenderables(part, part.isClosed(), output);
		}
	}
}


void LineSymbol::createSinglePathRenderables(const VirtualPath& path, bool path_closed, ObjectRenderables& output) const
{
	if (path.size() < 2)
		return;
	
	// Dash symbols?
	if (dash_symbol && !dash_symbol->isEmpty())
	{
		createDashSymbolRenderables(path, path_closed, output);
	}
	
	auto create_line = color && line_width > 0;
	auto create_border = have_border_lines && (border.isVisible() || right_border.isVisible());
	auto use_offset = !path.isClosed() && (start_offset > 0 || end_offset > 0);
	if (!use_offset && !dashed)
	{
		// This is a simple plain line (no pointed line ends, no dashes).
		// It may be drawn directly from the given path.
		if (create_line)
			output.insertRenderable(new LineRenderable(this, path, path_closed));
		
		auto create_mid_symbols = mid_symbol && !mid_symbol->isEmpty() && segment_length > 0;
		if (create_mid_symbols || create_border)
		{
			auto start = SplitPathCoord::begin(path.path_coords);
			auto end   = SplitPathCoord::end(path.path_coords);
			
			if (create_mid_symbols)
				createMidSymbolRenderables(path, start, end, path_closed, output);
			
			if (create_border)
				createBorderLines(path, start, end, output);
		}
		
		return;
	}
	
	// This is a non-trivial line symbol.
	// First we preprocess coordinates and handle mid symbols.
	// Later we create the line renderable and border.
	auto start = SplitPathCoord::begin(path.path_coords);
	auto end   = SplitPathCoord::end(path.path_coords);
	
	if (use_offset)
	{
		// Create offset at line ends, or pointed line ends
		auto effective_start_len = start.clen;
		auto effective_end_len = end.clen;
		{
			auto effective_start_offset = length_type(0.001) * std::max(0, start_offset);
			auto effective_end_offset = length_type(0.001) * std::max(0, end_offset);
			auto total_offset = effective_start_offset + effective_end_offset;
			auto available_length = std::max(length_type(0), end.clen - start.clen);
			
			if (total_offset > available_length)
			{
				create_line = false;
				effective_start_offset *= available_length / total_offset;
				effective_end_offset *= available_length / total_offset;
			}
			effective_start_len += effective_start_offset;
			effective_end_len -= effective_end_offset;
			// Safeguard against floating point quirks
			effective_end_len = std::max(effective_start_len, effective_end_len);
		}
		if (start_offset > 0)
		{
			auto split = SplitPathCoord::at(effective_start_len, start);
			if (cap_style == PointedCap)
			{
				createPointedLineCap(path, start, split, false, output);
			}
			start = split;
		}
		if (end_offset > 0)
		{
			auto split = SplitPathCoord::at(effective_end_len, start);
			if (cap_style == PointedCap)
			{
				// Get updated curve end
				if (end.is_curve_end)
					end = SplitPathCoord::at(end.clen, split);
				createPointedLineCap(path, split, end, true, output);
			}
			end = split;
		}
		if (!create_line)
		{
			return;
		}
	}
	
	MapCoordVector processed_flags;
	MapCoordVectorF processed_coords;
	if (dashed)
	{
		// Dashed lines
		if (dash_length > 0)
			processDashedLine(path, start, end, path_closed, processed_flags, processed_coords, output);
	}
	else
	{
		// Symbols?
		if (mid_symbol && !mid_symbol->isEmpty() && segment_length > 0)
			createMidSymbolRenderables(path, start, end, path_closed, output);
		
		if (line_width > 0)
			processContinuousLine(path, start, end, false,
			                      processed_flags, processed_coords, output);
	}
	
	Q_ASSERT(processed_coords.size() != 1);
	
	if ((create_line || create_border) && processed_coords.size() > 1)
	{
		VirtualPath processed_path = { processed_flags, processed_coords };
		processed_path.path_coords.update(path.first_index);
		if (create_line)
		{
			output.insertRenderable(new LineRenderable(this, processed_path, path_closed));
		}
		if (create_border)
		{
			const auto processed_start = SplitPathCoord::begin(processed_path.path_coords);
			const auto processed_end = SplitPathCoord::end(processed_path.path_coords);
			createBorderLines(processed_path, processed_start, processed_end, output);
		}
	}
}


void LineSymbol::createBorderLines(
        const VirtualPath& path,
        const SplitPathCoord& start,
        const SplitPathCoord& end,
        ObjectRenderables& output) const
{
	const auto main_shift = 0.0005 * line_width;
	const auto path_closed = path.isClosed();
	
	MapCoordVector border_flags;
	MapCoordVectorF border_coords;
	
	auto border_dashed = false;
	// The following variables are needed for dashed borders only, but default
	// initialization should be cheap enough so that we can always create them
	// on the stack for easy sharing of state between symmetrical dashed borders,
	// and even for reusing allocated memory in case of different dash patterns.
	MapCoordVector dashed_flags;
	MapCoordVectorF dashed_coords;
	
	LineSymbol border_symbol;
	border_symbol.setJoinStyle(join_style == RoundJoin ? RoundJoin : MiterJoin);
	
	if (border.isVisible())
	{
		border.setupSymbol(border_symbol);
		border_dashed = border.dashed && border.dash_length > 0 && border.break_length > 0;
		if (border_dashed)
		{
			// Left border is dashed
			border_symbol.processDashedLine(path, start, end, path_closed, dashed_flags, dashed_coords, output);
			border_symbol.dashed = false;	// important, otherwise more dashes might be added by createRenderables()!
			shiftCoordinates({dashed_flags, dashed_coords}, -main_shift, border_flags, border_coords);
		}
		else
		{
			// Solid left border
			shiftCoordinates(path, -main_shift, border_flags, border_coords);
		}
		auto border_path = VirtualPath{border_flags, border_coords};
		auto last = border_path.path_coords.update(0);
		Q_ASSERT(last+1 == border_coords.size()); Q_UNUSED(last);
		output.insertRenderable(new LineRenderable(&border_symbol, border_path, path_closed));
	}
		
	if (right_border.isVisible())
	{
		right_border.setupSymbol(border_symbol);
		auto right_border_dashed = right_border.dashed && right_border.dash_length > 0 && right_border.break_length > 0;
		if (right_border_dashed)
		{
			if (areBordersDifferent()
			    && (!border_dashed
			        || (border.dash_length != right_border.dash_length
			            || border.break_length != right_border.break_length)))
			{
				// Right border is dashed, but different from left border
				dashed_flags.clear();
				dashed_coords.clear();
				border_symbol.processDashedLine(path, start, end, path_closed, dashed_flags, dashed_coords, output);
			}
			border_symbol.dashed = false;	// important, otherwise more dashes might be added by createRenderables()!
			shiftCoordinates({dashed_flags, dashed_coords}, main_shift, border_flags, border_coords);
		}
		else
		{
			// Solid right border
			shiftCoordinates(path, main_shift, border_flags, border_coords);
		}
		auto border_path = VirtualPath{border_flags, border_coords};
		auto last = border_path.path_coords.update(0);
		Q_ASSERT(last+1 == border_coords.size()); Q_UNUSED(last);
		output.insertRenderable(new LineRenderable(&border_symbol, border_path, path_closed));
	}
}


void LineSymbol::shiftCoordinates(const VirtualPath& path, double main_shift, MapCoordVector& out_flags, MapCoordVectorF& out_coords) const
{
	const float curve_threshold = 0.03f;	// TODO: decrease for export/print?
	const int MAX_OFFSET = 16;
	QBezier offsetCurves[MAX_OFFSET];
	
	double miter_limit = 2.0 * miterLimit(); // needed more than once
	if (miter_limit <= 0.0)
		miter_limit = 1.0e6;                 // Q_ASSERT(miter_limit != 0)
	double miter_reference = 0.0;            // reference value: 
	if (join_style == MiterJoin)             //  when to bevel MiterJoins
		miter_reference = cos(atan(4.0 / miter_limit));
	
	// sign of shift and main shift indicates left or right border
	// but u_border_shift is unsigned
	double u_border_shift = 0.001 * ((main_shift > 0.0 && areBordersDifferent()) ? right_border.shift : border.shift);
	double shift = main_shift + ((main_shift > 0.0) ? u_border_shift : -u_border_shift);
	
	auto size = path.size();
	out_flags.clear();
	out_coords.clear();
	out_flags.reserve(size);
	out_coords.reserve(size);
	
	const MapCoord no_flags;
	
	bool ok_in, ok_out;
	MapCoordF segment_start, 
	          right_vector, end_right_vector,
	          vector_in, vector_out,
	          tangent_in, tangent_out, 
	          middle0, middle1;
	auto last_i = path.last_index;
	for (auto i = path.first_index; i < size; ++i)
	{
		auto coords_i = path.coords[i];
		const auto& flags_i  = path.coords.flags[i];

		vector_in  = path.calculateIncomingTangent(i, ok_in);
		vector_out = path.calculateOutgoingTangent(i, ok_out);
		
		if (!ok_in)
		{
			vector_in = vector_out;
			ok_in = ok_out;
		}
		if (ok_in)
		{
			tangent_in = vector_in;
			tangent_in.normalize();
		}
		
		if (!ok_out)
		{
			vector_out = vector_in;
			ok_out = ok_in;
		}
		if (ok_out)
		{
			tangent_out = vector_out;
			tangent_out.normalize();
		}
		
		if (!ok_in && !ok_out)
		{
			// Rare but existing case.  No valid solution, but
			// at least we need to output a point to handle the flags correctly.
			//qDebug("No valid tangent");
			segment_start = coords_i;
		}
		else if (i == 0 && !path.isClosed())
		{
			// Simple start point
			right_vector = tangent_out.perpRight();
			segment_start = coords_i + shift * right_vector;
		}
		else if (i == last_i && !path.isClosed())
		{
			// Simple end point
			right_vector = tangent_in.perpRight();
			segment_start = coords_i + shift * right_vector;
		}
		else
		{
			// Corner point
			right_vector = tangent_out.perpRight();
			
			middle0 = tangent_in + tangent_out;
			middle0.normalize();
			double offset;
				
			// Determine type of corner (inner vs. outer side of corner)
			double a = (tangent_out.x() * tangent_in.y() - tangent_in.x() * tangent_out.y()) * main_shift;
			if (a > 0.0)
			{
				// Outer side of corner
				
				if (join_style == BevelJoin || join_style == RoundJoin)
				{
					middle1 = tangent_in + middle0;
					middle1.normalize();
					double phi1 = acos(MapCoordF::dotProduct(middle1, tangent_in));
					offset = tan(phi1) * u_border_shift;
					
					if (i > 0 && !qIsNaN(offset))
					{
						// First border corner point
						end_right_vector = tangent_in.perpRight();
						out_flags.push_back(no_flags);
						out_coords.push_back(coords_i + shift * end_right_vector + offset * tangent_in);
						
						if (join_style == RoundJoin)
						{
							// Extra border corner point
							// TODO: better approximation of round corner / use bezier curve
							out_flags.push_back(no_flags);
							out_coords.push_back(coords_i + shift * middle0.perpRight());
						}
					}
				}
				else /* join_style == MiterJoin */
				{
					// miter_check has no concrete interpretation, 
					// but was derived by mathematical simplifications.
					double miter_check = MapCoordF::dotProduct(middle0, tangent_in);
					if (miter_check <= miter_reference)
					{
						// Two border corner points
						middle1 = tangent_in + middle0;
						middle1.normalize();
						double phi1 = acos(MapCoordF::dotProduct(middle1, tangent_in));
						offset = miter_limit * fabs(main_shift) + tan(phi1) * u_border_shift;
						
						if (i > 0 && !qIsNaN(offset))
						{
							// First border corner point
							end_right_vector = tangent_in.perpRight();
							out_flags.push_back(no_flags);
							out_coords.push_back(coords_i + shift * end_right_vector + offset * tangent_in);
						}
					}
					else
					{
						double phi = acos(MapCoordF::dotProduct(middle0.perpRight(), tangent_in));
						offset = fabs(1.0/tan(phi) * shift);
					}
				}
				
				if (qIsNaN(offset))
				{
					offset = 0.0;
				}
				
				// single or second border corner point
				segment_start = coords_i + shift * right_vector - offset * tangent_out;
			}
			else if (i > 2 && path.coords.flags[i-3].isCurveStart() && flags_i.isCurveStart())
			{
				// Inner side of corner (or no corner), and both sides are beziers
				// old behaviour
				right_vector = middle0.perpRight();
				double phi = acos(MapCoordF::dotProduct(right_vector, tangent_in));
				double sin_phi = sin(phi);
				double inset = (sin_phi > (1.0/miter_limit)) ? (1.0 / sin_phi) : miter_limit;
				segment_start = coords_i + (shift * inset) * right_vector;
			}
			else
			{
				// Inner side of corner (or no corner), and no more than on bezier involved
				
				// Default solution
				double phi = acos(MapCoordF::dotProduct(middle0.perpRight(), tangent_in));
				double tan_phi = tan(phi);
				offset = -fabs(shift/tan_phi);
				
				if (tan_phi >= 1.0)
				{
					segment_start = coords_i + shift * right_vector - offset * tangent_out;
				}
				else
				{
					// Critical case
					double len_in  = vector_in.length();
					double len_out = vector_out.length();
					a = qIsNaN(offset) ? 0.0 : (fabs(offset) - qMin(len_in, len_out));
						
					if (a < -0.0)
					{
						// Offset acceptable
						segment_start = coords_i + shift * right_vector - offset * tangent_out;
						
#ifdef MAPPER_FILL_MITER_LIMIT_GAP
						// Avoid miter limit effects by extra points
						// This is be nice for ISOM roads, but not for powerlines...
						// TODO: add another flag to the signature?
						out_flags.push_back(no_flags);
						out_coords.push_back(coords_i + shift * right_vector - offset * tangent_out);
						
						out_flags.push_back(no_flags);
						out_coords.push_back(out_coords.back() - (shift - main_shift) * qMax(0.0, 1.0 / sin(phi) - miter_limit) * middle0);
						
						// single or second border corner point
						segment_start = coords_i + shift * right_vector - offset * tangent_out;
#endif
					}
					else
					{
						// Default solution is too far off from main path
						if (len_in < len_out)
						{
							segment_start = coords_i + shift * right_vector + len_in * tangent_out;
						}
						else
						{
							right_vector = tangent_in.perpRight();
							segment_start = coords_i + shift * right_vector - len_out * tangent_in;
						}
					}
				}
			}
		}
		
		out_flags.emplace_back(flags_i);
		out_flags.back().setCurveStart(false); // Must not be set if bezier.shifted() fails!
		out_coords.emplace_back(segment_start);
		
		if (flags_i.isCurveStart())
		{
			Q_ASSERT(i+2 < path.last_index);
			
			// Use QBezierCopy code to shift the curve, but set start and end point manually to get the correct end points (because of line joins)
			// TODO: it may be necessary to remove some of the generated curves in the case an outer point is moved inwards
			if (main_shift > 0.0)
			{
				QBezier bezier = QBezier::fromPoints(path.coords[i+3], path.coords[i+2], path.coords[i+1], coords_i);
				auto count = bezier.shifted(offsetCurves, MAX_OFFSET, qAbs(shift), curve_threshold);
				for (auto j = count - 1; j >= 0; --j)
				{
					out_flags.back().setCurveStart(true);
					
					out_flags.emplace_back();
					out_coords.emplace_back(offsetCurves[j].pt3());
					
					out_flags.emplace_back();
					out_coords.emplace_back(offsetCurves[j].pt2());
					
					if (j > 0)
					{
						out_flags.emplace_back();
						out_coords.emplace_back(offsetCurves[j].pt1());
					}
				}
			}
			else
			{
				QBezier bezier = QBezier::fromPoints(path.coords[i], path.coords[i+1], path.coords[i+2], path.coords[i+3]);
				int count = bezier.shifted(offsetCurves, MAX_OFFSET, qAbs(shift), curve_threshold);
				for (int j = 0; j < count; ++j)
				{
					out_flags.back().setCurveStart(true);
					
					out_flags.emplace_back();
					out_coords.emplace_back(offsetCurves[j].pt2());
					
					out_flags.emplace_back();
					out_coords.emplace_back(offsetCurves[j].pt3());
					
					if (j < count - 1)
					{
						out_flags.emplace_back();
						out_coords.emplace_back(offsetCurves[j].pt4());
					}
				}
			}
			
			i += 2;
		}
	}
}

void LineSymbol::processContinuousLine(
        const VirtualPath& path,
        const SplitPathCoord& start,
        const SplitPathCoord& end,
        bool set_mid_symbols,
        MapCoordVector& processed_flags,
        MapCoordVectorF& processed_coords,
        ObjectRenderables& output ) const
{
	auto mid_symbol_distance_f = length_type(0.001) * mid_symbol_distance;
	auto mid_symbols_length   = (qMax(1, mid_symbols_per_spot) - 1) * mid_symbol_distance_f;
	auto effective_end_length = end.clen;
	
	auto split = start;
	
	set_mid_symbols = set_mid_symbols && mid_symbol && !mid_symbol->isEmpty() && mid_symbols_per_spot;
	if (set_mid_symbols && mid_symbols_length <= effective_end_length - split.clen)
	{
		auto mid_position = (split.clen + effective_end_length - mid_symbols_length) / 2;
		auto next_split = SplitPathCoord::at(mid_position, split);
		path.copy(split, next_split, processed_flags, processed_coords);
		split = next_split;
		
		auto orientation = qreal(0);
		bool mid_symbol_rotatable = bool(mid_symbol) && mid_symbol->isRotatable();
		for (auto i = mid_symbols_per_spot; i > 0; --i)
		{
			if (mid_symbol_rotatable)
				orientation = split.tangentVector().angle();
			mid_symbol->createRenderablesScaled(split.pos, orientation, output);
			
			if (i > 1)
			{
				mid_position += mid_symbol_distance_f;
				next_split = SplitPathCoord::at(mid_position, split);
				path.copy(split, next_split, processed_flags, processed_coords);
				split = next_split;
			}
		}
	}
	
	path.copy(split, end, processed_flags, processed_coords);
	
	processed_flags.back().setGapPoint(true);
	Q_ASSERT(!processed_flags[processed_flags.size()-2].isCurveStart());
}

void LineSymbol::createPointedLineCap(
        const VirtualPath& path,
        const SplitPathCoord& start,
        const SplitPathCoord& end,
        bool is_end,
        ObjectRenderables& output ) const
{
	AreaSymbol area_symbol;
	area_symbol.setColor(color);
	
	auto line_half_width = length_type(0.0005) * line_width;
	auto cap_length = length_type(0.001) * (is_end ? end_offset : start_offset);
	auto tan_angle = qreal(line_half_width / cap_length);
	
	MapCoordVector cap_flags;
	MapCoordVectorF cap_middle_coords;
	path.copy(start, end, cap_flags, cap_middle_coords);
	
	auto cap_size = VirtualPath::size_type(cap_middle_coords.size());
	auto cap_middle_path = VirtualPath { cap_flags, cap_middle_coords };
	
	std::vector<float> cap_lengths;
	cap_lengths.reserve(cap_size);
	path.copyLengths(start, end, cap_lengths);
	Q_ASSERT(cap_middle_coords.size() == cap_lengths.size());
	
	// Calculate coordinates on the left and right side of the line
	MapCoordVectorF cap_coords;
	MapCoordVectorF cap_coords_left;
	auto sign = is_end ? -1 : 1;
	for (MapCoordVectorF::size_type i = 0; i < cap_size; ++i)
	{
		auto dist_from_start = is_end ? (end.clen - cap_lengths[i]) : (cap_lengths[i] - start.clen);
		auto factor = dist_from_start / cap_length;
		//Q_ASSERT(factor >= -0.01f && factor <= 1.01f); happens when using large break lengths as these are not adjusted
		factor = qBound(length_type(0), factor, length_type(1));
		
		auto tangent_info = cap_middle_path.calculateTangentScaling(i);
		auto right_vector = tangent_info.first.perpRight();
		right_vector.normalize();
		
		auto radius = qBound(qreal(0), qreal(line_half_width * factor) * tangent_info.second, qreal(line_half_width) * 2);
		
		cap_flags[i].setHolePoint(false);
		cap_coords.emplace_back(cap_middle_coords[i] + radius * right_vector);
		cap_coords_left.emplace_back(cap_middle_coords[i] - radius * right_vector);
		
		// Control points for bezier curves
		if (i >= 3 && cap_flags[i-3].isCurveStart())
		{
			cap_coords.emplace_back(cap_coords.back());
			cap_coords_left.emplace_back(cap_coords_left.back());
			
			MapCoordF tangent = cap_middle_coords[i] - cap_middle_coords[i-1];
			Q_ASSERT(tangent.lengthSquared() < 999*999);
			auto right_scale = tangent.length() * tan_angle * sign;
			cap_coords[i-1] = cap_coords[i] - tangent - right_vector * right_scale;
			cap_coords_left[i-1] = cap_coords_left[i] - tangent + right_vector * right_scale;
		}
		if (cap_flags[i].isCurveStart())
		{
			// TODO: Tangent scaling depending on curvature? Adaptive subdivision of the curves?
			MapCoordF tangent = cap_middle_coords[i+1] - cap_middle_coords[i];
			Q_ASSERT(tangent.lengthSquared() < 999*999);
			auto right_scale = tangent.length() * tan_angle * sign;
			cap_coords.emplace_back(cap_coords[i] + tangent + right_vector * right_scale);
			cap_coords_left.emplace_back(cap_coords_left[i] + tangent - right_vector * right_scale);
			i += 2;
		}
	}
	
	// Create small overlap to avoid visual glitches with the on-screen display (should not matter for printing, could probably turn it off for this)
	constexpr auto overlap_length = length_type(0.05);
	if ((end.clen - start.clen) > 4 * overlap_length)
	{
		bool ok;
		auto end_pos = is_end ? 0 : (cap_size - 1);
		auto end_cap_pos = is_end ? 0 : (cap_coords.size() - 1);
		MapCoordF tangent = cap_middle_path.calculateTangent(end_pos, !is_end, ok);
		if (ok)
		{
			tangent.setLength(qreal(overlap_length * sign));
			auto right = MapCoordF{ is_end ? tangent : -tangent }.perpRight();
			
			MapCoordF shifted_coord = cap_coords[end_cap_pos];
			shifted_coord += tangent + right;
			
			MapCoordF shifted_coord_left = cap_coords_left[end_cap_pos];
			shifted_coord_left += tangent - right;
			
			if (is_end)
			{
				cap_flags.insert(cap_flags.begin(), MapCoord());
				cap_coords.insert(cap_coords.begin(), shifted_coord);
				cap_coords_left.insert(cap_coords_left.begin(), shifted_coord_left);
			}
			else
			{
				cap_flags.emplace_back();
				cap_coords.emplace_back(shifted_coord);
				cap_coords_left.emplace_back(shifted_coord_left);
			}
		}
	}
	
	// Concatenate left and right side coordinates
	cap_flags.reserve(2 * cap_flags.size());
	cap_coords.reserve(2 * cap_coords.size());
	
	MapCoord curve_start;
	curve_start.setCurveStart(true);
	if (!is_end)
	{
		for (auto i = cap_coords_left.size(); i > 0; )
		{
			--i;
			if (i >= 3 && cap_flags[i-3].isCurveStart())
			{
				cap_flags.emplace_back(curve_start);
				cap_flags.emplace_back();
				cap_flags.emplace_back();
				cap_coords.emplace_back(cap_coords_left[i]);
				cap_coords.emplace_back(cap_coords_left[i-1]);
				cap_coords.emplace_back(cap_coords_left[i-2]);
				i -= 2;
			}
			else
			{
				cap_flags.emplace_back();
				cap_coords.emplace_back(cap_coords_left[i]);
			}
		}
	}
	else
	{
		for (auto i = cap_coords_left.size() - 1; i > 0; )
		{
			--i;
			if (i >= 3 && cap_flags[i - 3].isCurveStart())
			{
				cap_flags.emplace_back(curve_start);
				cap_flags.emplace_back();
				cap_flags.emplace_back();
				cap_coords.emplace_back(cap_coords_left[i]);
				cap_coords.emplace_back(cap_coords_left[i-1]);
				cap_coords.emplace_back(cap_coords_left[i-2]);
				i -= 2;
			}
			else if (i >= 2 && i == cap_coords_left.size() - 2 && cap_flags[i - 2].isCurveStart())
			{
				cap_flags[cap_flags.size() - 1].setCurveStart(true);
				cap_flags.emplace_back();
				cap_flags.emplace_back();
				cap_coords.emplace_back(cap_coords_left[i]);
				cap_coords.emplace_back(cap_coords_left[i-1]);
				i -= 1;
			}
			else
			{
				cap_flags.emplace_back();
				cap_coords.emplace_back(cap_coords_left[i]);
			}
		}
	}
	
	// Add renderable
	Q_ASSERT(cap_coords.size() >= 3);
	Q_ASSERT(cap_coords.size() == cap_flags.size());
	
	VirtualPath cap_path { cap_flags, cap_coords };
	cap_path.path_coords.update(0);
	output.insertRenderable(new AreaRenderable(&area_symbol, cap_path));
}

void LineSymbol::processDashedLine(
        const VirtualPath& path,
        const SplitPathCoord& start,
        const SplitPathCoord& end,
        bool path_closed,
        MapCoordVector& out_flags,
        MapCoordVectorF& out_coords,
        ObjectRenderables& output ) const
{
	auto& path_coords = path.path_coords;
	Q_ASSERT(!path_coords.empty());
	
	auto out_coords_size = path.size() * 4;
	out_flags.reserve(out_coords_size);
	out_coords.reserve(out_coords_size);
	
	auto groups_start = start;
	auto line_start   = groups_start;
	for (auto is_part_start = true, is_part_end = false; !is_part_end; is_part_start = false)
	{
		auto groups_end_path_coord_index = path_coords.findNextDashPoint(groups_start.path_coord_index);
		is_part_end = path.path_coords[groups_end_path_coord_index].clen >= end.clen;
		auto groups_end = is_part_end ? end : SplitPathCoord::at(path_coords, groups_end_path_coord_index);
		
		line_start = createDashGroups(path, path_closed,
		                              line_start, groups_start, groups_end,
		                              is_part_start, is_part_end,
		                              out_flags, out_coords, output);
		
		groups_start = groups_end; // Search then next split (node) after groups_end (current node).
	}
	Q_ASSERT(line_start.clen == groups_start.clen);
}

SplitPathCoord LineSymbol::createDashGroups(
        const VirtualPath& path,
        bool path_closed,
        const SplitPathCoord& line_start,
        const SplitPathCoord& start,
        const SplitPathCoord& end,
        bool is_part_start,
        bool is_part_end,
        MapCoordVector& out_flags,
        MapCoordVectorF& out_coords,
        ObjectRenderables& output ) const
{
	auto& flags = path.coords.flags;
	auto& path_coords = path.path_coords;
	
	auto orientation = qreal(0);
	bool mid_symbol_rotatable = bool(mid_symbol) && mid_symbol->isRotatable();
	
	const auto mid_symbols = (mid_symbols_per_spot > 0 && mid_symbol && !mid_symbol->isEmpty()) ? mid_symbol_placement : LineSymbol::NoMidSymbols;
	auto mid_symbol_distance_f = length_type(0.001) * mid_symbol_distance;
	
	bool half_first_group = is_part_start ? (half_outer_dashes || path_closed)
	                                      : (flags[start.index].isDashPoint() && dashes_in_group == 1);
	
	bool ends_with_dashpoint  = is_part_end ? path_closed : true;
	
	auto dash_length_f           = length_type(0.001) * dash_length;
	auto break_length_f          = length_type(0.001) * break_length;
	auto in_group_break_length_f = length_type(0.001) * in_group_break_length;
	
	auto total_in_group_dash_length  = dashes_in_group * dash_length_f;
	auto total_in_group_break_length = (dashes_in_group - 1) * in_group_break_length_f;
	auto total_group_length = total_in_group_dash_length + total_in_group_break_length;
	auto total_group_and_break_length = total_group_length + break_length_f;
	
	auto length = end.clen - start.clen;
	auto length_plus_break = length + break_length_f;
	
	bool half_last_group  = is_part_end ? (half_outer_dashes || path_closed)
	                                    : (ends_with_dashpoint && dashes_in_group == 1);
	int num_half_groups = (half_first_group ? 1 : 0) + (half_last_group ? 1 : 0);
	
	auto num_dashgroups_f = num_half_groups +
	                        (length_plus_break - num_half_groups * dash_length_f / 2) / total_group_and_break_length;

	int lower_dashgroup_count = int(std::floor(num_dashgroups_f));
	
	auto minimum_optimum_num_dashes = dashes_in_group * 2 - num_half_groups / 2;
	auto minimum_optimum_length  = 2 * total_group_and_break_length;
	auto switch_deviation        = length_type(0.2) * total_group_and_break_length / dashes_in_group;
	
	bool set_mid_symbols = mid_symbols != LineSymbol::NoMidSymbols
	                       && (length >= dash_length_f - switch_deviation || show_at_least_one_symbol);
	if (mid_symbols == LineSymbol::CenterOfDash)
	{
		if (line_start.clen < start.clen || (!is_part_start && flags[start.index].isDashPoint()))
		{
			set_mid_symbols = false;
			
			// Handle mid symbols at begin for explicit dash points when we draw the whole dash here
			auto split = SplitPathCoord::begin(path_coords);
			
			auto position = start.clen - (mid_symbols_per_spot - 1) * mid_symbol_distance_f / 2;
			for (int s = 0; s < mid_symbols_per_spot; s+=2)
			{
				if (position >= split.clen)
				{
					auto next_split = SplitPathCoord::at(position, split);
					if (mid_symbol_rotatable)
						orientation = next_split.tangentVector().angle();
					mid_symbol->createRenderablesScaled(next_split.pos, orientation, output);
					split = next_split;
				}
				position  += mid_symbol_distance_f;
			}
		}
		if (half_first_group)
		{
			// Handle mid symbols at start for closing point or explicit dash points.
			auto split = start;
			
			auto position = split.clen + (mid_symbols_per_spot % 2 + 1) * mid_symbol_distance_f / 2;
			for (int s = 1; s < mid_symbols_per_spot && position <= path_coords.back().clen; s += 2)
			{
				auto next_split = SplitPathCoord::at(position, split);
				if (mid_symbol_rotatable)
					orientation = next_split.tangentVector().angle();
				mid_symbol->createRenderablesScaled(next_split.pos, orientation, output);
				
				position  += mid_symbol_distance_f;
				split = next_split;
			}
		}
	}
	
	if (length <= 0 || (lower_dashgroup_count <= 1 && length < minimum_optimum_length - minimum_optimum_num_dashes * switch_deviation))
	{
		// Line part too short for dashes
		if (is_part_end)
		{
			// Can't be handled correctly by the next round of dash groups drawing:
			// Just draw a continuous line.
			processContinuousLine(path, line_start, end, set_mid_symbols,
			                      out_flags, out_coords, output);
		}
		else
		{
			// Give this length to the next round of dash groups drawing.
			return line_start;
		}
	}
	else
	{
		auto higher_dashgroup_count = int(std::ceil(num_dashgroups_f));
		auto lower_dashgroup_deviation  = (length_plus_break - lower_dashgroup_count * total_group_and_break_length)  / lower_dashgroup_count;
		auto higher_dashgroup_deviation = (higher_dashgroup_count * total_group_and_break_length - length_plus_break) / higher_dashgroup_count;
		Q_ASSERT(half_first_group || half_last_group || (lower_dashgroup_deviation >= length_type(-0.001) && higher_dashgroup_deviation >= length_type(-0.001))); // TODO; seems to fail as long as halving first/last dashes affects the outermost dash only
		auto num_dashgroups = (lower_dashgroup_deviation > higher_dashgroup_deviation) ? higher_dashgroup_count : lower_dashgroup_count;
		Q_ASSERT(num_dashgroups >= 2);
		
		auto num_half_dashes = 2 * num_dashgroups * dashes_in_group - num_half_groups;
		auto adjusted_dash_length = (length - (num_dashgroups-1) * break_length_f - num_dashgroups * total_in_group_break_length) * 2 / num_half_dashes;
		adjusted_dash_length = qMax(adjusted_dash_length, 0.0f);	// could be negative for large break lengths
		
		auto cur_length = start.clen;
		SplitPathCoord dash_start = line_start;
		
		for (int dashgroup = 1; dashgroup <= num_dashgroups; ++dashgroup)
		{
			bool is_first_dashgroup = dashgroup == 1;
			bool is_last_dashgroup  = dashgroup == num_dashgroups;
			
			if (mid_symbols == CenterOfDashGroup)
			{
				auto position = cur_length;
				if (is_last_dashgroup && half_last_group)
				{
					position = end.clen;
				}
				else if (!(is_first_dashgroup && half_first_group))
				{
					position += (adjusted_dash_length * dashes_in_group
					             + in_group_break_length_f * (dashes_in_group - 1)) / 2;
				}
				position -= (mid_symbol_distance_f * (mid_symbols_per_spot - 1)) / 2;
				auto split = dash_start;
				for (int i = 0; i < mid_symbols_per_spot && position <= end.clen; ++i, position += mid_symbol_distance_f)
				{
					if (position <= start.clen)
						continue;
					split = SplitPathCoord::at(position, split);
					if (mid_symbol_rotatable)
						orientation = split.tangentVector().angle();
					mid_symbol->createRenderablesScaled(split.pos, orientation, output);
				}
			}
			
			for (int dash = 1; dash <= dashes_in_group; ++dash)
			{
				// Draw a single dash
				bool is_first_dash = is_first_dashgroup && dash == 1;
				bool is_last_dash  = is_last_dashgroup  && dash == dashes_in_group;
				
				// The dash has an start if it is not the first dash in a half first group.
				bool has_start = !(is_first_dash && half_first_group);
				// The dash has an end if it is not the last dash in a half last group.
				bool has_end   = !(is_last_dash && half_last_group);
				
				Q_ASSERT(has_start || has_end); // At least one half must be left...
				
				bool is_half_dash = has_start != has_end;
				auto cur_dash_length = is_half_dash ? adjusted_dash_length / 2 : adjusted_dash_length;
				const auto set_mid_symbols = mid_symbols == CenterOfDash && !is_half_dash;
				
				if (!is_first_dash)
				{
					auto next_dash_start = SplitPathCoord::at(cur_length, dash_start);
					path.copy(dash_start, next_dash_start, out_flags, out_coords);
					out_flags.back().setGapPoint(true);
					dash_start = next_dash_start;
				}
				if (is_last_dash && !is_part_end)
				{
					// Last dash before a dash point (which is not the closing point):
					// Give the remaining length to the next round of dash groups drawing.
					return dash_start;
				}
				
				SplitPathCoord dash_end = SplitPathCoord::at(cur_length + cur_dash_length, dash_start);
				processContinuousLine(path, dash_start, dash_end, set_mid_symbols,
				                      out_flags, out_coords, output);
				cur_length += cur_dash_length;
				dash_start = dash_end;
				
				if (dash < dashes_in_group)
					cur_length += in_group_break_length_f;
			}
			
			if (dashgroup < num_dashgroups)
			{
				if (Q_UNLIKELY(mid_symbols == CenterOfGap))
				{
					auto position = cur_length
					                + (break_length_f
					                   - mid_symbol_distance_f * (mid_symbols_per_spot - 1)) / 2;
					auto split = dash_start;
					for (int i = 0; i < mid_symbols_per_spot; ++i, position += mid_symbol_distance_f)
					{
						split = SplitPathCoord::at(position, split);
						if (mid_symbol_rotatable)
							orientation = split.tangentVector().angle();
						mid_symbol->createRenderablesScaled(split.pos, orientation, output);
					}
				}
				
				cur_length += break_length_f;
			}
		}
		
		if (half_last_group && mid_symbols == LineSymbol::CenterOfDash)
		{
			// Handle mid symbols at end for closing point or for (some) explicit dash points.
			auto split = start;
			
			auto position = end.clen - (mid_symbols_per_spot-1) * mid_symbol_distance_f / 2;
			for (int s = 0; s < mid_symbols_per_spot; s+=2)
			{
				auto next_split = SplitPathCoord::at(position, split);
				if (mid_symbol_rotatable)
					orientation = next_split.tangentVector().angle();
				mid_symbol->createRenderablesScaled(next_split.pos, orientation, output);
				
				position  += mid_symbol_distance_f;
				split = next_split;
			}
		}
	}
	
	return end;
}

void LineSymbol::createDashSymbolRenderables(
        const VirtualPath& path,
        bool path_closed ,
        ObjectRenderables& output) const
{
	Q_ASSERT(dash_symbol);
	
	auto& flags  = path.coords.flags;
	auto& coords = path.coords;
	
	auto last = path.last_index;
	auto i = path.first_index;
	if (suppress_dash_symbol_at_ends && path.size() > 0)
	{
		// Suppress dash symbol at line ends
		++i;
		--last;
	}
	else if (path_closed)
	{
		++i;
	}
	
	for (; i <= last; ++i)
	{
		if (flags[i].isDashPoint())
		{
			const auto params = path.calculateTangentScaling(i);
			//params.first.perpRight();
			auto rotation = dash_symbol->isRotatable() ? params.first.angle() : 0.0;
			auto scale = scale_dash_symbol ? qMin(params.second, 2.0 * LineSymbol::miterLimit()) : 1.0;
			dash_symbol->createRenderablesScaled(coords[i], rotation, output, scale);
		}
	}
}

void LineSymbol::createMidSymbolRenderables(
        const VirtualPath& path,
        const SplitPathCoord& start,
        const SplitPathCoord& end,
        bool path_closed,
        ObjectRenderables& output) const
{
	Q_ASSERT(mid_symbol);
	auto orientation = qreal(0);
	bool mid_symbol_rotatable = bool(mid_symbol) && mid_symbol->isRotatable();
	
	int mid_symbol_num_gaps       = mid_symbols_per_spot - 1;
	
	auto segment_length_f       = length_type(0.001) * segment_length;
	auto end_length_f           = length_type(0.001) * end_length;
	auto end_length_twice_f     = length_type(0.002) * end_length;
	auto mid_symbol_distance_f  = length_type(0.001) * mid_symbol_distance;
	auto mid_symbols_length     = mid_symbol_num_gaps * mid_symbol_distance_f;
	
	auto& path_coords = path.path_coords;
	Q_ASSERT(!path_coords.empty());
	
	if (end_length == 0 && !path_closed)
	{
		// Insert point at start coordinate
		if (mid_symbol_rotatable)
			orientation = start.tangentVector().angle();
		mid_symbol->createRenderablesScaled(start.pos, orientation, output);
	}
	
	auto groups_start = start;
	for (auto is_part_end = false; !is_part_end; )
	{
		auto groups_end_path_coord_index = path_coords.findNextDashPoint(groups_start.path_coord_index);
		is_part_end = path.path_coords[groups_end_path_coord_index].clen >= end.clen;
		auto groups_end = is_part_end ? end : SplitPathCoord::at(path_coords, groups_end_path_coord_index);
		
		// The total length of the current continuous part
		auto length = groups_end.clen - groups_start.clen;
		// The length which is available for placing mid symbols
		auto segmented_length = qMax(length_type(0), length - end_length_twice_f) - mid_symbols_length;
		// The number of segments to be created by mid symbols
		auto segment_count_raw = qMax((end_length == 0) ? length_type(1) : length_type(0), (segmented_length / (segment_length_f + mid_symbols_length)));
		auto lower_segment_count = int(std::floor(segment_count_raw));
		auto higher_segment_count = int(std::ceil(segment_count_raw));
		
		if (end_length > 0)
		{
			if (length <= mid_symbols_length)
			{
				if (show_at_least_one_symbol)
				{
					// Insert point at start coordinate
					if (mid_symbol_rotatable)
						orientation = groups_start.tangentVector().angle();
					mid_symbol->createRenderablesScaled(groups_start.pos, orientation, output);
					
					// Insert point at end coordinate
					if (mid_symbol_rotatable)
						orientation = groups_end.tangentVector().angle();
					mid_symbol->createRenderablesScaled(groups_end.pos, orientation, output);
				}
			}
			else
			{
				auto lower_abs_deviation = qAbs(length - lower_segment_count * segment_length_f - (lower_segment_count+1)*mid_symbols_length - end_length_twice_f);
				auto higher_abs_deviation = qAbs(length - higher_segment_count * segment_length_f - (higher_segment_count+1)*mid_symbols_length - end_length_twice_f);
				auto segment_count = (lower_abs_deviation >= higher_abs_deviation) ? higher_segment_count : lower_segment_count;
				
				auto deviation = (lower_abs_deviation >= higher_abs_deviation) ? -higher_abs_deviation : lower_abs_deviation;
				auto ideal_length = segment_count * segment_length_f + end_length_twice_f;
				auto adjusted_end_length = end_length_f + deviation * (end_length_f / ideal_length);
				auto adjusted_segment_length = segment_length_f + deviation * (segment_length_f / ideal_length);
				Q_ASSERT(qAbs(2*adjusted_end_length + segment_count*adjusted_segment_length + (segment_count + 1)*mid_symbols_length - length) < 0.001f);
				
				if (adjusted_segment_length >= 0 && (show_at_least_one_symbol || higher_segment_count > 0 || length > end_length_twice_f - (segment_length_f + mid_symbols_length) / 2))
				{
					adjusted_segment_length += mid_symbols_length;
					auto split = groups_start;
					for (int i = 0; i < segment_count + 1; ++i)
					{
						auto position = groups_start.clen + adjusted_end_length + i * adjusted_segment_length - mid_symbol_distance_f;
						for (int s = 0; s < mid_symbols_per_spot; ++s)
						{
							position += mid_symbol_distance_f;
							split = SplitPathCoord::at(position, split);
							if (mid_symbol_rotatable)
								orientation = split.tangentVector().angle();
							mid_symbol->createRenderablesScaled(split.pos, orientation, output);
						}
					}
				}
			}
		}
		else
		{
			// end_length == 0
			if (length > mid_symbols_length)
			{
				auto lower_segment_deviation = qAbs(length - lower_segment_count * segment_length_f - (lower_segment_count+1)*mid_symbols_length) / lower_segment_count;
				auto higher_segment_deviation = qAbs(length - higher_segment_count * segment_length_f - (higher_segment_count+1)*mid_symbols_length) / higher_segment_count;
				int segment_count = (lower_segment_deviation > higher_segment_deviation) ? higher_segment_count : lower_segment_count;
				auto adapted_segment_length = (length - (segment_count+1)*mid_symbols_length) / segment_count + mid_symbols_length;
				Q_ASSERT(qAbs(segment_count * adapted_segment_length + mid_symbols_length) - length < length_type(0.001));
				
				if (adapted_segment_length >= mid_symbols_length)
				{
					auto split = groups_start;
					for (int i = 0; i <= segment_count; ++i)
					{
						auto position = groups_start.clen + i * adapted_segment_length - mid_symbol_distance_f;
						for (int s = 0; s < mid_symbols_per_spot; ++s)
						{
							position += mid_symbol_distance_f;
							
							// The outermost symbols are handled outside this loop
							if (i == 0 && s == 0)
								continue;
							if (i == segment_count && s == mid_symbol_num_gaps)
								break;
							
							split = SplitPathCoord::at(position, split);
							if (mid_symbol_rotatable)
								orientation = split.tangentVector().angle();
							mid_symbol->createRenderablesScaled(split.pos, orientation, output);
						}
					}
				}
			}
			
			// Insert point at end coordinate
			if (mid_symbol_rotatable)
				orientation = groups_end.tangentVector().angle();
			mid_symbol->createRenderablesScaled(groups_end.pos, orientation, output);
		}
		
		groups_start = groups_end; // Search then next split (node) after groups_end (current node).
	}
}


void LineSymbol::createStartEndSymbolRenderables(
            const PathPartVector& path_parts,
            ObjectRenderables& output) const
{
	if (path_parts.empty())
		return;
	
	if (start_symbol && !start_symbol->isEmpty())
	{
		const auto& path = path_parts.front();
		auto orientation = qreal(0);
		if (start_symbol->isRotatable())
		{
			bool ok;
			MapCoordF tangent = path.calculateOutgoingTangent(path.first_index, ok);
			if (ok)
				orientation = tangent.angle();
		}
		start_symbol->createRenderablesScaled(path.coords[path.first_index], orientation, output);
	}
	
	if (end_symbol && !end_symbol->isEmpty())
	{
		const auto& path = path_parts.back();
		auto orientation = qreal(0);
		if (end_symbol->isRotatable())
		{
			bool ok;
			MapCoordF tangent = path.calculateIncomingTangent(path.last_index, ok);
			if (ok)
				orientation = tangent.angle();
		}				
		end_symbol->createRenderablesScaled(path.coords[path.last_index], orientation, output);
	}
}



void LineSymbol::colorDeletedEvent(const MapColor* color)
{
	bool have_changes = false;
	if (mid_symbol && mid_symbol->containsColor(color))
	{
		mid_symbol->colorDeletedEvent(color);
		have_changes = true;
	}
	if (start_symbol && start_symbol->containsColor(color))
	{
		start_symbol->colorDeletedEvent(color);
		have_changes = true;
	}
	if (end_symbol && end_symbol->containsColor(color))
	{
		end_symbol->colorDeletedEvent(color);
		have_changes = true;
	}
	if (dash_symbol && dash_symbol->containsColor(color))
	{
		dash_symbol->colorDeletedEvent(color);
		have_changes = true;
	}
    if (color == this->color)
	{
		this->color = nullptr;
		have_changes = true;
	}
	if (color == border.color)
	{
		this->border.color = nullptr;
		have_changes = true;
	}
	if (color == right_border.color)
	{
		this->right_border.color = nullptr;
		have_changes = true;
	}
	if (have_changes)
		resetIcon();
}

bool LineSymbol::containsColor(const MapColor* color) const
{
	if (color == this->color || color == border.color || color == right_border.color)
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

const MapColor* LineSymbol::guessDominantColor() const
{
	bool has_main_line = line_width > 0 && color;
	bool has_border = hasBorder() && border.width > 0 && border.color;
	bool has_right_border = hasBorder() && right_border.width > 0 && right_border.color;
	
	// Use the color of the thickest line
	if (has_main_line)
	{
		if (has_border)
		{
			if (has_right_border)
			{
				if (line_width > 2 * border.width)
					return (line_width > 2 * right_border.width) ? color : right_border.color;
				else
					return (border.width > right_border.width) ? border.color : right_border.color;
			}
			else
				return (line_width > 2 * border.width) ? color : border.color;
		}
		else
		{
			if (has_right_border)
				return (line_width > 2 * right_border.width) ? color : right_border.color;
			else
				return color;
		}
	}
	else
	{
		if (has_border)
		{
			if (has_right_border)
				return (border.width > right_border.width) ? border.color : right_border.color;
			else
				return border.color;
		}
		else
		{
			if (has_right_border)
				return right_border.color;
		}
	}
	
	const MapColor* dominant_color = mid_symbol ? mid_symbol->guessDominantColor() : nullptr;
	if (dominant_color) return dominant_color;
	
	dominant_color = start_symbol ? start_symbol->guessDominantColor() : nullptr;
	if (dominant_color) return dominant_color;
	
	dominant_color = end_symbol ? end_symbol->guessDominantColor() : nullptr;
	if (dominant_color) return dominant_color;
	
	dominant_color = dash_symbol ? dash_symbol->guessDominantColor() : nullptr;
	if (dominant_color) return dominant_color;
	
	return nullptr;
}


void LineSymbol::replaceColors(const MapColorMap& color_map)
{
	color = color_map.value(color);
	border.color = color_map.value(border.color);
	right_border.color = color_map.value(right_border.color);
	for (auto member : { &LineSymbol::start_symbol, &LineSymbol::mid_symbol, &LineSymbol::end_symbol, &LineSymbol::dash_symbol })
	{
		if (auto sub_symbol = this->*member)
			sub_symbol->replaceColors(color_map);
	}
}



void LineSymbol::scale(double factor)
{
	line_width = qRound(factor * line_width);
	
	minimum_length = qRound(factor * minimum_length);
	start_offset = qRound(factor * start_offset);
	end_offset = qRound(factor * end_offset);
	
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

	border.scale(factor);
	right_border.scale(factor);
	
	resetIcon();
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
	if (start_symbol && start_symbol->isEmpty())
	{
		delete start_symbol;
		start_symbol = nullptr;
	}
	if (mid_symbol && mid_symbol->isEmpty())
	{
		delete mid_symbol;
		mid_symbol = nullptr;
	}
	if (end_symbol && end_symbol->isEmpty())
	{
		delete end_symbol;
		end_symbol = nullptr;
	}
	if (dash_symbol && dash_symbol->isEmpty())
	{
		delete dash_symbol;
		dash_symbol = nullptr;
	}
}



qreal LineSymbol::dimensionForIcon() const
{
	// calculateLargestLineExtent() gives half line width, and for the icon,
	// we suggest a half line width of white space on each side.
	auto size = 4 * calculateLargestLineExtent();
	if (start_symbol && !start_symbol->isEmpty())
		size = qMax(size, start_symbol->dimensionForIcon());
	if (mid_symbol && !mid_symbol->isEmpty())
		size = qMax(size, 2 * mid_symbol->dimensionForIcon() + getMidSymbolDistance() * (getMidSymbolsPerSpot() - 1) / 1000);
	if (dash_symbol && !dash_symbol->isEmpty())
		size = qMax(size, 2 * dash_symbol->dimensionForIcon());
	if (end_symbol && !end_symbol->isEmpty())
		size = qMax(size, end_symbol->dimensionForIcon());
	return size;
}


qreal LineSymbol::calculateLargestLineExtent() const
{
	auto line_extent_f = 0.001 * 0.5 * getLineWidth();
	auto result = line_extent_f;
	if (hasBorder())
	{
		result = qMax(result, line_extent_f + 0.001 * (getBorder().shift + getBorder().width)/2);
		result = qMax(result, line_extent_f + 0.001 * (getRightBorder().shift + getRightBorder().width)/2);
	}
	return result;
}



void LineSymbol::setStartSymbol(PointSymbol* symbol)
{
	replaceSymbol(start_symbol, symbol, QCoreApplication::translate("OpenOrienteering::LineSymbolSettings", "Start symbol"));
}
void LineSymbol::setMidSymbol(PointSymbol* symbol)
{
	replaceSymbol(mid_symbol, symbol, QCoreApplication::translate("OpenOrienteering::LineSymbolSettings", "Mid symbol"));
}
void LineSymbol::setEndSymbol(PointSymbol* symbol)
{
	replaceSymbol(end_symbol, symbol, QCoreApplication::translate("OpenOrienteering::LineSymbolSettings", "End symbol"));
}
void LineSymbol::setDashSymbol(PointSymbol* symbol)
{
	replaceSymbol(dash_symbol, symbol, QCoreApplication::translate("OpenOrienteering::LineSymbolSettings", "Dash symbol"));
}

void LineSymbol::setMidSymbolPlacement(LineSymbol::MidSymbolPlacement placement)
{
	mid_symbol_placement = placement;
}


void LineSymbol::replaceSymbol(PointSymbol*& old_symbol, PointSymbol* replace_with, const QString& name)
{
	delete old_symbol;
	old_symbol = replace_with;
	replace_with->setName(name);
}



void LineSymbol::saveImpl(QXmlStreamWriter& xml, const Map& map) const
{
	xml.writeStartElement(QString::fromLatin1("line_symbol"));
	xml.writeAttribute(QString::fromLatin1("color"), QString::number(map.findColorIndex(color)));
	xml.writeAttribute(QString::fromLatin1("line_width"), QString::number(line_width));
	xml.writeAttribute(QString::fromLatin1("minimum_length"), QString::number(minimum_length));
	xml.writeAttribute(QString::fromLatin1("join_style"), QString::number(join_style));
	xml.writeAttribute(QString::fromLatin1("cap_style"), QString::number(cap_style));
	if (cap_style == LineSymbol::PointedCap)
	{
		// Deprecated, only for backwards compatibility.
		/// \todo Remove "pointed_cap_length" in Mapper 1.0
		xml.writeAttribute(QString::fromLatin1("pointed_cap_length"), QString::number(qRound((start_offset+end_offset)/2.0)));
	}
	xml.writeAttribute(QString::fromLatin1("start_offset"), QString::number(start_offset));
	xml.writeAttribute(QString::fromLatin1("end_offset"), QString::number(end_offset));
	
	if (dashed)
		xml.writeAttribute(QString::fromLatin1("dashed"), QString::fromLatin1("true"));
	xml.writeAttribute(QString::fromLatin1("segment_length"), QString::number(segment_length));
	xml.writeAttribute(QString::fromLatin1("end_length"), QString::number(end_length));
	if (show_at_least_one_symbol)
		xml.writeAttribute(QString::fromLatin1("show_at_least_one_symbol"), QString::fromLatin1("true"));
	xml.writeAttribute(QString::fromLatin1("minimum_mid_symbol_count"), QString::number(minimum_mid_symbol_count));
	xml.writeAttribute(QString::fromLatin1("minimum_mid_symbol_count_when_closed"), QString::number(minimum_mid_symbol_count_when_closed));
	xml.writeAttribute(QString::fromLatin1("dash_length"), QString::number(dash_length));
	xml.writeAttribute(QString::fromLatin1("break_length"), QString::number(break_length));
	xml.writeAttribute(QString::fromLatin1("dashes_in_group"), QString::number(dashes_in_group));
	xml.writeAttribute(QString::fromLatin1("in_group_break_length"), QString::number(in_group_break_length));
	if (half_outer_dashes)
		xml.writeAttribute(QString::fromLatin1("half_outer_dashes"), QString::fromLatin1("true"));
	xml.writeAttribute(QString::fromLatin1("mid_symbols_per_spot"), QString::number(mid_symbols_per_spot));
	xml.writeAttribute(QString::fromLatin1("mid_symbol_distance"), QString::number(mid_symbol_distance));
	if (mid_symbol_placement != 0)
		xml.writeAttribute(QString::fromLatin1("mid_symbol_placement"), QString::number(mid_symbol_placement));
	if (suppress_dash_symbol_at_ends)
		xml.writeAttribute(QString::fromLatin1("suppress_dash_symbol_at_ends"), QString::fromLatin1("true"));
	if (!scale_dash_symbol)
		xml.writeAttribute(QString::fromLatin1("scale_dash_symbol"), QString::fromLatin1("false"));
	
	if (start_symbol)
	{
		xml.writeStartElement(QString::fromLatin1("start_symbol"));
		start_symbol->save(xml, map);
		xml.writeEndElement();
	}
	
	if (mid_symbol)
	{
		xml.writeStartElement(QString::fromLatin1("mid_symbol"));
		mid_symbol->save(xml, map);
		xml.writeEndElement();
	}
	
	if (end_symbol)
	{
		xml.writeStartElement(QString::fromLatin1("end_symbol"));
		end_symbol->save(xml, map);
		xml.writeEndElement();
	}
	
	if (dash_symbol)
	{
		xml.writeStartElement(QString::fromLatin1("dash_symbol"));
		dash_symbol->save(xml, map);
		xml.writeEndElement();
	}
	
	if (have_border_lines)
	{
		xml.writeStartElement(QString::fromLatin1("borders"));
		bool are_borders_different = areBordersDifferent();
		if (are_borders_different)
			xml.writeAttribute(QString::fromLatin1("borders_different"), QString::fromLatin1("true"));
		border.save(xml, map);
		if (are_borders_different)
			right_border.save(xml, map);
		xml.writeEndElement(/*borders*/);
	}
	
	xml.writeEndElement(/*line_symbol*/);
}

bool LineSymbol::loadImpl(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict)
{
	if (xml.name() != QLatin1String("line_symbol"))
		return false;
	
	QXmlStreamAttributes attributes = xml.attributes();
	int temp = attributes.value(QLatin1String("color")).toInt();
	color = map.getColor(temp);
	line_width = attributes.value(QLatin1String("line_width")).toInt();
	minimum_length = attributes.value(QLatin1String("minimum_length")).toInt();
	join_style = static_cast<LineSymbol::JoinStyle>(attributes.value(QLatin1String("join_style")).toInt());
	cap_style = static_cast<LineSymbol::CapStyle>(attributes.value(QLatin1String("cap_style")).toInt());
	if (attributes.hasAttribute(QLatin1String("start_offset")) || attributes.hasAttribute(QLatin1String("end_offset")))
	{
		// since version 8
		start_offset = attributes.value(QLatin1String("start_offset")).toInt();
		end_offset = attributes.value(QLatin1String("end_offset")).toInt();
	}
	else if (cap_style == LineSymbol::PointedCap)
	{
		// until version 7
		start_offset = end_offset = attributes.value(QLatin1String("pointed_cap_length")).toInt();
	}
	
	dashed = (attributes.value(QLatin1String("dashed")) == QLatin1String("true"));
	segment_length = attributes.value(QLatin1String("segment_length")).toInt();
	end_length = attributes.value(QLatin1String("end_length")).toInt();
	show_at_least_one_symbol = (attributes.value(QLatin1String("show_at_least_one_symbol")) == QLatin1String("true"));
	minimum_mid_symbol_count = attributes.value(QLatin1String("minimum_mid_symbol_count")).toInt();
	minimum_mid_symbol_count_when_closed = attributes.value(QLatin1String("minimum_mid_symbol_count_when_closed")).toInt();
	dash_length = attributes.value(QLatin1String("dash_length")).toInt();
	break_length = attributes.value(QLatin1String("break_length")).toInt();
	dashes_in_group = attributes.value(QLatin1String("dashes_in_group")).toInt();
	in_group_break_length = attributes.value(QLatin1String("in_group_break_length")).toInt();
	half_outer_dashes = (attributes.value(QLatin1String("half_outer_dashes")) == QLatin1String("true"));
	mid_symbols_per_spot = attributes.value(QLatin1String("mid_symbols_per_spot")).toInt();
	mid_symbol_distance = attributes.value(QLatin1String("mid_symbol_distance")).toInt();
	mid_symbol_placement = static_cast<LineSymbol::MidSymbolPlacement>(attributes.value(QLatin1String("mid_symbol_placement")).toInt());
	suppress_dash_symbol_at_ends = (attributes.value(QLatin1String("suppress_dash_symbol_at_ends")) == QLatin1String("true"));
	scale_dash_symbol = (attributes.value(QLatin1String("scale_dash_symbol")) != QLatin1String("false"));
	
	have_border_lines = false;
	while (xml.readNextStartElement())
	{
		if (xml.name() == QLatin1String("start_symbol"))
			start_symbol = loadPointSymbol(xml, map, symbol_dict);
		else if (xml.name() == QLatin1String("mid_symbol"))
			mid_symbol = loadPointSymbol(xml, map, symbol_dict);
		else if (xml.name() == QLatin1String("end_symbol"))
			end_symbol = loadPointSymbol(xml, map, symbol_dict);
		else if (xml.name() == QLatin1String("dash_symbol"))
			dash_symbol = loadPointSymbol(xml, map, symbol_dict);
		else if (xml.name() == QLatin1String("borders"))
		{
//			bool are_borders_different = (xml.attributes().value("borders_different") == "true");
			
			bool right_border_loaded = false;
			while (xml.readNextStartElement())
			{
				if (xml.name() == QLatin1String("border"))
				{
					if (!have_border_lines)
					{
						border.load(xml, map);
						have_border_lines = true;
					}
					else
					{
						right_border.load(xml, map);
						right_border_loaded = true;
						xml.skipCurrentElement();
						break;
					}
				}
				else
					xml.skipCurrentElement();
			}
			
			if (have_border_lines && !right_border_loaded)
				right_border = border;
		}
		else
			xml.skipCurrentElement(); // unknown
	}
	
	cleanupPointSymbols();
	
	return true;
}

PointSymbol* LineSymbol::loadPointSymbol(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict)
{
	while (xml.readNextStartElement())
	{
		if (xml.name() == QLatin1String("symbol"))
		{
			auto symbol = static_cast<PointSymbol*>(Symbol::load(xml, map, symbol_dict).release());
			xml.skipCurrentElement();
			return symbol;
		}
		else
			xml.skipCurrentElement();
	}
	return nullptr;
}

bool LineSymbol::equalsImpl(const Symbol* other, Qt::CaseSensitivity case_sensitivity) const
{
	Q_UNUSED(case_sensitivity);
	
	const LineSymbol* line = static_cast<const LineSymbol*>(other);
	if (line_width != line->line_width)
		return false;
	if (minimum_length != line->minimum_length)
		return false;
	if (line_width > 0)
	{
		if (!MapColor::equal(color, line->color))
			return false;
		
		if ((cap_style != line->cap_style) ||
			(join_style != line->join_style))
			return false;
		
		if (start_offset != line->start_offset || end_offset != line->end_offset)
			return false;
		
		if (dashed != line->dashed)
			return false;
		if (dashed)
		{
			if (dash_length != line->dash_length ||
				break_length != line->break_length ||
				dashes_in_group != line->dashes_in_group ||
				half_outer_dashes != line->half_outer_dashes ||
				(dashes_in_group > 1 && (in_group_break_length != line->in_group_break_length)))
				return false;
		}
		else
		{
			if (segment_length != line->segment_length ||
				end_length != line->end_length ||
				(mid_symbol && (show_at_least_one_symbol != line->show_at_least_one_symbol ||
								minimum_mid_symbol_count != line->minimum_mid_symbol_count ||
								minimum_mid_symbol_count_when_closed != line->minimum_mid_symbol_count_when_closed)))
				return false;
		}
	}

	if (bool(start_symbol) != bool(line->start_symbol))
		return false;
	if (start_symbol && !start_symbol->equals(line->start_symbol))
		return false;
	
	if (bool(mid_symbol) != bool(line->mid_symbol))
		return false;
	if (mid_symbol && !mid_symbol->equals(line->mid_symbol))
		return false;
	
	if (bool(end_symbol) != bool(line->end_symbol))
		return false;
	if (end_symbol && !end_symbol->equals(line->end_symbol))
		return false;
	
	if (bool(dash_symbol) != bool(line->dash_symbol))
		return false;
	if (dash_symbol && !dash_symbol->equals(line->dash_symbol))
		return false;
	if (suppress_dash_symbol_at_ends != line->suppress_dash_symbol_at_ends)
		return false;
	if (scale_dash_symbol != line->scale_dash_symbol)
		return false;
	
	if (mid_symbol)
	{
		if (mid_symbols_per_spot != line->mid_symbols_per_spot)
			return false;
		if (mid_symbol_distance != line->mid_symbol_distance)
			return false;
		if (mid_symbol_placement != line->mid_symbol_placement)
			return false;
	}

	if (have_border_lines != line->have_border_lines)
		return false;
	if (have_border_lines)
	{
		if (border != line->border || right_border != line->right_border)
			return false;
	}
	
	return true;
}


}  // namespace OpenOrienteering
