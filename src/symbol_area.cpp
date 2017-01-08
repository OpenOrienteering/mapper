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


#include "symbol_area.h"

#include <QDebug>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QIODevice>
#include <QListWidget>
#include <QMenu>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolButton>
#include <QXmlStreamWriter>

#include "core/map_color.h"
#include "core/map.h"
#include "core/objects/object.h"
#include "core/renderables/renderable_implementation.h"
#include "symbol_line.h"
#include "symbol_point.h"
#include "symbol_point_editor.h"
#include "symbol_properties_widget.h"
#include "symbol_setting_dialog.h"
#include "gui/widgets/color_dropdown.h"
#include "util_gui.h"
#include "util.h"

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
	
	auto line_width_f = 0.001*line_width;
	auto line_spacing_f = 0.001*line_spacing;
	
	// Make rotation unique
	double rotation = angle;
	if (rotatable)
		rotation += delta_rotation;
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
	
	PointObject point_object(point);
	if (point && point->isRotatable())
		point_object.setRotation(delta_rotation);
	
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
	else
	{
		// TODO: Ugly method to get the point's extent
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
			createLine(first, second, delta_along_line_offset, &line, &point_object, output);
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
			createLine(first, second, delta_along_line_offset, &line, &point_object, output);
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
				createLine(first, second, delta_along_line_offset, &line, &point_object, output);
				
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
				createLine(first, second, delta_along_line_offset, &line, &point_object, output);
				
				// Move to next position
				start_x += dist_x;
				end_y += dist_y;
			} while (true);
		}
	}
}

void AreaSymbol::FillPattern::createLine(MapCoordF first, MapCoordF second, float delta_offset, LineSymbol* line, PointObject* point_object, ObjectRenderables& output) const
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
		auto rotation = -point_object->getRotation();
		for (auto cur = start_length; cur < length; cur += step_length)
		{
			point->createRenderablesScaled(coord, rotation, output);
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

SymbolPropertiesWidget* AreaSymbol::createPropertiesWidget(SymbolSettingDialog* dialog)
{
	return new AreaSymbolSettings(this, dialog);
}


// ### AreaSymbolSettings ###

AreaSymbolSettings::AreaSymbolSettings(AreaSymbol* symbol, SymbolSettingDialog* dialog)
 : SymbolPropertiesWidget(symbol, dialog), symbol(symbol)
{
	map = dialog->getPreviewMap();
	controller = dialog->getPreviewController();
	
	
	QFormLayout* general_layout = new QFormLayout();
	
	color_edit = new ColorDropDown(map);
	general_layout->addRow(tr("Area color:"), color_edit);
	
	minimum_size_edit = Util::SpinBox::create(1, 0.0, 999999.9, trUtf8("mm²"));
	general_layout->addRow(tr("Minimum size:"), minimum_size_edit);
	
	general_layout->addItem(Util::SpacerItem::create(this));
	
	
	QVBoxLayout* fill_patterns_list_layout = new QVBoxLayout();
	
	fill_patterns_list_layout->addWidget(Util::Headline::create(tr("Fills")), 0);
	
	pattern_list = new QListWidget();
	fill_patterns_list_layout->addWidget(pattern_list, 1);
	
	del_pattern_button = new QPushButton(QIcon(QString::fromLatin1(":/images/minus.png")), QString{});
	add_pattern_button = new QToolButton();
	add_pattern_button->setIcon(QIcon(QString::fromLatin1(":/images/plus.png")));
	add_pattern_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	add_pattern_button->setPopupMode(QToolButton::InstantPopup);
	add_pattern_button->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
	add_pattern_button->setMinimumSize(del_pattern_button->sizeHint());
	QMenu* add_fill_button_menu = new QMenu(add_pattern_button);
	add_fill_button_menu->addAction(tr("Line fill"), this, SLOT(addLinePattern()));
	add_fill_button_menu->addAction(tr("Pattern fill"), this, SLOT(addPointPattern()));
	add_pattern_button->setMenu(add_fill_button_menu);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(add_pattern_button);
	buttons_layout->addWidget(del_pattern_button);
	buttons_layout->addStretch(1);
	fill_patterns_list_layout->addLayout(buttons_layout, 0);
	
	
	QFormLayout* fill_pattern_layout = new QFormLayout();
	
	pattern_name_edit = new QLabel();
	fill_pattern_layout->addRow(pattern_name_edit);
	
	fill_pattern_layout->addItem(Util::SpacerItem::create(this));
	
	
	/* From here, stacked widgets are used to unify the layout of pattern type dependant fields. */
	QStackedWidget* single_line_headline = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_headline, SLOT(setCurrentIndex(int)));
	fill_pattern_layout->addRow(single_line_headline);
	
	QStackedWidget* single_line_label1 = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_label1, SLOT(setCurrentIndex(int)));
	QStackedWidget* single_line_edit1  = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_edit1, SLOT(setCurrentIndex(int)));
	fill_pattern_layout->addRow(single_line_label1, single_line_edit1);
	
	QStackedWidget* single_line_label2 = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_label2, SLOT(setCurrentIndex(int)));
	QStackedWidget* single_line_edit2  = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_edit2, SLOT(setCurrentIndex(int)));
	fill_pattern_layout->addRow(single_line_label2, single_line_edit2);
	
	QStackedWidget* single_line_label3 = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_label3, SLOT(setCurrentIndex(int)));
	pattern_line_offset_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"));
	fill_pattern_layout->addRow(single_line_label3, pattern_line_offset_edit);
	
	fill_pattern_layout->addItem(Util::SpacerItem::create(this));
	
	QStackedWidget* parallel_lines_headline = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), parallel_lines_headline, SLOT(setCurrentIndex(int)));
	fill_pattern_layout->addRow(parallel_lines_headline);
	
	QStackedWidget* parallel_lines_label1 = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), parallel_lines_label1, SLOT(setCurrentIndex(int)));
	pattern_spacing_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	fill_pattern_layout->addRow(parallel_lines_label1, pattern_spacing_edit);
	
	/* Stacked widgets, index 0: line pattern fields */
	single_line_headline->addWidget(Util::Headline::create(tr("Single line")));
	
	QLabel* pattern_linewidth_label = new QLabel(tr("Line width:"));
	pattern_linewidth_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	single_line_label1->addWidget(pattern_linewidth_label);
	single_line_edit1 ->addWidget(pattern_linewidth_edit);
	
	QLabel* pattern_color_label = new QLabel(tr("Line color:"));
	pattern_color_edit = new ColorDropDown(map);
	single_line_label2->addWidget(pattern_color_label);
	single_line_edit2 ->addWidget(pattern_color_edit);
	
	single_line_label3->addWidget(new QLabel(tr("Line offset:")));
	
	parallel_lines_headline->addWidget(Util::Headline::create(tr("Parallel lines")));
	
	parallel_lines_label1->addWidget(new QLabel(tr("Line spacing:")));
	
	/* Stacked widgets, index 1: point pattern fields */
	single_line_headline->addWidget(Util::Headline::create(tr("Single row")));
	
	QLabel* pattern_pointdist_label = new QLabel(tr("Pattern interval:"));
	pattern_pointdist_edit = Util::SpinBox::create(2, 0, 999999.9, tr("mm"));
	single_line_label1->addWidget(pattern_pointdist_label);
	single_line_edit1 ->addWidget(pattern_pointdist_edit);
	
	QLabel* pattern_offset_along_line_label = new QLabel(tr("Pattern offset:"));
	pattern_offset_along_line_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"));
	single_line_label2->addWidget(pattern_offset_along_line_label);
	single_line_edit2 ->addWidget(pattern_offset_along_line_edit);
	
	single_line_label3->addWidget(new QLabel(tr("Row offset:")));
	
	parallel_lines_headline->addWidget(Util::Headline::create(tr("Parallel rows")));
	
	parallel_lines_label1->addWidget(new QLabel(tr("Row spacing:")));
	
	/* General fields again. Not stacked. */
	fill_pattern_layout->addItem(Util::SpacerItem::create(this));
	
	
	fill_pattern_layout->addRow(Util::Headline::create(tr("Fill rotation")));
	
	pattern_angle_edit = Util::SpinBox::create(1, 0.0, 360.0, trUtf8("°"));
	pattern_angle_edit->setWrapping(true);
	fill_pattern_layout->addRow(tr("Angle:"), pattern_angle_edit);
	
	pattern_rotatable_check = new QCheckBox(tr("adjustable per object"));
	fill_pattern_layout->addRow(QString{}, pattern_rotatable_check);
	
	
	QHBoxLayout* fill_patterns_layout = new QHBoxLayout();
	fill_patterns_layout->addLayout(fill_patterns_list_layout, 1);
	fill_patterns_layout->addItem(Util::SpacerItem::create(this));
	fill_patterns_layout->addLayout(fill_pattern_layout, 3);
	
	QBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(general_layout);
	layout->addLayout(fill_patterns_layout);
	
	QWidget* area_tab = new QWidget();
	area_tab->setLayout(layout);
	addPropertiesGroup(tr("Area settings"), area_tab);
	
	connect(color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(colorChanged()));
	connect(minimum_size_edit, SIGNAL(valueChanged(double)), this, SLOT(minimumSizeChanged(double)));
	connect(del_pattern_button, SIGNAL(clicked(bool)), this, SLOT(deleteActivePattern()));
	connect(pattern_list, SIGNAL(currentRowChanged(int)), this, SLOT(selectPattern(int)));
	connect(pattern_angle_edit, SIGNAL(valueChanged(double)), this, SLOT(patternAngleChanged(double)));
	connect(pattern_rotatable_check, SIGNAL(clicked(bool)), this, SLOT(patternRotatableClicked(bool)));
	connect(pattern_spacing_edit, SIGNAL(valueChanged(double)), this, SLOT(patternSpacingChanged(double)));
	connect(pattern_line_offset_edit, SIGNAL(valueChanged(double)), this, SLOT(patternLineOffsetChanged(double)));
	connect(pattern_offset_along_line_edit, SIGNAL(valueChanged(double)), this, SLOT(patternOffsetAlongLineChanged(double)));
	
	connect(pattern_color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(patternColorChanged()));
	connect(pattern_linewidth_edit, SIGNAL(valueChanged(double)), this, SLOT(patternLineWidthChanged(double)));
	connect(pattern_pointdist_edit, SIGNAL(valueChanged(double)), this, SLOT(patternPointDistChanged(double)));
	
	updateAreaGeneral();
	updatePatternWidgets();
	loadPatterns();
}

void AreaSymbolSettings::reset(Symbol* symbol)
{
	Q_ASSERT(symbol->getType() == Symbol::Area);
	
	clearPatterns();
	
	SymbolPropertiesWidget::reset(symbol);
	this->symbol = reinterpret_cast<AreaSymbol*>(symbol);
	
	updateAreaGeneral();
	loadPatterns();
}

void AreaSymbolSettings::updateAreaGeneral()
{
	color_edit->blockSignals(true);
	color_edit->setColor(symbol->getColor());
	color_edit->blockSignals(false);
	minimum_size_edit->blockSignals(true);
	minimum_size_edit->setValue(0.001 * symbol->minimum_area);
	minimum_size_edit->blockSignals(false);
}

void AreaSymbolSettings::clearPatterns()
{
	pattern_list->clear();
	for (int i = 0; i < (int)this->symbol->patterns.size(); ++i)
	{
		if (this->symbol->patterns[i].type == AreaSymbol::FillPattern::PointPattern)
			removePropertiesGroup(2);
	}
}

void AreaSymbolSettings::loadPatterns()
{
	Q_ASSERT(pattern_list->count() == 0);
	Q_ASSERT(count() == 2); // General + Area tab
	
	for (int i = 0; i < symbol->getNumFillPatterns(); ++i)
	{
		pattern_list->addItem(symbol->patterns[i].name);
		if (symbol->getFillPattern(i).type == AreaSymbol::FillPattern::PointPattern)
		{
			PointSymbolEditorWidget* editor = new PointSymbolEditorWidget(controller, symbol->getFillPattern(i).point, 16);
			connect(editor, SIGNAL(symbolEdited()), this, SIGNAL(propertiesModified()) );
			addPropertiesGroup(symbol->patterns[i].name, editor);
		}
	}
	updatePatternNames();
	pattern_list->setCurrentRow(0);
}

void AreaSymbolSettings::updatePatternNames()
{
	int line_pattern_num = 0, point_pattern_num = 0;
	for (int i = 0; i < (int)symbol->patterns.size(); ++i)
	{
		if (symbol->patterns[i].type == AreaSymbol::FillPattern::PointPattern)
		{
			point_pattern_num++;
			QString name = tr("Pattern fill %1").arg(point_pattern_num);
			symbol->patterns[i].name = name;
			symbol->patterns[i].point->setName(name);
			pattern_list->item(i)->setText(name);
			renamePropertiesGroup(point_pattern_num + 1, name);
		}
		else
		{
			line_pattern_num++;
			QString name = tr("Line fill %1").arg(line_pattern_num);
			symbol->patterns[i].name = name;
			pattern_list->item(i)->setText(symbol->patterns[i].name);
		}
	}
}

void AreaSymbolSettings::updatePatternWidgets()
{
	int index = pattern_list->currentRow();
	bool pattern_active = (index >= 0);
	if (pattern_active)
		active_pattern = symbol->patterns.begin() + index;
	
	del_pattern_button->setEnabled(pattern_active);
	
	AreaSymbol::FillPattern* pattern = pattern_active ? &*active_pattern : new AreaSymbol::FillPattern();
	pattern_name_edit->setText(pattern_active ? (QLatin1String("<b>") + active_pattern->name + QLatin1String("</b>")) : tr("No fill selected"));
	
	pattern_angle_edit->blockSignals(true);
	pattern_rotatable_check->blockSignals(true);
	pattern_spacing_edit->blockSignals(true);
	pattern_line_offset_edit->blockSignals(true);
	
	pattern_angle_edit->setValue(pattern->angle * 180.0 / M_PI);
	pattern_angle_edit->setEnabled(pattern_active);
	pattern_rotatable_check->setChecked(pattern->rotatable);
	pattern_rotatable_check->setEnabled(pattern_active);
	pattern_spacing_edit->setValue(0.001 * pattern->line_spacing);
	pattern_spacing_edit->setEnabled(pattern_active);
	pattern_line_offset_edit->setValue(0.001 * pattern->line_offset);
	pattern_line_offset_edit->setEnabled(pattern_active);
	
	pattern_angle_edit->blockSignals(false);
	pattern_rotatable_check->blockSignals(false);
	pattern_spacing_edit->blockSignals(false);
	pattern_line_offset_edit->blockSignals(false);
	
	if (pattern->type == AreaSymbol::FillPattern::LinePattern)
	{
		emit switchPatternEdits(0);
		
		pattern_linewidth_edit->blockSignals(true);
		pattern_linewidth_edit->setValue(0.001 * pattern->line_width);
		pattern_linewidth_edit->setEnabled(pattern_active);
		pattern_linewidth_edit->blockSignals(false);
		
		pattern_color_edit->blockSignals(true);
		pattern_color_edit->setColor(pattern->line_color);
		pattern_color_edit->setEnabled(pattern_active);
		pattern_color_edit->blockSignals(false);
	}
	else
	{
		emit switchPatternEdits(1);
		
		pattern_offset_along_line_edit->blockSignals(true);
		pattern_offset_along_line_edit->setValue(0.001 * pattern->offset_along_line);
		pattern_offset_along_line_edit->setEnabled(pattern_active);
		pattern_offset_along_line_edit->blockSignals(false);
		
		pattern_pointdist_edit->blockSignals(true);
		pattern_pointdist_edit->setValue(0.001 * pattern->point_distance);
		pattern_pointdist_edit->setEnabled(pattern_active);
		pattern_pointdist_edit->blockSignals(false);
	}
	
	if (!pattern_active)
		delete pattern;
}

void AreaSymbolSettings::addLinePattern()
{
	addPattern(AreaSymbol::FillPattern::LinePattern);
}

void AreaSymbolSettings::addPointPattern()
{
	addPattern(AreaSymbol::FillPattern::PointPattern);
}

void AreaSymbolSettings::addPattern(AreaSymbol::FillPattern::Type type)
{
	int index = pattern_list->currentRow() + 1;
	if (index < 0)
		index = symbol->patterns.size();
	
	active_pattern = symbol->patterns.insert(symbol->patterns.begin() + index, AreaSymbol::FillPattern());
	auto temp_name = QString::fromLatin1("new");
	pattern_list->insertItem(index, temp_name);
	Q_ASSERT(int(symbol->patterns.size()) == pattern_list->count());
	
	active_pattern->type = type;
	if (type == AreaSymbol::FillPattern::PointPattern)
	{
		active_pattern->point = new PointSymbol();
		active_pattern->point->setRotatable(true);
		PointSymbolEditorWidget* editor = new PointSymbolEditorWidget(controller, active_pattern->point, 16);
		connect(editor, SIGNAL(symbolEdited()), this, SIGNAL(propertiesModified()) );
		if (pattern_list->currentRow() == int(symbol->patterns.size()) - 1)
		{
			addPropertiesGroup(temp_name, editor);
		}
		else
		{
			int group_index = indexOfPropertiesGroup(symbol->patterns[pattern_list->currentRow()+1].name);
			insertPropertiesGroup(group_index, temp_name, editor);
		}
	}
	else if (map->getNumColors() > 0)
	{
		// Default color for lines
		active_pattern->line_color = map->getColor(0);
	}
	updatePatternNames();
	emit propertiesModified();
	
	pattern_list->setCurrentRow(index);
}

void AreaSymbolSettings::deleteActivePattern()
{
	int index = pattern_list->currentRow();
	if (index < 0)
	{
		qWarning() << "AreaSymbolSettings::deleteActivePattern(): no fill selected."; // not translated
		return;
	}
	
	if (active_pattern->type == AreaSymbol::FillPattern::PointPattern)
	{
		removePropertiesGroup(active_pattern->name);
		delete active_pattern->point;
		active_pattern->point = NULL;
	}
	symbol->patterns.erase(active_pattern);
	
	delete pattern_list->takeItem(index);
	updatePatternNames();
	updatePatternWidgets();
	
	emit propertiesModified();
	
	index = pattern_list->currentRow();
	selectPattern(index);
}

void AreaSymbolSettings::selectPattern(int index)
{
	active_pattern = symbol->patterns.begin() + index;
	updatePatternWidgets();
}

void AreaSymbolSettings::colorChanged()
{
	symbol->color = color_edit->color();
	emit propertiesModified();
}

void AreaSymbolSettings::minimumSizeChanged(double value)
{
	symbol->minimum_area = qRound(1000.0 * value);
	emit propertiesModified();
}

void AreaSymbolSettings::patternAngleChanged(double value)
{
	active_pattern->angle = value * M_PI / 180.0;
	emit propertiesModified();
}

void AreaSymbolSettings::patternRotatableClicked(bool checked)
{
	active_pattern->rotatable = checked;
	emit propertiesModified();
}

void AreaSymbolSettings::patternSpacingChanged(double value)
{
	active_pattern->line_spacing = qRound(1000.0 * value);
	emit propertiesModified();
}

void AreaSymbolSettings::patternLineOffsetChanged(double value)
{
	active_pattern->line_offset = qRound(1000.0 * value);
	emit propertiesModified();
}

void AreaSymbolSettings::patternOffsetAlongLineChanged(double value)
{
	Q_ASSERT(active_pattern->type == AreaSymbol::FillPattern::PointPattern);
	active_pattern->offset_along_line = qRound(1000.0 * value);
	emit propertiesModified();
}

void AreaSymbolSettings::patternColorChanged()
{
	Q_ASSERT(active_pattern->type == AreaSymbol::FillPattern::LinePattern);
	active_pattern->line_color = pattern_color_edit->color();
	emit propertiesModified();
}

void AreaSymbolSettings::patternLineWidthChanged(double value)
{
	Q_ASSERT(active_pattern->type == AreaSymbol::FillPattern::LinePattern);
	active_pattern->line_width = qRound(1000.0 * value);
	emit propertiesModified();
}

void AreaSymbolSettings::patternPointDistChanged(double value)
{
	Q_ASSERT(active_pattern->type == AreaSymbol::FillPattern::PointPattern);
	active_pattern->point_distance = qRound(1000.0 * value);
	emit propertiesModified();
}
