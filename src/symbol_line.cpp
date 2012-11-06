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

#include <cassert>

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QIODevice>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "map.h"
#include "map_color.h"
#include "object.h"
#include "qbezier_p.h"
#include "renderable_implementation.h"
#include "symbol_setting_dialog.h"
#include "symbol_point_editor.h"
#include "symbol_point.h"
#include "symbol_area.h"
#include "util.h"
#include "util_gui.h"

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

void LineSymbolBorder::save(QIODevice* file, Map* map)
{
	int temp = map->findColorIndex(color);
	file->write((const char*)&temp, sizeof(int));
	file->write((const char*)&width, sizeof(int));
	file->write((const char*)&shift, sizeof(int));
	file->write((const char*)&dashed, sizeof(bool));
	file->write((const char*)&dash_length, sizeof(int));
	file->write((const char*)&break_length, sizeof(int));
}

bool LineSymbolBorder::load(QIODevice* file, int version, Map* map)
{
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

void LineSymbolBorder::save(QXmlStreamWriter& xml, const Map& map) const
{
	xml.writeStartElement("border");
	xml.writeAttribute("color", QString::number(map.findColorIndex(color)));
	xml.writeAttribute("width", QString::number(width));
	xml.writeAttribute("shift", QString::number(shift));
	if (dashed)
		xml.writeAttribute("dashed", "true");
	xml.writeAttribute("dash_length", QString::number(dash_length));
	xml.writeAttribute("break_length", QString::number(break_length));
	xml.writeEndElement(/*border*/);
}

bool LineSymbolBorder::load(QXmlStreamReader& xml, Map& map)
{
	Q_ASSERT(xml.name() == "border");
	
	QXmlStreamAttributes attributes = xml.attributes();
	int temp = attributes.value("color").toString().toInt();
	color = (temp >= 0) ? map.getColor(temp) : NULL;
	width = attributes.value("width").toString().toInt();
	shift = attributes.value("shift").toString().toInt();
	dashed = (attributes.value("dashed") == "true");
	dash_length = attributes.value("dash_length").toString().toInt();
	break_length = attributes.value("break_length").toString().toInt();
	xml.skipCurrentElement();
	return !xml.error();
}

bool LineSymbolBorder::equals(const LineSymbolBorder* other) const
{
	if (!Symbol::colorEquals(color, other->color))
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

void LineSymbolBorder::assign(const LineSymbolBorder& other, const QHash<MapColor*, MapColor*>* color_map)
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

void LineSymbolBorder::createSymbol(LineSymbol& out)
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

Symbol* LineSymbol::duplicate(const QHash<MapColor*, MapColor*>* color_map) const
{
	LineSymbol* new_line = new LineSymbol();
	new_line->duplicateImplCommon(this);
	new_line->line_width = line_width;
	new_line->color = color_map ? color_map->value(color) : color;
	new_line->minimum_length = minimum_length;
	new_line->cap_style = cap_style;
	new_line->join_style = join_style;
	new_line->pointed_cap_length = pointed_cap_length;
	new_line->start_symbol = start_symbol ? static_cast<PointSymbol*>(start_symbol->duplicate(color_map)) : NULL;
	new_line->mid_symbol = mid_symbol ? static_cast<PointSymbol*>(mid_symbol->duplicate(color_map)) : NULL;
	new_line->end_symbol = end_symbol ? static_cast<PointSymbol*>(end_symbol->duplicate(color_map)) : NULL;
	new_line->dash_symbol = dash_symbol ? static_cast<PointSymbol*>(dash_symbol->duplicate(color_map)) : NULL;
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
	new_line->border.assign(border, color_map);
	new_line->right_border.assign(right_border, color_map);
	new_line->cleanupPointSymbols();
	return new_line;
}

void LineSymbol::createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output)
{
	PathObject* path = reinterpret_cast<PathObject*>(object);
	int num_parts = path->getNumParts();
	if (num_parts == 1)
		createRenderables(object, path->getPart(0).isClosed(), flags, coords, (PathCoordVector*)&path->getPathCoordinateVector(), output);
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
			createRenderables(object, part.isClosed(), part_flags, part_coords, &part_path_coords, output);
		}
	}
}

void LineSymbol::createRenderables(Object* object, bool path_closed, const MapCoordVector& flags, const MapCoordVectorF& coords, PathCoordVector* path_coords, ObjectRenderables& output)
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
			if (point_object.getSymbol()->asPoint()->isRotatable())
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
			if (point_object.getSymbol()->asPoint()->isRotatable())
				point_object.setRotation(atan2(-tangent.getY(), tangent.getX()));
			MapCoordVectorF end_coord;
			end_coord.push_back(coords[last]);
			end_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), end_coord, output);
		}
	}
	
	// Dash symbols?
	if (dash_symbol && !dash_symbol->isEmpty())
		createDashSymbolRenderables(object, path_closed, flags, coords, output);
	
	// The line itself
	MapCoordVector processed_flags;
	MapCoordVectorF processed_coords;
	bool create_border = have_border_lines && (border.isVisible() || right_border.isVisible());
	if (!dashed)
	{
		// Base line?
		if (line_width > 0)
		{
			if (color && (cap_style != PointedCap || pointed_cap_length == 0) && !create_border)
				output.insertRenderable(new LineRenderable(this, coords, flags, *path_coords, path_closed));
			else if (create_border || (cap_style == PointedCap && pointed_cap_length > 0))
			{
				int part_start = 0;
				int part_end = 0;
				PathCoordVector line_coords;
				while (PathCoord::getNextPathPart(flags, coords, part_start, part_end, &line_coords, false, false))
				{
					int cur_line_coord = 1;
					bool has_start = !(part_start == 0 && path_closed);
					bool has_end = !(part_end == (int)flags.size() - 1 && path_closed);
					
					processContinuousLine(object, path_closed, flags, coords, line_coords, 0, line_coords[line_coords.size() - 1].clen,
										  has_start, has_end, cur_line_coord, processed_flags, processed_coords, true, false, output);
				}
			}
		}
		
		// Symbols?
		if (mid_symbol && !mid_symbol->isEmpty() && segment_length > 0)
			createDottedRenderables(object, path_closed, flags, coords, output);
	}
	else
	{
		// Dashed lines
		if (dash_length > 0)
			processDashedLine(object, path_closed, flags, coords, processed_flags, processed_coords, output);
		else
			return;	// wrong parameter
	}
	
	assert(processed_coords.size() != 1);
	if (processed_coords.size() >= 2)
	{
		// Create main line renderable
		if (color)
			output.insertRenderable(new LineRenderable(this, processed_coords, processed_flags, *path_coords, path_closed));
		
		// Border lines?
		if (create_border)
			createBorderLines(object, processed_flags, processed_coords, path_closed, output);
	}
}

void LineSymbol::createBorderLines(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, bool path_closed, ObjectRenderables& output)
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
			
			border_symbol.processDashedLine(object, path_closed, flags, coords, dashed_flags, dashed_coords, output);
			border_symbol.dashed = false;	// important, otherwise more dashes might be added by createRenderables()!
			
			shiftCoordinates(dashed_flags, dashed_coords, path_closed, main_shift, border_flags, border_coords);
			border_symbol.createRenderables(object, path_closed, border_flags, border_coords, NULL, output);
			shiftCoordinates(dashed_flags, dashed_coords, path_closed, -main_shift, border_flags, border_coords);
			border_symbol.createRenderables(object, path_closed, border_flags, border_coords, NULL, output);
		}
		else
		{
			shiftCoordinates(flags, coords, path_closed, main_shift, border_flags, border_coords);
			border_symbol.createRenderables(object, path_closed, border_flags, border_coords, NULL, output);
			shiftCoordinates(flags, coords, path_closed, -main_shift, border_flags, border_coords);
			border_symbol.createRenderables(object, path_closed, border_flags, border_coords, NULL, output);
		}
	}
	else
	{
		createBorderLine(object, flags, coords, path_closed, output, border, -main_shift);
		createBorderLine(object, flags, coords, path_closed, output, right_border, main_shift);
	}
}

void LineSymbol::createBorderLine(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, bool path_closed, ObjectRenderables& output, LineSymbolBorder& border, double main_shift)
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
		
		border_symbol.processDashedLine(object, path_closed, flags, coords, dashed_flags, dashed_coords, output);
		border_symbol.dashed = false;	// important, otherwise more dashes might be added by createRenderables()!
		
		shiftCoordinates(dashed_flags, dashed_coords, path_closed, main_shift, border_flags, border_coords);
		border_symbol.createRenderables(object, path_closed, border_flags, border_coords, NULL, output);
	}
	else
	{
		shiftCoordinates(flags, coords, path_closed, main_shift, border_flags, border_coords);
		border_symbol.createRenderables(object, path_closed, border_flags, border_coords, NULL, output);
	}
}

void LineSymbol::shiftCoordinates(const MapCoordVector& flags, const MapCoordVectorF& coords, bool path_closed, double main_shift, MapCoordVector& out_flags, MapCoordVectorF& out_coords)
{
	const float curve_threshold = 0.03f;	// TODO: decrease for export/print?
	const int MAX_OFFSET = 16;
	QBezierCopy offsetCurves[MAX_OFFSET];
	
	double miter_limit = 2.0 * miterLimit(); // needed more than once
	if (miter_limit <= 0.0)
		miter_limit = 1.0e6;                 // assert(miter_limit != 0)
	double miter_reference = 0.0;            // reference value: 
	if (join_style == MiterJoin)             //  when to bevel MiterJoins
		miter_reference = cos(atan(4.0 / miter_limit));
	
	// sign of shift and main shift indicates left or right border
	// but u_border_shift is unsigned
	double u_border_shift = 0.001 * ((main_shift > 0.0 && areBordersDifferent()) ? right_border.shift : border.shift);
	double shift = main_shift + ((main_shift > 0.0) ? u_border_shift : -u_border_shift);
	
	int size = flags.size();
	out_flags.clear();
	out_coords.clear();
	out_flags.reserve(size);
	out_coords.reserve(size);
	
	const MapCoord no_flags;
	
	bool ok_in, ok_out;
	MapCoordF segment_start, 
	          right_vector, end_right_vector,
	          tangent_in, tangent_out, 
	          middle0, middle1;
	int last_i = size-1;
	for (int i = 0; i < size; ++i)
	{
		const MapCoordF& coords_i = coords[i];
		const MapCoord& flags_i  = flags[i];
		
		tangent_in = PathCoord::calculateIncomingTangent(coords, path_closed, i, ok_in);
		tangent_in.normalize();
		tangent_out = PathCoord::calculateOutgoingTangent(coords, path_closed, i, ok_out);
		tangent_out.normalize();
		
		if (!ok_in && !ok_out)
		{
			// Rare but existing case.  No valid solution, but
			// at least we need to output a point to handle the flags correctly.
			qDebug() << "No valid tangent at" << __FILE__ << __LINE__;
			segment_start = coords_i;
		}
		else if (!path_closed && i == 0)
		{
			// Simple start point
			right_vector = tangent_out;
			right_vector.perpRight();
			segment_start = coords_i + shift * right_vector;
		}
		else if (!path_closed && i == last_i)
		{
			// Simple end point
			right_vector = tangent_in;
			right_vector.perpRight();
			segment_start = coords_i + shift * right_vector;
		}
		else
		{
			// Corner point
			if (!ok_in)
				tangent_in = tangent_out;
			else if (!ok_out)
				tangent_out = tangent_in;
			
			right_vector = tangent_out;
			right_vector.perpRight();
			
			middle0 = MapCoordF(tangent_in.getX() + tangent_out.getX(), tangent_in.getY() + tangent_out.getY());
			middle0.normalize();
			double offset;
				
			// Determine type of corner (inner vs. outer side of corner)
			double a = (tangent_out.getX() * tangent_in.getY() - tangent_in.getX() * tangent_out.getY()) * main_shift;
			if (a > 0.0)
			{
				// Outer side of corner
				
				if (join_style == BevelJoin || join_style == RoundJoin)
				{
					middle1 = MapCoordF(tangent_in.getX() + middle0.getX(), tangent_in.getY() + middle0.getY());
					middle1.normalize();
					double phi1 = acos(middle1.getX() * tangent_in.getX() + middle1.getY() * tangent_in.getY());
					offset = tan(phi1) * u_border_shift;
					
					if (i > 0)
					{
						// First border corner point
						end_right_vector = tangent_in;
						end_right_vector.perpRight();
						out_flags.push_back(no_flags);
						out_coords.push_back(coords_i + shift * end_right_vector + offset * tangent_in);
						
						if (join_style == RoundJoin)
						{
							// Extra border corner point
							// TODO: better approximation of round corner / use bezier curve
							middle0.perpRight();
							out_flags.push_back(no_flags);
							out_coords.push_back(coords_i + shift * middle0);
						}
					}
				}
				else /* join_style == MiterJoin */
				{
					// miter_check has no concrete interpretation, 
					// but was derived by mathematical simplifications.
					double miter_check = middle0.getX() * tangent_in.getX() + middle0.getY() * tangent_in.getY();
					if (miter_check <= miter_reference)
					{
						// Two border corner points
						middle1 = MapCoordF(tangent_in.getX() + middle0.getX(), tangent_in.getY() + middle0.getY());
						middle1.normalize();
						double phi1 = acos(middle1.getX() * tangent_in.getX() + middle1.getY() * tangent_in.getY());
						offset = miter_limit * fabs(main_shift) + tan(phi1) * u_border_shift;
						
						if (i > 0)
						{
							// First border corner point
							end_right_vector = tangent_in;
							end_right_vector.perpRight();
							out_flags.push_back(no_flags);
							out_coords.push_back(coords_i + shift * end_right_vector + offset * tangent_in);
						}
					}
					else
					{
						middle0.perpRight();
						double phi = acos(middle0.getX() * tangent_in.getX() + middle0.getY() * tangent_in.getY());
						offset = fabs(1.0/tan(phi) * shift);
					}
				}
				
				// single or second border corner point
				segment_start = coords_i + shift * right_vector - offset * tangent_out;
			}
			else if (i > 2 && flags[i-3].isCurveStart() && flags_i.isCurveStart())
			{
				// Inner side of corner (or no corner), and both sides are beziers
				// old behaviour
				right_vector = middle0;
				right_vector.perpRight();
				double phi = acos(right_vector.getX() * tangent_in.getX() + right_vector.getY() * tangent_in.getY());
				double sin_phi = sin(phi);
				double inset = (sin_phi > (1.0/miter_limit)) ? (1.0 / sin_phi) : miter_limit;
				segment_start = coords_i + (shift * inset) * right_vector;
			}
			else
			{
				// Inner side of corner (or no corner), and no more than on bezier involved
				
				// Default solution
				middle0.perpRight();
				double phi = acos(middle0.getX() * tangent_in.getX() + middle0.getY() * tangent_in.getY());
				double tan_phi = tan(phi);
				offset = -fabs(shift/tan_phi);
				segment_start = coords_i + shift * right_vector - offset * tangent_out;
				
				// Check critical cases
				if (tan_phi < 1.0)
				{
					a = 0.0; // negative values indicate inappropriate default solution
					double len_in = coords_i.lengthToSquared(coords[(i - 1) % size]);
					double len_out = coords_i.lengthToSquared(coords[(i + 1) % size]);
					if (len_in < len_out)
					{
						// Check if default solution is further away than start of incoming border
						MapCoordF predecessor = coords[(i - 1) % size];
						MapCoordF pred_right = PathCoord::calculateOutgoingTangent(coords, path_closed, (i - 1) % size, ok_in);
						pred_right.perpRight();
						pred_right.normalize();
						pred_right *= shift;
						predecessor = predecessor + pred_right - segment_start;
						a = (pred_right.getX() * predecessor.getY() - pred_right.getY() * predecessor.getX()) * shift;
					}
					else
					{
						// Check if default solution is further away than end of outgoing border
						MapCoordF successor = coords[(i + 1) % size];
						MapCoordF succ_right = PathCoord::calculateIncomingTangent(coords, path_closed, (i + 1) % size, ok_out);
						succ_right.perpRight();
						succ_right.normalize();
						succ_right *= shift;
						successor = segment_start - successor - succ_right;
						a = (succ_right.getX() * successor.getY() - succ_right.getY() * successor.getX()) * shift;
					}
						
					if (a < -0.0) 
					{
						// Default solution is too far off from main path
						if (len_in < len_out)
						{
							segment_start = coords_i + shift * right_vector + sqrt(len_in) * tangent_out;
						}
						else
						{
							right_vector = tangent_in;
							right_vector.perpRight();
							segment_start = coords_i + shift * right_vector - sqrt(len_out) * tangent_in;
						}
					}
#ifdef MAPPER_FILL_MITER_LIMIT_GAP
					else
					{
						// Avoid miter limit effects by extra points
						// This is be nice for ISOM roads, but not for powerlines...
						// TODO: add another flag to the signature?
						out_flags.push_back(no_flags);
						out_coords.push_back(coords_i + shift * right_vector - offset * tangent_out);
						
						out_flags.push_back(no_flags);
						out_coords.push_back(out_coords.back() - (shift - main_shift) * qMax(0.0, 1.0 / sin(phi) - miter_limit) * middle0);
						
						// single or second border corner point
						segment_start = coords_i + shift * right_vector - offset * tangent_out;
					}
#endif
				}
			}
		}
		
		out_flags.push_back(flags_i);
		out_coords.push_back(segment_start);
		
		if (flags_i.isCurveStart())
		{
			// Use QBezierCopy code to shift the curve, but set start and end point manually to get the correct end points (because of line joins)
			// TODO: it may be necessary to remove some of the generated curves in the case an outer point is moved inwards
			if (main_shift > 0.0)
			{
				QBezierCopy bezier = QBezierCopy::fromPoints(coords[(i+3) % size].toQPointF(), coords[i+2].toQPointF(), coords[i+1].toQPointF(), coords_i.toQPointF());
				int count = bezier.shifted(offsetCurves, MAX_OFFSET, qAbs(shift), curve_threshold);
				for (int j = count - 1; j >= 0; --j)
				{
					out_flags[out_flags.size() - 1].setCurveStart(true);
					
					out_flags.push_back(no_flags);
					out_coords.push_back(MapCoordF(offsetCurves[j].pt3().x(), offsetCurves[j].pt3().y()));
					
					out_flags.push_back(no_flags);
					out_coords.push_back(MapCoordF(offsetCurves[j].pt2().x(), offsetCurves[j].pt2().y()));
					
					if (j > 0)
					{
						out_flags.push_back(no_flags);
						out_coords.push_back(MapCoordF(offsetCurves[j].pt1().x(), offsetCurves[j].pt1().y()));
					}
				}
			}
			else
			{
				QBezierCopy bezier = QBezierCopy::fromPoints(coords[i].toQPointF(), coords[i+1].toQPointF(), coords[i+2].toQPointF(), coords[(i+3) % size].toQPointF());
				int count = bezier.shifted(offsetCurves, MAX_OFFSET, qAbs(shift), curve_threshold);
				for (int j = 0; j < count; ++j)
				{
					out_flags[out_flags.size() - 1].setCurveStart(true);
					
					out_flags.push_back(no_flags);
					out_coords.push_back(MapCoordF(offsetCurves[j].pt2().x(), offsetCurves[j].pt2().y()));
					
					out_flags.push_back(no_flags);
					out_coords.push_back(MapCoordF(offsetCurves[j].pt3().x(), offsetCurves[j].pt3().y()));
					
					if (j < count - 1)
					{
						out_flags.push_back(no_flags);
						out_coords.push_back(MapCoordF(offsetCurves[j].pt4().x(), offsetCurves[j].pt4().y()));
					}
				}
			}
			
			i += 2;
		}
	}
}

void LineSymbol::processContinuousLine(Object* object, bool path_closed, const MapCoordVector& flags, const MapCoordVectorF& coords, const PathCoordVector& line_coords,
									   float start, float end, bool has_start, bool has_end, int& cur_line_coord,
									   MapCoordVector& processed_flags, MapCoordVectorF& processed_coords, bool include_first_point, bool set_mid_symbols, ObjectRenderables& output)
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
		createPointedLineCap(object, flags, coords, line_coords, start, start + adapted_cap_length, cur_line_coord, false, output);
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
		createPointedLineCap(object, flags, coords, line_coords, end - adapted_cap_length, end, cur_line_coord, true, output);
	}
	if (has_end && !processed_flags.empty())
		processed_flags[processed_flags.size() - 1].setHolePoint(true);
}

void LineSymbol::createPointedLineCap(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, const PathCoordVector& line_coords,
									  float start, float end, int& cur_line_coord, bool is_end, ObjectRenderables& output)
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
		//assert(factor >= -0.01f && factor <= 1.01f); happens when using large break lengths as these are not adjusted
		factor = qMax(0.0f, qMin(1.0f, factor));
		
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
	
	// Create small overlap to avoid visual glitches with the on-screen display (should not matter for printing, could probably turn it off for this)
	const float overlap_length = 0.05f;
	if (end - start > 4 * overlap_length)
	{
		bool ok;
		int end_pos = is_end ? 0 : (cap_size - 1);
		int end_cap_pos = is_end ? 0 : (cap_coords.size() - 1);
		MapCoordF tangent = PathCoord::calculateTangent(cap_middle_coords, end_pos, !is_end, ok);
		if (ok)
		{
			tangent.normalize();
			tangent = MapCoordF(tangent.getX() * overlap_length * sign, tangent.getY() * overlap_length * sign);
			MapCoordF right = is_end ? tangent : -tangent;
			right.perpRight();
			
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
				cap_flags.push_back(MapCoord());
				cap_coords.push_back(shifted_coord);
				cap_coords_left.push_back(shifted_coord_left);
			}
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
	output.insertRenderable(new AreaRenderable(&area_symbol, cap_coords, cap_flags, NULL));
}

void LineSymbol::processDashedLine(Object* object, bool path_closed, const MapCoordVector& flags, const MapCoordVectorF& coords, MapCoordVector& out_flags, MapCoordVectorF& out_coords, ObjectRenderables& output)
{
	int last_coord = (int)coords.size() - 1;
	
	PointObject point_object(mid_symbol);
	bool point_object_rotatable = mid_symbol ? point_object.getSymbol()->asPoint()->isRotatable() : false;
	MapCoordVectorF point_coord;
	point_coord.push_back(MapCoordF(0, 0));
	MapCoordF right_vector;
	
	double dash_length_f           = 0.001 * dash_length;
	double break_length_f          = 0.001 * break_length;
	double in_group_break_length_f = 0.001 * in_group_break_length;
	double mid_symbol_distance_f   = 0.001 * mid_symbol_distance;
	
	double switch_deviation = 0.2 * ((dashes_in_group*dash_length_f + break_length_f + (dashes_in_group-1)*in_group_break_length_f)) / dashes_in_group;
	double minimum_optimum_length = (2*dashes_in_group*dash_length_f + break_length_f + 2*(dashes_in_group-1)*in_group_break_length_f);
	
	int part_start = 0;
	int part_end = 0;
	PathCoordVector line_coords;
	std::size_t out_coords_size = 4 * coords.size();
	out_flags.reserve(out_coords_size);
	out_coords.reserve(out_coords_size);
	
	bool dash_point_incomplete = false;
	double cur_length = 0.0;
	double old_length = 0.0;	// length from line part(s) before dash point(s) which is not accounted for yet
	int first_line_coord = 0;
	int cur_line_coord = 1;
	while (PathCoord::getNextPathPart(flags, coords, part_start, part_end, &line_coords, true, true))
	{
		if (dash_point_incomplete)
		{
			double position = line_coords[first_line_coord].clen + ((mid_symbols_per_spot+1)%2) * 0.5 * mid_symbol_distance_f;
			int line_coord_search_start = first_line_coord;
			for (int s = 1; s < mid_symbols_per_spot; s+=2)
			{
				PathCoord::calculatePositionAt(flags, coords, line_coords, position, line_coord_search_start, &point_coord[0], &right_vector);
				if (point_object_rotatable)
					point_object.setRotation(atan2(right_vector.getX(), right_vector.getY()));
				mid_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), point_coord, output);
				position += mid_symbol_distance_f;
			}
			dash_point_incomplete = false;
		}
		
		if (part_start > 0 && flags[part_start-1].isHolePoint())
		{
			++first_line_coord;
			cur_line_coord = first_line_coord + 1;
			while (line_coords[cur_line_coord].clen == 0.0f)
				++cur_line_coord;	// TODO: Debug where double identical line coords with clen == 0 can come from? Happened with pointed line ends + dashed border lines in a closed path
			cur_length = line_coords[first_line_coord].clen;
		}
		
		bool starts_with_dashpoint = (part_start > 0 && flags[part_start].isDashPoint());
		bool ends_with_dashpoint = (part_end < last_coord && flags[part_end].isDashPoint());
		bool half_first_dash = (part_start == 0 && (half_outer_dashes || path_closed)) || (starts_with_dashpoint && dashes_in_group == 1);
		bool half_last_dash = (part_end == last_coord && (half_outer_dashes || path_closed)) || (ends_with_dashpoint && dashes_in_group == 1);
		int half_first_last_dash = (half_first_dash ? 1 : 0) + (half_last_dash ? 1 : 0);
		
		int last_line_coord = (int)line_coords.size() - 1;
		double length = line_coords[last_line_coord].clen - line_coords[first_line_coord].clen;
		
		double num_dashgroups_f = 
		  (length + break_length_f - half_first_last_dash * 0.5 * dash_length_f) /
		  (break_length_f + dashes_in_group * dash_length_f + (dashes_in_group-1) * in_group_break_length_f) +
		  half_first_last_dash;
		int lower_dashgroup_count = qRound(floor(num_dashgroups_f));
		double minimum_optimum_num_dashes = dashes_in_group * 2.0 - half_first_last_dash * 0.5;
		
		if (length <= 0.0 || (lower_dashgroup_count <= 1 && length < minimum_optimum_length - minimum_optimum_num_dashes * switch_deviation))
		{
			// Line part too short for dashes, use just one continuous line for it
			if (!ends_with_dashpoint)
			{
				processContinuousLine(object, path_closed, flags, coords, line_coords, cur_length, cur_length + length + old_length,
									  (!path_closed && out_flags.empty()) || (!out_flags.empty() && out_flags.back().isHolePoint()), !(path_closed && part_end == last_coord),
									  cur_line_coord, out_flags, out_coords, true, old_length == 0 && length >= dash_length_f - switch_deviation, output);
				cur_length += length + old_length;
				old_length = 0.0;
			}
			else
				old_length += length;
		}
		else
		{
			int higher_dashgroup_count = qRound(ceil(num_dashgroups_f));
			double lower_dashgroup_deviation = (length - (lower_dashgroup_count*dashes_in_group*dash_length_f + (lower_dashgroup_count-1)*break_length_f + lower_dashgroup_count*(dashes_in_group-1)*in_group_break_length_f)) / (lower_dashgroup_count*dashes_in_group);
			double higher_dashgroup_deviation = (-1) * (length - (higher_dashgroup_count*dashes_in_group*dash_length_f + (higher_dashgroup_count-1)*break_length_f + higher_dashgroup_count*(dashes_in_group-1)*in_group_break_length_f)) / (higher_dashgroup_count*dashes_in_group);
			if (!half_first_dash && !half_last_dash)
				assert(lower_dashgroup_deviation >= -0.001 && higher_dashgroup_deviation >= -0.001); // TODO; seems to fail as long as halving first/last dashes affects the outermost dash only
			int num_dashgroups = (lower_dashgroup_deviation > higher_dashgroup_deviation) ? higher_dashgroup_count : lower_dashgroup_count;
			assert(num_dashgroups >= 2);
			
			int num_half_dashes = 2*num_dashgroups*dashes_in_group - half_first_last_dash;
			double adapted_dash_length = (length - (num_dashgroups-1)*break_length_f - num_dashgroups*(dashes_in_group-1)*in_group_break_length_f) / (0.5*num_half_dashes);
			adapted_dash_length = qMax(adapted_dash_length, 0.0);	// could be negative for large break lengths
			
			for (int dashgroup = 1; dashgroup <= num_dashgroups; ++dashgroup)
			{
				for (int dash = 1; dash <= dashes_in_group; ++dash)
				{
					bool is_first_dash = dashgroup == 1 && dash == 1;
					bool is_half_dash = (is_first_dash && half_first_dash) || (dashgroup == num_dashgroups && dash == dashes_in_group && half_last_dash);
					double cur_dash_length = is_half_dash ? adapted_dash_length / 2 : adapted_dash_length;
					
					// Process immediately if this is not the last dash before a dash point
					if (!(ends_with_dashpoint && dash == dashes_in_group && dashgroup == num_dashgroups))
					{
						// The dash has an end if it is not the last dash in a closed path
						bool has_end = !(dash == dashes_in_group && dashgroup == num_dashgroups && path_closed && part_end == last_coord);
						
						processContinuousLine(object, path_closed, flags, coords, line_coords, cur_length, cur_length + old_length + cur_dash_length,
											  (!path_closed && out_flags.empty()) || (!out_flags.empty() && out_flags.back().isHolePoint()), has_end,
											  cur_line_coord, out_flags, out_coords, true, !is_half_dash, output);
						cur_length += old_length + cur_dash_length;
						old_length = 0;
						//dash_point_before = false;
						
						if (dash < dashes_in_group)
							cur_length += in_group_break_length_f;
					}
					else
						old_length += cur_dash_length;
				}
				
				if (dashgroup < num_dashgroups)
					cur_length += break_length_f;
			}
		}
		
		if (ends_with_dashpoint && dashes_in_group == 1 && mid_symbol && !mid_symbol->isEmpty())
		{
			double position = line_coords[last_line_coord].clen - (mid_symbols_per_spot-1) * 0.5 * mid_symbol_distance_f;
			int line_coord_search_start = 0;
			for (int s = 0; s < mid_symbols_per_spot; s+=2)
			{
				PathCoord::calculatePositionAt(flags, coords, line_coords, position, line_coord_search_start, &point_coord[0], &right_vector);
				if (point_object_rotatable)
					point_object.setRotation(atan2(right_vector.getX(), right_vector.getY()));
				mid_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), point_coord, output);
				position += mid_symbol_distance_f;
			}
			if (mid_symbols_per_spot > 1)
				dash_point_incomplete = true;
		}
		
		cur_length = line_coords[last_line_coord].clen - old_length;
		
		first_line_coord = last_line_coord;
	}
}

void LineSymbol::createDashSymbolRenderables(Object* object, bool path_closed, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output)
{
	PointObject point_object(dash_symbol);
	MapCoordVectorF point_coord;
	point_coord.push_back(MapCoordF(0, 0));
	MapCoordF right_vector;
	
	int size = (int)coords.size();
	for (int i = 0; i < size; ++i)
	{
		// Only apply the symbol at dash points
		if (!flags[i].isDashPoint()) continue;
		
		float scaling;
		MapCoordF right = PathCoord::calculateRightVector(flags, coords, path_closed, i, &scaling);
		
		if (point_object.getSymbol()->asPoint()->isRotatable())
			point_object.setRotation(atan2(right.getX(), right.getY()));
		point_coord[0] = coords[i];
		dash_symbol->createRenderablesScaled(&point_object, point_object.getRawCoordinateVector(), point_coord, output, scaling);
	}
}

void LineSymbol::createDottedRenderables(Object* object, bool path_closed, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output)
{
	Q_ASSERT(mid_symbol != NULL);
	PointObject point_object(mid_symbol);
	bool point_object_rotatable = point_object.getSymbol()->asPoint()->isRotatable();
	MapCoordVectorF point_coord;
	point_coord.push_back(MapCoordF(0, 0));
	MapCoordF right_vector;
	
	int mid_symbol_num_gaps       = mid_symbols_per_spot - 1;
	
	double segment_length_f       = 0.001 * segment_length;
	double end_length_f           = 0.001 * end_length;
	double end_length_twice_f     = 0.002 * end_length;
	double mid_symbol_distance_f  = 0.001 * mid_symbol_distance;
	double mid_symbols_length     = mid_symbol_num_gaps * mid_symbol_distance_f;
	
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
				if (point_object_rotatable)
					point_object.setRotation(atan2(right_vector.getX(), right_vector.getY()));
				point_coord[0] = coords[part_start];
				mid_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), point_coord, output);
			}
			
			is_first_part = false;
		}
		
		// The total length of the current continuous part
		double length = line_coords.back().clen;
		// The length which is available for placing mid symbols
		double segmented_length = qMax(0.0, length - end_length_twice_f) - mid_symbols_length;
		// The number of segments to be created by mid symbols
		double segment_count_raw = qMax((end_length == 0) ? 1.0 : 0.0, (segmented_length / (segment_length_f + mid_symbols_length)));
		int lower_segment_count = (int)floor(segment_count_raw);
		int higher_segment_count = (int)ceil(segment_count_raw);

		int line_coord_search_start = 0;
		if (end_length > 0)
		{
			if (length <= mid_symbols_length)
			{
				if (show_at_least_one_symbol)
				{
					// Insert point at start coordinate
					if (point_object_rotatable)
					{
						right_vector = PathCoord::calculateRightVector(flags, coords, path_closed, part_start, NULL);
						point_object.setRotation(atan2(right_vector.getX(), right_vector.getY()));
					}
					point_coord[0] = coords[part_start];
					mid_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), point_coord, output);
					
					// Insert point at end coordinate
					if (point_object_rotatable)
					{
						right_vector = PathCoord::calculateRightVector(flags, coords, path_closed, part_end, NULL);
						point_object.setRotation(atan2(right_vector.getX(), right_vector.getY()));
					}
					point_coord[0] = coords[part_end];
					mid_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), point_coord, output);
				}
			}
			else
			{
				double lower_abs_deviation = qAbs(length - lower_segment_count * segment_length_f - (lower_segment_count+1)*mid_symbols_length - end_length_twice_f);
				double higher_abs_deviation = qAbs(length - higher_segment_count * segment_length_f - (higher_segment_count+1)*mid_symbols_length - end_length_twice_f);
				int segment_count = (lower_abs_deviation >= higher_abs_deviation) ? higher_segment_count : lower_segment_count;
				
				double deviation = (lower_abs_deviation >= higher_abs_deviation) ? -higher_abs_deviation : lower_abs_deviation;
				double ideal_length = segment_count * segment_length_f + end_length_twice_f;
				double adapted_end_length = end_length_f + deviation * (end_length_f / ideal_length);
				double adapted_segment_length = segment_length_f + deviation * (segment_length_f / ideal_length);
				assert(qAbs(2*adapted_end_length + segment_count*adapted_segment_length + (segment_count + 1)*mid_symbols_length - length) < 0.001);
				
				if (adapted_segment_length >= 0 && (show_at_least_one_symbol || higher_segment_count > 0 || length > end_length_twice_f - 0.5 * (segment_length_f + mid_symbols_length)))
				{
					adapted_segment_length += mid_symbols_length;
					for (int i = 0; i < segment_count + 1; ++i)
					{
						double base_position = adapted_end_length + i * adapted_segment_length;
						for (int s = 0; s < mid_symbols_per_spot; ++s)
						{
							PathCoord::calculatePositionAt(flags, coords, line_coords, base_position + s * mid_symbol_distance_f, line_coord_search_start, &point_coord[0], &right_vector);
							if (point_object_rotatable)
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
						double base_position = i * adapted_segment_length;
						for (int s = 0; s < mid_symbols_per_spot; ++s)
						{
							// The outermost symbols are handled outside this loop
							if (i == 0 && s == 0)
								continue;
							if (i == segment_count && s == mid_symbol_num_gaps)
								break;
							
							PathCoord::calculatePositionAt(flags, coords, line_coords, base_position + s * mid_symbol_distance_f, line_coord_search_start, &point_coord[0], &right_vector);
							if (point_object_rotatable)
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
			if (point_object_rotatable)
			{
				right_vector = PathCoord::calculateRightVector(flags, coords, path_closed, part_end, NULL);
				point_object.setRotation(atan2(right_vector.getX(), right_vector.getY()));
			}
			point_coord[0] = coords[part_end];
			mid_symbol->createRenderables(&point_object, point_object.getRawCoordinateVector(), point_coord, output);
		}
		
		if (flags[part_end].isHolePoint())
			is_first_part = true;
	}
}

void LineSymbol::calculateCoordinatesForRange(const MapCoordVector& flags, const MapCoordVectorF& coords, const PathCoordVector& line_coords,
										float start, float end, int& cur_line_coord, bool include_start_coord, MapCoordVector& out_flags, MapCoordVectorF& out_coords,
										std::vector<float>* out_lengths, bool set_mid_symbols, ObjectRenderables& output)
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
		//assert(factor >= -0.01f && factor <= 1.01f); happens when using large break lengths as these are not adjusted
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
			
			if (point_object.getSymbol()->asPoint()->isRotatable())
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
		//assert(factor >= -0.01f && factor <= 1.01f); happens when using large break lengths as these are not adjusted
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
			if (!(p <= 1))
				p = 1;
			else if (!(p >= 0))
				p = 0;
			
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

void LineSymbol::colorDeleted(MapColor* color)
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

bool LineSymbol::containsColor(MapColor* color)
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

MapColor* LineSymbol::getDominantColorGuess()
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
	
	MapColor* dominant_color = mid_symbol ? mid_symbol->getDominantColorGuess() : NULL;
	if (dominant_color) return dominant_color;
	
	dominant_color = start_symbol ? start_symbol->getDominantColorGuess() : NULL;
	if (dominant_color) return dominant_color;
	
	dominant_color = end_symbol ? end_symbol->getDominantColorGuess() : NULL;
	if (dominant_color) return dominant_color;
	
	dominant_color = dash_symbol ? dash_symbol->getDominantColorGuess() : NULL;
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

void LineSymbol::setStartSymbol(PointSymbol* symbol)
{
	replaceSymbol(start_symbol, symbol, QObject::tr("Start symbol"));
}
void LineSymbol::setMidSymbol(PointSymbol* symbol)
{
	replaceSymbol(mid_symbol, symbol, QObject::tr("Mid symbol"));
}
void LineSymbol::setEndSymbol(PointSymbol* symbol)
{
	replaceSymbol(end_symbol, symbol, QObject::tr("End symbol"));
}
void LineSymbol::setDashSymbol(PointSymbol* symbol)
{
	replaceSymbol(dash_symbol, symbol, QObject::tr("Dash symbol"));
}
void LineSymbol::replaceSymbol(PointSymbol*& old_symbol, PointSymbol* replace_with, const QString& name)
{
	delete old_symbol;
	old_symbol = replace_with;
	replace_with->setName(name);
}

void LineSymbol::saveImpl(QIODevice* file, Map* map)
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
	
	bool are_borders_different = areBordersDifferent();
	file->write((const char*)&are_borders_different, sizeof(bool));
	
	border.save(file, map);
	if (are_borders_different)
		right_border.save(file, map);
}

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
	return true;
}

void LineSymbol::saveImpl(QXmlStreamWriter& xml, const Map& map) const
{
	xml.writeStartElement("line_symbol");
	xml.writeAttribute("color", QString::number(map.findColorIndex(color)));
	xml.writeAttribute("line_width", QString::number(line_width));
	xml.writeAttribute("minimum_length", QString::number(minimum_length));
	xml.writeAttribute("join_style", QString::number(join_style));
	xml.writeAttribute("cap_style", QString::number(cap_style));
	xml.writeAttribute("pointed_cap_length", QString::number(pointed_cap_length));
	
	if (dashed)
		xml.writeAttribute("dashed", "true");
	xml.writeAttribute("segment_length", QString::number(segment_length));
	xml.writeAttribute("end_length", QString::number(end_length));
	if (show_at_least_one_symbol)
		xml.writeAttribute("show_at_least_one_symbol", "true");
	xml.writeAttribute("minimum_mid_symbol_count", QString::number(minimum_mid_symbol_count));
	xml.writeAttribute("minimum_mid_symbol_count_when_closed", QString::number(minimum_mid_symbol_count_when_closed));
	xml.writeAttribute("dash_length", QString::number(dash_length));
	xml.writeAttribute("break_length", QString::number(break_length));
	xml.writeAttribute("dashes_in_group", QString::number(dashes_in_group));
	xml.writeAttribute("in_group_break_length", QString::number(in_group_break_length));
	if (half_outer_dashes)
		xml.writeAttribute("half_outer_dashes", "true");
	xml.writeAttribute("mid_symbols_per_spot", QString::number(mid_symbols_per_spot));
	xml.writeAttribute("mid_symbol_distance", QString::number(mid_symbol_distance));
	
	if (start_symbol != NULL)
	{
		xml.writeStartElement("start_symbol");
		start_symbol->save(xml, map);
		xml.writeEndElement();
	}
	
	if (mid_symbol != NULL)
	{
		xml.writeStartElement("mid_symbol");
		mid_symbol->save(xml, map);
		xml.writeEndElement();
	}
	
	if (end_symbol != NULL)
	{
		xml.writeStartElement("end_symbol");
		end_symbol->save(xml, map);
		xml.writeEndElement();
	}
	
	if (dash_symbol != NULL)
	{
		xml.writeStartElement("dash_symbol");
		dash_symbol->save(xml, map);
		xml.writeEndElement();
	}
	
	if (have_border_lines)
	{
		xml.writeStartElement("borders");
		bool are_borders_different = areBordersDifferent();
		if (are_borders_different)
			xml.writeAttribute("borders_different", "true");
		border.save(xml, map);
		if (are_borders_different)
			right_border.save(xml, map);
		xml.writeEndElement(/*borders*/);
	}
	
	xml.writeEndElement(/*line_symbol*/);
}

bool LineSymbol::loadImpl(QXmlStreamReader& xml, Map& map, SymbolDictionary& symbol_dict)
{
	if (xml.name() != "line_symbol")
		return false;
	
	QXmlStreamAttributes attributes = xml.attributes();
	int temp = attributes.value("color").toString().toInt();
	color = (temp >= 0) ? map.getColor(temp) : NULL;
	line_width = attributes.value("line_width").toString().toInt();
	minimum_length = attributes.value("minimum_length").toString().toInt();
	join_style = static_cast<LineSymbol::JoinStyle>(attributes.value("join_style").toString().toInt());
	cap_style = static_cast<LineSymbol::CapStyle>(attributes.value("cap_style").toString().toInt());
	pointed_cap_length = attributes.value("pointed_cap_length").toString().toInt();
	
	dashed = (attributes.value("dashed") == "true");
	segment_length = attributes.value("segment_length").toString().toInt();
	end_length = attributes.value("end_length").toString().toInt();
	show_at_least_one_symbol = (attributes.value("show_at_least_one_symbol") == "true");
	minimum_mid_symbol_count = attributes.value("minimum_mid_symbol_count").toString().toInt();
	minimum_mid_symbol_count_when_closed = attributes.value("minimum_mid_symbol_count_when_closed").toString().toInt();
	dash_length = attributes.value("dash_length").toString().toInt();
	break_length = attributes.value("break_length").toString().toInt();
	dashes_in_group = attributes.value("dashes_in_group").toString().toInt();
	in_group_break_length = attributes.value("in_group_break_length").toString().toInt();
	half_outer_dashes = (attributes.value("half_outer_dashes") == "true");
	mid_symbols_per_spot = attributes.value("mid_symbols_per_spot").toString().toInt();
	mid_symbol_distance = attributes.value("mid_symbol_distance").toString().toInt();
	
	have_border_lines = false;
	while (xml.readNextStartElement())
	{
		if (xml.name() == "start_symbol")
			start_symbol = loadPointSymbol(xml, map, symbol_dict);
		else if (xml.name() == "mid_symbol")
			mid_symbol = loadPointSymbol(xml, map, symbol_dict);
		else if (xml.name() == "end_symbol")
			end_symbol = loadPointSymbol(xml, map, symbol_dict);
		else if (xml.name() == "dash_symbol")
			dash_symbol = loadPointSymbol(xml, map, symbol_dict);
		else if (xml.name() == "borders")
		{
//			bool are_borders_different = (xml.attributes().value("borders_different") == "true");
			
			bool right_border_loaded = false;
			while (xml.readNextStartElement())
			{
				if (xml.name() == "border")
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
	return true;
}

PointSymbol* LineSymbol::loadPointSymbol(QXmlStreamReader& xml, Map& map, SymbolDictionary& symbol_dict)
{
	while (xml.readNextStartElement())
	{
		if (xml.name() == "symbol")
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

bool LineSymbol::equalsImpl(Symbol* other, Qt::CaseSensitivity case_sensitivity)
{
	LineSymbol* line = static_cast<LineSymbol*>(other);
	if (line_width != line->line_width)
		return false;
	if (minimum_length != line->minimum_length)
		return false;
	if (line_width > 0)
	{
		if (!colorEquals(color, line->color))
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
	layout->addWidget(new QLabel(QString("<b>%1</b>").arg(tr("Dashed line"))), row, col++, 1, -1);
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
	layout->addWidget(new QLabel(QString("<b>%1</b>").arg(tr("Mid symbols"))), row, col, 1, -1);
	
	
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
	layout->addWidget(new QLabel(QString("<b>%1</b>").arg(tr("Borders"))), row, col++, 1, -1);
	row++; col = 0;
	layout->addWidget(border_check, row, col, 1, -1);
	
	different_borders_check = new QCheckBox(tr("Different borders on left and right sides"));
	different_borders_check->setChecked(symbol->areBordersDifferent());
	row++; col = 0;
	layout->addWidget(different_borders_check, row, col, 1, -1);
	
	QLabel* left_border_label = new QLabel(QString("<b>%1</b>").arg(tr("Left border:")));
	row++; col = 0;
	layout->addWidget(left_border_label, row, col++, 1, -1);
	createBorderWidgets(symbol->getBorder(), map, row, col, layout, border_widgets);
	
	QLabel* right_border_label = new QLabel(QString("<b>%1</b>").arg(tr("Right border:")));
	row++; col = 0;
	layout->addWidget(right_border_label, row, col++, 1, -1);
	createBorderWidgets(symbol->getRightBorder(), map, row, col, layout, right_border_widgets);
	
	border_widget_list << different_borders_check << border_widgets.widget_list;
	different_borders_widget_list << left_border_label << right_border_label << right_border_widgets.widget_list;
	
	
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
	Q_FOREACH(PointSymbol* point_symbol, point_symbols)
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
	assert(this->signalsBlocked());
	
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
	Q_FOREACH(QWidget* line_settings_widget, line_settings_list)
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
			undashed_widget->setEnabled(!symbol->mid_symbol->isEmpty());
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
	Q_FOREACH(QWidget* border_dash_widget, border_widgets.dash_widget_list)
	{
		border_dash_widget->setVisible(border_active);
		border_dash_widget->setEnabled(border_active && symbol->getBorder().dashed);
	}
	const bool different_borders = border_active && different_borders_check->isChecked();
	Q_FOREACH(QWidget* different_border_widget, different_borders_widget_list)
	{
		different_border_widget->setVisible(different_borders);
		different_border_widget->setEnabled(different_borders);
	}
	Q_FOREACH(QWidget* border_dash_widget, right_border_widgets.dash_widget_list)
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
	
	blockSignalsRecursively(this, false);
/*	
	PointSymbolEditorWidget* point_symbol_editor = 0;
	MapEditorController* controller = dialog->getPreviewController();
	
	QList<PointSymbol*> point_symbols;
	point_symbols << symbol->getStartSymbol() << symbol->getMidSymbol() << symbol->getEndSymbol() << symbol->getDashSymbol();
	Q_FOREACH(PointSymbol* point_symbol, point_symbols)
	{
		point_symbol_editor = new PointSymbolEditorWidget(controller, point_symbol, 16);
		addPropertiesGroup(point_symbol->getName(), point_symbol_editor);
		connect(point_symbol_editor, SIGNAL(symbolEdited()), this, SLOT(pointSymbolEdited()));
	}	*/

	updateStates();
}

void LineSymbolSettings::reset(Symbol* symbol)
{
	assert(symbol->getType() == Symbol::Line);
	
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
	Q_FOREACH(PointSymbol* point_symbol, point_symbols)
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
