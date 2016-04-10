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


#include "symbol_line.h"

#include <QtNumeric>
#include <QGridLayout>
#include <QIODevice>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <private/qbezier_p.h>

#include "core/map_color.h"
#include "map.h"
#include "object.h"
#include "renderable_implementation.h"
#include "symbol_area.h"
#include "symbol_point.h"
#include "symbol_point_editor.h"
#include "symbol_setting_dialog.h"
#include "util.h"
#include "util_gui.h"
#include "gui/widgets/color_dropdown.h"


// ### LineSymbolBorder ###

void LineSymbolBorder::reset()
{
	color = NULL;
	width = 0;
	shift = 0;
	dashed = false;
	dash_length = 2 * 1000;
	break_length = 1 * 1000;
}

#ifndef NO_NATIVE_FILE_FORMAT

bool LineSymbolBorder::load(QIODevice* file, int version, Map* map)
{
	Q_UNUSED(version);
	int temp;
	file->read((char*)&temp, sizeof(int));
	color = (temp >= 0) ? map->getColor(temp) : NULL;
	file->read((char*)&width, sizeof(int));
	file->read((char*)&shift, sizeof(int));
	file->read((char*)&dashed, sizeof(bool));
	file->read((char*)&dash_length, sizeof(int));
	file->read((char*)&break_length, sizeof(int));
	return true;
}

#endif

void LineSymbolBorder::save(QXmlStreamWriter& xml, const Map& map) const
{
	xml.writeStartElement(QString::fromLatin1("border"));
	xml.writeAttribute(QString::fromLatin1("color"), QString::number(map.findColorIndex(color)));
	xml.writeAttribute(QString::fromLatin1("width"), QString::number(width));
	xml.writeAttribute(QString::fromLatin1("shift"), QString::number(shift));
	if (dashed)
		xml.writeAttribute(QString::fromLatin1("dashed"), QString::fromLatin1("true"));
	xml.writeAttribute(QString::fromLatin1("dash_length"), QString::number(dash_length));
	xml.writeAttribute(QString::fromLatin1("break_length"), QString::number(break_length));
	xml.writeEndElement(/*border*/);
}

bool LineSymbolBorder::load(QXmlStreamReader& xml, const Map& map)
{
	Q_ASSERT(xml.name() == QLatin1String("border"));
	
	QXmlStreamAttributes attributes = xml.attributes();
	int temp = attributes.value(QLatin1String("color")).toInt();
	color = map.getColor(temp);
	width = attributes.value(QLatin1String("width")).toInt();
	shift = attributes.value(QLatin1String("shift")).toInt();
	dashed = (attributes.value(QLatin1String("dashed")) == QLatin1String("true"));
	dash_length = attributes.value(QLatin1String("dash_length")).toInt();
	break_length = attributes.value(QLatin1String("break_length")).toInt();
	xml.skipCurrentElement();
	return !xml.error();
}

bool LineSymbolBorder::equals(const LineSymbolBorder* other) const
{
	if (!MapColor::equal(color, other->color))
		return false;
	
	if (width != other->width)
		return false;
	if (shift != other->shift)
		return false;
	if (dashed != other->dashed)
		return false;
	if (dashed)
	{
		if (dash_length != other->dash_length)
			return false;
		if (break_length != other->break_length)
			return false;
	}
	return true;
}

void LineSymbolBorder::assign(const LineSymbolBorder& other, const MapColorMap* color_map)
{
	color = color_map ? color_map->value(other.color) : other.color;
	width = other.width;
	shift = other.shift;
	dashed = other.dashed;
	dash_length = other.dash_length;
	break_length = other.break_length;
}

bool LineSymbolBorder::isVisible() const
{
	return width > 0 && color != NULL && !(dash_length == 0 && dashed);
}

void LineSymbolBorder::createSymbol(LineSymbol& out) const
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


// ### LineSymbol ###

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
	suppress_dash_symbol_at_ends = false;
	
	// Border lines
	have_border_lines = false;
	border.reset();
	right_border.reset();
}
LineSymbol::~LineSymbol()
{
	delete start_symbol;
	delete mid_symbol;
	delete end_symbol;
	delete dash_symbol;
}

Symbol* LineSymbol::duplicate(const MapColorMap* color_map) const
{
	LineSymbol* new_line = new LineSymbol();
	new_line->duplicateImplCommon(this);
	new_line->line_width = line_width;
	new_line->color = color_map ? color_map->value(color) : color;
	new_line->minimum_length = minimum_length;
	new_line->cap_style = cap_style;
	new_line->join_style = join_style;
	new_line->pointed_cap_length = pointed_cap_length;
	for (auto member : { &LineSymbol::start_symbol, &LineSymbol::mid_symbol, &LineSymbol::end_symbol, &LineSymbol::dash_symbol })
	{
		auto sub_symbol = this->*member;
		if (sub_symbol && !sub_symbol->isEmpty())
			new_line->*member = static_cast<PointSymbol*>(sub_symbol->duplicate(color_map));
	}
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
	new_line->suppress_dash_symbol_at_ends = suppress_dash_symbol_at_ends;
	new_line->have_border_lines = have_border_lines;
	new_line->border.assign(border, color_map);
	new_line->right_border.assign(right_border, color_map);
	return new_line;
}

void LineSymbol::createRenderables(
        const Object* object,
        const VirtualCoordVector& coords,
        ObjectRenderables& output,
        RenderableOptions options ) const
{
	Q_UNUSED(options);
	PathPartVector path_parts = PathPart::calculatePathParts(coords);
	for (const auto& part : path_parts)
	{
		createPathCoordRenderables(object, part, part.isClosed(), output);
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
		for (const auto& part : path_parts)
		{
			createPathCoordRenderables(object, part, part.isClosed(), output);
		}
	}
}

void LineSymbol::createPathRenderables(const Object* object, bool path_closed, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output) const
{
	auto path = VirtualPath { flags, coords };
	auto last = path.path_coords.update(0);
	createPathCoordRenderables(object, path, path_closed, output);
	
	Q_ASSERT(last+1 == coords.size()); Q_UNUSED(last);
}

void LineSymbol::createPathCoordRenderables(const Object* object, const VirtualPath& path, bool path_closed, ObjectRenderables& output) const
{
	if (path.size() < 2)
		return;
	
	auto& coords = path.coords;
	
	// Start or end symbol?
	if (start_symbol && !start_symbol->isEmpty())
	{
		auto orientation = 0.0f;
		if (start_symbol->isRotatable())
		{
			bool ok;
			MapCoordF tangent = path.calculateOutgoingTangent(0, ok);
			if (ok)
				orientation = tangent.angle();
		}
		start_symbol->createRenderablesScaled(coords[0], orientation, output);
	}
	
	if (end_symbol && !end_symbol->isEmpty())
	{
		size_t last = coords.size() - 1;
		auto orientation = 0.0f;
		if (end_symbol->isRotatable())
		{
			bool ok;
			MapCoordF tangent = path.calculateIncomingTangent(last, ok);
			if (ok)
				orientation = tangent.angle();
		}				
		end_symbol->createRenderablesScaled(coords[last], orientation, output);
	}
	
	// Dash symbols?
	if (dash_symbol && !dash_symbol->isEmpty())
	{
		createDashSymbolRenderables(path, path_closed, output);
	}
	
	// The line itself
	MapCoordVector processed_flags;
	MapCoordVectorF processed_coords;
	bool create_border = have_border_lines && (border.isVisible() || right_border.isVisible());
	bool pointed_cap = cap_style == PointedCap && pointed_cap_length > 0;
	if (!dashed)
	{
		// Base line?
		if (line_width > 0)
		{
			if (color && !pointed_cap && !create_border)
			{
				output.insertRenderable(new LineRenderable(this, path, path_closed));
			}
			else if (create_border || pointed_cap)
			{
				auto last = coords.size();
				auto part_start = MapCoordVector::size_type { 0 };
				auto next_part_start = last; //path_coords.update(part_start);
				
				bool has_start = !(part_start == 0 && path_closed);
				bool has_end = !(next_part_start == last && path_closed);
				
				auto start = SplitPathCoord::begin(path.path_coords);
				auto end   = SplitPathCoord::end(path.path_coords);
				processContinuousLine(path, start, end,
				                      has_start, has_end, processed_flags, processed_coords, false, output);
				
			}
		}
		
		// Symbols?
		if (mid_symbol && !mid_symbol->isEmpty() && segment_length > 0)
			createMidSymbolRenderables(path, path_closed, output);
	}
	else if (dash_length > 0)
	{
		// Dashed lines
		processDashedLine(path, path_closed, processed_flags, processed_coords, output);
	}
	else
	{
		// Invalid configuration
		return;	
	}
	
	if (!processed_coords.empty() && (color || create_border))
	{
		Q_ASSERT(processed_coords.size() != 1);
		
		VirtualPath path = { processed_flags, processed_coords };
		path.path_coords.update(path.first_index);
		
		if (color)
		{
			output.insertRenderable(new LineRenderable(this, path, path_closed));
		}
		
		if (create_border)
		{
			createBorderLines(object, path, output);
		}
	}
}

void LineSymbol::createBorderLines(
        const Object* object,
        const VirtualPath& path,
        ObjectRenderables& output) const
{
	double main_shift = 0.0005 * line_width;
	
	if (!areBordersDifferent())
	{
		MapCoordVector border_flags;
		MapCoordVectorF border_coords;
		LineSymbol border_symbol;
		border.createSymbol(border_symbol);
		border_symbol.setJoinStyle(join_style == RoundJoin ? RoundJoin : MiterJoin);
		
		if (border.dashed && border.dash_length > 0 && border.break_length > 0)
		{
			MapCoordVector dashed_flags;
			MapCoordVectorF dashed_coords;
			border_symbol.processDashedLine(path, path.isClosed(), dashed_flags, dashed_coords, output);
			border_symbol.dashed = false;	// important, otherwise more dashes might be added by createRenderables()!
			
			auto dashed_path = VirtualPath { dashed_flags, dashed_coords };
			shiftCoordinates(dashed_path, main_shift, border_flags, border_coords);
			border_symbol.createPathRenderables(object, path.isClosed(), border_flags, border_coords, output);
			shiftCoordinates(dashed_path, -main_shift, border_flags, border_coords);
			border_symbol.createPathRenderables(object, path.isClosed(), border_flags, border_coords, output);
		}
		else
		{
			shiftCoordinates(path, main_shift, border_flags, border_coords);
			border_symbol.createPathRenderables(object, path.isClosed(), border_flags, border_coords, output);
			shiftCoordinates(path, -main_shift, border_flags, border_coords);
			border_symbol.createPathRenderables(object, path.isClosed(), border_flags, border_coords, output);
		}
	}
	else
	{
		createBorderLine(object, path, path.isClosed(), output, border, -main_shift);
		createBorderLine(object, path, path.isClosed(), output, right_border, main_shift);
	}
}

void LineSymbol::createBorderLine(
        const Object* object,
        const VirtualPath& path,
        bool path_closed,
        ObjectRenderables& output,
        const LineSymbolBorder& border,
        double main_shift ) const
{
	MapCoordVector border_flags;
	MapCoordVectorF border_coords;
	LineSymbol border_symbol;
	border.createSymbol(border_symbol);
	border_symbol.setJoinStyle(join_style == RoundJoin ? RoundJoin : MiterJoin);
	
	if (border.dashed && border.dash_length > 0 && border.break_length > 0)
	{
		MapCoordVector dashed_flags;
		MapCoordVectorF dashed_coords;
		
		border_symbol.processDashedLine(path, path_closed, dashed_flags, dashed_coords, output);
		border_symbol.dashed = false;	// important, otherwise more dashes might be added by createRenderables()!
		
		auto dashed_path = VirtualPath { dashed_flags, dashed_coords };
		shiftCoordinates(dashed_path, main_shift, border_flags, border_coords);
	}
	else
	{
		shiftCoordinates(path, main_shift, border_flags, border_coords);
	}
	
	border_symbol.createPathRenderables(object, path_closed, border_flags, border_coords, output);
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
			//qDebug() << "No valid tangent at" << __FILE__ << __LINE__;
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
        bool has_start,
        bool has_end,
        MapCoordVector& processed_flags,
        MapCoordVectorF& processed_coords,
        bool set_mid_symbols,
        ObjectRenderables& output ) const
{
	bool create_line = true;
	float effective_cap_length = 0.0f;
	if (cap_style == PointedCap && (has_start || has_end))
	{
		effective_cap_length = 0.001f * pointed_cap_length;
		int num_caps = 2 - (has_end ? 0 : 1) - (has_start ? 0 : 1);
		auto max_effective_cap_length = (end.clen - start.clen) / num_caps;
		if (effective_cap_length >= max_effective_cap_length)
		{
			create_line = false;
			effective_cap_length = max_effective_cap_length;
		}
	}
	
	auto split      = start;
	auto next_split = SplitPathCoord();
	
	if (has_start && effective_cap_length > 0.0f)
	{
		// Create pointed line cap start
		next_split = SplitPathCoord::at(start.clen + effective_cap_length, start);
		createPointedLineCap(path, split, next_split, false, output);
		split = next_split;
	}
	
	if (create_line)
	{
		// Add line to processed_coords/flags
		
		PathCoord::length_type mid_symbol_distance_f = 0.001 * mid_symbol_distance;
		auto mid_symbols_length   = (qMax(1, mid_symbols_per_spot) - 1) * mid_symbol_distance_f;
		auto effective_end_length = end.clen;
		if (has_end)
		{
			effective_end_length -= effective_cap_length;
		}
		
		set_mid_symbols = set_mid_symbols && mid_symbol && !mid_symbol->isEmpty() && mid_symbols_per_spot;
		if (set_mid_symbols && mid_symbols_length <= effective_end_length - split.clen)
		{
			auto mid_position = (split.clen + effective_end_length - mid_symbols_length) / 2;
			next_split   = SplitPathCoord::at(mid_position, split);
			path.copy(split, next_split, processed_flags, processed_coords);
			split = next_split;
			
			auto orientation = 0.0f;
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
		
		// End position
		next_split = SplitPathCoord::at(effective_end_length, split);
		path.copy(split, next_split, processed_flags, processed_coords);
		split = next_split;
		
		processed_flags.back().setGapPoint(true);
		Q_ASSERT(!processed_flags[processed_flags.size()-2].isCurveStart());
	}
	
	if (has_end && effective_cap_length > 0.0f)
	{
		// Create pointed line cap end
		next_split = end;
		if (end.is_curve_end)
		{
			// Get updated curve end
			next_split = SplitPathCoord::at(end.clen, split);
		}
		createPointedLineCap(path, split, next_split, true, output);
	}
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
	
	float line_half_width = 0.001f * 0.5f * line_width;
	float cap_length = 0.001f * pointed_cap_length;
	float tan_angle = line_half_width / cap_length;
	
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
	float sign = is_end ? (-1) : 1;
	for (MapCoordVectorF::size_type i = 0; i < cap_size; ++i)
	{
		float dist_from_start = is_end ? (end.clen - cap_lengths[i]) : (cap_lengths[i] - start.clen);
		float factor = dist_from_start / cap_length;
		//Q_ASSERT(factor >= -0.01f && factor <= 1.01f); happens when using large break lengths as these are not adjusted
		factor = qBound(0.0f, factor, 1.0f);
		
		auto tangent_info = cap_middle_path.calculateTangentScaling(i);
		auto right_vector = tangent_info.first.perpRight();
		right_vector.normalize();
		
		auto radius = qBound(0.0, line_half_width*factor*tangent_info.second, line_half_width*2.0);
		
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
			float right_scale = tangent.length() * tan_angle * sign;
			cap_coords.emplace_back(cap_coords[i] + tangent + right_vector * right_scale);
			cap_coords_left.emplace_back(cap_coords_left[i] + tangent - right_vector * right_scale);
			i += 2;
		}
	}
	
	// Create small overlap to avoid visual glitches with the on-screen display (should not matter for printing, could probably turn it off for this)
	const float overlap_length = 0.05f;
	if (end.clen - start.clen > 4 * overlap_length)
	{
		bool ok;
		int end_pos = is_end ? 0 : (cap_size - 1);
		int end_cap_pos = is_end ? 0 : (cap_coords.size() - 1);
		MapCoordF tangent = cap_middle_path.calculateTangent(end_pos, !is_end, ok);
		if (ok)
		{
			tangent.setLength(overlap_length * sign);
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
	
	auto last = path.last_index;
	
	auto groups_start = SplitPathCoord::begin(path_coords);
	auto line_start   = groups_start;
	for (bool is_part_end = false; !is_part_end; )
	{
		auto groups_end_path_coord_index = path_coords.findNextDashPoint(groups_start.path_coord_index);
		auto groups_end_index = path_coords[groups_end_path_coord_index].index;
		auto groups_end = SplitPathCoord::at(path_coords, groups_end_path_coord_index);
		
		// Intentionally look at groups_start (current node),
		// not line_start (where to continue drawing)!
		bool is_part_start = (groups_start.index == path.first_index);
		is_part_end = (groups_end_index == last);
		
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
	
	auto orientation = 0.0f;
	bool mid_symbol_rotatable = bool(mid_symbol) && mid_symbol->isRotatable();
	
	double mid_symbol_distance_f = 0.001 * mid_symbol_distance;
	
	bool half_first_group = is_part_start ? (half_outer_dashes || path_closed)
	                                      : (flags[start.index].isDashPoint() && dashes_in_group == 1);
	
	bool ends_with_dashpoint  = is_part_end ? path_closed : true;
	
	double dash_length_f           = 0.001 * dash_length;
	double break_length_f          = 0.001 * break_length;
	double in_group_break_length_f = 0.001 * in_group_break_length;
	
	double total_in_group_dash_length  = dashes_in_group * dash_length_f;
	double total_in_group_break_length = (dashes_in_group - 1) * in_group_break_length_f;
	double total_group_length = total_in_group_dash_length + total_in_group_break_length;
	double total_group_and_break_length = total_group_length + break_length_f;
	
	double length = end.clen - start.clen;
	double length_plus_break = length + break_length_f;
	
	bool half_last_group  = is_part_end ? (half_outer_dashes || path_closed)
	                                    : (ends_with_dashpoint && dashes_in_group == 1);
	int num_half_groups = (half_first_group ? 1 : 0) + (half_last_group ? 1 : 0);
	
	double num_dashgroups_f = num_half_groups +
	                          (length_plus_break - num_half_groups * 0.5 * dash_length_f) / total_group_and_break_length;

	int lower_dashgroup_count = qRound(floor(num_dashgroups_f));
	
	double minimum_optimum_num_dashes = dashes_in_group * 2.0 - num_half_groups * 0.5;
	double minimum_optimum_length  = 2.0 * total_group_and_break_length;
	double switch_deviation        = 0.2 * total_group_and_break_length / dashes_in_group;
	
	bool set_mid_symbols = length >= dash_length_f - switch_deviation;
	if (mid_symbols_per_spot > 0 && mid_symbol && !mid_symbol->isEmpty())
	{
		if (line_start.clen < start.clen || (!is_part_start && flags[start.index].isDashPoint()))
		{
			set_mid_symbols = false;
			
			// Handle mid symbols at begin for explicit dash points when we draw the whole dash here
			auto split = SplitPathCoord::begin(path_coords);
			
			PathCoord::length_type position = start.clen - (mid_symbols_per_spot - 1) * 0.5 * mid_symbol_distance_f;
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
			
			PathCoord::length_type position = split.clen + (mid_symbols_per_spot%2 + 1) * 0.5 * mid_symbol_distance_f;
			for (int s = 1; s < mid_symbols_per_spot && position <= path_coords.back().clen; s+=2)
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
	
	if (length <= 0.0 || (lower_dashgroup_count <= 1 && length < minimum_optimum_length - minimum_optimum_num_dashes * switch_deviation))
	{
		// Line part too short for dashes
		if (is_part_end)
		{
			// Can't be handled correctly by the next round of dash groups drawing:
			// Just draw a continuous line.
			processContinuousLine(path,
			                      line_start, end,
			                      !half_first_group, !half_last_group,
			                      out_flags, out_coords, set_mid_symbols, output);
		}
		else
		{
			// Give this length to the next round of dash groups drawing.
			return line_start;
		}
	}
	else
	{
		int higher_dashgroup_count = qRound(ceil(num_dashgroups_f));
		double lower_dashgroup_deviation  = (length_plus_break - lower_dashgroup_count * total_group_and_break_length)  / lower_dashgroup_count;
		double higher_dashgroup_deviation = (higher_dashgroup_count * total_group_and_break_length - length_plus_break) / higher_dashgroup_count;
		Q_ASSERT(half_first_group || half_last_group || (lower_dashgroup_deviation >= -0.001 && higher_dashgroup_deviation >= -0.001)); // TODO; seems to fail as long as halving first/last dashes affects the outermost dash only
		int num_dashgroups = (lower_dashgroup_deviation > higher_dashgroup_deviation) ? higher_dashgroup_count : lower_dashgroup_count;
		Q_ASSERT(num_dashgroups >= 2);
		
		int num_half_dashes = 2*num_dashgroups*dashes_in_group - num_half_groups;
		double adjusted_dash_length = (length - (num_dashgroups-1) * break_length_f - num_dashgroups * total_in_group_break_length) / (0.5 * num_half_dashes);
		adjusted_dash_length = qMax(adjusted_dash_length, 0.0);	// could be negative for large break lengths
		
		double cur_length = start.clen;
		SplitPathCoord dash_start = line_start;
		
		for (int dashgroup = 1; dashgroup <= num_dashgroups; ++dashgroup)
		{
			bool is_first_dashgroup = dashgroup == 1;
			bool is_last_dashgroup  = dashgroup == num_dashgroups;
			
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
				double cur_dash_length = is_half_dash ? adjusted_dash_length / 2 : adjusted_dash_length;
				set_mid_symbols = !is_half_dash;
				
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
				processContinuousLine(path,
				                      dash_start, dash_end,
				                      has_start, has_end,
				                      out_flags, out_coords, set_mid_symbols, output);
				cur_length += cur_dash_length;
				dash_start = dash_end;
				
				if (dash < dashes_in_group)
					cur_length += in_group_break_length_f;
			}
			
			if (dashgroup < num_dashgroups)
				cur_length += break_length_f;
		}
		
		if (half_last_group && mid_symbols_per_spot > 0 && mid_symbol && !mid_symbol->isEmpty())
		{
			// Handle mid symbols at end for closing point or for (some) explicit dash points.
			auto split = start;
			
			PathCoord::length_type position = end.clen - (mid_symbols_per_spot-1) * 0.5 * mid_symbol_distance_f;
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
			auto params = path.calculateTangentScaling(i);
			//params.first.perpRight();
			params.second = qMin(params.second, 2.0 * LineSymbol::miterLimit());
			dash_symbol->createRenderablesScaled(coords[i], params.first.angle(), output, params.second);
		}
	}
}

void LineSymbol::createMidSymbolRenderables(
        const VirtualPath& path,
        bool path_closed,
        ObjectRenderables& output) const
{
	Q_ASSERT(mid_symbol);
	auto orientation = 0.0f;
	bool mid_symbol_rotatable = bool(mid_symbol) && mid_symbol->isRotatable();
	
	int mid_symbol_num_gaps       = mid_symbols_per_spot - 1;
	
	double segment_length_f       = 0.001 * segment_length;
	double end_length_f           = 0.001 * end_length;
	double end_length_twice_f     = 0.002 * end_length;
	double mid_symbol_distance_f  = 0.001 * mid_symbol_distance;
	double mid_symbols_length     = mid_symbol_num_gaps * mid_symbol_distance_f;
	
	auto& path_coords = path.path_coords;
	Q_ASSERT(!path_coords.empty());
	
	auto groups_start = SplitPathCoord::begin(path_coords);
	if (end_length == 0 && !path_closed)
	{
		// Insert point at start coordinate
		if (mid_symbol_rotatable)
			orientation = groups_start.tangentVector().angle();
		mid_symbol->createRenderablesScaled(groups_start.pos, orientation, output);
	}
	
	auto part_end = path.last_index;
	while (groups_start.index != part_end)
	{
		auto groups_end_path_coord_index = path_coords.findNextDashPoint(groups_start.path_coord_index);
		auto groups_end = SplitPathCoord::at(path_coords, groups_end_path_coord_index);
		
		// The total length of the current continuous part
		double length = groups_end.clen - groups_start.clen;
		// The length which is available for placing mid symbols
		double segmented_length = qMax(0.0, length - end_length_twice_f) - mid_symbols_length;
		// The number of segments to be created by mid symbols
		double segment_count_raw = qMax((end_length == 0) ? 1.0 : 0.0, (segmented_length / (segment_length_f + mid_symbols_length)));
		int lower_segment_count = qFloor(segment_count_raw);
		int higher_segment_count = qCeil(segment_count_raw);
		
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
				double lower_abs_deviation = qAbs(length - lower_segment_count * segment_length_f - (lower_segment_count+1)*mid_symbols_length - end_length_twice_f);
				double higher_abs_deviation = qAbs(length - higher_segment_count * segment_length_f - (higher_segment_count+1)*mid_symbols_length - end_length_twice_f);
				int segment_count = (lower_abs_deviation >= higher_abs_deviation) ? higher_segment_count : lower_segment_count;
				
				double deviation = (lower_abs_deviation >= higher_abs_deviation) ? -higher_abs_deviation : lower_abs_deviation;
				double ideal_length = segment_count * segment_length_f + end_length_twice_f;
				double adjusted_end_length = end_length_f + deviation * (end_length_f / ideal_length);
				double adjusted_segment_length = segment_length_f + deviation * (segment_length_f / ideal_length);
				Q_ASSERT(qAbs(2*adjusted_end_length + segment_count*adjusted_segment_length + (segment_count + 1)*mid_symbols_length - length) < 0.001);
				
				if (adjusted_segment_length >= 0 && (show_at_least_one_symbol || higher_segment_count > 0 || length > end_length_twice_f - 0.5 * (segment_length_f + mid_symbols_length)))
				{
					adjusted_segment_length += mid_symbols_length;
					auto split = groups_start;
					for (int i = 0; i < segment_count + 1; ++i)
					{
						double position = groups_start.clen + adjusted_end_length + i * adjusted_segment_length - mid_symbol_distance_f;
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
				double lower_segment_deviation = qAbs(length - lower_segment_count * segment_length_f - (lower_segment_count+1)*mid_symbols_length) / lower_segment_count;
				double higher_segment_deviation = qAbs(length - higher_segment_count * segment_length_f - (higher_segment_count+1)*mid_symbols_length) / higher_segment_count;
				int segment_count = (lower_segment_deviation > higher_segment_deviation) ? higher_segment_count : lower_segment_count;
				double adapted_segment_length = (length - (segment_count+1)*mid_symbols_length) / segment_count + mid_symbols_length;
				Q_ASSERT(qAbs(segment_count * adapted_segment_length + mid_symbols_length) - length < 0.001f);
				
				if (adapted_segment_length >= mid_symbols_length)
				{
					auto split = groups_start;
					for (int i = 0; i <= segment_count; ++i)
					{
						double position = groups_start.clen + i * adapted_segment_length - mid_symbol_distance_f;
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

void LineSymbol::colorDeleted(const MapColor* color)
{
	bool have_changes = false;
	if (mid_symbol && mid_symbol->containsColor(color))
	{
		mid_symbol->colorDeleted(color);
		have_changes = true;
	}
	if (start_symbol && start_symbol->containsColor(color))
	{
		start_symbol->colorDeleted(color);
		have_changes = true;
	}
	if (end_symbol && end_symbol->containsColor(color))
	{
		end_symbol->colorDeleted(color);
		have_changes = true;
	}
	if (dash_symbol && dash_symbol->containsColor(color))
	{
		dash_symbol->colorDeleted(color);
		have_changes = true;
	}
    if (color == this->color)
	{
		this->color = NULL;
		have_changes = true;
	}
	if (color == border.color)
	{
		this->border.color = NULL;
		have_changes = true;
	}
	if (color == right_border.color)
	{
		this->right_border.color = NULL;
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
	
	const MapColor* dominant_color = mid_symbol ? mid_symbol->guessDominantColor() : NULL;
	if (dominant_color) return dominant_color;
	
	dominant_color = start_symbol ? start_symbol->guessDominantColor() : NULL;
	if (dominant_color) return dominant_color;
	
	dominant_color = end_symbol ? end_symbol->guessDominantColor() : NULL;
	if (dominant_color) return dominant_color;
	
	dominant_color = dash_symbol ? dash_symbol->guessDominantColor() : NULL;
	if (dominant_color) return dominant_color;
	
	return NULL;
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

float LineSymbol::calculateLargestLineExtent(Map* map) const
{
	Q_UNUSED(map);
	float line_extent_f = 0.001f * 0.5f * asLine()->getLineWidth();
	float result = line_extent_f;
	if (asLine()->hasBorder())
	{
		result = qMax(result, line_extent_f + 0.001f * (asLine()->getBorder().shift + 0.5f * asLine()->getBorder().width));
		result = qMax(result, line_extent_f + 0.001f * (asLine()->getRightBorder().shift + 0.5f * asLine()->getRightBorder().width));
	}
	return result;
}

void LineSymbol::setStartSymbol(PointSymbol* symbol)
{
	replaceSymbol(start_symbol, symbol, LineSymbolSettings::tr("Start symbol"));
}
void LineSymbol::setMidSymbol(PointSymbol* symbol)
{
	replaceSymbol(mid_symbol, symbol, LineSymbolSettings::tr("Mid symbol"));
}
void LineSymbol::setEndSymbol(PointSymbol* symbol)
{
	replaceSymbol(end_symbol, symbol, LineSymbolSettings::tr("End symbol"));
}
void LineSymbol::setDashSymbol(PointSymbol* symbol)
{
	replaceSymbol(dash_symbol, symbol, LineSymbolSettings::tr("Dash symbol"));
}
void LineSymbol::replaceSymbol(PointSymbol*& old_symbol, PointSymbol* replace_with, const QString& name)
{
	delete old_symbol;
	old_symbol = replace_with;
	replace_with->setName(name);
}

#ifndef NO_NATIVE_FILE_FORMAT

bool LineSymbol::loadImpl(QIODevice* file, int version, Map* map)
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
	
	if (version <= 22)
	{
		if (!border.load(file, version, map))
			return false;
		right_border.assign(border, NULL);
	}
	else
	{
		bool are_borders_different;
		file->read((char*)&are_borders_different, sizeof(bool));
		
		if (!border.load(file, version, map))
			return false;
		if (are_borders_different)
		{
			if (!right_border.load(file, version, map))
				return false;
		}
		else
			right_border.assign(border, NULL);
	}
	
	cleanupPointSymbols();
	
	return true;
}

#endif

void LineSymbol::saveImpl(QXmlStreamWriter& xml, const Map& map) const
{
	xml.writeStartElement(QString::fromLatin1("line_symbol"));
	xml.writeAttribute(QString::fromLatin1("color"), QString::number(map.findColorIndex(color)));
	xml.writeAttribute(QString::fromLatin1("line_width"), QString::number(line_width));
	xml.writeAttribute(QString::fromLatin1("minimum_length"), QString::number(minimum_length));
	xml.writeAttribute(QString::fromLatin1("join_style"), QString::number(join_style));
	xml.writeAttribute(QString::fromLatin1("cap_style"), QString::number(cap_style));
	xml.writeAttribute(QString::fromLatin1("pointed_cap_length"), QString::number(pointed_cap_length));
	
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
	if (suppress_dash_symbol_at_ends)
		xml.writeAttribute(QString::fromLatin1("suppress_dash_symbol_at_ends"), QString::fromLatin1("true"));
	
	if (start_symbol != NULL)
	{
		xml.writeStartElement(QString::fromLatin1("start_symbol"));
		start_symbol->save(xml, map);
		xml.writeEndElement();
	}
	
	if (mid_symbol != NULL)
	{
		xml.writeStartElement(QString::fromLatin1("mid_symbol"));
		mid_symbol->save(xml, map);
		xml.writeEndElement();
	}
	
	if (end_symbol != NULL)
	{
		xml.writeStartElement(QString::fromLatin1("end_symbol"));
		end_symbol->save(xml, map);
		xml.writeEndElement();
	}
	
	if (dash_symbol != NULL)
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
	pointed_cap_length = attributes.value(QLatin1String("pointed_cap_length")).toInt();
	
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
	suppress_dash_symbol_at_ends = (attributes.value(QLatin1String("suppress_dash_symbol_at_ends")) == QLatin1String("true"));
	
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
				right_border.assign(border, NULL);
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
			PointSymbol* symbol = static_cast<PointSymbol*>(Symbol::load(xml, map, symbol_dict));
			xml.skipCurrentElement();
			return symbol;
		}
		else
			xml.skipCurrentElement();
	}
	return NULL;
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
		if (cap_style == PointedCap && (pointed_cap_length != line->pointed_cap_length))
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

	if ((start_symbol == NULL && line->start_symbol != NULL) ||
		(start_symbol != NULL && line->start_symbol == NULL))
		return false;
	if (start_symbol && !start_symbol->equals(line->start_symbol))
		return false;
	
	if ((mid_symbol == NULL && line->mid_symbol != NULL) ||
		(mid_symbol != NULL && line->mid_symbol == NULL))
		return false;
	if (mid_symbol && !mid_symbol->equals(line->mid_symbol))
		return false;
	
	if ((end_symbol == NULL && line->end_symbol != NULL) ||
		(end_symbol != NULL && line->end_symbol == NULL))
		return false;
	if (end_symbol && !end_symbol->equals(line->end_symbol))
		return false;
	
	if ((dash_symbol == NULL && line->dash_symbol != NULL) ||
		(dash_symbol != NULL && line->dash_symbol == NULL))
		return false;
	if (dash_symbol && !dash_symbol->equals(line->dash_symbol))
		return false;
	if (suppress_dash_symbol_at_ends != line->suppress_dash_symbol_at_ends)
		return false;
	
	if (mid_symbol)
	{
		if (mid_symbols_per_spot != line->mid_symbols_per_spot)
			return false;
		if (mid_symbol_distance != line->mid_symbol_distance)
			return false;
	}

	if (have_border_lines != line->have_border_lines)
		return false;
	if (have_border_lines)
	{
		if (!border.equals(&line->border) || !right_border.equals(&line->right_border))
			return false;
	}
	
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
	
	QWidget* line_tab = new QWidget();
	QGridLayout* layout = new QGridLayout();
	layout->setColumnStretch(1, 1);
	line_tab->setLayout(layout);
	
	QLabel* width_label = new QLabel(tr("Line width:"));
	width_edit = Util::SpinBox::create(2, 0.0f, 999999.9f, tr("mm"));
	
	QLabel* color_label = new QLabel(tr("Line color:"));
	color_edit = new ColorDropDown(map, symbol->getColor());
	
	int row = 0, col = 0;
	layout->addWidget(width_label, row, col++);
	layout->addWidget(width_edit,  row, col);
	
	row++; col = 0;
	layout->addWidget(color_label, row, col++);
	layout->addWidget(color_edit,  row, col, 1, -1);
	
	
	QLabel* minimum_length_label = new QLabel(tr("Minimum line length:"));
	minimum_length_edit = Util::SpinBox::create(2, 0.0f, 999999.9f, tr("mm"));
	
	QLabel* line_cap_label = new QLabel(tr("Line cap:"));
	line_cap_combo = new QComboBox();
	line_cap_combo->addItem(tr("flat"), QVariant(LineSymbol::FlatCap));
	line_cap_combo->addItem(tr("round"), QVariant(LineSymbol::RoundCap));
	line_cap_combo->addItem(tr("square"), QVariant(LineSymbol::SquareCap));
	line_cap_combo->addItem(tr("pointed"), QVariant(LineSymbol::PointedCap));
	line_cap_combo->setCurrentIndex(line_cap_combo->findData(symbol->cap_style));
	
	pointed_cap_length_label = new QLabel(tr("Cap length:"));
	pointed_cap_length_edit = Util::SpinBox::create(2, 0.0f, 999999.9f, tr("mm"));
	
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
	layout->addWidget(Util::Headline::create(tr("Dashed line")), row, col++, 1, -1);
	row++; col = 0;
	layout->addWidget(dashed_check, row, col, 1, -1);
	
	QLabel* dash_length_label = new QLabel(tr("Dash length:"));
	dash_length_edit = Util::SpinBox::create(2, 0.0f, 999999.9f, tr("mm"));
	
	QLabel* break_length_label = new QLabel(tr("Break length:"));
	break_length_edit = Util::SpinBox::create(2, 0.0f, 999999.9f, tr("mm"));
	
	QLabel* dash_group_label = new QLabel(tr("Dashes grouped together:"));
	dash_group_combo = new QComboBox();
	dash_group_combo->addItem(tr("none"), QVariant(1));
	dash_group_combo->addItem(tr("2"), QVariant(2));
	dash_group_combo->addItem(tr("3"), QVariant(3));
	dash_group_combo->addItem(tr("4"), QVariant(4));
	dash_group_combo->setCurrentIndex(dash_group_combo->findData(QVariant(symbol->dashes_in_group)));
	
	in_group_break_length_label = new QLabel(tr("In-group break length:"));
	in_group_break_length_edit = Util::SpinBox::create(2, 0.0f, 999999.9f, tr("mm"));
	
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
	layout->addWidget(Util::Headline::create(tr("Mid symbols")), row, col, 1, -1);
	
	
	QLabel* mid_symbol_per_spot_label = new QLabel(tr("Mid symbols per spot:"));
	mid_symbol_per_spot_edit = Util::SpinBox::create(1, 99);
	
	mid_symbol_distance_label = new QLabel(tr("Mid symbol distance:"));
	mid_symbol_distance_edit = Util::SpinBox::create(2, 0.0f, 999999.9f, tr("mm"));
	
	mid_symbol_widget_list
	  << mid_symbol_per_spot_label << mid_symbol_per_spot_edit
	  << mid_symbol_distance_label << mid_symbol_distance_edit;
	
	row++; col = 0;
	layout->addWidget(mid_symbol_per_spot_label, row, col++);
	layout->addWidget(mid_symbol_per_spot_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(mid_symbol_distance_label, row, col++);
	layout->addWidget(mid_symbol_distance_edit, row, col, 1, -1);
	
	
	QLabel* segment_length_label = new QLabel(tr("Distance between spots:"));
	segment_length_edit = Util::SpinBox::create(2, 0.0f, 999999.9f, tr("mm"));
	
	QLabel* end_length_label = new QLabel(tr("Distance from line end:"));
	end_length_edit = Util::SpinBox::create(2, 0.0f, 999999.9f, tr("mm"));
	
	show_at_least_one_symbol_check = new QCheckBox(tr("Show at least one mid symbol"));
	show_at_least_one_symbol_check->setChecked(symbol->show_at_least_one_symbol);
	
	QLabel* minimum_mid_symbol_count_label = new QLabel(tr("Minimum mid symbol count:"));
	minimum_mid_symbol_count_edit = Util::SpinBox::create(0, 99);
	
	QLabel* minimum_mid_symbol_count_when_closed_label = new QLabel(tr("Minimum mid symbol count when closed:"));
	minimum_mid_symbol_count_when_closed_edit = Util::SpinBox::create(0, 99);
	
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
	layout->addWidget(Util::Headline::create(tr("Borders")), row, col++, 1, -1);
	row++; col = 0;
	layout->addWidget(border_check, row, col, 1, -1);
	
	different_borders_check = new QCheckBox(tr("Different borders on left and right sides"));
	different_borders_check->setChecked(symbol->areBordersDifferent());
	row++; col = 0;
	layout->addWidget(different_borders_check, row, col, 1, -1);
	
	QLabel* left_border_label = Util::Headline::create(tr("Left border:"));
	row++; col = 0;
	layout->addWidget(left_border_label, row, col++, 1, -1);
	createBorderWidgets(symbol->getBorder(), map, row, col, layout, border_widgets);
	
	QLabel* right_border_label = Util::Headline::create(tr("Right border:"));
	row++; col = 0;
	layout->addWidget(right_border_label, row, col++, 1, -1);
	createBorderWidgets(symbol->getRightBorder(), map, row, col, layout, right_border_widgets);
	
	border_widget_list << different_borders_check << border_widgets.widget_list;
	different_borders_widget_list << left_border_label << right_border_label << right_border_widgets.widget_list;
	
	
	row++; col = 0;
	layout->addWidget(new QWidget(), row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(Util::Headline::create(tr("Dash symbol")), row, col++, 1, -1);
	
	supress_dash_symbol_check = new QCheckBox(tr("Suppress the dash symbol at line start and line end"));
	supress_dash_symbol_check->setChecked(symbol->getSuppressDashSymbolAtLineEnds());
	row++; col = 0;
	layout->addWidget(supress_dash_symbol_check, row, col, 1, -1);
	
	row++;
	layout->setRowStretch(row, 1);
	
	const int line_tab_width = line_tab->sizeHint().width();
	
	scroll_area = new QScrollArea();
	scroll_area->setWidget(line_tab);
	scroll_area->setWidgetResizable(true);
	scroll_area->setFrameShape(QFrame::NoFrame);
	scroll_area->setMinimumWidth(line_tab_width + scroll_area->verticalScrollBar()->sizeHint().width());
	addPropertiesGroup(tr("Line settings"), scroll_area);
	
	PointSymbolEditorWidget* point_symbol_editor = 0;
	MapEditorController* controller = dialog->getPreviewController();
	
	symbol->ensurePointSymbols(tr("Start symbol"), tr("Mid symbol"), tr("End symbol"), tr("Dash symbol"));
	QList<PointSymbol*> point_symbols;
	point_symbols << symbol->getStartSymbol() << symbol->getMidSymbol() << symbol->getEndSymbol() << symbol->getDashSymbol();
	for (auto point_symbol : point_symbols)
	{
		point_symbol_editor = new PointSymbolEditorWidget(controller, point_symbol, 16);
		addPropertiesGroup(point_symbol->getName(), point_symbol_editor);
		connect(point_symbol_editor, SIGNAL(symbolEdited()), this, SLOT(pointSymbolEdited()));
	}
	
	updateStates();
	updateContents();
	
	connect(width_edit, SIGNAL(valueChanged(double)), this, SLOT(widthChanged(double)));
	connect(color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(colorChanged()));
	connect(minimum_length_edit, SIGNAL(valueChanged(double)), this, SLOT(minimumDimensionsEdited()));
	connect(line_cap_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(lineCapChanged(int)));
	connect(line_join_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(lineJoinChanged(int)));
	connect(pointed_cap_length_edit, SIGNAL(valueChanged(double)), this, SLOT(pointedLineCapLengthChanged(double)));
	connect(dashed_check, SIGNAL(clicked(bool)), this, SLOT(dashedChanged(bool)));
	connect(segment_length_edit, SIGNAL(valueChanged(double)), this, SLOT(segmentLengthChanged(double)));
	connect(end_length_edit, SIGNAL(valueChanged(double)), this, SLOT(endLengthChanged(double)));
	connect(show_at_least_one_symbol_check, SIGNAL(clicked(bool)), this, SLOT(showAtLeastOneSymbolChanged(bool)));
	connect(minimum_mid_symbol_count_edit, SIGNAL(valueChanged(int)), this, SLOT(minimumDimensionsEdited()));
	connect(minimum_mid_symbol_count_when_closed_edit, SIGNAL(valueChanged(int)), this, SLOT(minimumDimensionsEdited()));
	connect(dash_length_edit, SIGNAL(valueChanged(double)), this, SLOT(dashLengthChanged(double)));
	connect(break_length_edit, SIGNAL(valueChanged(double)), this, SLOT(breakLengthChanged(double)));
	connect(dash_group_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(dashGroupsChanged(int)));
	connect(in_group_break_length_edit, SIGNAL(valueChanged(double)), this, SLOT(inGroupBreakLengthChanged(double)));
	connect(half_outer_dashes_check, SIGNAL(clicked(bool)), this, SLOT(halfOuterDashesChanged(bool)));
	connect(mid_symbol_per_spot_edit, SIGNAL(valueChanged(int)), this, SLOT(midSymbolsPerDashChanged(int)));
	connect(mid_symbol_distance_edit, SIGNAL(valueChanged(double)), this, SLOT(midSymbolDistanceChanged(double)));
	connect(border_check, SIGNAL(clicked(bool)), this, SLOT(borderCheckClicked(bool)));
	connect(different_borders_check, SIGNAL(clicked(bool)), this, SLOT(differentBordersClicked(bool)));
	connect(supress_dash_symbol_check, SIGNAL(clicked(bool)), this, SLOT(suppressDashSymbolClicked(bool)));
}

LineSymbolSettings::~LineSymbolSettings()
{
}

void LineSymbolSettings::pointSymbolEdited()
{
	emit propertiesModified();
	updateStates();
}

void LineSymbolSettings::widthChanged(double value)
{
	symbol->line_width = 1000.0 * value;
	emit propertiesModified();
	updateStates();
}

void LineSymbolSettings::colorChanged()
{
	symbol->color = color_edit->color();
	emit propertiesModified();
	updateStates();
}

void LineSymbolSettings::minimumDimensionsEdited()
{
	symbol->minimum_length = qRound(1000.0 * minimum_length_edit->value());
	symbol->minimum_mid_symbol_count = minimum_mid_symbol_count_edit->value();
	symbol->minimum_mid_symbol_count_when_closed = minimum_mid_symbol_count_when_closed_edit->value();
	emit propertiesModified();
}

void LineSymbolSettings::lineCapChanged(int index)
{
	symbol->cap_style = (LineSymbol::CapStyle)line_cap_combo->itemData(index).toInt();
	emit propertiesModified();
	updateStates();
}

void LineSymbolSettings::lineJoinChanged(int index)
{
	symbol->join_style = (LineSymbol::JoinStyle)line_join_combo->itemData(index).toInt();
	emit propertiesModified();
}

void LineSymbolSettings::pointedLineCapLengthChanged(double value)
{
	symbol->pointed_cap_length = qRound(1000.0 * value);
	emit propertiesModified();
}

void LineSymbolSettings::dashedChanged(bool checked)
{
	symbol->dashed = checked;
	if (symbol->color == NULL)
	{
		symbol->break_length = 0;
		break_length_edit->setValue(0);
	}
	emit propertiesModified();
	updateStates();
	if (checked && symbol->color != NULL)
		ensureWidgetVisible(half_outer_dashes_check);
}

void LineSymbolSettings::segmentLengthChanged(double value)
{
	symbol->segment_length = qRound(1000.0 * value);
	emit propertiesModified();
}

void LineSymbolSettings::endLengthChanged(double value)
{
	symbol->end_length = qRound(1000.0 * value);
	emit propertiesModified();
	updateStates();
}

void LineSymbolSettings::showAtLeastOneSymbolChanged(bool checked)
{
	symbol->show_at_least_one_symbol = checked;
	emit propertiesModified();
}

void LineSymbolSettings::dashLengthChanged(double value)
{
	symbol->dash_length = qRound(1000.0 * value);
	emit propertiesModified();
}

void LineSymbolSettings::breakLengthChanged(double value)
{
	symbol->break_length = qRound(1000.0 * value);
	emit propertiesModified();
}

void LineSymbolSettings::dashGroupsChanged(int index)
{
	symbol->dashes_in_group = dash_group_combo->itemData(index).toInt();
	if (symbol->dashes_in_group > 1)
	{
		symbol->half_outer_dashes = false;
		half_outer_dashes_check->setChecked(false);
	}
	emit propertiesModified();
	updateStates();
}

void LineSymbolSettings::inGroupBreakLengthChanged(double value)
{
	symbol->in_group_break_length = qRound(1000.0 * value);
	emit propertiesModified();
}

void LineSymbolSettings::halfOuterDashesChanged(bool checked)
{
	symbol->half_outer_dashes = checked;
	emit propertiesModified();
}

void LineSymbolSettings::midSymbolsPerDashChanged(int value)
{
	symbol->mid_symbols_per_spot = qMax(1, value);
	emit propertiesModified();
	updateStates();
}

void LineSymbolSettings::midSymbolDistanceChanged(double value)
{
	symbol->mid_symbol_distance = qRound(1000.0 * value);
	emit propertiesModified();
}

void LineSymbolSettings::borderCheckClicked(bool checked)
{
	symbol->have_border_lines = checked;
	emit propertiesModified();
	updateStates();
	if (checked)
	{
		if (symbol->areBordersDifferent())
			ensureWidgetVisible(right_border_widgets.break_length_edit);
		else
			ensureWidgetVisible(border_widgets.break_length_edit);
	}
}

void LineSymbolSettings::differentBordersClicked(bool checked)
{
	if (checked)
	{
		updateStates();
		blockSignalsRecursively(this, true);
		updateBorderContents(symbol->getRightBorder(), right_border_widgets);
		blockSignalsRecursively(this, false);
		ensureWidgetVisible(right_border_widgets.break_length_edit);
	}
	else
	{
		symbol->getRightBorder().assign(symbol->getBorder(), NULL);
		emit propertiesModified();
		updateStates();
	}
}

void LineSymbolSettings::borderChanged()
{
	updateBorder(symbol->getBorder(), border_widgets);
	if (different_borders_check->isChecked())
		updateBorder(symbol->getRightBorder(), right_border_widgets);
	else
		symbol->getRightBorder().assign(symbol->getBorder(), NULL);
	
	emit propertiesModified();
}

void LineSymbolSettings::suppressDashSymbolClicked(bool checked)
{
	symbol->suppress_dash_symbol_at_ends = checked;
	emit propertiesModified();
}

void LineSymbolSettings::createBorderWidgets(LineSymbolBorder& border, Map* map, int& row, int col, QGridLayout* layout, BorderWidgets& widgets)
{
	QLabel* width_label = new QLabel(tr("Border width:"));
	widgets.width_edit = Util::SpinBox::create(2, 0.0f, 999999.9f, tr("mm"));
	
	QLabel* color_label = new QLabel(tr("Border color:"));
	widgets.color_edit = new ColorDropDown(map, border.color);
	
	QLabel* shift_label = new QLabel(tr("Border shift:"));
	widgets.shift_edit = Util::SpinBox::create(2, 0.0f, 999999.9f, tr("mm"));
	
	widgets.dashed_check = new QCheckBox(tr("Border is dashed"));
	widgets.dashed_check->setChecked(border.dashed);
	
	widgets.widget_list
		<< width_label << widgets.width_edit
		<< color_label << widgets.color_edit
		<< shift_label << widgets.shift_edit
		<< widgets.dashed_check;
	
	row++; col = 0;
	layout->addWidget(width_label, row, col++);
	layout->addWidget(widgets.width_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(color_label, row, col++);
	layout->addWidget(widgets.color_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(shift_label, row, col++);
	layout->addWidget(widgets.shift_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(widgets.dashed_check, row, col, 1, -1);
	
	QLabel* dash_length_label = new QLabel(tr("Border dash length:"));
	widgets.dash_length_edit = Util::SpinBox::create(2, 0.0f, 999999.9f, tr("mm"));
	
	QLabel* break_length_label = new QLabel(tr("Border break length:"));
	widgets.break_length_edit = Util::SpinBox::create(2, 0.0f, 999999.9f, tr("mm"));
	
	widgets.dash_widget_list
		<< dash_length_label << widgets.dash_length_edit
		<< break_length_label << widgets.break_length_edit;
	
	row++; col = 0;
	layout->addWidget(dash_length_label, row, col++);
	layout->addWidget(widgets.dash_length_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(break_length_label, row, col++);
	layout->addWidget(widgets.break_length_edit, row, col, 1, -1);
	
	
	connect(widgets.width_edit, SIGNAL(valueChanged(double)), this, SLOT(borderChanged()));
	connect(widgets.color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(borderChanged()));
	connect(widgets.shift_edit, SIGNAL(valueChanged(double)), this, SLOT(borderChanged()));
	connect(widgets.dashed_check, SIGNAL(clicked(bool)), this, SLOT(borderChanged()));
	connect(widgets.dash_length_edit, SIGNAL(valueChanged(double)), this, SLOT(borderChanged()));
	connect(widgets.break_length_edit, SIGNAL(valueChanged(double)), this, SLOT(borderChanged()));
}

void LineSymbolSettings::updateBorder(LineSymbolBorder& border, LineSymbolSettings::BorderWidgets& widgets)
{
	bool ensure_last_widget_visible = false;
	border.width = qRound(1000.0 * widgets.width_edit->value());
	border.color = widgets.color_edit->color();
	border.shift = qRound(1000.0 * widgets.shift_edit->value());
	if (widgets.dashed_check->isChecked() && !border.dashed)
		ensure_last_widget_visible = true;
	border.dashed = widgets.dashed_check->isChecked();
	border.dash_length = qRound(1000.0 * widgets.dash_length_edit->value());
	border.break_length = qRound(1000.0 * widgets.break_length_edit->value());
	
	updateStates();
	if (ensure_last_widget_visible)
		ensureWidgetVisible(widgets.break_length_edit);
}

void LineSymbolSettings::updateBorderContents(LineSymbolBorder& border, LineSymbolSettings::BorderWidgets& widgets)
{
	Q_ASSERT(this->signalsBlocked());
	
	widgets.width_edit->setValue(0.001 * border.width);
	widgets.color_edit->setColor(border.color);
	widgets.shift_edit->setValue(0.001 * border.shift);
	widgets.dashed_check->setChecked(border.dashed);
	
	widgets.dash_length_edit->setValue(0.001 * border.dash_length);
	widgets.break_length_edit->setValue(0.001 * border.break_length);
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

void LineSymbolSettings::updateStates()
{
	const bool symbol_active = symbol->line_width > 0;
	color_edit->setEnabled(symbol_active);
	
	const bool line_active = symbol_active && symbol->color != NULL;
	for (auto line_settings_widget : line_settings_list)
	{
		line_settings_widget->setEnabled(line_active);
	}
	if (line_active && symbol->cap_style != LineSymbol::PointedCap)
	{
		pointed_cap_length_label->setEnabled(false);
		pointed_cap_length_edit->setEnabled(false);
	}
	
	const bool line_dashed = symbol->dashed && symbol->color != NULL;
	if (line_dashed)
	{
		for (auto undashed_widget : undashed_widget_list)
		{
			undashed_widget->setVisible(false);
		}
		for (auto dashed_widget : dashed_widget_list)
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
		for (auto undashed_widget : undashed_widget_list)
		{
			undashed_widget->setVisible(true);
			undashed_widget->setEnabled(!symbol->mid_symbol->isEmpty());
		}
		show_at_least_one_symbol_check->setEnabled(show_at_least_one_symbol_check->isEnabled() && symbol->end_length > 0);
		for (auto dashed_widget : dashed_widget_list)
		{
			dashed_widget->setVisible(false);
		}
	}
	
	for (auto mid_symbol_widget : mid_symbol_widget_list)
	{
		mid_symbol_widget->setEnabled(!symbol->mid_symbol->isEmpty());
	}
	mid_symbol_distance_label->setEnabled(mid_symbol_distance_label->isEnabled() && symbol->mid_symbols_per_spot > 1);
	mid_symbol_distance_edit->setEnabled(mid_symbol_distance_edit->isEnabled() && symbol->mid_symbols_per_spot > 1);
	
	const bool border_active = symbol_active && symbol->have_border_lines;
	for (auto border_widget : border_widget_list)
	{
		border_widget->setVisible(border_active);
		border_widget->setEnabled(border_active);
	}
	for (auto border_dash_widget : border_widgets.dash_widget_list)
	{
		border_dash_widget->setVisible(border_active);
		border_dash_widget->setEnabled(border_active && symbol->getBorder().dashed);
	}
	const bool different_borders = border_active && different_borders_check->isChecked();
	for (auto different_border_widget : different_borders_widget_list)
	{
		different_border_widget->setVisible(different_borders);
		different_border_widget->setEnabled(different_borders);
	}
	for (auto border_dash_widget : right_border_widgets.dash_widget_list)
	{
		border_dash_widget->setVisible(different_borders);
		border_dash_widget->setEnabled(different_borders && symbol->getRightBorder().dashed);
	}
}

void LineSymbolSettings::updateContents()
{
	blockSignalsRecursively(this, true);
	width_edit->setValue(0.001 * symbol->getLineWidth());
	color_edit->setColor(symbol->getColor());
	
	minimum_length_edit->setValue(0.001 * symbol->minimum_length);
	line_cap_combo->setCurrentIndex(line_cap_combo->findData(symbol->cap_style));
	pointed_cap_length_edit->setValue(0.001 * symbol->pointed_cap_length);
	line_join_combo->setCurrentIndex(line_join_combo->findData(symbol->join_style));
	dashed_check->setChecked(symbol->dashed);
	border_check->setChecked(symbol->have_border_lines);
	different_borders_check->setChecked(symbol->areBordersDifferent());
	
	dash_length_edit->setValue(0.001 * symbol->dash_length);
	break_length_edit->setValue(0.001 * symbol->break_length);
	dash_group_combo->setCurrentIndex(dash_group_combo->findData(QVariant(symbol->dashes_in_group)));
	in_group_break_length_edit->setValue(0.001 * symbol->in_group_break_length);
	half_outer_dashes_check->setChecked(symbol->half_outer_dashes);
	
	mid_symbol_per_spot_edit->setValue(symbol->mid_symbols_per_spot);
	mid_symbol_distance_edit->setValue(0.001 * symbol->mid_symbol_distance);
	
	segment_length_edit->setValue(0.001 * symbol->segment_length);
	end_length_edit->setValue(0.001 * symbol->end_length);
	show_at_least_one_symbol_check->setChecked(symbol->show_at_least_one_symbol);
	minimum_mid_symbol_count_edit->setValue(symbol->minimum_mid_symbol_count);
	minimum_mid_symbol_count_when_closed_edit->setValue(symbol->minimum_mid_symbol_count_when_closed);
	
	updateBorderContents(symbol->getBorder(), border_widgets);
	updateBorderContents(symbol->getRightBorder(), right_border_widgets);
	
	supress_dash_symbol_check->setChecked(symbol->getSuppressDashSymbolAtLineEnds());
	
	blockSignalsRecursively(this, false);
/*	
	PointSymbolEditorWidget* point_symbol_editor = 0;
	MapEditorController* controller = dialog->getPreviewController();
	
	QList<PointSymbol*> point_symbols;
	point_symbols << symbol->getStartSymbol() << symbol->getMidSymbol() << symbol->getEndSymbol() << symbol->getDashSymbol();
	for (auto point_symbol : point_symbols)
	{
		point_symbol_editor = new PointSymbolEditorWidget(controller, point_symbol, 16);
		addPropertiesGroup(point_symbol->getName(), point_symbol_editor);
		connect(point_symbol_editor, SIGNAL(symbolEdited()), this, SLOT(pointSymbolEdited()));
	}	*/

	updateStates();
}

void LineSymbolSettings::reset(Symbol* symbol)
{
	Q_ASSERT(symbol->getType() == Symbol::Line);
	
	SymbolPropertiesWidget::reset(symbol);
	
	LineSymbol* old_symbol = this->symbol;
	this->symbol = reinterpret_cast<LineSymbol*>(symbol);
	
	PointSymbolEditorWidget* point_symbol_editor = 0;
	MapEditorController* controller = dialog->getPreviewController();
	
	int current = currentIndex();
	setUpdatesEnabled(false);
	this->symbol->ensurePointSymbols(tr("Start symbol"), tr("Mid symbol"), tr("End symbol"), tr("Dash symbol"));
	QList<PointSymbol*> point_symbols;
	point_symbols << this->symbol->getStartSymbol() << this->symbol->getMidSymbol() << this->symbol->getEndSymbol() << this->symbol->getDashSymbol();
	for (auto point_symbol : point_symbols)
	{
		point_symbol_editor = new PointSymbolEditorWidget(controller, point_symbol, 16);
		connect(point_symbol_editor, SIGNAL(symbolEdited()), this, SLOT(pointSymbolEdited()));
		
		int index = indexOfPropertiesGroup(point_symbol->getName()); // existing symbol editor
		removePropertiesGroup(index);
		insertPropertiesGroup(index, point_symbol->getName(), point_symbol_editor);
		if (index == current)
			setCurrentIndex(current);
	}
	updateContents();
	setUpdatesEnabled(true);
	old_symbol->cleanupPointSymbols();
}
