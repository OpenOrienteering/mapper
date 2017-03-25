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


#include "area_symbol.h"

#include <QIODevice>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/map.h"
#include "core/renderables/renderable_implementation.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"


// ### FillPattern ###

AreaSymbol::FillPattern::FillPattern()
{
	type = LinePattern;
	angle = 0;
	rotatable = false;
	line_spacing = 5000; // 5 mm
	line_offset = 0;
	offset_along_line = 0;
	
	line_color = NULL;
	line_width = 200; // 0.2 mm
	
	point_distance = 5000; // 5 mm
	point = NULL;
}

#ifndef NO_NATIVE_FILE_FORMAT

bool AreaSymbol::FillPattern::load(QIODevice* file, int version, Map* map)
{
	qint32 itype;
	file->read((char*)&itype, sizeof(qint32));
	type = (Type)itype;
	file->read((char*)&angle, sizeof(float));
	if (version >= 4)
		file->read((char*)&rotatable, sizeof(bool));
	file->read((char*)&line_spacing, sizeof(int));
	if (version >= 3)
	{
		file->read((char*)&line_offset, sizeof(int));
		file->read((char*)&offset_along_line, sizeof(int));
	}
	
	if (type == LinePattern)
	{
		int color_index;
		file->read((char*)&color_index, sizeof(int));
		line_color = (color_index >= 0) ? map->getColor(color_index) : NULL;
		file->read((char*)&line_width, sizeof(int));
	}
	else
	{
		file->read((char*)&point_distance, sizeof(int));
		bool have_point;
		file->read((char*)&have_point, sizeof(bool));
		if (have_point)
		{
			point = new PointSymbol();
			if (!point->load(file, version, map))
				return false;
			if (version < 21)
				point->setRotatable(true);
		}
		else
			point = NULL;
	}
	return true;
}

#endif

void AreaSymbol::FillPattern::save(QXmlStreamWriter& xml, const Map& map) const
{
	xml.writeStartElement(QString::fromLatin1("pattern"));
	xml.writeAttribute(QString::fromLatin1("type"), QString::number(type));
	xml.writeAttribute(QString::fromLatin1("angle"), QString::number(angle));
	if (rotatable)
		xml.writeAttribute(QString::fromLatin1("rotatable"), QString::fromLatin1("true"));
	xml.writeAttribute(QString::fromLatin1("line_spacing"), QString::number(line_spacing));
	xml.writeAttribute(QString::fromLatin1("line_offset"), QString::number(line_offset));
	xml.writeAttribute(QString::fromLatin1("offset_along_line"), QString::number(offset_along_line));
	
	if (type == LinePattern)
	{
		xml.writeAttribute(QString::fromLatin1("color"), QString::number(map.findColorIndex(line_color)));
		xml.writeAttribute(QString::fromLatin1("line_width"), QString::number(line_width));
	}
	else
	{
		xml.writeAttribute(QString::fromLatin1("point_distance"), QString::number(point_distance));
		if (point != NULL)
			point->save(xml, map);
	}
	
	xml.writeEndElement(/*pattern*/);
}

void AreaSymbol::FillPattern::load(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict)
{
	Q_ASSERT (xml.name() == QLatin1String("pattern"));
	
	QXmlStreamAttributes attributes = xml.attributes();
	type = static_cast<Type>(attributes.value(QLatin1String("type")).toInt());
	angle = attributes.value(QLatin1String("angle")).toFloat();
	rotatable = (attributes.value(QLatin1String("rotatable")) == QLatin1String("true"));
	line_spacing = attributes.value(QLatin1String("line_spacing")).toInt();
	line_offset = attributes.value(QLatin1String("line_offset")).toInt();
	offset_along_line = attributes.value(QLatin1String("offset_along_line")).toInt();
	
	if (type == LinePattern)
	{
		int temp = attributes.value(QLatin1String("color")).toInt();
		line_color = map.getColor(temp);
		line_width = attributes.value(QLatin1String("line_width")).toInt();
		xml.skipCurrentElement();
	}
	else
	{
		point_distance = attributes.value(QLatin1String("point_distance")).toInt();
		while (xml.readNextStartElement())
		{
			if (xml.name() == QLatin1String("symbol"))
				point = static_cast<PointSymbol*>(Symbol::load(xml, map, symbol_dict));
			else
				xml.skipCurrentElement();
		}
	}
}

bool AreaSymbol::FillPattern::equals(const AreaSymbol::FillPattern& other, Qt::CaseSensitivity case_sensitivity) const
{
	if (type != other.type)
		return false;
	if (qAbs(angle - other.angle) > 1e-05)
		return false;
	if (rotatable != other.rotatable)
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
		if ((point == NULL && other.point != NULL) ||
			(point != NULL && other.point == NULL))
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

void AreaSymbol::FillPattern::createRenderables(QRectF extent, float delta_rotation, const MapCoord& pattern_origin, ObjectRenderables& output) const
{
	if (line_spacing <= 0)
		return;
	if (type == PointPattern && point_distance <= 0)
		return;
	
	if (!rotatable)
		delta_rotation = 0;
	
	auto line_width_f = 0.001*line_width;
	auto line_spacing_f = 0.001*line_spacing;
	
	// Make rotation unique
	auto rotation = double(angle + delta_rotation);
	rotation = fmod(1.0 * rotation, M_PI);
	if (rotation < 0)
		rotation = M_PI + rotation;
	Q_ASSERT(rotation >= 0 && rotation <= M_PI);
	
	// Helpers
	LineSymbol line;
	if (type == LinePattern)
	{
		line.setColor(line_color);
		line.setLineWidth(line_width_f);
	}
	
	MapCoordF first, second;
	
	// Determine real extent to fill
	QRectF fill_extent;
	if (type == LinePattern)
	{
		
		if (qAbs(rotation - M_PI/2) < 0.0001)
			fill_extent = QRectF(-0.5 * line_width_f, 0, line_width_f, 0);	// horizontal lines
		else if (qAbs(rotation - 0) < 0.0001)
			fill_extent = QRectF(0, -0.5 * line_width_f, 0, line_width_f);	// verticallines
		else
			fill_extent = QRectF(-0.5 * line_width_f, -0.5 * line_width_f, line_width_f, line_width_f);	// not the 100% optimum but the performance gain is surely neglegible
	}
	else if (point)
	{
		// TODO: Ugly method to get the point's extent
		PointObject point_object(point);
		point_object.setRotation(delta_rotation);
		point_object.update();
		fill_extent = point_object.getExtent();
	}
	extent = QRectF(extent.topLeft() - fill_extent.bottomRight(), extent.bottomRight() - fill_extent.topLeft());
	
	// Fill
	float delta_line_offset = 0;
	float delta_along_line_offset = 0;
	if (rotatable)
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
	
	const float offset = 0.001f * line_offset + delta_line_offset;
	if (qAbs(rotation - M_PI/2) < 0.0001)
	{
		// Special case: vertical lines
		delta_along_line_offset = -delta_along_line_offset;
		
		double first_offset = offset + ceil((extent.left() - offset) / line_spacing_f) * line_spacing_f;
		for (double cur = first_offset; cur < extent.right(); cur += line_spacing_f)
		{
			first = MapCoordF(cur, extent.top());
			second = MapCoordF(cur, extent.bottom());
			createLine(first, second, delta_along_line_offset, &line, delta_rotation, output);
		}
	}
	else if (qAbs(rotation - 0) < 0.0001)
	{
		// Special case: horizontal lines
		double first_offset = offset + ceil((extent.top() - offset) / line_spacing_f) * line_spacing_f;
		for (double cur = first_offset; cur < extent.bottom(); cur += line_spacing_f)
		{
			first = MapCoordF(extent.left(), cur);
			second = MapCoordF(extent.right(), cur);
			createLine(first, second, delta_along_line_offset, &line, delta_rotation, output);
		}
	}
	else
	{
		// General case
		if (rotation < M_PI / 2)
			delta_along_line_offset = -delta_along_line_offset;
		
		float xfactor = 1.0f / sin(rotation);
		float yfactor = 1.0f / cos(rotation);
		
		float dist_x = xfactor * line_spacing_f;
		float dist_y = yfactor * line_spacing_f;
		float offset_x = xfactor * offset;
		float offset_y = yfactor * offset;
		
		if (rotation < M_PI/2)
		{
			// Start with the upper left corner
			offset_x += (-extent.top()) / tan(rotation);
			offset_y -= extent.left() * tan(rotation);
			float start_x = offset_x + ceil((extent.x() - offset_x) / dist_x) * dist_x;
			float start_y = extent.top();
			float end_x = extent.left();
			float end_y = offset_y + ceil((extent.y() - offset_y) / dist_y) * dist_y;
			
			do
			{
				// Correct coordinates
				if (start_x > extent.right())
				{
					start_y += ((start_x - extent.right()) / dist_x) * dist_y;
					start_x = extent.right();
				}
				if (end_y > extent.bottom())
				{
					end_x += ((end_y - extent.bottom()) / dist_y) * dist_x;
					end_y = extent.bottom();
				}
				
				if (start_y > extent.bottom())
					break;
				
				// Create the renderable(s)
				first = MapCoordF(start_x, start_y);
				second = MapCoordF(end_x, end_y);
				createLine(first, second, delta_along_line_offset, &line, delta_rotation, output);
				
				// Move to next position
				start_x += dist_x;
				end_y += dist_y;
			} while (true);
		}
		else
		{
			// Start with left lower corner
			offset_x += (-extent.bottom()) / tan(rotation);
			offset_y -= extent.x() * tan(rotation);
			float start_x = offset_x + ceil((extent.x() - offset_x) / dist_x) * dist_x;
			float start_y = extent.bottom();
			float end_x = extent.x();
			float end_y = offset_y + ceil((extent.bottom() - offset_y) / dist_y) * dist_y;
			
			do
			{
				// Correct coordinates
				if (start_x > extent.right())
				{
					start_y += ((start_x - extent.right()) / dist_x) * dist_y;
					start_x = extent.right();
				}
				if (end_y < extent.y())
				{
					end_x += ((end_y - extent.y()) / dist_y) * dist_x;
					end_y = extent.y();
				}
				
				if (start_y < extent.y())
					break;
				
				// Create the renderable(s)
				first = MapCoordF(start_x, start_y);
				second = MapCoordF(end_x, end_y);
				createLine(first, second, delta_along_line_offset, &line, delta_rotation, output);
				
				// Move to next position
				start_x += dist_x;
				end_y += dist_y;
			} while (true);
		}
	}
}

void AreaSymbol::FillPattern::createLine(MapCoordF first, MapCoordF second, float delta_offset, LineSymbol* line, float rotation, ObjectRenderables& output) const
{
	if (type == LinePattern)
	{
		output.insertRenderable(new LineRenderable(line, first, second));
	}
	else
	{
		auto direction = second - first;
		auto length = direction.length();
		direction /= length; // normalize
		
		auto offset       = MapCoordF::dotProduct(direction, first) - 0.001 * offset_along_line - delta_offset;
		auto step_length  = 0.001 * point_distance;
		auto start_length = ceil((offset) / step_length) * step_length - offset;
		
		auto to_next = direction * step_length;
		
		auto coord = first + direction * start_length;
		for (auto cur = start_length; cur < length; cur += step_length)
		{
			point->createRenderablesScaled(coord, -rotation, output);
			coord += to_next;
		}
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

// ### AreaSymbol ###

AreaSymbol::AreaSymbol() : Symbol(Symbol::Area)
{
	color = NULL;
	minimum_area = 0;
}

AreaSymbol::~AreaSymbol()
{
	for (int i = 0; i < (int)patterns.size(); ++i)
	{
		if (patterns[i].type == FillPattern::PointPattern)
			delete patterns[i].point;
	}
}

Symbol* AreaSymbol::duplicate(const MapColorMap* color_map) const
{
	AreaSymbol* new_area = new AreaSymbol();
	new_area->duplicateImplCommon(this);
	new_area->color = color_map ? color_map->value(color) : color;
	new_area->minimum_area = minimum_area;
	new_area->patterns = patterns;
	for (int i = 0; i < (int)new_area->patterns.size(); ++i)
	{
		if (new_area->patterns[i].type == FillPattern::PointPattern)
			new_area->patterns[i].point = static_cast<PointSymbol*>(new_area->patterns[i].point->duplicate(color_map));
		else if (new_area->patterns[i].type == FillPattern::LinePattern && color_map)
			new_area->patterns[i].line_color = color_map->value(new_area->patterns[i].line_color);
	}
	return new_area;
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
	AreaRenderable* color_fill = new AreaRenderable(this, path_parts);
	output.insertRenderable(color_fill);
	
	if (!patterns.empty())
	{
		const QPainterPath* old_clip_path = output.getClipPath();
		output.setClipPath(color_fill->painterPath());
		auto extent = color_fill->getExtent();
		auto rotation = object->getPatternRotation();
		auto origin = object->getPatternOrigin();
		for (const auto& pattern: patterns)
		{
			pattern.createRenderables(extent, rotation, origin, output);
		}
		output.setClipPath(old_clip_path);
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
		pattern.angle = 45 * M_PI / 180.0f;
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
				pattern.rotatable = orig_pattern.rotatable;
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

void AreaSymbol::colorDeleted(const MapColor* color)
{
	bool change = false;
	if (color == this->color)
	{
		this->color = NULL;
		change = true;
	}
	
	for (int i = 0; i < (int)patterns.size(); ++i)
	{
		if (patterns[i].type == FillPattern::PointPattern)
			patterns[i].point->colorDeleted(color);
		else if (patterns[i].line_color == color)
		{
			patterns[i].line_color = NULL;
			change = true;
		}
	}
	
	if (change)
		resetIcon();
}
bool AreaSymbol::containsColor(const MapColor* color) const
{
	if (color == this->color)
		return true;
	
	for (int i = 0; i < (int)patterns.size(); ++i)
	{
		if (patterns[i].type == FillPattern::PointPattern && patterns[i].point && patterns[i].point->containsColor(color))
			return true;
		else if (patterns[i].type == FillPattern::LinePattern && patterns[i].line_color == color)
			return true;
	}
	
	return false;
}

const MapColor* AreaSymbol::guessDominantColor() const
{
	if (color)
		return color;
	
	// Hope that the first pattern's color is representative
	for (int i = 0; i < (int)patterns.size(); ++i)
	{
		if (patterns[i].type == FillPattern::PointPattern && patterns[i].point)
		{
			const MapColor* dominant_color = patterns[i].point->guessDominantColor();
			if (dominant_color) return dominant_color;
		}
		else if (patterns[i].type == FillPattern::LinePattern && patterns[i].line_color)
			return patterns[i].line_color;
	}
	
	return NULL;
}

void AreaSymbol::scale(double factor)
{
	minimum_area = qRound(factor*factor * minimum_area);
	
	int size = (int)patterns.size();
	for (int i = 0; i < size; ++i)
		patterns[i].scale(factor);
	
	resetIcon();
}

bool AreaSymbol::hasRotatableFillPattern() const
{
	for (int i = 0, size = (int)patterns.size(); i < size; ++i)
	{
		if (patterns[i].rotatable)
			return true;
	}
	return false;
}

#ifndef NO_NATIVE_FILE_FORMAT

bool AreaSymbol::loadImpl(QIODevice* file, int version, Map* map)
{
	int temp;
	file->read((char*)&temp, sizeof(int));
	color = (temp >= 0) ? map->getColor(temp) : NULL;
	if (version >= 2)
		file->read((char*)&minimum_area, sizeof(int));
	
	int size;
	file->read((char*)&size, sizeof(int));
	patterns.resize(size);
	for (int i = 0; i < size; ++i)
		if (!patterns[i].load(file, version, map))
			return false;
	
	return true;
}

#endif

void AreaSymbol::saveImpl(QXmlStreamWriter& xml, const Map& map) const
{
	xml.writeStartElement(QString::fromLatin1("area_symbol"));
	xml.writeAttribute(QString::fromLatin1("inner_color"), QString::number(map.findColorIndex(color)));
	xml.writeAttribute(QString::fromLatin1("min_area"), QString::number(minimum_area));
	
	int num_patterns = (int)patterns.size();
	xml.writeAttribute(QString::fromLatin1("patterns"), QString::number(num_patterns));
	for (int i = 0; i < num_patterns; ++i)
		patterns[i].save(xml, map);
	xml.writeEndElement(/*area_symbol*/);
}

bool AreaSymbol::loadImpl(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict)
{
	if (xml.name() != QLatin1String("area_symbol"))
		return false;
	
	QXmlStreamAttributes attributes = xml.attributes();
	int temp = attributes.value(QLatin1String("inner_color")).toInt();
	color = map.getColor(temp);
	minimum_area = attributes.value(QLatin1String("min_area")).toInt();
	
	int num_patterns = attributes.value(QLatin1String("patterns")).toInt();
	patterns.reserve(num_patterns % 5); // 5 is not the limit
	while (xml.readNextStartElement())
	{
		if (xml.name() == QLatin1String("pattern"))
		{
			patterns.push_back(FillPattern());
			patterns.back().load(xml, map, symbol_dict);
		}
		else
			xml.skipCurrentElement();
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
	
	// TODO: Fill patterns are only compared in order
	if (patterns.size() != area->patterns.size())
		return false;
	for (size_t i = 0, end = patterns.size(); i < end; ++i)
	{
		if (!patterns[i].equals(area->patterns[i], case_sensitivity))
			return false;
	}
	return true;
}
