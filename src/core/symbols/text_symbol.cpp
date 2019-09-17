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


#include "text_symbol.h"

#include <cmath>
#include <cstddef>
#include <memory>
// IWYU pragma: no_include <ext/alloc_traits.h>

#include <QtGlobal>
#include <QCoreApplication>
#include <QFont>
#include <QLatin1String>
#include <QPointF>
#include <QRectF>
#include <QStringRef>
#include <QTransform>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/map.h"
#include "core/map_color.h"
#include "core/map_coord.h"
#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "core/renderables/renderable.h"
#include "core/renderables/renderable_implementation.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/symbol.h"
#include "core/virtual_coord_vector.h"
#include "core/virtual_path.h"
#include "util/util.h"


namespace OpenOrienteering {

TextSymbol::TextSymbol()
: Symbol { Symbol::Text }
, qfont {}
, metrics { qfont }
, font_family { QStringLiteral("Arial") }
, icon_text { }
, color { nullptr }
, framing_color { nullptr }
, line_below_color { nullptr }
, line_spacing { 1 }
, character_spacing { 0 }
, font_size { 4 * 1000 }
, paragraph_spacing { 0 }
, framing_mode { LineFraming }
, framing_line_half_width { 200 }
, framing_shadow_x_offset { 200 }
, framing_shadow_y_offset { 200 }
, line_below_width { 0 }
, line_below_distance { 0 }
, bold { false }
, italic { false }
, underline { false }
, kerning { true }
, framing { false }
, line_below { false }
{
	updateQFont();
}


TextSymbol::TextSymbol(const TextSymbol& proto)
: Symbol { proto }
, qfont { proto.qfont }
, metrics { proto.metrics }
, font_family { proto.font_family }
, icon_text { proto.icon_text }
, color { proto.color }
, framing_color { proto.framing_color }
, line_below_color { proto.line_below_color }
, custom_tabs { proto.custom_tabs }
, tab_interval { proto.tab_interval }
, line_spacing { proto.line_spacing }
, character_spacing { proto.character_spacing }
, font_size { proto.font_size }
, paragraph_spacing { proto.paragraph_spacing }
, framing_mode { proto.framing_mode }
, framing_line_half_width { proto.framing_line_half_width }
, framing_shadow_x_offset { proto.framing_shadow_x_offset }
, framing_shadow_y_offset { proto.framing_shadow_y_offset }
, line_below_width { proto.line_below_width }
, line_below_distance { proto.line_below_distance }
, bold { proto.bold }
, italic { proto.italic }
, underline { proto.underline }
, kerning { proto.kerning }
, framing { proto.framing }
, line_below { proto.line_below }
{
	// nothing else
}


TextSymbol::~TextSymbol() = default;


TextSymbol* TextSymbol::duplicate() const
{
	return new TextSymbol(*this);
}



void TextSymbol::createRenderables(
        const Object *object,
        const VirtualCoordVector &coords,
        ObjectRenderables &output,
        Symbol::RenderableOptions options) const
{
	Q_ASSERT(object);
	
	const TextObject* text_object = static_cast<const TextObject*>(object);
	text_object->prepareLineInfos();
	
	if (options.testFlag(Symbol::RenderBaselines))
	{
		createBaselineRenderables(text_object, coords, output);
	}
	else
	{
		auto anchor = coords[0];
		double anchor_x = anchor.x();
		double anchor_y = anchor.y();
		
		if (color)
			output.insertRenderable(new TextRenderable(this, text_object, color, anchor_x, anchor_y));
		
		if (line_below && line_below_color && line_below_width > 0)
			createLineBelowRenderables(object, output);
		
		if (framing && framing_color)
		{
			if (framing_mode == LineFraming && framing_line_half_width > 0)
			{
				output.insertRenderable(new TextFramingRenderable(this, text_object, framing_color, anchor_x, anchor_y));
			}
			else if (framing_mode == ShadowFraming)
			{
				output.insertRenderable(new TextRenderable(this, text_object, framing_color, anchor_x + 0.001 * framing_shadow_x_offset, anchor_y + 0.001 * framing_shadow_y_offset));
			}
		}
	}
}

void TextSymbol::createBaselineRenderables(
        const TextObject* text_object,
        const VirtualCoordVector& coords,
        ObjectRenderables& output) const
{
	const MapColor* dominant_color = guessDominantColor();
	if (dominant_color && text_object->getNumLines() > 0)
	{
		// Insert text boundary
		LineSymbol line_symbol;
		line_symbol.setColor(dominant_color);
		line_symbol.setLineWidth(0);
		
		const TextObjectLineInfo* line = text_object->getLineInfo(0);
		QRectF text_bbox(line->line_x, line->line_y - line->ascent, line->width, line->ascent + line->descent);
		for (int i = 1; i < text_object->getNumLines(); ++i)
		{
			const TextObjectLineInfo* line = text_object->getLineInfo(i);
			rectInclude(text_bbox, QRectF(line->line_x, line->line_y - line->ascent, line->width, line->ascent + line->descent));
		}
		
		Q_UNUSED(coords); // coords should be used for calcTextToMapTransform()
		QTransform text_to_map = text_object->calcTextToMapTransform();
		PathObject path;
		path.addCoordinate(MapCoord(text_to_map.map(text_bbox.topLeft())));
		path.addCoordinate(MapCoord(text_to_map.map(text_bbox.topRight())));
		path.addCoordinate(MapCoord(text_to_map.map(text_bbox.bottomRight())));
		path.addCoordinate(MapCoord(text_to_map.map(text_bbox.bottomLeft())));
		path.parts().front().setClosed(true, true);
		path.updatePathCoords();
		
		auto line_renderable = new LineRenderable(&line_symbol, path.parts().front(), false);
		output.insertRenderable(line_renderable);
	}
}

void TextSymbol::createLineBelowRenderables(const Object* object, ObjectRenderables& output) const
{
	auto text_object = reinterpret_cast<const TextObject*>(object);
	if (text_object->getNumLines())
	{
		double scale_factor = calculateInternalScaling();
		AreaSymbol area_symbol;
		area_symbol.setColor(line_below_color);
		
		MapCoordVector  line_flags(4);
		MapCoordVectorF line_coords(4);
		VirtualPath line_path = { line_flags, line_coords };
		line_flags.back().setHolePoint(true);
		
		QTransform transform = text_object->calcTextToMapTransform();
		
		for (int i = 0; i < text_object->getNumLines(); ++i)
		{
			const TextObjectLineInfo* line_info = text_object->getLineInfo(i);
			if (!line_info->paragraph_end)
				continue;
			
			double line_below_x0;
			double line_below_x1;
			if (text_object->hasSingleAnchor())
			{
				line_below_x0 = line_info->line_x;
				line_below_x1 = line_below_x0 + line_info->width;
			}
			else
			{
				double box_width = text_object->getBoxWidth() * scale_factor;
				line_below_x0 = -0.5 * box_width;
				line_below_x1 = line_below_x0 + box_width;
			}
			double line_below_y0 = line_info->line_y + getLineBelowDistance() * scale_factor;
			double line_below_y1 = line_below_y0 + getLineBelowWidth() * scale_factor;
			line_coords[0] = MapCoordF(transform.map(QPointF(line_below_x0, line_below_y0)));
			line_coords[1] = MapCoordF(transform.map(QPointF(line_below_x1, line_below_y0)));
			line_coords[2] = MapCoordF(transform.map(QPointF(line_below_x1, line_below_y1)));
			line_coords[3] = MapCoordF(transform.map(QPointF(line_below_x0, line_below_y1)));
			
			line_path.path_coords.update(0);
			output.insertRenderable(new AreaRenderable(&area_symbol, line_path));
		}
	}
}

void TextSymbol::colorDeletedEvent(const MapColor* c)
{
	auto changes = 0;
	if (c == color)
	{
		color = nullptr;
		++changes;
	}
	if (c == framing_color)
	{
		framing_color = nullptr;
		++changes;
	}
	if (c == line_below_color)
	{
		line_below_color = nullptr;
		++changes;
	}
	if (changes)
	{
		resetIcon();
	}
}

bool TextSymbol::containsColor(const MapColor* c) const
{
	return c == color
	       || c == framing_color
	       || c == line_below_color;
}

const MapColor* TextSymbol::guessDominantColor() const
{
	auto c = color;
	if (!c)
		c = framing_color;
	if (!c)
		c = line_below_color;
	return c;
}

void TextSymbol::replaceColors(const MapColorMap& color_map)
{
	color = color_map.value(color);
	framing_color = color_map.value(framing_color);
	line_below_color = color_map.value(line_below_color);
}

void TextSymbol::scale(double factor)
{
	font_size = qMax(10, qRound(factor * font_size)); // minimum 0.01 mm
	framing_line_half_width = qRound(factor * framing_line_half_width);
	framing_shadow_x_offset = qRound(factor * framing_shadow_x_offset);
	framing_shadow_y_offset = qRound(factor * framing_shadow_y_offset);
	
	updateQFont();
	
	resetIcon();
}

void TextSymbol::updateQFont()
{
	qfont = QFont();
	qfont.setBold(bold);
	qfont.setItalic(italic);
	qfont.setUnderline(underline);
	qfont.setPixelSize(internal_point_size);
	qfont.setFamily(font_family);
	qfont.setHintingPreference(QFont::PreferNoHinting);
	qfont.setKerning(kerning);
	metrics = QFontMetricsF(qfont);
	qfont.setLetterSpacing(QFont::AbsoluteSpacing, metrics.width(QString(QLatin1String{" "})) * character_spacing);
	
	qfont.setStyleStrategy(QFont::ForceOutline);

	metrics = QFontMetricsF(qfont);
	tab_interval = 8.0 * metrics.averageCharWidth();
}



void TextSymbol::saveImpl(QXmlStreamWriter& xml, const Map& map) const
{
	xml.writeStartElement(QStringLiteral("text_symbol"));
	xml.writeAttribute(QStringLiteral("icon_text"), icon_text);
	
	xml.writeStartElement(QStringLiteral("font"));
	xml.writeAttribute(QStringLiteral("family"), font_family);
	xml.writeAttribute(QStringLiteral("size"), QString::number(font_size));
	if (bold)
		xml.writeAttribute(QStringLiteral("bold"), QStringLiteral("true"));
	if (italic)
		xml.writeAttribute(QStringLiteral("italic"), QStringLiteral("true"));
	if (underline)
		xml.writeAttribute(QStringLiteral("underline"), QStringLiteral("true"));
	xml.writeEndElement(/*font*/);
	
	xml.writeStartElement(QStringLiteral("text"));
	xml.writeAttribute(QStringLiteral("color"), QString::number(map.findColorIndex(color)));
	xml.writeAttribute(QStringLiteral("line_spacing"), QString::number(line_spacing));
	xml.writeAttribute(QStringLiteral("paragraph_spacing"), QString::number(paragraph_spacing));
	xml.writeAttribute(QStringLiteral("character_spacing"), QString::number(character_spacing));
	if (kerning)
		xml.writeAttribute(QStringLiteral("kerning"), QStringLiteral("true"));
	xml.writeEndElement(/*text*/);
	
	if (framing)
	{
		xml.writeStartElement(QStringLiteral("framing"));
		xml.writeAttribute(QStringLiteral("color"), QString::number(map.findColorIndex(framing_color)));
		xml.writeAttribute(QStringLiteral("mode"), QString::number(framing_mode));
		xml.writeAttribute(QStringLiteral("line_half_width"), QString::number(framing_line_half_width));
		xml.writeAttribute(QStringLiteral("shadow_x_offset"), QString::number(framing_shadow_x_offset));
		xml.writeAttribute(QStringLiteral("shadow_y_offset"), QString::number(framing_shadow_y_offset));
		xml.writeEndElement(/*framing*/);
	}
	
	if (line_below)
	{
		xml.writeStartElement(QStringLiteral("line_below"));
		xml.writeAttribute(QStringLiteral("color"), QString::number(map.findColorIndex(line_below_color)));
		xml.writeAttribute(QStringLiteral("width"), QString::number(line_below_width));
		xml.writeAttribute(QStringLiteral("distance"), QString::number(line_below_distance));
		xml.writeEndElement(/*line_below*/);
	}
	
	int num_custom_tabs = getNumCustomTabs();
	if (num_custom_tabs > 0)
	{
		xml.writeStartElement(QStringLiteral("tabs"));
		xml.writeAttribute(QStringLiteral("count"), QString::number(num_custom_tabs));
		for (int i = 0; i < num_custom_tabs; ++i)
			xml.writeTextElement(QStringLiteral("tab"), QString::number(custom_tabs[i]));
		xml.writeEndElement(/*tabs*/);
	}
	
	xml.writeEndElement(/*text_symbol*/);
}

bool TextSymbol::loadImpl(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict)
{
	Q_UNUSED(symbol_dict);
	
	if (xml.name() != QLatin1String("text_symbol"))
		return false;
	
	icon_text = xml.attributes().value(QLatin1String("icon_text")).toString();
	framing = false;
	line_below = false;
	custom_tabs.clear();
	
	while (xml.readNextStartElement())
	{
		QXmlStreamAttributes attributes(xml.attributes());
		if (xml.name() == QLatin1String("font"))
		{
			font_family = xml.attributes().value(QLatin1String("family")).toString();
			font_size = attributes.value(QLatin1String("size")).toInt();
			bold = (attributes.value(QLatin1String("bold")) == QLatin1String("true"));
			italic = (attributes.value(QLatin1String("italic")) == QLatin1String("true"));
			underline = (attributes.value(QLatin1String("underline")) == QLatin1String("true"));
			xml.skipCurrentElement();
		}
		else if (xml.name() == QLatin1String("text"))
		{
			int temp = attributes.value(QLatin1String("color")).toInt();
			color = map.getColor(temp);
			line_spacing = attributes.value(QLatin1String("line_spacing")).toFloat();
			paragraph_spacing = attributes.value(QLatin1String("paragraph_spacing")).toInt();
			character_spacing = attributes.value(QLatin1String("character_spacing")).toFloat();
			kerning = (attributes.value(QLatin1String("kerning")) == QLatin1String("true"));
			xml.skipCurrentElement();
		}
		else if (xml.name() == QLatin1String("framing"))
		{
			framing = true;
			int temp = attributes.value(QLatin1String("color")).toInt();
			framing_color = map.getColor(temp);
			framing_mode = attributes.value(QLatin1String("mode")).toInt();
			framing_line_half_width = attributes.value(QLatin1String("line_half_width")).toInt();
			framing_shadow_x_offset = attributes.value(QLatin1String("shadow_x_offset")).toInt();
			framing_shadow_y_offset = attributes.value(QLatin1String("shadow_y_offset")).toInt();
			xml.skipCurrentElement();
		}
		else if (xml.name() == QLatin1String("line_below"))
		{
			line_below = true;
			int temp = attributes.value(QLatin1String("color")).toInt();
			line_below_color = map.getColor(temp);
			line_below_width = attributes.value(QLatin1String("width")).toInt();
			line_below_distance = attributes.value(QLatin1String("distance")).toInt();
			xml.skipCurrentElement();
		}
		else if (xml.name() == QLatin1String("tabs"))
		{
			int num_custom_tabs = attributes.value(QLatin1String("count")).toInt();
			custom_tabs.reserve(num_custom_tabs % 20); // 20 is not the limit
			while (xml.readNextStartElement())
			{
				if (xml.name() == QLatin1String("tab"))
					custom_tabs.push_back(xml.readElementText().toInt());
				else
					xml.skipCurrentElement();
			}
		}
		else
			xml.skipCurrentElement(); // unknown
	}
	
	updateQFont();
	return true;
}

bool TextSymbol::equalsImpl(const Symbol* other, Qt::CaseSensitivity case_sensitivity) const
{
	Q_UNUSED(case_sensitivity);
	
	const TextSymbol* text = static_cast<const TextSymbol*>(other);
	
	if (!MapColor::equal(color, text->color))
		return false;
	if (font_family.compare(text->font_family, Qt::CaseInsensitive) != 0)
		return false;
	if (font_size != text->font_size ||
		bold != text->bold ||
		italic != text->italic ||
		underline != text->underline ||
		qAbs(line_spacing - text->line_spacing) > 0.0005f ||
		paragraph_spacing != text->paragraph_spacing ||
		qAbs(character_spacing - text->character_spacing) > 0.0005f ||
		kerning != text->kerning ||
		line_below != text->line_below ||
		framing != text->framing)
		return false;
	if (framing)
	{
		if (!MapColor::equal(framing_color, text->framing_color))
			return false;
		if (framing_mode != text->framing_mode ||
			(framing_mode == LineFraming && framing_line_half_width != text->framing_line_half_width) ||
			(framing_mode == ShadowFraming && (framing_shadow_x_offset != text->framing_shadow_x_offset || framing_shadow_y_offset != text->framing_shadow_y_offset)))
			return false;
	}
	if (line_below)
	{
		if (!MapColor::equal(line_below_color, text->line_below_color))
			return false;
		if (line_below_width != text->line_below_width ||
			line_below_distance != text->line_below_distance)
			return false;
	}
	if (custom_tabs.size() != text->custom_tabs.size())
		return false;
	for (size_t i = 0, end = custom_tabs.size(); i < end; ++i)
	{
		if (custom_tabs[i] != text->custom_tabs[i])
			return false;
	}
	
	return true;
}

QString TextSymbol::getIconText() const
{
	if (icon_text.isEmpty())
		return QCoreApplication::translate("OpenOrienteering::TextSymbolSettings", "A", "First capital letter of the local alphabet");
	return icon_text;
}

double TextSymbol::getNextTab(double pos) const
{
	if (!custom_tabs.empty())
	{
		double scaling = calculateInternalScaling();
		double map_pos = pos / scaling;
		for (auto tab : custom_tabs)
		{
			if (0.001 * tab > map_pos)
				return scaling * 0.001 * tab;
		}
		
		// After the given positions, OCAD repeats the distance between the last two tab positions
		double custom_tab_interval = (custom_tabs.size() > 1) ?
									   (custom_tabs[custom_tabs.size() - 1] - custom_tabs[custom_tabs.size() - 2]) :
									   custom_tabs[0];
		custom_tab_interval *= 0.001;
		return scaling * (0.001 * custom_tabs[custom_tabs.size() - 1] + (floor((map_pos - 0.001 * custom_tabs[custom_tabs.size() - 1]) / custom_tab_interval) + 1.0) * custom_tab_interval);
	}
	
	double next_tab = (floor(pos / tab_interval) + 1.0) * tab_interval;
	Q_ASSERT(next_tab > pos);
 	return next_tab;
}


}  // namespace OpenOrienteering
