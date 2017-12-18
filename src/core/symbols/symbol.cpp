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


#include "symbol.h"

#include <memory>

#include <QtGlobal>
#include <QIODevice>
#include <QLatin1Char>
#include <QLatin1String>
#include <QPainter>
#include <QRectF>
#include <QStringRef>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/map.h"
#include "core/map_color.h"
#include "core/map_coord.h"
#include "core/map_view.h"
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
#include "gui/util_gui.h"
#include "util/util.h"
#include "settings.h"

// IWYU pragma: no_include <QObject>


Symbol::Symbol(Type type) noexcept
 : type { type }
 , number { -1, -1, -1 }
 , is_helper_symbol(false)
 , is_hidden(false)
 , is_protected(false)
{
	// nothing else
}


Symbol::~Symbol() = default;



bool Symbol::validate() const
{
	return true;
}



bool Symbol::equals(const Symbol* other, Qt::CaseSensitivity case_sensitivity, bool compare_state) const
{
	if (type != other->type)
		return false;
	for (int i = 0; i < number_components; ++i)
	{
		if (number[i] != other->number[i])
			return false;
		if (number[i] == -1 && other->number[i] == -1)
			break;
	}
	if (is_helper_symbol != other->is_helper_symbol)
		return false;
	
	if (compare_state && (is_hidden != other->is_hidden || is_protected != other->is_protected))
		return false;
	
	if (name.compare(other->name, case_sensitivity) != 0)
		return false;
	if (description.compare(other->description, case_sensitivity) != 0)
		return false;
	
	return equalsImpl(other, case_sensitivity);
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

bool Symbol::isTypeCompatibleTo(const Object* object) const
{
	if (type == Point && object->getType() == Object::Point)
		return true;
	else if ((type == Line || type == Area || type == Combined) && object->getType() == Object::Path)
		return true;
	else if (type == Text && object->getType() == Object::Text)
		return true;

	return false;
}

bool Symbol::numberEquals(const Symbol* other, bool ignore_trailing_zeros) const
{
	if (ignore_trailing_zeros)
	{
		for (int i = 0; i < number_components; ++i)
		{
			if (number[i] == -1 && other->number[i] == -1)
				return true;
			if ((number[i] == 0 || number[i] == -1) &&
				(other->number[i] == 0 || other->number[i] == -1))
				continue;
			if (number[i] != other->number[i])
				return false;
		}
	}
	else
	{
		for (int i = 0; i < number_components; ++i)
		{
			if (number[i] != other->number[i])
				return false;
			if (number[i] == -1)
				return true;
		}
	}
	return true;
}

#ifndef NO_NATIVE_FILE_FORMAT

bool Symbol::load(QIODevice* file, int version, Map* map)
{
	loadString(file, name);
	for (int i = 0; i < number_components; ++i)
		file->read((char*)&number[i], sizeof(int));
	loadString(file, description);
	file->read((char*)&is_helper_symbol, sizeof(bool));
	if (version >= 10)
		file->read((char*)&is_hidden, sizeof(bool));
	if (version >= 11)
		file->read((char*)&is_protected, sizeof(bool));
	
	return loadImpl(file, version, map);
}

#endif

void Symbol::save(QXmlStreamWriter& xml, const Map& map) const
{
	xml.writeStartElement(QString::fromLatin1("symbol"));
	xml.writeAttribute(QString::fromLatin1("type"), QString::number(type));
	int id = map.findSymbolIndex(this);
	if (id >= 0)
		xml.writeAttribute(QString::fromLatin1("id"), QString::number(id)); // unique if given
	xml.writeAttribute(QString::fromLatin1("code"), getNumberAsString()); // not always unique
	if (!name.isEmpty())
		xml.writeAttribute(QString::fromLatin1("name"), name);
	if (is_helper_symbol)
		xml.writeAttribute(QString::fromLatin1("is_helper_symbol"), QString::fromLatin1("true"));
	if (is_hidden)
		xml.writeAttribute(QString::fromLatin1("is_hidden"), QString::fromLatin1("true"));
	if (is_protected)
		xml.writeAttribute(QString::fromLatin1("is_protected"), QString::fromLatin1("true"));
	if (!description.isEmpty())
		xml.writeTextElement(QString::fromLatin1("description"), description);
	saveImpl(xml, map);
	xml.writeEndElement(/*symbol*/);
}

Symbol* Symbol::load(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict)
{
	Q_ASSERT(xml.name() == QLatin1String("symbol"));
	
	int symbol_type = xml.attributes().value(QLatin1String("type")).toInt();
	Symbol* symbol = Symbol::getSymbolForType(static_cast<Symbol::Type>(symbol_type));
	if (!symbol)
		throw FileFormatException(ImportExport::tr("Error while loading a symbol of type %1 at line %2 column %3.").arg(symbol_type).arg(xml.lineNumber()).arg(xml.columnNumber()));
	
	QXmlStreamAttributes attributes = xml.attributes();
	QString code = attributes.value(QLatin1String("code")).toString();
	if (attributes.hasAttribute(QLatin1String("id")))
	{
		QString id = attributes.value(QLatin1String("id")).toString();
		if (symbol_dict.contains(id)) 
			throw FileFormatException(ImportExport::tr("Symbol ID '%1' not unique at line %2 column %3.").arg(id).arg(xml.lineNumber()).arg(xml.columnNumber()));
		
		symbol_dict[id] = symbol;
		
		if (code.isEmpty())
			code = id;
	}
	
	if (code.isEmpty())
		symbol->number[0] = -1;
	else
	{
		for (int i = 0, index = 0; i < number_components && index >= 0; ++i)
		{
			if (index == -1)
				symbol->number[i] = -1;
			else
			{
				int dot = code.indexOf(QLatin1Char('.'), index+1);
				int num = code.midRef(index, (dot == -1) ? -1 : (dot - index)).toInt();
				symbol->number[i] = num;
				index = dot;
				if (index != -1)
					index++;
			}
		}
	}
	
	symbol->name = attributes.value(QLatin1String("name")).toString();
	symbol->is_helper_symbol = (attributes.value(QLatin1String("is_helper_symbol")) == QLatin1String("true"));
	symbol->is_hidden = (attributes.value(QLatin1String("is_hidden")) == QLatin1String("true"));
	symbol->is_protected = (attributes.value(QLatin1String("is_protected")) == QLatin1String("true"));
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == QLatin1String("description"))
			symbol->description = xml.readElementText();
		else
		{
			if (!symbol->loadImpl(xml, map, symbol_dict))
				xml.skipCurrentElement();
		}
	}
	
	if (xml.error())
	{
		delete symbol;
		throw FileFormatException(
		            ImportExport::tr("Error while loading a symbol of type %1 at line %2 column %3: %4")
		            .arg(symbol_type)
		            .arg(xml.lineNumber())
		            .arg(xml.columnNumber())
		            .arg(xml.errorString()) );
	}
	
	return symbol;
}

bool Symbol::loadFinished(Map* map)
{
	Q_UNUSED(map);
	return true;
}

void Symbol::createRenderables(const PathObject*, const PathPartVector&, ObjectRenderables&, Symbol::RenderableOptions) const
{
	qWarning("Missing implementation of Symbol::createRenderables(const PathObject*, const PathPartVector&, ObjectRenderables&)");
}

void Symbol::createBaselineRenderables(
        const PathObject*,
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
			auto line_renderable = new LineRenderable(&line_symbol, part, false);
			output.insertRenderable(line_renderable);
		}
	}
}

bool Symbol::symbolChanged(const Symbol* old_symbol, const Symbol* new_symbol)
{
	Q_UNUSED(old_symbol);
	Q_UNUSED(new_symbol);
	return false;
}

bool Symbol::containsSymbol(const Symbol* symbol) const
{
	Q_UNUSED(symbol);
	return false;
}

QImage Symbol::getIcon(const Map* map) const
{
	if (icon.isNull())
		icon = createIcon(map, Settings::getInstance().getSymbolWidgetIconSizePx(), true, 1);
	
	return icon;
}

QImage Symbol::createIcon(const Map* map, int side_length, bool antialiasing, int bottom_right_border, qreal zoom) const
{
	// Desktop default used to be 2x zoom for 8 mm side length.
	if (zoom < 0.1)
		zoom = side_length / Util::mmToPixelLogical(8) * map->symbolIconZoom();
	
	Type contained_types = getContainedTypes();
	
	// Create icon map and view
	Map icon_map;
	// const_cast promise: We won't change the colors, thus we won't change map.
	icon_map.useColorsFrom(const_cast<Map*>(map));
	icon_map.setScaleDenominator(map->getScaleDenominator());
	MapView view{ &icon_map };
	
	// If the icon is bigger than the rectangle with this zoom factor, it is zoomed out to fit into the rectangle
	view.setZoom(zoom);
	int white_border_pixels = 0;
	if (contained_types & Line || contained_types & Area || type == Combined)
		white_border_pixels = 0;
	else if (contained_types & Point || contained_types & Text)
		white_border_pixels = 2;
	else
		Q_ASSERT(false);
	const auto max_icon_mm = view.pixelToLength(side_length - bottom_right_border - white_border_pixels) / 1000;
	const auto max_icon_mm_half = max_icon_mm / 2;
	
	// Create image
	QImage image(side_length, side_length, QImage::Format_ARGB32_Premultiplied);
	QPainter painter;
	painter.begin(&image);
	if (antialiasing)
		painter.setRenderHint(QPainter::Antialiasing);
	
	// Make background transparent
	QPainter::CompositionMode mode = painter.compositionMode();
	painter.setCompositionMode(QPainter::CompositionMode_Clear);
	painter.fillRect(image.rect(), Qt::transparent);
	painter.setCompositionMode(mode);
	
	// Create geometry
	Object* object = nullptr;
	std::unique_ptr<Symbol> symbol_copy;
	auto offset = MapCoord{};
	if (type == Point)
	{
		auto point = new PointObject(static_cast<const PointSymbol*>(this));
		point->setPosition(0, 0);
		object = point;
	}
	else if (type == Area || (type == Combined && getContainedTypes() & Area))
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
			if (line->getCapStyle() == LineSymbol::RoundCap)
			{
				offset.setNativeX(-line->getLineWidth()/3);
			}
			else if (line->getCapStyle() == LineSymbol::PointedCap)
			{
				line_length_half = std::max(line_length_half, 0.0012 * line->getPointedCapLength());
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
						symbol_copy.reset(line->duplicate());
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

			// If there are breaks in the line, scale them down so they fit into the icon exactly
			// TODO: does not work for combined lines yet. Could be done by checking every contained line and scaling the painter horizontally
			if (line->isDashed() && line->getBreakLength() > 0)
			{
				if (!symbol_copy)
					symbol_copy.reset(line->duplicate());
				auto icon_line = static_cast<LineSymbol*>(symbol_copy.get());
				
				auto ideal_length_half = qreal(2 * line->getDashesInGroup() * line->getDashLength() + 2 * (line->getDashesInGroup() - 1) * line->getInGroupBreakLength() + line->getBreakLength()) / 2000;
				auto factor = qMin(qreal(0.5), line_length_half / qMax(qreal(0.001), ideal_length_half));
				
				icon_line->setDashLength(qRound(factor * icon_line->getDashLength()));
				icon_line->setBreakLength(qRound(factor * icon_line->getBreakLength()));
				icon_line->setInGroupBreakLength(qRound(factor * icon_line->getInGroupBreakLength()));
				icon_line->setShowAtLeastOneSymbol(true);
			}
		}
		else if (type == Combined)
		{
			const auto* combined = asCombined();
			for (int i = 0; i < combined->getNumParts(); ++i)
			{
				auto part = combined->getPart(i);
				if (part && part->getType() == Line && part->asLine()->getDashSymbol())
				{
					show_dash_symbol = true;
					break;
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
	else if (type == Text)
	{
		auto text = new TextObject(this);
		text->setAnchorPosition(0, 0);
		text->setHorizontalAlignment(TextObject::AlignHCenter);
		text->setVerticalAlignment(TextObject::AlignVCenter);
		text->setText(dynamic_cast<const TextSymbol*>(this)->getIconText());
		object = text;
	}
	else
	{
		qWarning("Unhandled symbol: %s", qPrintable(getDescription()));
		return image;
	}
	
	icon_map.addObject(object);
	
	const auto& extent = object->getExtent();
	if (type == Text)
	{
		// Center text
		view.setCenter(MapCoord{ extent.center() });
	}
	else if (type == Point)
	{
		// Do not completely offset the symbols relative position
		view.setCenter(MapCoord{ extent.center() / 2 });
	}
	else if (contained_types & Line && !(contained_types & Area))
	{
		view.setCenter(offset);
	}
	
	auto w = std::max(std::abs(extent.left()), std::abs(extent.right()));
	auto h = std::max(std::abs(extent.top()), std::abs(extent.bottom()));
	auto real_icon_mm_half = std::max(w, h);
	if (real_icon_mm_half > max_icon_mm_half)
	{
		view.setZoom(zoom * max_icon_mm_half / real_icon_mm_half);
	}
	
	painter.translate((side_length - bottom_right_border)/2, (side_length - bottom_right_border)/2);
	painter.setWorldTransform(view.worldTransform(), true);
	
	RenderConfig config = { *map, QRectF(-10000, -10000, 20000, 20000), view.calculateFinalZoomFactor(), RenderConfig::HelperSymbols, 1.0 };
	bool was_hidden = is_hidden;
	is_hidden = false; // ensure that an icon is created for hidden symbols.
	icon_map.draw(&painter, config);
	is_hidden = was_hidden;
	
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



QString Symbol::getNumberAsString() const
{
	QString str;
	for (auto n : number)
	{
		if (n < 0)
			break;
		if (!str.isEmpty())
			str += QLatin1Char('.');
		str += QString::number(n);
	}
	return str;
}

Symbol* Symbol::getSymbolForType(Symbol::Type type)
{
	if (type == Symbol::Point)
		return new PointSymbol();
	else if (type == Symbol::Line)
		return new LineSymbol();
	else if (type == Symbol::Area)
		return new AreaSymbol();
	else if (type == Symbol::Text)
		return new TextSymbol();
	else if (type == Symbol::Combined)
		return new CombinedSymbol();
	else
	{
		Q_ASSERT(false);
		return nullptr;
	}
}

#ifndef NO_NATIVE_FILE_FORMAT

bool Symbol::loadSymbol(Symbol*& symbol, QIODevice* stream, int version, Map* map)
{
	int save_type;
	stream->read((char*)&save_type, sizeof(int));
	symbol = Symbol::getSymbolForType(static_cast<Symbol::Type>(save_type));
	if (!symbol)
		return false;
	if (!symbol->load(stream, version, map))
		return false;
	return true;
}

#endif

bool Symbol::areTypesCompatible(Symbol::Type a, Symbol::Type b)
{
	return (getCompatibleTypes(a) & b) != 0;
}

int Symbol::getCompatibleTypes(Symbol::Type type)
{
	if (type == Point)
		return Point;
	else if (type == Line || type == Area || type == Combined)
		return Line | Area | Combined;
	else if (type == Text)
		return Text;
	
	Q_ASSERT(false);
	return type;
}

void Symbol::duplicateImplCommon(const Symbol* other)
{
	type = other->type;
	name = other->name;
	for (int i = 0; i < number_components; ++i)
		number[i] = other->number[i];
	description = other->description;
	is_helper_symbol = other->is_helper_symbol;
	is_hidden = other->is_hidden;
	icon = other->icon;
}


bool Symbol::compareByNumber(const Symbol* s1, const Symbol* s2)
{
	int n1 = s1->number_components, n2 = s2->number_components;
	for (int i = 0; i < n1 && i < n2; i++)
	{
		if (s1->getNumberComponent(i) < s2->getNumberComponent(i)) return true;  // s1 < s2
		if (s1->getNumberComponent(i) > s2->getNumberComponent(i)) return false; // s1 > s2
		// if s1 == s2, loop to the next component
	}
	return false; // s1 == s2
}

bool Symbol::compareByColorPriority(const Symbol* s1, const Symbol* s2)
{
	const MapColor* c1 = s1->guessDominantColor();
	const MapColor* c2 = s2->guessDominantColor();
	
	if (c1 && c2)
		return c1->comparePriority(*c2);
	else if (c2)
		return true;
	else if (c1)
		return false;
	
	return false; // s1 == s2
}

Symbol::compareByColor::compareByColor(Map* map)
{
	int next_priority = map->getNumColors() - 1;
	// Iterating in reverse order so identical colors are at the position where they appear with lowest priority.
	for (int i = map->getNumColors() - 1; i >= 0; --i)
	{
		QRgb color_code = QRgb(*map->getColor(i));
		if (!color_map.contains(color_code))
		{
			color_map.insert(color_code, next_priority);
			--next_priority;
		}
	}
}

bool Symbol::compareByColor::operator() (const Symbol* s1, const Symbol* s2)
{
	const MapColor* c1 = s1->guessDominantColor();
	const MapColor* c2 = s2->guessDominantColor();
	
	if (c1 && c2)
		return color_map.value(QRgb(*c1)) < color_map.value(QRgb(*c2));
	else if (c2)
		return true;
	else if (c1)
		return false;
	
	return false; // s1 == s2
}
