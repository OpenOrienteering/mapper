/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2019 Kai Pastor
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


#include "area_symbol.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>

#include <QtMath>
#include <QLatin1String>
#include <QStringRef>
#include <QXmlStreamReader> // IWYU pragma: keep

#include "core/map.h"
#include "core/map_color.h"
#include "core/map_coord.h"
#include "core/objects/object.h"
#include "core/renderables/renderable.h"
#include "core/renderables/renderable_implementation.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"
#include "core/virtual_coord_vector.h"
#include "util/xml_stream_util.h"

class QXmlStreamWriter;
// IWYU pragma: no_forward_declare QXmlStreamReader


namespace OpenOrienteering {

// ### FillPattern ###

AreaSymbol::FillPattern::FillPattern() noexcept
: type { LinePattern }
, flags { Default }
, angle { 0 }
, line_spacing { 5000 } // 5 mm
, line_offset { 0 }
, line_color { nullptr }
, line_width { 200 } // 0.2 mm
, offset_along_line { 0 }
, point_distance { 5000 } // 5 mm
, point { nullptr }
, name {}
{
	// nothing else
}



void AreaSymbol::FillPattern::save(QXmlStreamWriter& xml, const Map& map) const
{
	XmlElementWriter element { xml, QLatin1String("pattern") };
	element.writeAttribute(QLatin1String("type"), type);
	element.writeAttribute(QLatin1String("angle"), angle);
	if (auto no_clipping = int(flags & Option::AlternativeToClipping))
		element.writeAttribute(QLatin1String("no_clipping"), no_clipping);
	if (rotatable())
		element.writeAttribute(QLatin1String("rotatable"), true);
	element.writeAttribute(QLatin1String("line_spacing"), line_spacing);
	element.writeAttribute(QLatin1String("line_offset"), line_offset);
	element.writeAttribute(QLatin1String("offset_along_line"), offset_along_line);
	
	switch (type)
	{
	case LinePattern:
		element.writeAttribute(QLatin1String("color"), map.findColorIndex(line_color));
		element.writeAttribute(QLatin1String("line_width"), line_width);
		break;
		
	case PointPattern:
		element.writeAttribute(QLatin1String("point_distance"), point_distance);
		if (point)
			point->save(xml, map);
		break;
	
	}
}

void AreaSymbol::FillPattern::load(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict)
{
	Q_ASSERT (xml.name() == QLatin1String("pattern"));
	
	XmlElementReader element { xml };
	type = element.attribute<Type>(QLatin1String("type"));
	angle = element.attribute<float>(QLatin1String("angle"));
	flags = Options{element.attribute<int>(QLatin1String("no_clipping")) & Option::AlternativeToClipping};
	if (element.attribute<bool>(QLatin1String("rotatable")))
		flags |= Option::Rotatable;
	line_spacing = element.attribute<int>(QLatin1String("line_spacing"));
	line_offset = element.attribute<int>(QLatin1String("line_offset"));
	offset_along_line = element.attribute<int>(QLatin1String("offset_along_line"));
	
	switch (type)
	{
	case LinePattern:
		line_color = map.getColor(element.attribute<int>(QLatin1String("color")));
		line_width = element.attribute<int>(QLatin1String("line_width"));
		break;
		
	case PointPattern:
		point_distance = element.attribute<int>(QLatin1String("point_distance"));
		while (xml.readNextStartElement())
		{
			if (xml.name() == QLatin1String("symbol"))
				point = static_cast<PointSymbol*>(Symbol::load(xml, map, symbol_dict).release());
			else
				xml.skipCurrentElement();
		}
		break;
		
	}
}

bool AreaSymbol::FillPattern::equals(const AreaSymbol::FillPattern& other, Qt::CaseSensitivity case_sensitivity) const
{
	if (type != other.type)
		return false;
	if (qAbs(angle - other.angle) > 1e-05)
		return false;
	if (flags != other.flags)
		return false;
	if (line_spacing != other.line_spacing)
		return false;
	if (line_offset != other.line_offset)
		return false;
	
	if (type == PointPattern)
	{
		if (offset_along_line != other.offset_along_line)
			return false;
		if (point_distance != other.point_distance)
			return false;
		if (bool(point) != bool(other.point))
			return false;
		if (point && !point->equals(other.point, case_sensitivity))
			return false;
	}
	else if (type == LinePattern)
	{
		if (!MapColor::equal(line_color, other.line_color))
			return false;
		if (line_width != other.line_width)
			return false;
	}
	
	if (name.compare(other.name, case_sensitivity) != 0)
		return false;
	
	return true;
}



void AreaSymbol::FillPattern::setRotatable(bool value)
{
	flags = value ? (flags | Option::Rotatable) : (flags & ~Option::Rotatable);
}



void AreaSymbol::FillPattern::setClipping(Options clipping)
{
	flags = (flags & ~Option::AlternativeToClipping) | (clipping & Option::AlternativeToClipping);
}



void AreaSymbol::FillPattern::colorDeleted(const MapColor* color)
{
	switch (type)
	{
	case FillPattern::PointPattern:
		point->colorDeletedEvent(color);
		break;
	case FillPattern::LinePattern:
		if (line_color == color)
			line_color = nullptr;
		break;
	}
}


bool AreaSymbol::FillPattern::containsColor(const MapColor* color) const
{
	switch (type)
	{
	case FillPattern::PointPattern:
		return point && point->containsColor(color);
	case FillPattern::LinePattern:
		return line_color == color;
	}
	Q_UNREACHABLE();
}


const MapColor* AreaSymbol::FillPattern::guessDominantColor() const
{
	const MapColor* color = nullptr;
	switch (type)
	{
	case FillPattern::PointPattern:
		if (point)
			color = point->guessDominantColor();
		break;
	case FillPattern::LinePattern:
		color = line_color;
		break;
	}
	return color;
}



template <>
inline
void AreaSymbol::FillPattern::createLine<AreaSymbol::FillPattern::LinePattern>(
        MapCoordF first, MapCoordF second,
        qreal,
        LineSymbol* line,
        qreal,
        const AreaRenderable&,
        ObjectRenderables& output ) const
{
	// out of inlining
	output.insertRenderable(new LineRenderable(line, first, second));
}


template <>
inline
void AreaSymbol::FillPattern::createLine<AreaSymbol::FillPattern::PointPattern>(
        MapCoordF first, MapCoordF second,
        qreal delta_offset,
        LineSymbol*,
        qreal rotation,
        const AreaRenderable& outline,
        ObjectRenderables& output ) const
{
	// out of inlining
	createPointPatternLine(first, second, delta_offset, rotation, outline, output);
}


// This template will be instantiated in non-template createRenderables()
// once for each type of pattern, thus duplicating the complex body.
// This is by intention, in order to let the compiler optimize each
// instantiation independently, with regard to unused parameters in
// createLine(), and to eliminate any runtime checks for pattern type
// outside of non-template createRenderables().
template <int T>
void AreaSymbol::FillPattern::createRenderables(
        const AreaRenderable& outline,
        qreal delta_rotation,
        const MapCoord& pattern_origin,
        const QRectF& point_extent,
        LineSymbol* line,
        qreal rotation,
        ObjectRenderables& output ) const
{
	// Canvas is the entire rectangle which will be filled with renderables.
	// It will be clipped by the outline, later.
	auto canvas = outline.getExtent();
	canvas.adjust(-point_extent.right(), -point_extent.bottom(), -point_extent.left(), -point_extent.top());
	
	MapCoordF first, second;
	
	// Fill
	qreal delta_line_offset = 0;
	qreal delta_along_line_offset = 0;
	if (rotatable())
	{
		MapCoordF line_normal(0, -1);
		line_normal.rotate(rotation);
		line_normal.setY(-line_normal.y());
		delta_line_offset = MapCoordF::dotProduct(line_normal, MapCoordF(pattern_origin));
		
		MapCoordF line_tangent(1, 0);
		line_tangent.rotate(rotation);
		line_tangent.setY(-line_tangent.y());
		delta_along_line_offset = MapCoordF::dotProduct(line_tangent, MapCoordF(pattern_origin));
	}
	
	auto line_spacing_f = 0.001*line_spacing;
	
	const auto offset = 0.001 * line_offset + delta_line_offset;
	if (qAbs(rotation - M_PI/2) < 0.0001)
	{
		// Special case: vertical lines
		delta_along_line_offset = -delta_along_line_offset;
		
		double first_offset = offset + ceil((canvas.left() - offset) / line_spacing_f) * line_spacing_f;
		for (double cur = first_offset; cur < canvas.right(); cur += line_spacing_f)
		{
			first = MapCoordF(cur, canvas.top());
			second = MapCoordF(cur, canvas.bottom());
			createLine<T>(first, second, delta_along_line_offset, line, delta_rotation, outline, output);
		}
	}
	else if (qAbs(rotation - 0) < 0.0001)
	{
		// Special case: horizontal lines
		double first_offset = offset + ceil((canvas.top() - offset) / line_spacing_f) * line_spacing_f;
		for (double cur = first_offset; cur < canvas.bottom(); cur += line_spacing_f)
		{
			first = MapCoordF(canvas.left(), cur);
			second = MapCoordF(canvas.right(), cur);
			createLine<T>(first, second, delta_along_line_offset, line, delta_rotation, outline, output);
		}
	}
	else
	{
		// General case
		if (rotation < M_PI / 2)
			delta_along_line_offset = -delta_along_line_offset;
		
		auto xfactor = 1.0 / sin(rotation);
		auto yfactor = 1.0 / cos(rotation);
		
		auto dist_x = xfactor * line_spacing_f;
		auto dist_y = yfactor * line_spacing_f;
		auto offset_x = xfactor * offset;
		auto offset_y = yfactor * offset;
		
		if (rotation < M_PI/2)
		{
			// Start with the upper left corner
			offset_x += (-canvas.top()) / tan(rotation);
			offset_y -= canvas.left() * tan(rotation);
			auto start_x = offset_x + ceil((canvas.x() - offset_x) / dist_x) * dist_x;
			auto start_y = canvas.top();
			auto end_x = canvas.left();
			auto end_y = offset_y + ceil((canvas.y() - offset_y) / dist_y) * dist_y;
			
			do
			{
				// Correct coordinates
				if (start_x > canvas.right())
				{
					start_y += ((start_x - canvas.right()) / dist_x) * dist_y;
					start_x = canvas.right();
				}
				if (end_y > canvas.bottom())
				{
					end_x += ((end_y - canvas.bottom()) / dist_y) * dist_x;
					end_y = canvas.bottom();
				}
				
				if (start_y > canvas.bottom())
					break;
				
				// Create the renderable(s)
				first = MapCoordF(start_x, start_y);
				second = MapCoordF(end_x, end_y);
				createLine<T>(first, second, delta_along_line_offset, line, delta_rotation, outline, output);
				
				// Move to next position
				start_x += dist_x;
				end_y += dist_y;
			} while (true);
		}
		else
		{
			// Start with left lower corner
			offset_x += (-canvas.bottom()) / tan(rotation);
			offset_y -= canvas.x() * tan(rotation);
			auto start_x = offset_x + ceil((canvas.x() - offset_x) / dist_x) * dist_x;
			auto start_y = canvas.bottom();
			auto end_x = canvas.x();
			auto end_y = offset_y + ceil((canvas.bottom() - offset_y) / dist_y) * dist_y;
			
			do
			{
				// Correct coordinates
				if (start_x > canvas.right())
				{
					start_y += ((start_x - canvas.right()) / dist_x) * dist_y;
					start_x = canvas.right();
				}
				if (end_y < canvas.y())
				{
					end_x += ((end_y - canvas.y()) / dist_y) * dist_x;
					end_y = canvas.y();
				}
				
				if (start_y < canvas.y())
					break;
				
				// Create the renderable(s)
				first = MapCoordF(start_x, start_y);
				second = MapCoordF(end_x, end_y);
				createLine<T>(first, second, delta_along_line_offset, line, delta_rotation, outline, output);
				
				// Move to next position
				start_x += dist_x;
				end_y += dist_y;
			} while (true);
		}
	}
}


void AreaSymbol::FillPattern::createRenderables(const AreaRenderable& outline, qreal delta_rotation, const MapCoord& pattern_origin, ObjectRenderables& output) const
{
	if (line_spacing <= 0)
		return;
	
	if (!rotatable())
		delta_rotation = 0;
	
	// Make rotation unique
	auto rotation = angle + delta_rotation;
	rotation = fmod(1.0 * rotation, M_PI);
	if (rotation < 0)
		rotation = M_PI + rotation;
	Q_ASSERT(rotation >= 0 && rotation <= M_PI);
	
	// Handle clipping
	const auto old_clip_path = output.getClipPath();
	if (!(flags & Option::AlternativeToClipping))
	{
		output.setClipPath(outline.painterPath());
	}
	
	switch (type)
	{
	case LinePattern:
		{
			LineSymbol line;
			line.setColor(line_color);
			
			auto line_width_f = 0.001*line_width;
			line.setLineWidth(line_width_f);
			
			auto margin = line_width_f / 2;
			auto point_extent = QRectF{-margin, -margin, line_width_f, line_width_f};
			createRenderables<LinePattern>(outline, delta_rotation, pattern_origin, point_extent, &line, rotation, output);
		}
		break;
	case PointPattern:
		if (point && point_distance > 0)
		{
			PointObject point_object(point);
			point_object.setRotation(delta_rotation);
			point_object.update();
			auto point_extent = point_object.getExtent();
			createRenderables<PointPattern>(outline, delta_rotation, pattern_origin, point_extent, nullptr, rotation, output);
		}
		break;
	}
	
	output.setClipPath(old_clip_path);
}


void AreaSymbol::FillPattern::createPointPatternLine(
        MapCoordF first, MapCoordF second,
        qreal delta_offset,
        qreal rotation,
        const AreaRenderable& outline,
        ObjectRenderables& output ) const
{
	auto direction = second - first;
	auto length = direction.length();
	direction /= length; // normalize
	
	auto offset       = MapCoordF::dotProduct(direction, first) - 0.001 * offset_along_line - delta_offset;
	auto step_length  = 0.001 * point_distance;
	auto start_length = ceil((offset) / step_length) * step_length - offset;
	
	auto to_next = direction * step_length;
	auto coord = first + direction * start_length;
	
	// Duplicated loops for optimum locality of code
	switch (flags & Option::AlternativeToClipping)
	{
	case Option::NoClippingIfCenterInside:
		for (auto cur = start_length; cur < length; cur += step_length, coord += to_next)
			point->createRenderablesIfCenterInside(coord, -rotation, outline.painterPath(), output);
		break;
	case Option::NoClippingIfCompletelyInside:
		for (auto cur = start_length; cur < length; cur += step_length, coord += to_next)
			point->createRenderablesIfCompletelyInside(coord, -rotation, outline.painterPath(), output);
		break;
	case Option::Default:
#if 1
		// Avoids expensive check, but may create objects which won't be rendered.
		for (auto cur = start_length; cur < length; cur += step_length, coord += to_next)
			point->createRenderablesScaled(coord, -rotation, output);
		break;
#endif
	case Option::NoClippingIfPartiallyInside:
		for (auto cur = start_length; cur < length; cur += step_length, coord += to_next)
			point->createRenderablesIfPartiallyInside(coord, -rotation, outline.painterPath(), output);
		break;
	default:  
		Q_UNREACHABLE();
	}
}



void AreaSymbol::FillPattern::scale(double factor)
{
	line_spacing = qRound(factor * line_spacing);
	line_width = qRound(factor * line_width);
	line_offset = qRound(factor * line_offset);
	offset_along_line = qRound(factor * offset_along_line);
	point_distance = qRound(factor * point_distance);
	
	if (point)
		point->scale(factor);
}


qreal AreaSymbol::FillPattern::dimensionForIcon() const
{
	// Ignore large spacing for icon scaling
	auto size = qreal(0);
	switch (type)
	{
	case LinePattern:
		size = qreal(0.002 * line_width);
		break;
		
	case PointPattern:
		size = qreal(0.002 * point->dimensionForIcon());
		if (point_distance < 5000)
			size = std::max(size, qreal(0.002 * point_distance));
		break;
	}
	
	if (line_spacing < 5000)
		size = std::max(size, qreal(0.0015 * line_spacing));
	
	return size;
}



// ### AreaSymbol ###

AreaSymbol::AreaSymbol() noexcept
: Symbol { Symbol::Area }
, color { nullptr }
, minimum_area { 0 }
{
	// nothing else
}

AreaSymbol::AreaSymbol(const AreaSymbol& proto)
: Symbol { proto }
, patterns { proto.patterns }
, color { proto.color }
, minimum_area { proto.minimum_area }
{
	for (auto& new_pattern : patterns)
	{
		if (new_pattern.type == FillPattern::PointPattern)
			new_pattern.point = Symbol::duplicate(*new_pattern.point).release();
	}
}

AreaSymbol::~AreaSymbol()
{
	for (auto& pattern : patterns)
	{
		if (pattern.type == FillPattern::PointPattern)
			delete pattern.point;
	}
}


AreaSymbol* AreaSymbol::duplicate() const
{
	return new AreaSymbol(*this);
}



void AreaSymbol::createRenderables(
        const Object *object,
        const VirtualCoordVector &coords,
        ObjectRenderables &output,
        Symbol::RenderableOptions options) const        
{
	if (coords.size() < 3)
		return;
	
	auto path = static_cast<const PathObject*>(object);
	PathPartVector path_parts = PathPart::calculatePathParts(coords);
	createRenderables(path, path_parts, output, options);
}

void AreaSymbol::createRenderables(
        const PathObject* object,
        const PathPartVector& path_parts,
        ObjectRenderables &output,
        Symbol::RenderableOptions options) const
{
	if (options == Symbol::RenderNormal)
	{
		createRenderablesNormal(object, path_parts, output);
	}
	else
	{
		const MapColor* dominant_color = guessDominantColor();
		createBaselineRenderables(object, path_parts, output, dominant_color);
		
		if (options.testFlag(Symbol::RenderAreasHatched))
		{
			createHatchingRenderables(object, path_parts, output, dominant_color);
		}
	}
}

void AreaSymbol::createRenderablesNormal(
        const PathObject* object,
        const PathPartVector& path_parts,
        ObjectRenderables& output) const
{
	// The shape output is even created if the area is not filled with a color
	// because the QPainterPath created by it is needed as clip path for the fill objects
	auto color_fill = new AreaRenderable(this, path_parts);
	output.insertRenderable(color_fill);
	
	auto rotation = object->getPatternRotation();
	auto origin = object->getPatternOrigin();
	for (const auto& pattern : patterns)
	{
		pattern.createRenderables(*color_fill, rotation, origin, output);
	}
}

void AreaSymbol::createHatchingRenderables(
        const PathObject* object,
        const PathPartVector& path_parts,
        ObjectRenderables& output,
        const MapColor* color) const
{
	Q_ASSERT(getContainedTypes() & Symbol::Area);
	
	if (color)
	{
		// Insert hatched area renderable
		AreaSymbol area_symbol;
		area_symbol.setNumFillPatterns(1);
		AreaSymbol::FillPattern& pattern = area_symbol.getFillPattern(0);
		pattern.type = AreaSymbol::FillPattern::LinePattern;
		pattern.angle = qDegreesToRadians(45.0f);
		pattern.line_spacing = 1000;
		pattern.line_offset = 0;
		pattern.line_color = color;
		pattern.line_width = 70;
		
		auto symbol = object->getSymbol();
		if (symbol && symbol->getType() == Symbol::Area)
		{
			const AreaSymbol* orig_symbol = symbol->asArea();
			if (!orig_symbol->getColor()
			    && orig_symbol->getNumFillPatterns() >= 1)
			{
				const AreaSymbol::FillPattern& orig_pattern = orig_symbol->getFillPattern(0);
				pattern.angle = orig_pattern.angle;
				pattern.flags = orig_pattern.flags & ~FillPattern::AlternativeToClipping;
				if (orig_pattern.type == AreaSymbol::FillPattern::LinePattern)
				{
					pattern.line_spacing = std::max(1000, orig_pattern.line_spacing);
					pattern.line_offset = orig_pattern.line_offset;
				}
			}
		}
		
		area_symbol.createRenderablesNormal(object, path_parts, output);
	}
}



void AreaSymbol::colorDeletedEvent(const MapColor* color)
{
	if (containsColor(color))
	{
		if (color == this->color)
			this->color = nullptr;
		for (auto& pattern : patterns)
			pattern.colorDeleted(color);
		resetIcon();
	}
}


bool AreaSymbol::containsColor(const MapColor* color) const
{
	return color == this->color
	       || std::any_of(begin(patterns), end(patterns), [color](const auto& pattern){ return pattern.containsColor(color); });
}


const MapColor* AreaSymbol::guessDominantColor() const
{
	auto color = this->color;
	auto pattern = begin(patterns);
	while (!color && pattern != end(patterns))
	{
		color = pattern->guessDominantColor();
		++pattern;
	}
	return color;
}


void AreaSymbol::replaceColors(const MapColorMap& color_map)
{
	color = color_map.value(color);
	for (auto& pattern : patterns)
	{
		switch (pattern.type)
		{
		case FillPattern::LinePattern:
			pattern.line_color = color_map.value(pattern.line_color);
			break;
		case FillPattern::PointPattern:
			pattern.point->replaceColors(color_map);
			break;
		}
	}
}



void AreaSymbol::scale(double factor)
{
	minimum_area = qRound(factor*factor * minimum_area);
	
	for (auto& pattern : patterns)
		pattern.scale(factor);
	
	resetIcon();
}



qreal AreaSymbol::dimensionForIcon() const
{
	qreal size = 0;
	for (auto& pattern : patterns)
		size = qMax(size, pattern.dimensionForIcon());
	return size;
}



bool AreaSymbol::hasRotatableFillPattern() const
{
	return std::any_of(begin(patterns), end(patterns), [](auto& pattern){
		return pattern.rotatable();
	});
}



void AreaSymbol::saveImpl(QXmlStreamWriter& xml, const Map& map) const
{
	XmlElementWriter element { xml, QLatin1String("area_symbol") };
	element.writeAttribute(QLatin1String{"inner_color"}, map.findColorIndex(color));
	element.writeAttribute(QLatin1String{"min_area"}, minimum_area);
	element.writeAttribute(QLatin1String{"patterns"}, patterns.size());
	for (const auto& pattern : patterns)
		pattern.save(xml, map);
}

bool AreaSymbol::loadImpl(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict)
{
	if (xml.name() != QLatin1String("area_symbol"))
		return false;
	
	XmlElementReader element { xml };
	color = map.getColor(element.attribute<int>(QLatin1String("inner_color")));
	minimum_area = element.attribute<int>(QLatin1String("min_area"));
	
	auto num_patterns = element.attribute<int>(QLatin1String("patterns"));
	patterns.reserve(num_patterns % 100); // 100 is not the limit
	while (xml.readNextStartElement())
	{
		if (xml.name() == QLatin1String("pattern"))
		{
			patterns.push_back(FillPattern());
			patterns.back().load(xml, map, symbol_dict);
		}
		else
		{
			xml.skipCurrentElement();
		}
	}
	
	return true;
}

bool AreaSymbol::equalsImpl(const Symbol* other, Qt::CaseSensitivity case_sensitivity) const
{
	const AreaSymbol* area = static_cast<const AreaSymbol*>(other);
	if (!MapColor::equal(color, area->color))
		return false;
	if (minimum_area != area->minimum_area)
		return false;
	
	if (patterns.size() != area->patterns.size())
		return false;
	
	// std::is_permutation would identify equal sets of patterns.
	// However, guessDominantColor() depends on pattern order if there is no
	// AreaSymbol::color (or after the AreaSymbol::color is set to nullptr).
	// So equalsImpl cannot be changed unless guessDominantColor is changed.
	return std::equal(begin(patterns), end(patterns), begin(area->patterns), [case_sensitivity](auto& lhs, auto& rhs){
		return lhs.equals(rhs, case_sensitivity);
	});
}


}  // namespace OpenOrienteering
