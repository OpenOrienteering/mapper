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


#include "symbol.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <memory>

#include <QtGlobal>
#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QImageReader>
#include <QImageWriter>
#include <QLatin1Char>
#include <QLatin1String>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QStringRef>
#include <QVariant>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "settings.h"
#include "core/map.h"
#include "core/map_color.h"
#include "core/map_coord.h"
#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "core/renderables/renderable.h"
#include "core/renderables/renderable_implementation.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/combined_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/text_symbol.h"
#include "fileformats/file_format.h"
#include "fileformats/file_import_export.h"
#include "util/xml_stream_util.h"
#include "gui/util_gui.h"

// IWYU pragma: no_include <QObject>


namespace OpenOrienteering {

Symbol::Symbol(Type type) noexcept
: number { { -1, -1, -1 } }
, type { type }
, is_helper_symbol { false }
, is_hidden { false }
, is_protected { false }
{
	// nothing else
}


Symbol::Symbol(const Symbol& proto)
: icon { proto.icon }
, custom_icon { proto.custom_icon }
, name { proto.name }
, description { proto.description }
, number ( proto.number )  // Cannot use {} with Android gcc 4.9
, type { proto.type }
, is_helper_symbol { proto.is_helper_symbol }
, is_hidden { proto.is_hidden }
, is_protected { proto.is_protected }
{
	// nothing else
}


Symbol::~Symbol() = default;



bool Symbol::validate() const
{
	return true;
}



bool Symbol::equals(const Symbol* other, Qt::CaseSensitivity case_sensitivity) const
{
	return type == other->type
	       && numberEquals(other)
	       && is_helper_symbol == other->is_helper_symbol
	       && name.compare(other->name, case_sensitivity) == 0
	       && description.compare(other->description, case_sensitivity) == 0
	       && equalsImpl(other, case_sensitivity);
}


bool Symbol::stateEquals(const Symbol* other) const
{
	return is_hidden == other->is_hidden
	       && is_protected == other->is_protected;
}



const PointSymbol* Symbol::asPoint() const
{
	Q_ASSERT(type == Point);
	return static_cast<const PointSymbol*>(this);
}
PointSymbol* Symbol::asPoint()
{
	Q_ASSERT(type == Point);
	return static_cast<PointSymbol*>(this);
}
const LineSymbol* Symbol::asLine() const
{
	Q_ASSERT(type == Line);
	return static_cast<const LineSymbol*>(this);
}
LineSymbol* Symbol::asLine()
{
	Q_ASSERT(type == Line);
	return static_cast<LineSymbol*>(this);
}
const AreaSymbol* Symbol::asArea() const
{
	Q_ASSERT(type == Area);
	return static_cast<const AreaSymbol*>(this);
}
AreaSymbol* Symbol::asArea()
{
	Q_ASSERT(type == Area);
	return static_cast<AreaSymbol*>(this);
}
const TextSymbol* Symbol::asText() const
{
	Q_ASSERT(type == Text);
	return static_cast<const TextSymbol*>(this);
}
TextSymbol* Symbol::asText()
{
	Q_ASSERT(type == Text);
	return static_cast<TextSymbol*>(this);
}
const CombinedSymbol* Symbol::asCombined() const
{
	Q_ASSERT(type == Combined);
	return static_cast<const CombinedSymbol*>(this);
}
CombinedSymbol* Symbol::asCombined()
{
	Q_ASSERT(type == Combined);
	return static_cast<CombinedSymbol*>(this);
}



Symbol::TypeCombination Symbol::getContainedTypes() const
{
	return getType();
}


bool Symbol::isTypeCompatibleTo(const Object* object) const
{
	switch (object->getType())
	{
	case Object::Point:
		return type == Point;
	case Object::Path:
		return type == Line || type == Area || type == Combined;
	case Object::Text:
		return type == Text;
	}
	return false;
}



bool Symbol::numberEquals(const Symbol* other) const
{
	for (auto lhs = begin(number), rhs = begin(other->number); lhs != end(number); ++lhs, ++rhs)
	{
		if (*lhs != *rhs)
			return false;
		if (*lhs == -1 && *rhs == -1)
			break;
	}
	return true;
}


bool Symbol::numberEqualsRelaxed(const Symbol* other) const
{
	for (auto lhs = begin(number), rhs = begin(other->number); lhs != end(number) && rhs != end(other->number); ++lhs, ++rhs)
	{
		// When encountering -1 on one side and 0 on the other side,
		// move forward over all zeros on the other side.
		if (Q_UNLIKELY(*lhs == -1 && *rhs == 0))
		{
			do
			{
				++rhs;
				if (rhs == end(other->number))
					return true;
			}
			while (*rhs == 0);
		}
		else if (Q_UNLIKELY(*lhs == 0 && *rhs == -1))
		{
			do
			{
				++lhs;
				if (lhs == end(number))
					return true;
			}
			while (*lhs == 0);
		}
		
		if (*lhs != *rhs)
			return false;
		if (*lhs == -1 && *rhs == -1)
			break;
	}
	return true;
}



void Symbol::save(QXmlStreamWriter& xml, const Map& map) const
{
	XmlElementWriter symbol_element(xml, QLatin1String("symbol"));
	symbol_element.writeAttribute(QLatin1String("type"), int(type));
	auto id = map.findSymbolIndex(this);
	if (id >= 0)
		symbol_element.writeAttribute(QLatin1String("id"), id); // unique if given
	symbol_element.writeAttribute(QLatin1String("code"), getNumberAsString()); // not always unique
	if (!name.isEmpty())
		symbol_element.writeAttribute(QLatin1String("name"), name);
	symbol_element.writeAttribute(QLatin1String("is_helper_symbol"), is_helper_symbol);
	symbol_element.writeAttribute(QLatin1String("is_hidden"), is_hidden);
	symbol_element.writeAttribute(QLatin1String("is_protected"), is_protected);
	if (!description.isEmpty())
		xml.writeTextElement(QLatin1String("description"), description);
	saveImpl(xml, map);
	if (!custom_icon.isNull())
	{
		QBuffer buffer;
		QImageWriter writer{&buffer, QByteArrayLiteral("PNG")};
		if (writer.write(custom_icon))
		{
			auto data = buffer.data().toBase64();
			// The "data" URL scheme, RFC2397 (https://tools.ietf.org/html/rfc2397)
			data.insert(0, "data:image/png;base64,");
			xml.writeCharacters(QLatin1String("\n"));
			XmlElementWriter icon_element(xml, QLatin1String("icon"));
			icon_element.writeAttribute(QLatin1String("src"), QString::fromLatin1(data));
		}
		else
		{
			qDebug("Couldn't save symbol icon '%s': %s",
			       qPrintable(getPlainTextName()),
			       qPrintable(writer.errorString()) );
		}
	}
}

std::unique_ptr<Symbol> Symbol::load(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict)
{
	Q_ASSERT(xml.name() == QLatin1String("symbol"));
	XmlElementReader symbol_element(xml);
	auto symbol_type = symbol_element.attribute<int>(QLatin1String("type"));
	auto symbol = Symbol::makeSymbolForType(static_cast<Symbol::Type>(symbol_type));
	if (!symbol)
		throw FileFormatException(::OpenOrienteering::ImportExport::tr("Error while loading a symbol of type %1 at line %2 column %3.").arg(symbol_type).arg(xml.lineNumber()).arg(xml.columnNumber()));
	
	auto code = symbol_element.attribute<QString>(QLatin1String("code"));
	if (symbol_element.hasAttribute(QLatin1String("id")))
	{
		auto id = symbol_element.attribute<QString>(QLatin1String("id"));
		if (symbol_dict.contains(id)) 
			throw FileFormatException(::OpenOrienteering::ImportExport::tr("Symbol ID '%1' not unique at line %2 column %3.").arg(id).arg(xml.lineNumber()).arg(xml.columnNumber()));
		
		symbol_dict[id] = symbol.get();  // Will be dangling pointer when we throw an exception later
		
		if (code.isEmpty())
			code = id;
	}
	
	std::fill(begin(symbol->number), end(symbol->number), -1);
	if (!code.isEmpty())
	{
		int pos = 0;
		for (auto i = 0u; i < number_components; ++i)
		{
			int dot = code.indexOf(QLatin1Char('.'), pos+1);
			symbol->number[i] = code.midRef(pos, (dot == -1) ? -1 : (dot - pos)).toInt();
			pos = ++dot;
			if (pos < 1)
				break;
		}
	}
	
	symbol->name = symbol_element.attribute<QString>(QLatin1String("name"));
	symbol->is_helper_symbol = symbol_element.attribute<bool>(QLatin1String("is_helper_symbol"));
	symbol->is_hidden = symbol_element.attribute<bool>(QLatin1String("is_hidden"));
	symbol->is_protected = symbol_element.attribute<bool>(QLatin1String("is_protected"));
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == QLatin1String("description"))
		{
			symbol->description = xml.readElementText();
		}
		else if (xml.name() == QLatin1String("icon"))
		{
			XmlElementReader icon_element(xml);
			auto data = icon_element.attribute<QStringRef>(QLatin1String("src")).toLatin1();
			
			// The "data" URL scheme, RFC2397 (https://tools.ietf.org/html/rfc2397)
			auto start = data.indexOf(',') + 1;
			if (start > 0 && data.startsWith("data:image/"))
			{
				auto base64 = data.indexOf(";base64");
				data = data.remove(0, start);
				if (base64 + 8 == start)
					data = QByteArray::fromBase64(data);
			}
			else
			{
				data.clear();  // Ignore unknown data, warning is generated later.
			}
			
			QBuffer buffer{&data};
			QImageReader reader{&buffer, QByteArrayLiteral("PNG")};
			auto icon = reader.read();
			if (!icon.isNull())
				symbol->setCustomIcon(icon);
			else
				qDebug("Couldn't load symbol icon '%s': %s",
				       qPrintable(symbol->getPlainTextName()),
				       qPrintable(reader.errorString()) );
		}
		else
		{
			if (!symbol->loadImpl(xml, map, symbol_dict))
				xml.skipCurrentElement();
		}
	}
	
	if (xml.error())
	{
		throw FileFormatException(
		            ::OpenOrienteering::ImportExport::tr("Error while loading a symbol of type %1 at line %2 column %3: %4")
		            .arg(symbol_type)
		            .arg(xml.lineNumber())
		            .arg(xml.columnNumber())
		            .arg(xml.errorString()) );
	}
	
	return symbol;
}



bool Symbol::loadingFinishedEvent(Map* /*map*/)
{
	return true;
}



void Symbol::createRenderables(
        const PathObject* /*object*/,
        const PathPartVector& /*path_parts*/,
        ObjectRenderables& /*output*/,
        Symbol::RenderableOptions /*options*/) const
{
	qWarning("Missing implementation of Symbol::createRenderables(const PathObject*, const PathPartVector&, ObjectRenderables&, Symbol::RenderableOptions)");
}


void Symbol::createBaselineRenderables(
        const PathObject* /*object*/,
        const PathPartVector& path_parts,
        ObjectRenderables& output,
        const MapColor* color) const
{
	Q_ASSERT((getContainedTypes() & ~(Symbol::Line | Symbol::Area | Symbol::Combined)) == 0);
	
	if (color)
	{
		// Insert line renderable
		LineSymbol line_symbol;
		line_symbol.setColor(color);
		line_symbol.setLineWidth(0);
		for (const auto& part : path_parts)
		{
			output.insertRenderable(new LineRenderable(&line_symbol, part, false));
		}
	}
}



bool Symbol::symbolChangedEvent(const Symbol* /*old_symbol*/, const Symbol* /*new_symbol*/)
{
	return false;
}


bool Symbol::containsSymbol(const Symbol* /*symbol*/) const
{
	return false;
}



void Symbol::setCustomIcon(const QImage& image)
{
	resetIcon();  // Cache must become scaled version of custom icon.
	custom_icon = image;
}


QImage Symbol::getIcon(const Map* map) const
{
	if (icon.isNull())
	{
		auto size = Settings::getInstance().getSymbolWidgetIconSizePx();
		if (Settings::getInstance().getSetting(Settings::SymbolWidget_ShowCustomIcons).toBool()
		    && !custom_icon.isNull())
			icon = custom_icon.scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		else if (map)
			icon = createIcon(*map, size);
	}
	return icon;
}


QImage Symbol::createIcon(const Map& map, int side_length, bool antialiasing, qreal zoom) const
{
	// Desktop default used to be 2x zoom at 8 mm side length, plus/minus
	// a border of one white pixel around some objects.
	// If the icon is bigger than the rectangle with this zoom factor, the zoom
	// is reduced later to make the icon fit into the rectangle.
	if (zoom <= 0)
		zoom = map.symbolIconZoom();
	auto max_icon_mm_half = 0.5 / zoom;
	
	// Create image
	QImage image(side_length, side_length, QImage::Format_ARGB32_Premultiplied);
	QPainter painter(&image);
	if (antialiasing)
		painter.setRenderHint(QPainter::Antialiasing);
	
	// Make background transparent
	auto mode = painter.compositionMode();
	painter.setCompositionMode(QPainter::CompositionMode_Clear);
	painter.fillRect(image.rect(), Qt::transparent);
	painter.setCompositionMode(mode);
	painter.translate(0.5 * side_length, 0.5 * side_length);
	
	// Create geometry
	Object* object = nullptr;
	std::unique_ptr<Symbol> symbol_copy;
	auto offset = MapCoord{};
	auto contained_types = getContainedTypes();
	if (type == Point)
	{
		max_icon_mm_half *= 0.90; // white border
		object = new PointObject(static_cast<const PointSymbol*>(this));
	}
	else if (type == Text)
	{
		max_icon_mm_half *= 0.95; // white border
		auto text = new TextObject(this);
		text->setText(static_cast<const TextSymbol*>(this)->getIconText());
		object = text;
	}
	else if (type == Area || (type == Combined && contained_types & Area))
	{
		auto path = new PathObject(this);
		path->addCoordinate(0, MapCoord(-max_icon_mm_half, -max_icon_mm_half));
		path->addCoordinate(1, MapCoord(max_icon_mm_half, -max_icon_mm_half));
		path->addCoordinate(2, MapCoord(max_icon_mm_half, max_icon_mm_half));
		path->addCoordinate(3, MapCoord(-max_icon_mm_half, max_icon_mm_half));
		path->parts().front().setClosed(true, true);
		object = path;
	}
	else if (type == Line || type == Combined)
	{
		bool show_dash_symbol = false;
		auto line_length_half = max_icon_mm_half;
		if (type == Line)
		{
			auto line = static_cast<const LineSymbol*>(this);
			if (!line->isDashed() || line->getBreakLength() <= 0)
			{
				if (line->getCapStyle() == LineSymbol::RoundCap)
				{
					offset.setNativeX(-line->getLineWidth()/3);
				}
				else if (line->getCapStyle() == LineSymbol::PointedCap)
				{
					line_length_half = std::max(line_length_half, 0.0006 * (line->startOffset() + line->endOffset()));
				}
			}
			
			if (line->getDashSymbol() && !line->getDashSymbol()->isEmpty())
			{
				line_length_half = std::max(line_length_half, line->getDashSymbol()->dimensionForIcon());
				show_dash_symbol = true;
			}

			if (line->getMidSymbol() && !line->getMidSymbol()->isEmpty())
			{
				auto icon_size = line->getMidSymbol()->dimensionForIcon();
				icon_size += line->getMidSymbolDistance() * (line->getMidSymbolsPerSpot() - 1) / 2000;
				line_length_half = std::max(line_length_half, icon_size);

			    if (!line->getShowAtLeastOneSymbol())
				{
					if (!symbol_copy)
						symbol_copy = duplicate(*line);
					auto icon_line = static_cast<LineSymbol*>(symbol_copy.get());
					icon_line->setShowAtLeastOneSymbol(true);
				}
			}

			if (line->getStartSymbol() && !line->getStartSymbol()->isEmpty())
			{
				line_length_half = std::max(line_length_half, line->getStartSymbol()->dimensionForIcon() / 2);
			}

			if (line->getEndSymbol() && !line->getEndSymbol()->isEmpty())
			{
				line_length_half = std::max(line_length_half, line->getEndSymbol()->dimensionForIcon() / 2);
			}

			// If there are breaks in the line, scale them down so they fit into the icon exactly.
			auto max_ideal_length = 0;
			if (line->isDashed() && line->getBreakLength() > 0)
			{
				auto ideal_length = 2 * line->getDashesInGroup() * line->getDashLength()
				                    + 2 * (line->getDashesInGroup() - 1) * line->getInGroupBreakLength()
				                    + 3 * line->getBreakLength() / 2;
				if (max_ideal_length < ideal_length)
					max_ideal_length = ideal_length;
			}
			if (line->hasBorder())
			{
			    auto& border = line->getBorder();
				if (border.dashed && border.break_length > 0)
				{
					auto ideal_length = 2 * border.dash_length + border.break_length;
					if (max_ideal_length < ideal_length)
						max_ideal_length = ideal_length;
				}
				auto& right_border = line->getRightBorder();
				if (line->areBordersDifferent() && right_border.dashed && right_border.break_length > 0)
				{
					auto ideal_length = 2 * right_border.dash_length + right_border.break_length;
					if (max_ideal_length < ideal_length)
						max_ideal_length = ideal_length;
				}
			}
			if (max_ideal_length > 0)
			{
				auto cap_length = 0;
				auto offset_factor = qreal(0);
				auto ideal_length_half = qreal(max_ideal_length) / 2000;
				
				if (line->getCapStyle() == LineSymbol::PointedCap)
				{
					offset_factor = 1;
					ideal_length_half += qreal(line->startOffset() + line->endOffset()) / 2000;
				}
				else if (line->getCapStyle() != LineSymbol::FlatCap)
				{
					cap_length = line->getLineWidth();
					ideal_length_half += qreal(cap_length) / 1000;
				}
				
				auto factor = qMin(qreal(0.5), line_length_half / qMax(qreal(0.001), ideal_length_half));
				offset_factor *= factor;
				
				if (!symbol_copy)
					symbol_copy = duplicate(*line);
				
				auto icon_line = static_cast<LineSymbol*>(symbol_copy.get());
				icon_line->setDashLength(qRound(factor * icon_line->getDashLength()));
				icon_line->setBreakLength(cap_length + qRound(factor * (icon_line->getBreakLength() - cap_length)));
				icon_line->setInGroupBreakLength(qRound(factor * icon_line->getInGroupBreakLength()));
				icon_line->setShowAtLeastOneSymbol(true);
				icon_line->getBorder().dash_length *= factor;
				icon_line->getBorder().break_length *= factor;
				icon_line->getRightBorder().dash_length *= factor;
				icon_line->getRightBorder().break_length *= factor;
				icon_line->setStartOffset(qRound(offset_factor * icon_line->startOffset()));
				icon_line->setEndOffset(qRound(offset_factor * icon_line->endOffset()));
			}
		}
		else if (type == Combined)
		{
			auto max_ideal_length = 0;
			auto combined = static_cast<const CombinedSymbol*>(this);
			for (int i = 0; i < combined->getNumParts(); ++i)
			{
				auto part = combined->getPart(i);
				if (part && part->getType() == Line)
				{
					auto line = static_cast<const LineSymbol*>(part);
					auto dash_symbol = line->getDashSymbol();
					if (dash_symbol && !dash_symbol->isEmpty())
						show_dash_symbol = true;
					
					if (line->isDashed() && line->getBreakLength() > 0)
					{
						auto ideal_length = 2 * line->getDashesInGroup() * line->getDashLength()
						                    + 2 * (line->getDashesInGroup() - 1) * line->getInGroupBreakLength()
						                    + line->getBreakLength();
						if (max_ideal_length < ideal_length)
							max_ideal_length = ideal_length;
					}
					if (line->hasBorder())
					{
					    auto& border = line->getBorder();
						if (border.dashed && border.break_length > 0)
						{
							auto ideal_length = 2 * border.dash_length + border.break_length;
							if (max_ideal_length < ideal_length)
								max_ideal_length = ideal_length;
						}
						auto& right_border = line->getRightBorder();
						if (line->areBordersDifferent() && right_border.dashed && right_border.break_length > 0)
						{
							auto ideal_length = 2 * right_border.dash_length + right_border.break_length;
							if (max_ideal_length < ideal_length)
								max_ideal_length = ideal_length;
						}
					}
				}
			}
			// If there are breaks in the line, scale them down so they fit into the icon exactly.
			if (max_ideal_length > 0)
			{
				auto ideal_length_half = qreal(max_ideal_length) / 2000;
				auto factor = qMin(qreal(0.5), line_length_half / qMax(qreal(0.001), ideal_length_half));
				
				symbol_copy.reset(new CombinedSymbol());
				static_cast<CombinedSymbol*>(symbol_copy.get())->setNumParts(combined->getNumParts());
				
				for (int i = 0; i < combined->getNumParts(); ++i)
				{
					auto proto = static_cast<const CombinedSymbol*>(combined)->getPart(i);
					if (!proto)
						continue;
					
					auto copy = duplicate(*proto);
					if (copy->getType() == Line)
					{
						auto icon_line = static_cast<LineSymbol*>(copy.get());
						icon_line->setDashLength(qRound(factor * icon_line->getDashLength()));
						icon_line->setBreakLength(qRound(factor * icon_line->getBreakLength()));
						icon_line->setInGroupBreakLength(qRound(factor * icon_line->getInGroupBreakLength()));
						icon_line->setShowAtLeastOneSymbol(true);
						icon_line->getBorder().dash_length *= factor;
						icon_line->getBorder().break_length *= factor;
						icon_line->getRightBorder().dash_length *= factor;
						icon_line->getRightBorder().break_length *= factor;
					}
					static_cast<CombinedSymbol*>(symbol_copy.get())->setPart(i, copy.release(), true);
				}
			}
		}
		
		auto path = new PathObject(symbol_copy ? symbol_copy.get() : this);
		path->addCoordinate(0, MapCoord(-line_length_half, 0.0));
		path->addCoordinate(1, MapCoord(line_length_half, 0.0));
		if (show_dash_symbol)
		{
			MapCoord dash_coord(0, 0);
			dash_coord.setDashPoint(true);
			path->addCoordinate(1, dash_coord);
		}
		object = path;
	}
	else
	{
		qWarning("Unhandled symbol: %s", qPrintable(getDescription()));
		return image;
	}
	
	// Create icon map and view
	Map icon_map;
	// const_cast promise: We won't change the colors, thus we won't change map.
	icon_map.useColorsFrom(const_cast<Map*>(&map));
	icon_map.setScaleDenominator(map.getScaleDenominator());
	icon_map.addObject(object);
	
	const auto& extent = object->getExtent();
	auto w = std::max(std::abs(extent.left()), std::abs(extent.right()));
	auto h = std::max(std::abs(extent.top()), std::abs(extent.bottom()));
	auto real_icon_mm_half = std::max(w, h);
	if (real_icon_mm_half <= 0)
		return image;
	
	auto final_zoom = side_length * zoom * std::min(qreal(1), max_icon_mm_half / real_icon_mm_half);
	painter.scale(final_zoom, final_zoom);
	
	if (type == Text)
	{
		// Center text
		painter.translate(-extent.center());
	}
	else if (type == Point)
	{
		// Do not completely offset the symbols relative position
		painter.translate(-extent.center() / 2);
	}
	else if (contained_types & Line && !(contained_types & Area))
	{
		painter.translate(MapCoordF(-offset));
	}
	
	// Ensure that an icon is created for hidden symbols.
	if (is_hidden && !symbol_copy)
	{
		symbol_copy = duplicate(*this);
		object->setSymbol(symbol_copy.get(), true);
	}
	if (symbol_copy)
	{
		symbol_copy->setHidden(false);
	}
	
	auto config = RenderConfig { map, QRectF(-10000, -10000, 20000, 20000), final_zoom, RenderConfig::HelperSymbols, 1.0 };
	icon_map.draw(&painter, config);
	painter.end();
	
	// Add shadow to dominant white on transparent
	auto color = guessDominantColor();
	if (color && color->isWhite())
	{
		for (int y = image.height() - 1; y >= 0; --y)
		{
			for (int x = image.width() - 1; x >= 0; --x)
			{
				if (qAlpha(image.pixel(x, y)) > 0)
					continue;
				
				auto is_white = [](const QRgb& rgb) {
					return rgb > 0
					       && qAlpha(rgb) == qRed(rgb)
						   && qRed(rgb) == qGreen(rgb)
						   && qGreen(rgb) == qBlue(rgb);
				};
				if (x > 0)
				{
					auto preceding = image.pixel(x-1, y);
					if (is_white(preceding))
					{
						image.setPixel(x, y, qPremultiply(qRgba(0, 0, 0, 127)));
						continue;
					}
				}
				if (y > 0)
				{
					auto preceding = image.pixel(x, y-1);
					if (is_white(preceding))
					{
						image.setPixel(x, y, qPremultiply(qRgba(0, 0, 0, 127)));
					}
				}
			}
		}
	}
	
	return image;
}


void Symbol::resetIcon()
{
	icon = {};
}


qreal Symbol::dimensionForIcon() const
{
	return 0;
}



qreal Symbol::calculateLargestLineExtent() const
{
	return 0;
}



QString Symbol::getPlainTextName() const
{
	return Util::plainText(name);
}



QString Symbol::getNumberAsString() const
{
	QString string;
	string.reserve(4 * number_components - 1);
	for (auto n : number)
	{
		if (n < 0)
			break;
		string.append(QString::number(n));
		string.append(QLatin1Char('.'));
	}
	string.chop(1);
	return string;
}



std::unique_ptr<Symbol> Symbol::makeSymbolForType(Symbol::Type type)
{
	switch (type)
	{
	case Area:
		return std::make_unique<AreaSymbol>();
	case Combined:
		return std::make_unique<CombinedSymbol>();
	case Line:
		return std::make_unique<LineSymbol>();
	case Point:
		return std::make_unique<PointSymbol>();
	case Text:
		return std::make_unique<TextSymbol>();
	default:
		return std::unique_ptr<Symbol>();
	}
}



bool Symbol::areTypesCompatible(Symbol::Type a, Symbol::Type b)
{
	return (getCompatibleTypes(a) & b) != 0;
}


Symbol::TypeCombination Symbol::getCompatibleTypes(Symbol::Type type)
{
	switch (type)
	{
	case Area:
	case Combined:
	case Line:
		return Line | Area | Combined;
	case Point:
	case Text:
	default:
		return type;
	}
}



bool Symbol::lessByNumber(const Symbol* s1, const Symbol* s2)
{
	for (auto i = 0u; i < number_components; i++)
	{
		if (s1->number[i] < s2->number[i]) return true;  // s1 < s2
		if (s1->number[i] > s2->number[i]) return false; // s1 > s2
	}
	return false; // s1 == s2
}

bool Symbol::lessByColorPriority(const Symbol* s1, const Symbol* s2)
{
	const MapColor* c1 = s1->guessDominantColor();
	const MapColor* c2 = s2->guessDominantColor();
	
	if (c1 && c2)
		return c1->comparePriority(*c2);
	else if (c2)
		return true;
	else
		return false;
}



Symbol::lessByColor::lessByColor(const Map* map)
{
	colors.reserve(std::size_t(map->getNumColors()));
	for (int i = 0; i < map->getNumColors(); ++i)
		colors.push_back(QRgb(*map->getColor(i)));
}


bool Symbol::lessByColor::operator() (const Symbol* s1, const Symbol* s2) const
{
	auto c2 = s2->guessDominantColor();
	if (!c2)
		return false;
	
	auto c1 = s1->guessDominantColor();
	if (!c1)
		return true;
	
	const auto rgb_c1 = QRgb(*c1);
	const auto rgb_c2 = QRgb(*c2);
	if (rgb_c1 == rgb_c2)
		return false;
	
	const auto last = colors.rend();
	auto first = std::find_if(colors.rbegin(), last, [rgb_c2](const auto rgb) {
		return rgb == rgb_c2;
	});
	auto second = std::find_if(first, last, [rgb_c1](const auto rgb) {
		return rgb == rgb_c1;
	});
	return second != last;
}


}  // namespace OpenOrienteering
