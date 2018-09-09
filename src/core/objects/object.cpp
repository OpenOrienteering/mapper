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


#include "object.h"

#include <cmath>

#include <QtMath>
#include <QtNumeric>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <private/qbezier_p.h>

#include "settings.h"
#include "core/map.h"
#include "core/objects/text_object.h"
#include "core/renderables/renderable.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"
#include "core/symbols/text_symbol.h"
#include "fileformats/file_format.h"
#include "fileformats/file_import_export.h"
#include "util/util.h"
#include "util/xml_stream_util.h"


// ### A namespace which collects various string constants of type QLatin1String. ###

namespace literal
{
	static const QLatin1String object("object");
	static const QLatin1String symbol("symbol");
	static const QLatin1String type("type");
	static const QLatin1String text("text");
	static const QLatin1String count("count");
	static const QLatin1String coords("coords");
	static const QLatin1String h_align("h_align");
	static const QLatin1String v_align("v_align");
	static const QLatin1String pattern("pattern");
	static const QLatin1String rotation("rotation");
	static const QLatin1String tags("tags");
}



namespace OpenOrienteering {

// ### Object implementation ###

Object::Object(Object::Type type, const Symbol* symbol)
: type(type),
  symbol(symbol),
  map(nullptr),
  output_dirty(true),
  extent(),
  output(*this)
{
	// nothing
}

Object::Object(Object::Type type, const Symbol* symbol, const MapCoordVector& coords, Map* map)
 : type(type),
   symbol(symbol),
   coords(coords),
   map(map),
   output_dirty(true),
   extent(),
   output(*this)
{
	// nothing
}

Object::Object(const Object& proto)
 : type(proto.type)
 , symbol(proto.symbol)
 , coords(proto.coords)
 , map(nullptr)
 , object_tags(proto.object_tags)
 , output_dirty(true)
 , extent(proto.extent)
 , output(*this)
{
	// nothing
}

Object::~Object()
{
	// nothing
}

void Object::copyFrom(const Object& other)
{
	if (&other == this)
		return;
	
	if (type != other.type)
		throw std::invalid_argument(Q_FUNC_INFO);
	
	symbol = other.symbol;
	coords = other.coords;
	// map unchanged!
	object_tags = other.object_tags;
	output_dirty = true;
	extent = other.extent;
}

bool Object::equals(const Object* other, bool compare_symbol) const
{
	if (type != other->type)
		return false;
	if (compare_symbol)
	{
		if (bool(symbol) != bool(other->symbol))
			return false;
		if (symbol && !symbol->equals(other->symbol))
			return false;
	}
	
	if (type == Text)
	{
		if (coords.front() != other->coords.front())
			return false;
	}
	else
	{
		if (coords.size() != other->coords.size())
			return false;
		for (size_t i = 0, end = coords.size(); i < end; ++i)
		{
			if (coords[i] != other->coords[i])
				return false;
		}
	}
	
	if (object_tags != other->object_tags)
		return false;
	
	if (type == Point)
	{
		const PointObject* point_this = static_cast<const PointObject*>(this);
		const PointObject* point_other = static_cast<const PointObject*>(other);
		
		auto rotation_a = point_this->getRotation();
		auto rotation_b = point_other->getRotation();
		if (qAbs(rotation_a - rotation_b) >= 0.000001)  // six decimal places in XML
			return false;
	}
	else if (type == Path)
	{
		const PathObject* path_this = static_cast<const PathObject*>(this);
		const PathObject* path_other = static_cast<const PathObject*>(other);
		
		auto rotation_a = path_this->getPatternRotation();
		auto rotation_b = path_other->getPatternRotation();
		if (qAbs(rotation_a - rotation_b) >= 0.000001)  // six decimal places in XML
			return false;
		
		if (path_this->getPatternOrigin() != path_other->getPatternOrigin())
			return false;
	}
	else if (type == Text)
	{
		const TextObject* text_this = static_cast<const TextObject*>(this);
		const TextObject* text_other = static_cast<const TextObject*>(other);
		
		if (text_this->getBoxSize() != text_other->getBoxSize())
			return false;
		if (text_this->getText().compare(text_other->getText(), Qt::CaseSensitive) != 0)
			return false;
		if (text_this->getHorizontalAlignment() != text_other->getHorizontalAlignment())
			return false;
		if (text_this->getVerticalAlignment() != text_other->getVerticalAlignment())
			return false;
		
		auto rotation_a = text_this->getRotation();
		auto rotation_b = text_other->getRotation();
		if (qAbs(rotation_a - rotation_b) >= 0.000001)  // six decimal places in XML
			return false;
	}
	
	return true;
}



bool Object::validate() const
{
	return true;
}



PointObject* Object::asPoint()
{
	Q_ASSERT(type == Point);
	return static_cast<PointObject*>(this);
}

const PointObject* Object::asPoint() const
{
	Q_ASSERT(type == Point);
	return static_cast<const PointObject*>(this);
}

PathObject* Object::asPath()
{
	Q_ASSERT(type == Path);
	return static_cast<PathObject*>(this);
}

const PathObject* Object::asPath() const
{
	Q_ASSERT(type == Path);
	return static_cast<const PathObject*>(this);
}

TextObject* Object::asText()
{
	Q_ASSERT(type == Text);
	return static_cast<TextObject*>(this);
}

const TextObject* Object::asText() const
{
	Q_ASSERT(type == Text);
	return static_cast<const TextObject*>(this);
}



void Object::save(QXmlStreamWriter& xml) const
{
	XmlElementWriter object_element(xml, literal::object);
	object_element.writeAttribute(literal::type, type);
	int symbol_index = -1;
	if (map)
		symbol_index = map->findSymbolIndex(symbol);
	if (symbol_index != -1)
		object_element.writeAttribute(literal::symbol, symbol_index);
	
	if (type == Point)
	{
		const PointObject* point = reinterpret_cast<const PointObject*>(this);
		const PointSymbol* point_symbol = reinterpret_cast<const PointSymbol*>(point->getSymbol());
		if (point_symbol->isRotatable())
			object_element.writeAttribute(literal::rotation, point->getRotation());
	}
	else if (type == Text)
	{
		const TextObject* text = reinterpret_cast<const TextObject*>(this);
		object_element.writeAttribute(literal::rotation, text->getRotation());
		object_element.writeAttribute(literal::h_align, text->getHorizontalAlignment());
		object_element.writeAttribute(literal::v_align, text->getVerticalAlignment());
		// For compatibility, we must keep the box size in the second coord ATM.
		/// \todo Save box size separately
		auto object = const_cast<Object*>(this);
		if (text->hasSingleAnchor())
		{
			object->coords.resize(1);
		}
		else
		{
			object->coords.resize(2);
			object->coords.back() = text->getBoxSize();
		}
	}
	
	if (!object_tags.empty())
	{
		XmlElementWriter tags_element(xml, literal::tags);
		tags_element.write(object_tags);
	}
	
	{
		// Scope of coords XML element
		XmlElementWriter coords_element(xml, literal::coords);
		coords_element.write(coords);
	}
	
	if (type == Path)
	{
		const PathObject* path = reinterpret_cast<const PathObject*>(this);
		XmlElementWriter pattern_element(xml, literal::pattern);
		pattern_element.writeAttribute(literal::rotation, path->getPatternRotation());
		path->getPatternOrigin().save(xml);
	}
	else if (type == Text)
	{
		const TextObject* text = reinterpret_cast<const TextObject*>(this);
		xml.writeTextElement(literal::text, text->getText());
	}
}

Object* Object::load(QXmlStreamReader& xml, Map* map, const SymbolDictionary& symbol_dict, const Symbol* symbol)
{
	Q_ASSERT(xml.name() == literal::object);
	
	XmlElementReader object_element(xml);
	
	Object::Type object_type = object_element.attribute<Object::Type>(literal::type);
	Object* object = Object::getObjectForType(object_type);
	if (!object)
		throw FileFormatException(::OpenOrienteering::ImportExport::tr("Error while loading an object of type %1.").arg(object_type));
	
	object->map = map;
	
	if (symbol)
		object->symbol = symbol;
	else
	{
		QString symbol_id =  object_element.attribute<QString>(literal::symbol);
		object->symbol = symbol_dict[symbol_id]; // FIXME: cannot work for forward references
		// NOTE: object->symbol may be nullptr.
	}
	
	if (!object->symbol || !object->symbol->isTypeCompatibleTo(object))
	{
		// Throwing an exception will cause loading to fail.
		// Rather than not loading the broken file at all,
		// use the same symbols which are used for importing GPS tracks etc.
		// FIXME: Implement a way to send a warning to the user.
		switch (object_type)
		{
			case Point:
				object->symbol = Map::getUndefinedPoint();
				break;
			case Path:
				object->symbol = Map::getUndefinedLine();
				break;
			case Text:
				object->symbol = Map::getUndefinedText();
				break;
			default:
				throw FileFormatException(
				  ::OpenOrienteering::ImportExport::tr("Unable to find symbol for object at %1:%2.").
				  arg(xml.lineNumber()).arg(xml.columnNumber()) );
		}
	}
	
	if (object_type == Point)
	{
		PointObject* point = reinterpret_cast<PointObject*>(object);
		const PointSymbol* point_symbol = reinterpret_cast<const PointSymbol*>(point->getSymbol());
		if (point_symbol && point_symbol->isRotatable())
			point->setRotation(object_element.attribute<qreal>(literal::rotation));
		else if (!point_symbol)
			throw FileFormatException(::OpenOrienteering::ImportExport::tr("Point object with undefined or wrong symbol at %1:%2.").arg(xml.lineNumber()).arg(xml.columnNumber()));
	}
	else if (object_type == Text)
	{
		TextObject* text = reinterpret_cast<TextObject*>(object);
		text->setRotation(object_element.attribute<qreal>(literal::rotation));
		text->setHorizontalAlignment(object_element.attribute<TextObject::HorizontalAlignment>(literal::h_align));
		text->setVerticalAlignment(object_element.attribute<TextObject::VerticalAlignment>(literal::v_align));
	}
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == literal::coords)
		{
			XmlElementReader coords_element(xml);
			try {
				if (object_type == Text)
				{
					coords_element.readForText(object->coords);
					if (object->coords.size() > 1)
						static_cast<TextObject*>(object)->setBoxSize(object->coords[1]);
				}
				else
				{
					coords_element.read(object->coords);
				}
			}
			catch (FileFormatException& e)
			{
				throw FileFormatException(::OpenOrienteering::ImportExport::tr("Error while loading an object of type %1 at %2:%3: %4").
				  arg(object_type).arg(xml.lineNumber()).arg(xml.columnNumber()).arg(e.message()));
			}
		}
		else if (xml.name() == literal::pattern && object_type == Path)
		{
			XmlElementReader element(xml);
			
			PathObject* path = reinterpret_cast<PathObject*>(object);
			path->setPatternRotation(element.attribute<qreal>(literal::rotation));
			while (xml.readNextStartElement())
			{
				if (xml.name() == XmlStreamLiteral::coord)
				{
					try
					{
						path->setPatternOrigin(MapCoord::load(xml));
					}
					catch (const std::range_error& e)
					{
						/// \todo Add a warning, but don't throw - throwing lets loading fail.
						// throw FileFormatException(::OpenOrienteering::MapCoord::tr(e.what()));
						qDebug("%s", e.what());
					}
				}
				else
				{
					xml.skipCurrentElement(); // unknown
				}
			}
		}
		else if (xml.name() == literal::text && object_type == Text)
		{
			TextObject* text = reinterpret_cast<TextObject*>(object);
			text->setText(xml.readElementText());
		}
		else if (xml.name() == literal::tags)
		{
			XmlElementReader(xml).read(object->object_tags);
		}
		else
			xml.skipCurrentElement(); // unknown
	}
	
	if (object_type == Path)
	{
		PathObject* path = reinterpret_cast<PathObject*>(object);
		path->recalculateParts();
	}
	object->output_dirty = true;
	
	if (map &&
	    ( object->coords.empty()
	      || !object->coords.front().isRegular()
	      || !object->coords.back().isRegular() ) )
	{
		map->markAsIrregular(object);
	}
	
	return object;
}

void Object::forceUpdate() const
{
	output_dirty = true;
	update();
}

bool Object::update() const
{
	if (!output_dirty)
		return false;
	
	Symbol::RenderableOptions options = Symbol::RenderNormal;
	if (map)
	{
		options = QFlag(map->renderableOptions());
		if (extent.isValid())
			map->setObjectAreaDirty(extent);
	}
	
	output.deleteRenderables();
	
	extent = QRectF();
	
	updateEvent();
	
	createRenderables(output, options);
	
	Q_ASSERT(extent.right() < 60000000);	// assert if bogus values are returned
	output_dirty = false;
	
	if (map)
	{
		map->insertRenderablesOfObject(this);
		if (extent.isValid())
			map->setObjectAreaDirty(extent);
	}
	
	return true;
}

void Object::updateEvent() const
{
	// nothing here
}

void Object::createRenderables(ObjectRenderables& output, Symbol::RenderableOptions options) const
{
	symbol->createRenderables(this, VirtualCoordVector(coords), output, options);
}

void Object::move(qint32 dx, qint32 dy)
{
	for (MapCoord& coord : coords)
	{
		coord.setNativeX(dx + coord.nativeX());
		coord.setNativeY(dy + coord.nativeY());
	}
	
	setOutputDirty();
}

void Object::move(const MapCoord& offset)
{
	for (MapCoord& coord : coords)
	{
		coord += offset;
	}
	
	setOutputDirty();
}

void Object::scale(const MapCoordF& center, double factor)
{
	for (MapCoord& coord : coords)
	{
		coord.setX(center.x() + (coord.x() - center.x()) * factor);
		coord.setY(center.y() + (coord.y() - center.y()) * factor);
	}
	
	setOutputDirty();
}

void Object::scale(double factor_x, double factor_y)
{
	for (MapCoord& coord : coords)
	{
		coord.setX(coord.x() * factor_x);
		coord.setY(coord.y() * factor_y);
	}
	
	setOutputDirty();
}

void Object::rotateAround(const MapCoordF& center, qreal angle)
{
	auto sin_angle = std::sin(angle);
	auto cos_angle = std::cos(angle);
	
	int coords_size = coords.size();
	/// \todo range-for loop
	for (int c = 0; c < coords_size; ++c)
	{
		MapCoordF center_to_coord = MapCoordF(coords[c].x() - center.x(), coords[c].y() - center.y());
		coords[c].setX(center.x() + cos_angle * center_to_coord.x() + sin_angle * center_to_coord.y());
		coords[c].setY(center.y() - sin_angle * center_to_coord.x() + cos_angle * center_to_coord.y());
	}
	
	if (type == Point)
	{
		PointObject* point = reinterpret_cast<PointObject*>(this);
		const PointSymbol* point_symbol = reinterpret_cast<const PointSymbol*>(point->getSymbol());
		if (point_symbol->isRotatable())
			point->setRotation(point->getRotation() + angle);
	}
	else if (type == Text)
	{
		TextObject* text = reinterpret_cast<TextObject*>(this);
		text->setRotation(text->getRotation() + angle);
	}
	setOutputDirty();
}

void Object::rotate(qreal angle)
{
	auto sin_angle = std::sin(angle);
	auto cos_angle = std::cos(angle);
	
	int coords_size = coords.size();
	/// \todo range-for loop
	for (int c = 0; c < coords_size; ++c)
	{
		MapCoord coord = coords[c];
		coords[c].setX(cos_angle * coord.x() + sin_angle * coord.y());
		coords[c].setY(cos_angle * coord.y() - sin_angle * coord.x());
	}
	
	if (type == Point)
	{
		PointObject* point = reinterpret_cast<PointObject*>(this);
		const PointSymbol* point_symbol = reinterpret_cast<const PointSymbol*>(point->getSymbol());
		if (point_symbol->isRotatable())
			point->setRotation(point->getRotation() + angle);
	}
	else if (type == Text)
	{
		TextObject* text = reinterpret_cast<TextObject*>(this);
		text->setRotation(text->getRotation() + angle);
	}
	setOutputDirty();
}


int Object::isPointOnObject(const MapCoordF& coord, float tolerance, bool treat_areas_as_paths, bool extended_selection) const
{
	Symbol::Type type = symbol->getType();
	auto contained_types = symbol->getContainedTypes();
	
	// Points
	if (type == Symbol::Point)
	{
		if (!extended_selection)
			return (coord.distanceSquaredTo(MapCoordF(coords[0])) <= tolerance) ? Symbol::Point : Symbol::NoSymbol;
		else
			return extent.contains(coord) ? Symbol::Point : Symbol::NoSymbol;
	}
	
	// First check using extent
	float extent_extension = ((contained_types & Symbol::Line) || treat_areas_as_paths) ? tolerance : 0;
	if (coord.x() < extent.left() - extent_extension) return Symbol::NoSymbol;
	if (coord.y() < extent.top() - extent_extension) return Symbol::NoSymbol;
	if (coord.x() > extent.right() + extent_extension) return Symbol::NoSymbol;
	if (coord.y() > extent.bottom() + extent_extension) return Symbol::NoSymbol;
	
	if (type == Symbol::Text)
	{
		// Texts
		const TextObject* text_object = reinterpret_cast<const TextObject*>(this);
		return (text_object->calcTextPositionAt(coord, true) != -1) ? Symbol::Text : Symbol::NoSymbol;
	}
	else
	{
		// Path objects
		const PathObject* path = reinterpret_cast<const PathObject*>(this);
		return path->isPointOnPath(coord, tolerance, treat_areas_as_paths, true);
	}
}

void Object::takeRenderables()
{
	output.takeRenderables();
}

void Object::clearRenderables()
{
	output.deleteRenderables();
	extent = QRectF();
}

bool Object::setSymbol(const Symbol* new_symbol, bool no_checks)
{
	if (!no_checks && new_symbol)
	{
		if (!new_symbol->isTypeCompatibleTo(this))
			return false;
	}
	
	symbol = new_symbol;
	setOutputDirty();
	return true;
}

Object* Object::getObjectForType(Object::Type type, const Symbol* symbol)
{
	if (type == Point)
		return new PointObject(symbol);
	else if (type == Path)
		return new PathObject(symbol);
	else if (type == Text)
		return new TextObject(symbol);
	else
	{
		Q_ASSERT(false);
		return nullptr;
	}
}

void Object::setTags(const Object::Tags& tags)
{
	if (object_tags != tags)
	{
		object_tags = tags;
		if (map)
		{
			map->setObjectsDirty();
			if (map->isObjectSelected(this))
				map->emitSelectionEdited();
		}
	}
}

void Object::setTag(const QString& key, const QString& value)
{
	if (!object_tags.contains(key) || object_tags.value(key) != value)
	{
		object_tags.insert(key, value);
		if (map)
		{
			map->setObjectsDirty();
			if (map->isObjectSelected(this))
				map->emitSelectionEdited();
		}
	}
}

void Object::removeTag(const QString& key)
{
	if (object_tags.contains(key))
	{
		object_tags.remove(key);
		if (map)
			map->setObjectsDirty();
	}
}


void Object::includeControlPointsRect(QRectF& rect) const
{
	if (type == Object::Path)
	{
		const PathObject* path = asPath();
		int size = path->getCoordinateCount();
		for (int i = 0; i < size; ++i)
			rectInclude(rect, QPointF(path->coords[i]));
	}
	else if (type == Object::Text)
	{
		const TextObject* text = asText();
		std::vector<QPointF> text_handles(text->controlPoints());
		for (auto& text_handle : text_handles)
			rectInclude(rect, text_handle);
	}
}



// ### PathPart ###

/* 
 * Some headers may not want to include object.h but rather rely on
 * PathPartVector::size_type being a std::size_t.
 */
static_assert(std::is_same<PathPartVector::size_type, std::size_t>::value,
              "PathCoordVector::size_type is not std::size_t");

PathPart::PathPart(PathObject& path, const VirtualPath& proto)
 : VirtualPath(path.getRawCoordinateVector(), proto.first_index, proto.last_index)
 , path(&path)
{
	if (!proto.path_coords.empty())
	{
		path_coords.reserve(proto.path_coords.size());
		path_coords.insert(end(path_coords), begin(proto.path_coords), end(proto.path_coords));
	}
}

void PathPart::setClosed(bool closed, bool may_use_existing_close_point)
{
	Q_ASSERT(!empty());
	
	if (!isClosed() && closed)
	{
		if (size() == 1 || !may_use_existing_close_point ||
		    !path->coords[first_index].isPositionEqualTo(path->coords[last_index]))
		{
			path->coords[last_index].setHolePoint(false);
			path->coords[last_index].setClosePoint(false);
			path->coords.insert(path->coords.begin() + last_index + 1, path->coords[first_index]);
			path->partSizeChanged(path->findPartForIndex(first_index), 1);
		}
		path->setClosingPoint(last_index, path->coords[first_index]);
		path->setOutputDirty();
	}
	else if (isClosed() && !closed)
	{
		path->coords[last_index].setClosePoint(false);
		path->setOutputDirty();
	}
}

void PathPart::connectEnds()
{
	if (!isClosed())
	{
		path->coords[first_index] += path->coords[last_index];
		path->coords[first_index] /= 2;
		path->setClosingPoint(last_index, path->coords[first_index]);
		path->setOutputDirty();
	}
}

void PathPart::reverse()
{
	MapCoordVector& coords = path->coords;
	
	bool set_last_hole_point = false;
	auto half = (first_index + last_index + 1) / 2;
	for (auto c = first_index; c <= last_index; ++c)
	{
		MapCoord coord = coords[c];
		if (c < half)
		{
			auto mirror_index = last_index - (c - first_index);
			coords[c] = coords[mirror_index];
			coords[mirror_index] = coord;
		}
		
		if (!(c == first_index && isClosed()) && coords[c].isCurveStart())
		{
			Q_ASSERT((c - first_index) >= 3);
			coords[c - 3].setCurveStart(true);
			if (!(c == last_index && isClosed()))
				coords[c].setCurveStart(false);
		}
		else if (coords[c].isHolePoint())
		{
			coords[c].setHolePoint(false);
			if (c >= first_index + 1)
				coords[c - 1].setHolePoint(true);
			else
				set_last_hole_point = true;
		}
	}
	
	if (coords[first_index].isClosePoint())
	{
		coords[first_index].setClosePoint(false);
		coords[last_index].setClosePoint(true);
	}
	
	if (set_last_hole_point)
		coords[last_index].setHolePoint(true);
	
	path->setOutputDirty();
}

PathPartVector PathPart::calculatePathParts(const VirtualCoordVector& coords)
{
	PathPartVector parts;
	PathCoordVector path_coords(coords);
	auto last = coords.size();
	auto part_start = MapCoordVector::size_type { 0 };
	while (part_start != last)
	{
		auto part_end = path_coords.update(part_start);
		parts.emplace_back(coords, part_start, part_end);
		parts.back().path_coords.swap(path_coords);
		part_start = part_end + 1;
	}
	return parts;
}



//### PathPartVector ###

// Not inline because it is to be used by reference.
bool PathPartVector::compareEndIndex(const PathPart& part, VirtualPath::size_type index)
{
	return part.last_index+1 <= index;
}



// ### PathObject ###

PathObject::PathObject(const Symbol* symbol)
 : Object(Object::Path, symbol)
 , pattern_rotation(0.0)
 , pattern_origin(0, 0)
{
	Q_ASSERT(!symbol || (symbol->getType() == Symbol::Line || symbol->getType() == Symbol::Area || symbol->getType() == Symbol::Combined));
}

PathObject::PathObject(const Symbol* symbol, const MapCoordVector& coords, Map* map)
 : Object(Object::Path, symbol, coords, map)
 , pattern_rotation(0.0)
 , pattern_origin(0, 0)
{
	Q_ASSERT(!symbol || (symbol->getType() == Symbol::Line || symbol->getType() == Symbol::Area || symbol->getType() == Symbol::Combined));
	recalculateParts();
}

PathObject::PathObject(const Symbol* symbol, const PathObject& proto, MapCoordVector::size_type piece)
 : Object { Object::Path, symbol }
 , pattern_rotation { 0.0f }
 , pattern_origin { 0, 0 }
{
	auto begin = proto.coords.begin() + piece;
	auto part  = proto.findPartForIndex(piece);
	if (piece == part->last_index)
	{
		if (part->isClosed())
			begin = proto.coords.begin() + part->first_index;
		else
			begin = proto.coords.begin() + part->prevCoordIndex(piece);
	}
	
	auto end = begin + (begin->isCurveStart() ? 4 : 2);
	
	std::for_each(begin, end, [this](MapCoord c)
	{
		c.setHolePoint(false);
		c.setClosePoint(false);
		addCoordinate(c);
	});
	
	coords.back().setHolePoint(true);
}

PathObject::PathObject(const PathObject& proto)
 : Object(proto)
 , pattern_rotation(proto.pattern_rotation)
 , pattern_origin(proto.pattern_origin)
{
	path_parts.reserve(proto.path_parts.size());
	for (const PathPart& part : proto.path_parts)
	{
		path_parts.emplace_back(*this, part);
	}
}

PathObject::PathObject(const PathPart &proto_part)
 : Object(*proto_part.path)
 , pattern_rotation(proto_part.path->pattern_rotation)
 , pattern_origin(proto_part.path->pattern_origin)
{
	auto begin = proto_part.path->coords.begin();
	coords.reserve(proto_part.size());
	coords.assign(begin + proto_part.first_index, begin + (proto_part.last_index+1));
	path_parts.emplace_back(*this, proto_part);
}
   
PathObject* PathObject::duplicate() const
{
	return new PathObject(*this);
}

void PathObject::copyFrom(const Object& other)
{
	if (&other == this)
		return;
	
	Object::copyFrom(other);
	
	const PathObject& other_path = *other.asPath();
	pattern_rotation = other_path.getPatternRotation();
	pattern_origin = other_path.getPatternOrigin();
	
	path_parts.clear();
	path_parts.reserve(other_path.path_parts.size());
	for (const PathPart& part : other_path.path_parts)
	{
		path_parts.emplace_back(*this, part);
	}
}



bool PathObject::validate() const
{
	return std::all_of(begin(path_parts), end(path_parts), [](auto &part) {
		return !part.isClosed()
		       || part.coords[part.first_index] == part.coords[part.last_index];
	});
}



void PathObject::normalize()
{
	for (MapCoordVector::size_type i = 0; i < coords.size(); ++i)
	{
		if (coords[i].isCurveStart())
		{
			if (i+3 >= getCoordinateCount())
			{
				coords[i].setCurveStart(false);
				continue;
			}
			
			if (coords[i + 1].isClosePoint() || coords[i + 1].isHolePoint() ||
			    coords[i + 2].isClosePoint() || coords[i + 2].isHolePoint())
			{
				coords[i].setCurveStart(false);
				continue;
			}
			
			coords[i + 1].setCurveStart(false);
			coords[i + 1].setDashPoint(false);
			coords[i + 2].setCurveStart(false);
			coords[i + 2].setDashPoint(false);
			i += 2;
		}
		
		if (i > 0 && coords[i].isHolePoint())
		{
			if (coords[i-1].isHolePoint())
				deleteCoordinate(i, false);
		}
	}
}

bool PathObject::intersectsBox(const QRectF& box) const
{
	// Check path parts for an intersection with box
	if (std::any_of(begin(path_parts), end(path_parts), [&box](const PathPart& part) { return part.intersectsBox(box); }))
	{
		return true;
	};
	
	// If this is an area, additionally check if the area contains the box
	if (getSymbol()->getContainedTypes() & Symbol::Area)
	{
		return isPointOnObject(MapCoordF(box.center()), 0, false, false);
	}
	
	return false;
}

PathPartVector::const_iterator PathObject::findPartForIndex(MapCoordVector::size_type coords_index) const
{
	return std::lower_bound(begin(path_parts), end(path_parts), coords_index, PathPartVector::compareEndIndex);
}

PathPartVector::iterator PathObject::findPartForIndex(MapCoordVector::size_type coords_index)
{
	setOutputDirty();
	return std::lower_bound(begin(path_parts), end(path_parts), coords_index, PathPartVector::compareEndIndex);
}

PathPartVector::size_type PathObject::findPartIndexForIndex(MapCoordVector::size_type coords_index) const
{
	for (PathPartVector::size_type i = 0; i < path_parts.size(); ++i)
	{
		if (path_parts[i].first_index <= coords_index && path_parts[i].last_index >= coords_index)
			return i;
	}
	Q_ASSERT(false);
	return 0;
}

PathCoord PathObject::findPathCoordForIndex(MapCoordVector::size_type index) const
{
	auto part = findPartForIndex(index);
	if (part != end(path_parts))
	{
		return *(std::lower_bound(begin(part->path_coords), end(part->path_coords), index, PathCoord::indexLessThanValue));
	}
	return path_parts.front().path_coords.front();
}

bool PathObject::isCurveHandle(MapCoordVector::size_type index) const
{
	return ( index < coords.size() &&
	         !coords[index].isCurveStart() &&
	         (
	           ( index > 0 && coords[index-1].isCurveStart() ) ||
	           ( index > 1 && coords[index-2].isCurveStart() )
	         )
	       );
}

void PathObject::deletePart(PathPartVector::size_type part_index)
{
	setOutputDirty();
	
	auto part = begin(path_parts) + part_index;
	auto first_index = part->first_index;
	auto end_index   = part->last_index + 1;
	auto first_coord = begin(coords);
	coords.erase(first_coord + first_index, first_coord + end_index);
	partSizeChanged(part, first_index - end_index);
	path_parts.erase(part);
}

void PathObject::partSizeChanged(PathPartVector::iterator part, MapCoordVector::difference_type change)
{
	Q_ASSERT(isOutputDirty());
	
	part->last_index += change;
	auto last = end(path_parts);
	for (++part; part != last; ++part)
	{
		part->first_index += change;
		part->last_index += change;
	}
}


void PathObject::transform(const QTransform& t)
{
	if (t.isIdentity())
		return;
	
	for (auto& coord : coords)
	{
		const auto p = t.map(MapCoordF{coord});
		coord.setX(p.x());
		coord.setY(p.y());
	}
	pattern_origin = MapCoord{t.map(MapCoordF{getPatternOrigin()})};
	setOutputDirty();
}



void PathObject::setPatternRotation(qreal rotation)
{
	pattern_rotation = rotation;
	setOutputDirty();
}

void PathObject::setPatternOrigin(const MapCoord& origin)
{
	pattern_origin = origin;
	setOutputDirty();
}

void PathObject::calcClosestPointOnPath(
        MapCoordF coord,
        float& out_distance_sq,
        PathCoord& out_path_coord,
        MapCoordVector::size_type start_index,
        MapCoordVector::size_type end_index) const
{
	update();
	
	auto bound = std::numeric_limits<float>::max();
	for (const auto& part : path_parts)
	{
		if (part.first_index <= end_index && part.last_index >= start_index) /// \todo Legacy compatibility, review/remove
		{
			auto path_coord = part.findClosestPointTo(coord, out_distance_sq, bound, start_index, end_index);
			if (out_distance_sq < bound)
			{
				bound = out_distance_sq;
				out_path_coord = path_coord;
			}
		}
	}
}

void PathObject::calcClosestCoordinate(MapCoordF coord, float& out_distance_sq, MapCoordVector::size_type& out_index) const
{
	update();
	
	auto coords_size = coords.size();
	if (coords_size == 0)
	{
		out_distance_sq = -1;
		out_index = -1;
		return;	
	}
	
	// NOTE: do not try to optimize this by starting with index 1, it will overlook curve starts this way
	out_distance_sq = 999999;
	out_index = 0;
	for (MapCoordVector::size_type i = 0; i < coords_size; ++i)
	{
		double length_sq = (coord - MapCoordF(coords[i])).lengthSquared();
		if (length_sq < out_distance_sq)
		{
			out_distance_sq = length_sq;
			out_index = i;
		}
		
		if (coords[i].isCurveStart())
			i += 2;
	}
}

MapCoordVector::size_type PathObject::subdivide(const PathCoord& path_coord)
{
	return subdivide(path_coord.index, path_coord.param);
}

MapCoordVector::size_type PathObject::subdivide(MapCoordVector::size_type index, float param)
{
	Q_ASSERT(index < coords.size());
	
	if (coords[index].isCurveStart())
	{
		MapCoordF o0, o1, o2, o3, o4;
		PathCoord::splitBezierCurve(MapCoordF(coords[index]), MapCoordF(coords[index+1]),
									MapCoordF(coords[index+2]), MapCoordF(coords[index+3]),
									param, o0, o1, o2, o3, o4);
		coords[index + 1] = MapCoord(o0);
		coords[index + 2] = MapCoord(o4);
		addCoordinate(index + 2, MapCoord(o3));
		MapCoord middle_coord = MapCoord(o2);
		middle_coord.setCurveStart(true);
		addCoordinate(index + 2, middle_coord);
		addCoordinate(index + 2, MapCoord(o1));
		Q_ASSERT(isOutputDirty());
		return index + 3;
	}
	else
	{
		addCoordinate(index + 1, MapCoord(MapCoordF(coords[index]) + (MapCoordF(coords[index+1]) - MapCoordF(coords[index])) * param));
		Q_ASSERT(isOutputDirty());
		return index + 1;
	}
}

bool PathObject::canBeConnected(const PathObject* other, double connect_threshold_sq) const
{
	for (const auto& part : path_parts)
	{
		if (part.isClosed())
			continue;
		
		for (const auto& other_part : other->path_parts)
		{
			if (other_part.isClosed())
				continue;
			
			if (coords[part.first_index].distanceSquaredTo(other->coords[other_part.first_index]) <= connect_threshold_sq)
				return true;
			else if (coords[part.first_index].distanceSquaredTo(other->coords[other_part.last_index]) <= connect_threshold_sq)
				return true;
			else if (coords[part.last_index].distanceSquaredTo(other->coords[other_part.first_index]) <= connect_threshold_sq)
				return true;
			else if (coords[part.last_index].distanceSquaredTo(other->coords[other_part.last_index]) <= connect_threshold_sq)
				return true;
		}
	}
	
	return false;
}

bool PathObject::connectIfClose(PathObject* other, double connect_threshold_sq)
{
	bool did_connect_path = false;
	
	auto num_parts = parts().size();
	auto num_other_parts = other->parts().size();
	std::vector<bool> other_parts;	// Which parts have not been connected to this part yet?
	other_parts.assign(num_other_parts, true);
	for (std::size_t i = 0; i < num_parts; ++i)
	{
		if (path_parts[i].isClosed())
			continue;
		
		for (std::size_t k = 0; k < num_other_parts; ++k)
		{
			if (!other_parts[k] || other->path_parts[k].isClosed())
				continue;
	
			if (coords[path_parts[i].first_index].distanceSquaredTo(other->coords[other->path_parts[k].first_index]) <= connect_threshold_sq)
			{
				other->path_parts[k].reverse();
				connectPathParts(i, other, k, true);
			}
			else if (coords[path_parts[i].first_index].distanceSquaredTo(other->coords[other->path_parts[k].last_index]) <= connect_threshold_sq)
				connectPathParts(i, other, k, true);
			else if (coords[path_parts[i].last_index].distanceSquaredTo(other->coords[other->path_parts[k].first_index]) <= connect_threshold_sq)
				connectPathParts(i, other, k, false);
			else if (coords[path_parts[i].last_index].distanceSquaredTo(other->coords[other->path_parts[k].last_index]) <= connect_threshold_sq)
			{
				other->path_parts[k].reverse();
				connectPathParts(i, other, k, false);
			}
			else
				continue;
			
			if (coords[path_parts[i].first_index].distanceSquaredTo(coords[path_parts[i].last_index]) <= connect_threshold_sq)
				path_parts[i].connectEnds();
			
			did_connect_path = true;
			
			other_parts[k] = false;
		}
	}
	
	if (did_connect_path)
	{
		// Copy over all remaining parts of the other object
		getCoordinateRef(getCoordinateCount() - 1).setHolePoint(true);
		for (std::size_t i = 0; i < num_other_parts; ++i)
		{
			if (other_parts[i])
				appendPathPart(other->parts()[i]);
		}
		
		Q_ASSERT(isOutputDirty());
	}
	
	return did_connect_path;
}

void PathObject::connectPathParts(PathPartVector::size_type part_index, const PathObject* other, PathPartVector::size_type other_part_index, bool prepend, bool merge_ends)
{
	Q_ASSERT(part_index < path_parts.size());
	PathPart& part = path_parts[part_index];
	PathPart& other_part = other->path_parts[other_part_index];
	Q_ASSERT(!part.isClosed() && !other_part.isClosed());
	
	int other_part_size = other_part.size();
	int appended_part_size = other_part_size - (merge_ends ? 1 : 0);
	coords.resize(coords.size() + appended_part_size);
	
	if (prepend)
	{
		for (auto i = coords.size() - 1; i >= part.first_index + appended_part_size; --i)
			coords[i] = coords[i - appended_part_size];
		
		MapCoord& join_coord = coords[part.first_index + appended_part_size];
		if (merge_ends)
		{
			join_coord.setNativeX((join_coord.nativeX() + other->coords[other_part.last_index].nativeX()) / 2);
			join_coord.setNativeY((join_coord.nativeY() + other->coords[other_part.last_index].nativeY()) / 2);
		}
		join_coord.setHolePoint(false);
		join_coord.setClosePoint(false);
		
		for (auto i = part.first_index; i < part.first_index + appended_part_size; ++i)
			coords[i] = other->coords[i - part.first_index + other_part.first_index];
		
		if (!merge_ends)
		{
			MapCoord& second_join_coord = coords[part.first_index + appended_part_size - 1];
			second_join_coord.setHolePoint(false);
			second_join_coord.setClosePoint(false);
		}
	}
	else
	{
		auto end_index = part.last_index;
		
		if (merge_ends)
		{
			MapCoord coord = other->coords[other_part.first_index];	// take flags from first coord of path to append
			coord.setNativeX((coords[end_index].nativeX() + coord.nativeX()) / 2);
			coord.setNativeY((coords[end_index].nativeY() + coord.nativeY()) / 2);
			coords[end_index] = coord;
		}
		else
		{
			coords[end_index].setHolePoint(false);
			coords[end_index].setClosePoint(false);
		}
		
		for (auto i = coords.size() - 1; i > part.last_index + appended_part_size; --i)
			coords[i] = coords[i - appended_part_size];
		for (auto i = part.last_index+1; i < part.last_index + 1 + appended_part_size; ++i)
			coords[i] = other->coords[i - end_index + other_part.first_index - (merge_ends ? 0 : 1)];
	}
	
	setOutputDirty();
	partSizeChanged(begin(path_parts)+part_index, appended_part_size);
	Q_ASSERT(!part.isClosed());
}

std::vector<PathObject*> PathObject::removeFromLine(PathPartVector::size_type part_index, qreal begin, qreal end_index) const
{
	Q_ASSERT(path_parts.size() == 1); // TODO
	Q_ASSERT(symbol->getContainedTypes() == Symbol::Line);
	
	const PathPart& part = path_parts[part_index];
	Q_ASSERT(part.path_coords.front().clen <= begin);
	Q_ASSERT(part.path_coords.front().clen <= end_index);
	Q_ASSERT(part.path_coords.back().clen >= begin);
	Q_ASSERT(part.path_coords.back().clen >= end_index);
	
	std::vector<PathObject*> objects;
	
	if (end_index < begin || part.isClosed())
	{
		PathObject* obj = duplicate()->asPath();
		obj->changePathBounds(part_index, end_index, begin);
		objects.push_back(obj);
	}
	else
	{
		if (begin > part.path_coords.front().clen)
		{
			PathObject* obj = duplicate()->asPath();
			obj->changePathBounds(part_index, part.path_coords.front().clen, begin);
			objects.push_back(obj);
		}
		if (end_index < part.path_coords.back().clen)
		{
			PathObject* obj = duplicate()->asPath();
			obj->changePathBounds(part_index, end_index, part.path_coords.front().clen);
			objects.push_back(obj);
		}
	}
	
	return objects;
}

std::vector<PathObject*> PathObject::splitLineAt(const PathCoord& split_pos) const
{
	Q_ASSERT(path_parts.size() == 1);
	Q_ASSERT((symbol->getContainedTypes() & ~Symbol::Combined) == Symbol::Line);
	
	std::vector<PathObject*> objects;
	if (path_parts.size() == 1)
	{
		const auto  part_index = PathPartVector::size_type { 0 };
		const auto& part = path_parts[part_index];
		if (part.isClosed())
		{
			PathObject* obj = duplicate()->asPath();
			if ( split_pos.clen == part.path_coords.front().clen || 
			     split_pos.clen == part.path_coords.back().clen )
			{
				(obj->path_parts[part_index]).setClosed(false);
			}
			else
			{
				obj->changePathBounds(part_index, split_pos.clen, split_pos.clen);
			}
			objects.push_back(obj);
		}
		else if ( split_pos.clen != part.path_coords.front().clen &&
		          split_pos.clen != part.path_coords.back().clen)
		{
			PathObject* obj1 = duplicate()->asPath();
			obj1->changePathBounds(part_index, part.path_coords.front().clen, split_pos.clen);
			objects.push_back(obj1);
			
			PathObject* obj2 = duplicate()->asPath();
			obj2->changePathBounds(part_index, split_pos.clen, part.path_coords.back().clen);
			objects.push_back(obj2);
		}
	}
		
	return objects;
}

void PathObject::changePathBounds(
        PathPartVector::size_type part_index,
        PathCoord::length_type start_len,
        PathCoord::length_type end_len)
{
	update();
	
	PathPart& part = path_parts[part_index];
	auto part_size = part.size();

	MapCoordVector out_coords;
	out_coords.reserve(part_size + 2);
	
	if (end_len == 0.0)
		end_len = part.path_coords.back().clen;
	
	auto part_begin = SplitPathCoord::begin(part.path_coords);
	auto part_end   = SplitPathCoord::end(part.path_coords);
	auto start      = SplitPathCoord::at(start_len, part_begin);
	
	if (end_len <= start_len)
	{
		// Make sure part_end has the right curve end points for start.
		part_end = SplitPathCoord::at(part_end.clen, start);
		part.copy(start, part_end, out_coords);
		out_coords.back().setHolePoint(false);
		out_coords.back().setClosePoint(false);
		
		auto end = SplitPathCoord::at(end_len, part_begin);
		part.copy(part_begin, end, out_coords);
	}
	else
	{
		auto end = SplitPathCoord::at(end_len, start);
		part.copy(start, end, out_coords);
	}
	
	out_coords.back().setHolePoint(true);
	out_coords.back().setClosePoint(false);
	
	const auto copy_size  = qMin(out_coords.size(), (MapCoordVector::size_type)part_size);
	const auto part_start = begin(coords) + part.first_index;
	std::copy(begin(out_coords), begin(out_coords) + copy_size, part_start);
	if (copy_size < part_size)
		coords.erase(part_start + copy_size, part_start + part_size);
	else
		coords.insert(part_start + part_size, begin(out_coords) + copy_size, end(out_coords));
	
	recalculateParts();
	setOutputDirty();
}

void PathObject::calcBezierPointDeletionRetainingShapeFactors(MapCoord p0, MapCoord p1, MapCoord p2, MapCoord q0, MapCoord q1, MapCoord q2, MapCoord q3, double& out_pfactor, double& out_qfactor)
{
	// Heuristic for the split parameter sp (zero to one)
	QBezier p_curve = QBezier::fromPoints(QPointF(p0), QPointF(p1), QPointF(p2), QPointF(q0));
	double p_length = p_curve.length(PathCoord::bezierError());
	QBezier q_curve = QBezier::fromPoints(QPointF(q0), QPointF(q1), QPointF(q2), QPointF(q3));
	double q_length = q_curve.length(PathCoord::bezierError());
	double sp = p_length / qMax(1e-08, p_length + q_length);
	
	// Least squares curve fitting with the constraint that handles are on the same line as before.
	// To reproduce the formulas, run the following Matlab script:
	/*
	 
	 clear all; close all; clc;
	 
	 syms P0x P1x P2x P3x;
	 syms Q1x Q2x Q3x;
	 syms P0y P1y P2y P3y;
	 syms Q1y Q2y Q3y;
	 
	 syms u v w;
	 syms pfactor qfactor;
	 
	 Xx = (1-u)^3*P0x+3*(1-u)^2*u*P1x+3*(1-u)*u^2*P2x+u^3*P3x;
	 Yx = (1-v)^3*P3x+3*(1-v)^2*v*Q1x+3*(1-v)*v^2*Q2x+v^3*Q3x;
	 Zx = (1-w)^3*P0x+3*(1-w)^2*w*(P0x+pfactor*(P1x-P0x))+3*(1-w)*w^2*(Q3x+qfactor*(Q2x-Q3x))+w^3*Q3x;
	 
	 Xy = (1-u)^3*P0y+3*(1-u)^2*u*P1y+3*(1-u)*u^2*P2y+u^3*P3y;
	 Yy = (1-v)^3*P3y+3*(1-v)^2*v*Q1y+3*(1-v)*v^2*Q2y+v^3*Q3y;
	 Zy = (1-w)^3*P0y+3*(1-w)^2*w*(P0y+pfactor*(P1y-P0y))+3*(1-w)*w^2*(Q3y+qfactor*(Q2y-Q3y))+w^3*Q3y;
	 
	 syms sp;   %split_param
	 E = int((Zx - subs(Xx, u, w/sp))^2 + (Zy - subs(Xy, u, w/sp))^2, w, 0, sp) + int((Zx - subs(Yx, v, (w - sp) / (1 - sp)))^2 + (Zy - subs(Yy, v, (w - sp) / (1 - sp)))^2, w, sp, 1);
	 
	 Epfactor = diff(E, pfactor);
	 Eqfactor = diff(E, qfactor);
	 S = solve(Epfactor, Eqfactor, pfactor, qfactor);
	 S = [S.pfactor, S.qfactor]
	 
	 */
	
	double P0x = p0.x(), P1x = p1.x(), P2x = p2.x(), P3x = q0.x();
	double Q1x = q1.x(), Q2x = q2.x(), Q3x = q3.x();
	double P0y = p0.y(), P1y = p1.y(), P2y = p2.y(), P3y = q0.y();
	double Q1y = q1.y(), Q2y = q2.y(), Q3y = q3.y();
	
	out_pfactor = -(126*P0x*pow(Q2x,3.0)*pow(sp,3.0) - 49*pow(P0x,2.0)*pow(Q3x,2.0) - 88*pow(P0x,2.0)*pow(Q2y,2.0) - 88*pow(P0y,2.0)*pow(Q2x,2.0) - 88*pow(P0x,2.0)*pow(Q3y,2.0) - 88*pow(P0y,2.0)*pow(Q3x,2.0) - 49*pow(P0y,2.0)*pow(Q2y,2.0) -
	49*pow(P0y,2.0)*pow(Q3y,2.0) - 49*pow(P0x,2.0)*pow(Q2x,2.0) + 21*P0x*pow(Q3x,3.0)*pow(sp,2.0) - 84*P0x*pow(Q2x,3.0)*pow(sp,4.0) + 14*P0x*pow(Q3x,3.0)*pow(sp,3.0) - 126*P1x*pow(Q2x,3.0)*pow(sp,3.0) -
	21*P1x*pow(Q3x,3.0)*pow(sp,2.0) - 21*P0x*pow(Q3x,3.0)*pow(sp,4.0) + 84*P1x*pow(Q2x,3.0)*pow(sp,4.0) - 14*P1x*pow(Q3x,3.0)*pow(sp,3.0) + 21*P1x*pow(Q3x,3.0)*pow(sp,4.0) + 126*P0y*pow(Q2y,3.0)*pow(sp,3.0) +
	21*P0y*pow(Q3y,3.0)*pow(sp,2.0) - 84*P0y*pow(Q2y,3.0)*pow(sp,4.0) + 14*P0y*pow(Q3y,3.0)*pow(sp,3.0) - 126*P1y*pow(Q2y,3.0)*pow(sp,3.0) - 21*P1y*pow(Q3y,3.0)*pow(sp,2.0) - 21*P0y*pow(Q3y,3.0)*pow(sp,4.0) +
	84*P1y*pow(Q2y,3.0)*pow(sp,4.0) - 14*P1y*pow(Q3y,3.0)*pow(sp,3.0) + 21*P1y*pow(Q3y,3.0)*pow(sp,4.0) + 84*pow(P0x,2.0)*pow(Q2x,2.0)*pow(sp,2.0) - 77*pow(P0x,2.0)*pow(Q2x,2.0)*pow(sp,3.0) +
	84*pow(P0x,2.0)*pow(Q3x,2.0)*pow(sp,2.0) - 168*pow(P1x,2.0)*pow(Q2x,2.0)*pow(sp,2.0) + 21*pow(P0x,2.0)*pow(Q2x,2.0)*pow(sp,4.0) - 77*pow(P0x,2.0)*pow(Q3x,2.0)*pow(sp,3.0) + 231*pow(P1x,2.0)*pow(Q2x,2.0)*pow(sp,3.0) -
	168*pow(P1x,2.0)*pow(Q3x,2.0)*pow(sp,2.0) + 21*pow(P0x,2.0)*pow(Q3x,2.0)*pow(sp,4.0) - 84*pow(P1x,2.0)*pow(Q2x,2.0)*pow(sp,4.0) + 231*pow(P1x,2.0)*pow(Q3x,2.0)*pow(sp,3.0) - 84*pow(P1x,2.0)*pow(Q3x,2.0)*pow(sp,4.0) +
	84*pow(P0x,2.0)*pow(Q2y,2.0)*pow(sp,2.0) + 84*pow(P0y,2.0)*pow(Q2x,2.0)*pow(sp,2.0) - 56*pow(P0x,2.0)*pow(Q2y,2.0)*pow(sp,3.0) + 84*pow(P0x,2.0)*pow(Q3y,2.0)*pow(sp,2.0) - 168*pow(P1x,2.0)*pow(Q2y,2.0)*pow(sp,2.0) -
	56*pow(P0y,2.0)*pow(Q2x,2.0)*pow(sp,3.0) + 84*pow(P0y,2.0)*pow(Q3x,2.0)*pow(sp,2.0) - 168*pow(P1y,2.0)*pow(Q2x,2.0)*pow(sp,2.0) + 12*pow(P0x,2.0)*pow(Q2y,2.0)*pow(sp,4.0) - 56*pow(P0x,2.0)*pow(Q3y,2.0)*pow(sp,3.0) +
	168*pow(P1x,2.0)*pow(Q2y,2.0)*pow(sp,3.0) - 168*pow(P1x,2.0)*pow(Q3y,2.0)*pow(sp,2.0) + 12*pow(P0y,2.0)*pow(Q2x,2.0)*pow(sp,4.0) - 56*pow(P0y,2.0)*pow(Q3x,2.0)*pow(sp,3.0) + 168*pow(P1y,2.0)*pow(Q2x,2.0)*pow(sp,3.0) -
	168*pow(P1y,2.0)*pow(Q3x,2.0)*pow(sp,2.0) + 12*pow(P0x,2.0)*pow(Q3y,2.0)*pow(sp,4.0) - 48*pow(P1x,2.0)*pow(Q2y,2.0)*pow(sp,4.0) + 168*pow(P1x,2.0)*pow(Q3y,2.0)*pow(sp,3.0) + 12*pow(P0y,2.0)*pow(Q3x,2.0)*pow(sp,4.0) -
	48*pow(P1y,2.0)*pow(Q2x,2.0)*pow(sp,4.0) + 168*pow(P1y,2.0)*pow(Q3x,2.0)*pow(sp,3.0) - 48*pow(P1x,2.0)*pow(Q3y,2.0)*pow(sp,4.0) - 48*pow(P1y,2.0)*pow(Q3x,2.0)*pow(sp,4.0) + 84*pow(P0y,2.0)*pow(Q2y,2.0)*pow(sp,2.0) -
	77*pow(P0y,2.0)*pow(Q2y,2.0)*pow(sp,3.0) + 84*pow(P0y,2.0)*pow(Q3y,2.0)*pow(sp,2.0) - 168*pow(P1y,2.0)*pow(Q2y,2.0)*pow(sp,2.0) + 21*pow(P0y,2.0)*pow(Q2y,2.0)*pow(sp,4.0) - 77*pow(P0y,2.0)*pow(Q3y,2.0)*pow(sp,3.0) +
	231*pow(P1y,2.0)*pow(Q2y,2.0)*pow(sp,3.0) - 168*pow(P1y,2.0)*pow(Q3y,2.0)*pow(sp,2.0) + 21*pow(P0y,2.0)*pow(Q3y,2.0)*pow(sp,4.0) - 84*pow(P1y,2.0)*pow(Q2y,2.0)*pow(sp,4.0) + 231*pow(P1y,2.0)*pow(Q3y,2.0)*pow(sp,3.0) -
	84*pow(P1y,2.0)*pow(Q3y,2.0)*pow(sp,4.0) + 49*P0x*P1x*pow(Q2x,2.0) + 49*P0x*P1x*pow(Q3x,2.0) + 28*P0x*P3x*pow(Q2x,2.0) + 28*P0x*P3x*pow(Q3x,2.0) - 28*P1x*P3x*pow(Q2x,2.0) - 28*P1x*P3x*pow(Q3x,2.0) + 88*P0x*P1x*pow(Q2y,2.0) +
	88*P0x*P1x*pow(Q3y,2.0) + 40*P0x*P3x*pow(Q2y,2.0) + 40*P0x*P3x*pow(Q3y,2.0) - 40*P1x*P3x*pow(Q2y,2.0) - 40*P1x*P3x*pow(Q3y,2.0) + 88*P0y*P1y*pow(Q2x,2.0) + 88*P0y*P1y*pow(Q3x,2.0) + 40*P0y*P3y*pow(Q2x,2.0) +
	40*P0y*P3y*pow(Q3x,2.0) - 40*P1y*P3y*pow(Q2x,2.0) - 40*P1y*P3y*pow(Q3x,2.0) + 49*P0y*P1y*pow(Q2y,2.0) + 49*P0y*P1y*pow(Q3y,2.0) + 28*P0y*P3y*pow(Q2y,2.0) + 28*P0y*P3y*pow(Q3y,2.0) - 28*P1y*P3y*pow(Q2y,2.0) -
	28*P1y*P3y*pow(Q3y,2.0) + 21*P0x*Q1x*pow(Q2x,2.0) + 21*P0x*Q1x*pow(Q3x,2.0) - 21*P1x*Q1x*pow(Q2x,2.0) - 21*P1x*Q1x*pow(Q3x,2.0) + 98*pow(P0x,2.0)*Q2x*Q3x + 48*P0x*Q1x*pow(Q2y,2.0) + 48*P0x*Q1x*pow(Q3y,2.0) -
	48*P1x*Q1x*pow(Q2y,2.0) - 48*P1x*Q1x*pow(Q3y,2.0) + 176*pow(P0y,2.0)*Q2x*Q3x + 48*P0y*pow(Q2x,2.0)*Q1y + 48*P0y*pow(Q3x,2.0)*Q1y - 48*P1y*pow(Q2x,2.0)*Q1y - 48*P1y*pow(Q3x,2.0)*Q1y + 176*pow(P0x,2.0)*Q2y*Q3y +
	21*P0y*Q1y*pow(Q2y,2.0) + 21*P0y*Q1y*pow(Q3y,2.0) - 21*P1y*Q1y*pow(Q2y,2.0) - 21*P1y*Q1y*pow(Q3y,2.0) + 98*pow(P0y,2.0)*Q2y*Q3y - 42*P0x*pow(Q2x,3.0)*sp + 42*P1x*pow(Q2x,3.0)*sp - 42*P0y*pow(Q2y,3.0)*sp +
	42*P1y*pow(Q2y,3.0)*sp + 84*P0x*P3x*pow(Q2x,2.0)*sp + 84*P0x*P3x*pow(Q3x,2.0)*sp - 84*P1x*P3x*pow(Q2x,2.0)*sp - 84*P1x*P3x*pow(Q3x,2.0)*sp + 120*P0x*P3x*pow(Q2y,2.0)*sp + 120*P0x*P3x*pow(Q3y,2.0)*sp -
	120*P1x*P3x*pow(Q2y,2.0)*sp - 120*P1x*P3x*pow(Q3y,2.0)*sp + 120*P0y*P3y*pow(Q2x,2.0)*sp + 120*P0y*P3y*pow(Q3x,2.0)*sp - 120*P1y*P3y*pow(Q2x,2.0)*sp - 120*P1y*P3y*pow(Q3x,2.0)*sp + 84*P0y*P3y*pow(Q2y,2.0)*sp +
	84*P0y*P3y*pow(Q3y,2.0)*sp - 84*P1y*P3y*pow(Q2y,2.0)*sp - 84*P1y*P3y*pow(Q3y,2.0)*sp - 42*P0x*Q1x*pow(Q2x,2.0)*sp - 42*P0x*Q1x*pow(Q3x,2.0)*sp + 42*P1x*Q1x*pow(Q2x,2.0)*sp - 42*P0x*Q2x*pow(Q3x,2.0)*sp +
	84*P0x*pow(Q2x,2.0)*Q3x*sp + 42*P1x*Q1x*pow(Q3x,2.0)*sp + 42*P1x*Q2x*pow(Q3x,2.0)*sp - 84*P1x*pow(Q2x,2.0)*Q3x*sp - 24*P0x*Q1x*pow(Q2y,2.0)*sp - 24*P0x*Q1x*pow(Q3y,2.0)*sp - 42*P0x*Q2x*pow(Q2y,2.0)*sp +
	24*P1x*Q1x*pow(Q2y,2.0)*sp - 96*P0x*Q2x*pow(Q3y,2.0)*sp - 54*P0x*Q3x*pow(Q2y,2.0)*sp + 24*P1x*Q1x*pow(Q3y,2.0)*sp + 42*P1x*Q2x*pow(Q2y,2.0)*sp + 96*P1x*Q2x*pow(Q3y,2.0)*sp + 54*P1x*Q3x*pow(Q2y,2.0)*sp -
	24*P0y*pow(Q2x,2.0)*Q1y*sp - 42*P0y*pow(Q2x,2.0)*Q2y*sp - 24*P0y*pow(Q3x,2.0)*Q1y*sp + 24*P1y*pow(Q2x,2.0)*Q1y*sp - 54*P0y*pow(Q2x,2.0)*Q3y*sp - 96*P0y*pow(Q3x,2.0)*Q2y*sp + 42*P1y*pow(Q2x,2.0)*Q2y*sp +
	24*P1y*pow(Q3x,2.0)*Q1y*sp + 54*P1y*pow(Q2x,2.0)*Q3y*sp + 96*P1y*pow(Q3x,2.0)*Q2y*sp - 42*P0y*Q1y*pow(Q2y,2.0)*sp - 42*P0y*Q1y*pow(Q3y,2.0)*sp + 42*P1y*Q1y*pow(Q2y,2.0)*sp - 42*P0y*Q2y*pow(Q3y,2.0)*sp +
	84*P0y*pow(Q2y,2.0)*Q3y*sp + 42*P1y*Q1y*pow(Q3y,2.0)*sp + 42*P1y*Q2y*pow(Q3y,2.0)*sp - 84*P1y*pow(Q2y,2.0)*Q3y*sp + 84*P0x*P1x*pow(Q2x,2.0)*pow(sp,2.0) - 154*P0x*P1x*pow(Q2x,2.0)*pow(sp,3.0) +
	84*P0x*P1x*pow(Q3x,2.0)*pow(sp,2.0) + 252*P0x*P2x*pow(Q2x,2.0)*pow(sp,2.0) + 63*P0x*P1x*pow(Q2x,2.0)*pow(sp,4.0) - 154*P0x*P1x*pow(Q3x,2.0)*pow(sp,3.0) - 462*P0x*P2x*pow(Q2x,2.0)*pow(sp,3.0) +
	252*P0x*P2x*pow(Q3x,2.0)*pow(sp,2.0) - 336*P0x*P3x*pow(Q2x,2.0)*pow(sp,2.0) - 252*P1x*P2x*pow(Q2x,2.0)*pow(sp,2.0) + 63*P0x*P1x*pow(Q3x,2.0)*pow(sp,4.0) + 210*P0x*P2x*pow(Q2x,2.0)*pow(sp,4.0) -
	462*P0x*P2x*pow(Q3x,2.0)*pow(sp,3.0) + 210*P0x*P3x*pow(Q2x,2.0)*pow(sp,3.0) - 336*P0x*P3x*pow(Q3x,2.0)*pow(sp,2.0) + 462*P1x*P2x*pow(Q2x,2.0)*pow(sp,3.0) - 252*P1x*P2x*pow(Q3x,2.0)*pow(sp,2.0) +
	336*P1x*P3x*pow(Q2x,2.0)*pow(sp,2.0) + 210*P0x*P2x*pow(Q3x,2.0)*pow(sp,4.0) + 210*P0x*P3x*pow(Q3x,2.0)*pow(sp,3.0) - 210*P1x*P2x*pow(Q2x,2.0)*pow(sp,4.0) + 462*P1x*P2x*pow(Q3x,2.0)*pow(sp,3.0) -
	210*P1x*P3x*pow(Q2x,2.0)*pow(sp,3.0) + 336*P1x*P3x*pow(Q3x,2.0)*pow(sp,2.0) - 210*P1x*P2x*pow(Q3x,2.0)*pow(sp,4.0) - 210*P1x*P3x*pow(Q3x,2.0)*pow(sp,3.0) + 84*P0x*P1x*pow(Q2y,2.0)*pow(sp,2.0) -
	112*P0x*P1x*pow(Q2y,2.0)*pow(sp,3.0) + 84*P0x*P1x*pow(Q3y,2.0)*pow(sp,2.0) + 252*P0x*P2x*pow(Q2y,2.0)*pow(sp,2.0) + 36*P0x*P1x*pow(Q2y,2.0)*pow(sp,4.0) - 112*P0x*P1x*pow(Q3y,2.0)*pow(sp,3.0) -
	336*P0x*P2x*pow(Q2y,2.0)*pow(sp,3.0) + 252*P0x*P2x*pow(Q3y,2.0)*pow(sp,2.0) - 264*P0x*P3x*pow(Q2y,2.0)*pow(sp,2.0) - 252*P1x*P2x*pow(Q2y,2.0)*pow(sp,2.0) + 36*P0x*P1x*pow(Q3y,2.0)*pow(sp,4.0) +
	120*P0x*P2x*pow(Q2y,2.0)*pow(sp,4.0) - 336*P0x*P2x*pow(Q3y,2.0)*pow(sp,3.0) + 120*P0x*P3x*pow(Q2y,2.0)*pow(sp,3.0) - 264*P0x*P3x*pow(Q3y,2.0)*pow(sp,2.0) + 336*P1x*P2x*pow(Q2y,2.0)*pow(sp,3.0) -
	252*P1x*P2x*pow(Q3y,2.0)*pow(sp,2.0) + 264*P1x*P3x*pow(Q2y,2.0)*pow(sp,2.0) + 120*P0x*P2x*pow(Q3y,2.0)*pow(sp,4.0) + 120*P0x*P3x*pow(Q3y,2.0)*pow(sp,3.0) - 120*P1x*P2x*pow(Q2y,2.0)*pow(sp,4.0) +
	336*P1x*P2x*pow(Q3y,2.0)*pow(sp,3.0) - 120*P1x*P3x*pow(Q2y,2.0)*pow(sp,3.0) + 264*P1x*P3x*pow(Q3y,2.0)*pow(sp,2.0) - 120*P1x*P2x*pow(Q3y,2.0)*pow(sp,4.0) - 120*P1x*P3x*pow(Q3y,2.0)*pow(sp,3.0) +
	84*P0y*P1y*pow(Q2x,2.0)*pow(sp,2.0) - 112*P0y*P1y*pow(Q2x,2.0)*pow(sp,3.0) + 84*P0y*P1y*pow(Q3x,2.0)*pow(sp,2.0) + 252*P0y*P2y*pow(Q2x,2.0)*pow(sp,2.0) + 36*P0y*P1y*pow(Q2x,2.0)*pow(sp,4.0) -
	112*P0y*P1y*pow(Q3x,2.0)*pow(sp,3.0) - 336*P0y*P2y*pow(Q2x,2.0)*pow(sp,3.0) + 252*P0y*P2y*pow(Q3x,2.0)*pow(sp,2.0) - 264*P0y*P3y*pow(Q2x,2.0)*pow(sp,2.0) - 252*P1y*P2y*pow(Q2x,2.0)*pow(sp,2.0) +
	36*P0y*P1y*pow(Q3x,2.0)*pow(sp,4.0) + 120*P0y*P2y*pow(Q2x,2.0)*pow(sp,4.0) - 336*P0y*P2y*pow(Q3x,2.0)*pow(sp,3.0) + 120*P0y*P3y*pow(Q2x,2.0)*pow(sp,3.0) - 264*P0y*P3y*pow(Q3x,2.0)*pow(sp,2.0) +
	336*P1y*P2y*pow(Q2x,2.0)*pow(sp,3.0) - 252*P1y*P2y*pow(Q3x,2.0)*pow(sp,2.0) + 264*P1y*P3y*pow(Q2x,2.0)*pow(sp,2.0) + 120*P0y*P2y*pow(Q3x,2.0)*pow(sp,4.0) + 120*P0y*P3y*pow(Q3x,2.0)*pow(sp,3.0) -
	120*P1y*P2y*pow(Q2x,2.0)*pow(sp,4.0) + 336*P1y*P2y*pow(Q3x,2.0)*pow(sp,3.0) - 120*P1y*P3y*pow(Q2x,2.0)*pow(sp,3.0) + 264*P1y*P3y*pow(Q3x,2.0)*pow(sp,2.0) - 120*P1y*P2y*pow(Q3x,2.0)*pow(sp,4.0) -
	120*P1y*P3y*pow(Q3x,2.0)*pow(sp,3.0) + 84*P0y*P1y*pow(Q2y,2.0)*pow(sp,2.0) - 154*P0y*P1y*pow(Q2y,2.0)*pow(sp,3.0) + 84*P0y*P1y*pow(Q3y,2.0)*pow(sp,2.0) + 252*P0y*P2y*pow(Q2y,2.0)*pow(sp,2.0) +
	63*P0y*P1y*pow(Q2y,2.0)*pow(sp,4.0) - 154*P0y*P1y*pow(Q3y,2.0)*pow(sp,3.0) - 462*P0y*P2y*pow(Q2y,2.0)*pow(sp,3.0) + 252*P0y*P2y*pow(Q3y,2.0)*pow(sp,2.0) - 336*P0y*P3y*pow(Q2y,2.0)*pow(sp,2.0) -
	252*P1y*P2y*pow(Q2y,2.0)*pow(sp,2.0) + 63*P0y*P1y*pow(Q3y,2.0)*pow(sp,4.0) + 210*P0y*P2y*pow(Q2y,2.0)*pow(sp,4.0) - 462*P0y*P2y*pow(Q3y,2.0)*pow(sp,3.0) + 210*P0y*P3y*pow(Q2y,2.0)*pow(sp,3.0) -
	336*P0y*P3y*pow(Q3y,2.0)*pow(sp,2.0) + 462*P1y*P2y*pow(Q2y,2.0)*pow(sp,3.0) - 252*P1y*P2y*pow(Q3y,2.0)*pow(sp,2.0) + 336*P1y*P3y*pow(Q2y,2.0)*pow(sp,2.0) + 210*P0y*P2y*pow(Q3y,2.0)*pow(sp,4.0) +
	210*P0y*P3y*pow(Q3y,2.0)*pow(sp,3.0) - 210*P1y*P2y*pow(Q2y,2.0)*pow(sp,4.0) + 462*P1y*P2y*pow(Q3y,2.0)*pow(sp,3.0) - 210*P1y*P3y*pow(Q2y,2.0)*pow(sp,3.0) + 336*P1y*P3y*pow(Q3y,2.0)*pow(sp,2.0) -
	210*P1y*P2y*pow(Q3y,2.0)*pow(sp,4.0) - 210*P1y*P3y*pow(Q3y,2.0)*pow(sp,3.0) - 189*P0x*Q1x*pow(Q2x,2.0)*pow(sp,2.0) + 420*P0x*Q1x*pow(Q2x,2.0)*pow(sp,3.0) - 189*P0x*Q1x*pow(Q3x,2.0)*pow(sp,2.0) +
	189*P1x*Q1x*pow(Q2x,2.0)*pow(sp,2.0) - 210*P0x*Q1x*pow(Q2x,2.0)*pow(sp,4.0) + 420*P0x*Q1x*pow(Q3x,2.0)*pow(sp,3.0) - 42*P0x*Q2x*pow(Q3x,2.0)*pow(sp,2.0) + 21*P0x*pow(Q2x,2.0)*Q3x*pow(sp,2.0) -
	420*P1x*Q1x*pow(Q2x,2.0)*pow(sp,3.0) + 189*P1x*Q1x*pow(Q3x,2.0)*pow(sp,2.0) - 168*pow(P0x,2.0)*Q2x*Q3x*pow(sp,2.0) - 210*P0x*Q1x*pow(Q3x,2.0)*pow(sp,4.0) + 98*P0x*Q2x*pow(Q3x,2.0)*pow(sp,3.0) -
	238*P0x*pow(Q2x,2.0)*Q3x*pow(sp,3.0) + 210*P1x*Q1x*pow(Q2x,2.0)*pow(sp,4.0) - 420*P1x*Q1x*pow(Q3x,2.0)*pow(sp,3.0) + 42*P1x*Q2x*pow(Q3x,2.0)*pow(sp,2.0) - 21*P1x*pow(Q2x,2.0)*Q3x*pow(sp,2.0) +
	154*pow(P0x,2.0)*Q2x*Q3x*pow(sp,3.0) + 336*pow(P1x,2.0)*Q2x*Q3x*pow(sp,2.0) - 42*P0x*Q2x*pow(Q3x,2.0)*pow(sp,4.0) + 147*P0x*pow(Q2x,2.0)*Q3x*pow(sp,4.0) + 210*P1x*Q1x*pow(Q3x,2.0)*pow(sp,4.0) -
	98*P1x*Q2x*pow(Q3x,2.0)*pow(sp,3.0) + 238*P1x*pow(Q2x,2.0)*Q3x*pow(sp,3.0) - 42*pow(P0x,2.0)*Q2x*Q3x*pow(sp,4.0) - 462*pow(P1x,2.0)*Q2x*Q3x*pow(sp,3.0) + 42*P1x*Q2x*pow(Q3x,2.0)*pow(sp,4.0) -
	147*P1x*pow(Q2x,2.0)*Q3x*pow(sp,4.0) + 168*pow(P1x,2.0)*Q2x*Q3x*pow(sp,4.0) - 216*P0x*Q1x*pow(Q2y,2.0)*pow(sp,2.0) + 312*P0x*Q1x*pow(Q2y,2.0)*pow(sp,3.0) - 216*P0x*Q1x*pow(Q3y,2.0)*pow(sp,2.0) +
	216*P1x*Q1x*pow(Q2y,2.0)*pow(sp,2.0) - 120*P0x*Q1x*pow(Q2y,2.0)*pow(sp,4.0) + 312*P0x*Q1x*pow(Q3y,2.0)*pow(sp,3.0) + 126*P0x*Q2x*pow(Q2y,2.0)*pow(sp,3.0) - 45*P0x*Q2x*pow(Q3y,2.0)*pow(sp,2.0) -
	24*P0x*Q3x*pow(Q2y,2.0)*pow(sp,2.0) - 312*P1x*Q1x*pow(Q2y,2.0)*pow(sp,3.0) + 216*P1x*Q1x*pow(Q3y,2.0)*pow(sp,2.0) - 168*pow(P0y,2.0)*Q2x*Q3x*pow(sp,2.0) - 120*P0x*Q1x*pow(Q3y,2.0)*pow(sp,4.0) -
	84*P0x*Q2x*pow(Q2y,2.0)*pow(sp,4.0) + 114*P0x*Q2x*pow(Q3y,2.0)*pow(sp,3.0) + 2*P0x*Q3x*pow(Q2y,2.0)*pow(sp,3.0) + 21*P0x*Q3x*pow(Q3y,2.0)*pow(sp,2.0) + 120*P1x*Q1x*pow(Q2y,2.0)*pow(sp,4.0) -
	312*P1x*Q1x*pow(Q3y,2.0)*pow(sp,3.0) - 126*P1x*Q2x*pow(Q2y,2.0)*pow(sp,3.0) + 45*P1x*Q2x*pow(Q3y,2.0)*pow(sp,2.0) + 24*P1x*Q3x*pow(Q2y,2.0)*pow(sp,2.0) + 112*pow(P0y,2.0)*Q2x*Q3x*pow(sp,3.0) +
	336*pow(P1y,2.0)*Q2x*Q3x*pow(sp,2.0) - 39*P0x*Q2x*pow(Q3y,2.0)*pow(sp,4.0) + 24*P0x*Q3x*pow(Q2y,2.0)*pow(sp,4.0) + 14*P0x*Q3x*pow(Q3y,2.0)*pow(sp,3.0) + 120*P1x*Q1x*pow(Q3y,2.0)*pow(sp,4.0) +
	84*P1x*Q2x*pow(Q2y,2.0)*pow(sp,4.0) - 114*P1x*Q2x*pow(Q3y,2.0)*pow(sp,3.0) - 2*P1x*Q3x*pow(Q2y,2.0)*pow(sp,3.0) - 21*P1x*Q3x*pow(Q3y,2.0)*pow(sp,2.0) - 24*pow(P0y,2.0)*Q2x*Q3x*pow(sp,4.0) -
	336*pow(P1y,2.0)*Q2x*Q3x*pow(sp,3.0) - 21*P0x*Q3x*pow(Q3y,2.0)*pow(sp,4.0) + 39*P1x*Q2x*pow(Q3y,2.0)*pow(sp,4.0) - 24*P1x*Q3x*pow(Q2y,2.0)*pow(sp,4.0) - 14*P1x*Q3x*pow(Q3y,2.0)*pow(sp,3.0) +
	96*pow(P1y,2.0)*Q2x*Q3x*pow(sp,4.0) + 21*P1x*Q3x*pow(Q3y,2.0)*pow(sp,4.0) - 216*P0y*pow(Q2x,2.0)*Q1y*pow(sp,2.0) + 312*P0y*pow(Q2x,2.0)*Q1y*pow(sp,3.0) - 216*P0y*pow(Q3x,2.0)*Q1y*pow(sp,2.0) +
	216*P1y*pow(Q2x,2.0)*Q1y*pow(sp,2.0) - 120*P0y*pow(Q2x,2.0)*Q1y*pow(sp,4.0) + 126*P0y*pow(Q2x,2.0)*Q2y*pow(sp,3.0) - 24*P0y*pow(Q2x,2.0)*Q3y*pow(sp,2.0) + 312*P0y*pow(Q3x,2.0)*Q1y*pow(sp,3.0) -
	45*P0y*pow(Q3x,2.0)*Q2y*pow(sp,2.0) - 312*P1y*pow(Q2x,2.0)*Q1y*pow(sp,3.0) + 216*P1y*pow(Q3x,2.0)*Q1y*pow(sp,2.0) - 168*pow(P0x,2.0)*Q2y*Q3y*pow(sp,2.0) - 84*P0y*pow(Q2x,2.0)*Q2y*pow(sp,4.0) +
	2*P0y*pow(Q2x,2.0)*Q3y*pow(sp,3.0) - 120*P0y*pow(Q3x,2.0)*Q1y*pow(sp,4.0) + 114*P0y*pow(Q3x,2.0)*Q2y*pow(sp,3.0) + 21*P0y*pow(Q3x,2.0)*Q3y*pow(sp,2.0) + 120*P1y*pow(Q2x,2.0)*Q1y*pow(sp,4.0) -
	126*P1y*pow(Q2x,2.0)*Q2y*pow(sp,3.0) + 24*P1y*pow(Q2x,2.0)*Q3y*pow(sp,2.0) - 312*P1y*pow(Q3x,2.0)*Q1y*pow(sp,3.0) + 45*P1y*pow(Q3x,2.0)*Q2y*pow(sp,2.0) + 112*pow(P0x,2.0)*Q2y*Q3y*pow(sp,3.0) +
	336*pow(P1x,2.0)*Q2y*Q3y*pow(sp,2.0) + 24*P0y*pow(Q2x,2.0)*Q3y*pow(sp,4.0) - 39*P0y*pow(Q3x,2.0)*Q2y*pow(sp,4.0) + 14*P0y*pow(Q3x,2.0)*Q3y*pow(sp,3.0) + 84*P1y*pow(Q2x,2.0)*Q2y*pow(sp,4.0) -
	2*P1y*pow(Q2x,2.0)*Q3y*pow(sp,3.0) + 120*P1y*pow(Q3x,2.0)*Q1y*pow(sp,4.0) - 114*P1y*pow(Q3x,2.0)*Q2y*pow(sp,3.0) - 21*P1y*pow(Q3x,2.0)*Q3y*pow(sp,2.0) - 24*pow(P0x,2.0)*Q2y*Q3y*pow(sp,4.0) -
	336*pow(P1x,2.0)*Q2y*Q3y*pow(sp,3.0) - 21*P0y*pow(Q3x,2.0)*Q3y*pow(sp,4.0) - 24*P1y*pow(Q2x,2.0)*Q3y*pow(sp,4.0) + 39*P1y*pow(Q3x,2.0)*Q2y*pow(sp,4.0) - 14*P1y*pow(Q3x,2.0)*Q3y*pow(sp,3.0) +
	96*pow(P1x,2.0)*Q2y*Q3y*pow(sp,4.0) + 21*P1y*pow(Q3x,2.0)*Q3y*pow(sp,4.0) - 189*P0y*Q1y*pow(Q2y,2.0)*pow(sp,2.0) + 420*P0y*Q1y*pow(Q2y,2.0)*pow(sp,3.0) - 189*P0y*Q1y*pow(Q3y,2.0)*pow(sp,2.0) +
	189*P1y*Q1y*pow(Q2y,2.0)*pow(sp,2.0) - 210*P0y*Q1y*pow(Q2y,2.0)*pow(sp,4.0) + 420*P0y*Q1y*pow(Q3y,2.0)*pow(sp,3.0) - 42*P0y*Q2y*pow(Q3y,2.0)*pow(sp,2.0) + 21*P0y*pow(Q2y,2.0)*Q3y*pow(sp,2.0) -
	420*P1y*Q1y*pow(Q2y,2.0)*pow(sp,3.0) + 189*P1y*Q1y*pow(Q3y,2.0)*pow(sp,2.0) - 168*pow(P0y,2.0)*Q2y*Q3y*pow(sp,2.0) - 210*P0y*Q1y*pow(Q3y,2.0)*pow(sp,4.0) + 98*P0y*Q2y*pow(Q3y,2.0)*pow(sp,3.0) -
	238*P0y*pow(Q2y,2.0)*Q3y*pow(sp,3.0) + 210*P1y*Q1y*pow(Q2y,2.0)*pow(sp,4.0) - 420*P1y*Q1y*pow(Q3y,2.0)*pow(sp,3.0) + 42*P1y*Q2y*pow(Q3y,2.0)*pow(sp,2.0) - 21*P1y*pow(Q2y,2.0)*Q3y*pow(sp,2.0) +
	154*pow(P0y,2.0)*Q2y*Q3y*pow(sp,3.0) + 336*pow(P1y,2.0)*Q2y*Q3y*pow(sp,2.0) - 42*P0y*Q2y*pow(Q3y,2.0)*pow(sp,4.0) + 147*P0y*pow(Q2y,2.0)*Q3y*pow(sp,4.0) + 210*P1y*Q1y*pow(Q3y,2.0)*pow(sp,4.0) -
	98*P1y*Q2y*pow(Q3y,2.0)*pow(sp,3.0) + 238*P1y*pow(Q2y,2.0)*Q3y*pow(sp,3.0) - 42*pow(P0y,2.0)*Q2y*Q3y*pow(sp,4.0) - 462*pow(P1y,2.0)*Q2y*Q3y*pow(sp,3.0) + 42*P1y*Q2y*pow(Q3y,2.0)*pow(sp,4.0) -
	147*P1y*pow(Q2y,2.0)*Q3y*pow(sp,4.0) + 168*pow(P1y,2.0)*Q2y*Q3y*pow(sp,4.0) - 98*P0x*P1x*Q2x*Q3x - 56*P0x*P3x*Q2x*Q3x + 56*P1x*P3x*Q2x*Q3x + 78*P0x*P0y*Q2x*Q2y - 78*P0x*P0y*Q2x*Q3y - 78*P0x*P0y*Q3x*Q2y -
	39*P0x*P1y*Q2x*Q2y - 39*P1x*P0y*Q2x*Q2y - 176*P0x*P1x*Q2y*Q3y + 78*P0x*P0y*Q3x*Q3y + 39*P0x*P1y*Q2x*Q3y + 39*P0x*P1y*Q3x*Q2y + 39*P1x*P0y*Q2x*Q3y + 39*P1x*P0y*Q3x*Q2y - 176*P0y*P1y*Q2x*Q3x -
	39*P0x*P1y*Q3x*Q3y - 12*P0x*P3y*Q2x*Q2y - 39*P1x*P0y*Q3x*Q3y - 12*P3x*P0y*Q2x*Q2y - 80*P0x*P3x*Q2y*Q3y + 12*P0x*P3y*Q2x*Q3y + 12*P0x*P3y*Q3x*Q2y + 12*P1x*P3y*Q2x*Q2y + 12*P3x*P0y*Q2x*Q3y +
	12*P3x*P0y*Q3x*Q2y + 12*P3x*P1y*Q2x*Q2y - 80*P0y*P3y*Q2x*Q3x - 12*P0x*P3y*Q3x*Q3y + 80*P1x*P3x*Q2y*Q3y - 12*P1x*P3y*Q2x*Q3y - 12*P1x*P3y*Q3x*Q2y - 12*P3x*P0y*Q3x*Q3y - 12*P3x*P1y*Q2x*Q3y -
	12*P3x*P1y*Q3x*Q2y + 80*P1y*P3y*Q2x*Q3x + 12*P1x*P3y*Q3x*Q3y + 12*P3x*P1y*Q3x*Q3y - 98*P0y*P1y*Q2y*Q3y - 56*P0y*P3y*Q2y*Q3y + 56*P1y*P3y*Q2y*Q3y - 42*P0x*Q1x*Q2x*Q3x + 42*P1x*Q1x*Q2x*Q3x - 27*P0x*Q2x*Q1y*Q2y -
	27*P0y*Q1x*Q2x*Q2y - 96*P0x*Q1x*Q2y*Q3y + 27*P0x*Q2x*Q1y*Q3y + 27*P0x*Q3x*Q1y*Q2y + 27*P1x*Q2x*Q1y*Q2y + 27*P0y*Q1x*Q2x*Q3y + 27*P0y*Q1x*Q3x*Q2y - 96*P0y*Q2x*Q3x*Q1y + 27*P1y*Q1x*Q2x*Q2y - 27*P0x*Q3x*Q1y*Q3y +
	96*P1x*Q1x*Q2y*Q3y - 27*P1x*Q2x*Q1y*Q3y - 27*P1x*Q3x*Q1y*Q2y - 27*P0y*Q1x*Q3x*Q3y - 27*P1y*Q1x*Q2x*Q3y - 27*P1y*Q1x*Q3x*Q2y + 96*P1y*Q2x*Q3x*Q1y + 27*P1x*Q3x*Q1y*Q3y + 27*P1y*Q1x*Q3x*Q3y - 42*P0y*Q1y*Q2y*Q3y +
	42*P1y*Q1y*Q2y*Q3y - 168*P0x*P3x*Q2x*Q3x*sp + 168*P1x*P3x*Q2x*Q3x*sp - 36*P0x*P3y*Q2x*Q2y*sp - 36*P3x*P0y*Q2x*Q2y*sp - 240*P0x*P3x*Q2y*Q3y*sp + 36*P0x*P3y*Q2x*Q3y*sp + 36*P0x*P3y*Q3x*Q2y*sp + 36*P1x*P3y*Q2x*Q2y*sp +
	36*P3x*P0y*Q2x*Q3y*sp + 36*P3x*P0y*Q3x*Q2y*sp + 36*P3x*P1y*Q2x*Q2y*sp - 240*P0y*P3y*Q2x*Q3x*sp - 36*P0x*P3y*Q3x*Q3y*sp + 240*P1x*P3x*Q2y*Q3y*sp - 36*P1x*P3y*Q2x*Q3y*sp - 36*P1x*P3y*Q3x*Q2y*sp - 36*P3x*P0y*Q3x*Q3y*sp -
	36*P3x*P1y*Q2x*Q3y*sp - 36*P3x*P1y*Q3x*Q2y*sp + 240*P1y*P3y*Q2x*Q3x*sp + 36*P1x*P3y*Q3x*Q3y*sp + 36*P3x*P1y*Q3x*Q3y*sp - 168*P0y*P3y*Q2y*Q3y*sp + 168*P1y*P3y*Q2y*Q3y*sp + 84*P0x*Q1x*Q2x*Q3x*sp - 84*P1x*Q1x*Q2x*Q3x*sp -
	18*P0x*Q2x*Q1y*Q2y*sp - 18*P0y*Q1x*Q2x*Q2y*sp + 48*P0x*Q1x*Q2y*Q3y*sp + 18*P0x*Q2x*Q1y*Q3y*sp + 18*P0x*Q3x*Q1y*Q2y*sp + 18*P1x*Q2x*Q1y*Q2y*sp + 18*P0y*Q1x*Q2x*Q3y*sp + 18*P0y*Q1x*Q3x*Q2y*sp + 48*P0y*Q2x*Q3x*Q1y*sp +
	18*P1y*Q1x*Q2x*Q2y*sp + 138*P0x*Q2x*Q2y*Q3y*sp - 18*P0x*Q3x*Q1y*Q3y*sp - 48*P1x*Q1x*Q2y*Q3y*sp - 18*P1x*Q2x*Q1y*Q3y*sp - 18*P1x*Q3x*Q1y*Q2y*sp - 18*P0y*Q1x*Q3x*Q3y*sp + 138*P0y*Q2x*Q3x*Q2y*sp - 18*P1y*Q1x*Q2x*Q3y*sp -
	18*P1y*Q1x*Q3x*Q2y*sp - 48*P1y*Q2x*Q3x*Q1y*sp + 54*P0x*Q3x*Q2y*Q3y*sp - 138*P1x*Q2x*Q2y*Q3y*sp + 18*P1x*Q3x*Q1y*Q3y*sp + 54*P0y*Q2x*Q3x*Q3y*sp + 18*P1y*Q1x*Q3x*Q3y*sp - 138*P1y*Q2x*Q3x*Q2y*sp - 54*P1x*Q3x*Q2y*Q3y*sp -
	54*P1y*Q2x*Q3x*Q3y*sp + 84*P0y*Q1y*Q2y*Q3y*sp - 84*P1y*Q1y*Q2y*Q3y*sp - 168*P0x*P1x*Q2x*Q3x*pow(sp,2.0) + 308*P0x*P1x*Q2x*Q3x*pow(sp,3.0) - 504*P0x*P2x*Q2x*Q3x*pow(sp,2.0) - 126*P0x*P1x*Q2x*Q3x*pow(sp,4.0) +
	924*P0x*P2x*Q2x*Q3x*pow(sp,3.0) + 672*P0x*P3x*Q2x*Q3x*pow(sp,2.0) + 504*P1x*P2x*Q2x*Q3x*pow(sp,2.0) - 420*P0x*P2x*Q2x*Q3x*pow(sp,4.0) - 420*P0x*P3x*Q2x*Q3x*pow(sp,3.0) - 924*P1x*P2x*Q2x*Q3x*pow(sp,3.0) -
	672*P1x*P3x*Q2x*Q3x*pow(sp,2.0) + 420*P1x*P2x*Q2x*Q3x*pow(sp,4.0) + 420*P1x*P3x*Q2x*Q3x*pow(sp,3.0) - 42*P0x*P0y*Q2x*Q2y*pow(sp,3.0) - 168*P0x*P1x*Q2y*Q3y*pow(sp,2.0) + 18*P0x*P0y*Q2x*Q2y*pow(sp,4.0) +
	42*P0x*P0y*Q2x*Q3y*pow(sp,3.0) + 42*P0x*P0y*Q3x*Q2y*pow(sp,3.0) - 42*P0x*P1y*Q2x*Q2y*pow(sp,3.0) - 42*P1x*P0y*Q2x*Q2y*pow(sp,3.0) - 168*P0y*P1y*Q2x*Q3x*pow(sp,2.0) + 224*P0x*P1x*Q2y*Q3y*pow(sp,3.0) -
	504*P0x*P2x*Q2y*Q3y*pow(sp,2.0) - 18*P0x*P0y*Q2x*Q3y*pow(sp,4.0) - 18*P0x*P0y*Q3x*Q2y*pow(sp,4.0) - 42*P0x*P0y*Q3x*Q3y*pow(sp,3.0) + 27*P0x*P1y*Q2x*Q2y*pow(sp,4.0) + 42*P0x*P1y*Q2x*Q3y*pow(sp,3.0) +
	42*P0x*P1y*Q3x*Q2y*pow(sp,3.0) - 126*P0x*P2y*Q2x*Q2y*pow(sp,3.0) - 72*P0x*P3y*Q2x*Q2y*pow(sp,2.0) + 27*P1x*P0y*Q2x*Q2y*pow(sp,4.0) + 42*P1x*P0y*Q2x*Q3y*pow(sp,3.0) + 42*P1x*P0y*Q3x*Q2y*pow(sp,3.0) +
	126*P1x*P1y*Q2x*Q2y*pow(sp,3.0) - 126*P2x*P0y*Q2x*Q2y*pow(sp,3.0) - 72*P3x*P0y*Q2x*Q2y*pow(sp,2.0) + 224*P0y*P1y*Q2x*Q3x*pow(sp,3.0) - 504*P0y*P2y*Q2x*Q3x*pow(sp,2.0) - 72*P0x*P1x*Q2y*Q3y*pow(sp,4.0) +
	672*P0x*P2x*Q2y*Q3y*pow(sp,3.0) + 528*P0x*P3x*Q2y*Q3y*pow(sp,2.0) + 18*P0x*P0y*Q3x*Q3y*pow(sp,4.0) - 27*P0x*P1y*Q2x*Q3y*pow(sp,4.0) - 27*P0x*P1y*Q3x*Q2y*pow(sp,4.0) - 42*P0x*P1y*Q3x*Q3y*pow(sp,3.0) +
	90*P0x*P2y*Q2x*Q2y*pow(sp,4.0) + 126*P0x*P2y*Q2x*Q3y*pow(sp,3.0) + 126*P0x*P2y*Q3x*Q2y*pow(sp,3.0) + 90*P0x*P3y*Q2x*Q2y*pow(sp,3.0) + 72*P0x*P3y*Q2x*Q3y*pow(sp,2.0) + 72*P0x*P3y*Q3x*Q2y*pow(sp,2.0) +
	504*P1x*P2x*Q2y*Q3y*pow(sp,2.0) - 27*P1x*P0y*Q2x*Q3y*pow(sp,4.0) - 27*P1x*P0y*Q3x*Q2y*pow(sp,4.0) - 42*P1x*P0y*Q3x*Q3y*pow(sp,3.0) - 72*P1x*P1y*Q2x*Q2y*pow(sp,4.0) - 126*P1x*P1y*Q2x*Q3y*pow(sp,3.0) -
	126*P1x*P1y*Q3x*Q2y*pow(sp,3.0) + 126*P1x*P2y*Q2x*Q2y*pow(sp,3.0) + 72*P1x*P3y*Q2x*Q2y*pow(sp,2.0) + 90*P2x*P0y*Q2x*Q2y*pow(sp,4.0) + 126*P2x*P0y*Q2x*Q3y*pow(sp,3.0) + 126*P2x*P0y*Q3x*Q2y*pow(sp,3.0) +
	126*P2x*P1y*Q2x*Q2y*pow(sp,3.0) + 90*P3x*P0y*Q2x*Q2y*pow(sp,3.0) + 72*P3x*P0y*Q2x*Q3y*pow(sp,2.0) + 72*P3x*P0y*Q3x*Q2y*pow(sp,2.0) + 72*P3x*P1y*Q2x*Q2y*pow(sp,2.0) - 72*P0y*P1y*Q2x*Q3x*pow(sp,4.0) +
	672*P0y*P2y*Q2x*Q3x*pow(sp,3.0) + 528*P0y*P3y*Q2x*Q3x*pow(sp,2.0) + 504*P1y*P2y*Q2x*Q3x*pow(sp,2.0) - 240*P0x*P2x*Q2y*Q3y*pow(sp,4.0) - 240*P0x*P3x*Q2y*Q3y*pow(sp,3.0) + 27*P0x*P1y*Q3x*Q3y*pow(sp,4.0) -
	90*P0x*P2y*Q2x*Q3y*pow(sp,4.0) - 90*P0x*P2y*Q3x*Q2y*pow(sp,4.0) - 126*P0x*P2y*Q3x*Q3y*pow(sp,3.0) - 90*P0x*P3y*Q2x*Q3y*pow(sp,3.0) - 90*P0x*P3y*Q3x*Q2y*pow(sp,3.0) - 72*P0x*P3y*Q3x*Q3y*pow(sp,2.0) -
	672*P1x*P2x*Q2y*Q3y*pow(sp,3.0) - 528*P1x*P3x*Q2y*Q3y*pow(sp,2.0) + 27*P1x*P0y*Q3x*Q3y*pow(sp,4.0) + 72*P1x*P1y*Q2x*Q3y*pow(sp,4.0) + 72*P1x*P1y*Q3x*Q2y*pow(sp,4.0) + 126*P1x*P1y*Q3x*Q3y*pow(sp,3.0) -
	90*P1x*P2y*Q2x*Q2y*pow(sp,4.0) - 126*P1x*P2y*Q2x*Q3y*pow(sp,3.0) - 126*P1x*P2y*Q3x*Q2y*pow(sp,3.0) - 90*P1x*P3y*Q2x*Q2y*pow(sp,3.0) - 72*P1x*P3y*Q2x*Q3y*pow(sp,2.0) - 72*P1x*P3y*Q3x*Q2y*pow(sp,2.0) -
	90*P2x*P0y*Q2x*Q3y*pow(sp,4.0) - 90*P2x*P0y*Q3x*Q2y*pow(sp,4.0) - 126*P2x*P0y*Q3x*Q3y*pow(sp,3.0) - 90*P2x*P1y*Q2x*Q2y*pow(sp,4.0) - 126*P2x*P1y*Q2x*Q3y*pow(sp,3.0) - 126*P2x*P1y*Q3x*Q2y*pow(sp,3.0) -
	90*P3x*P0y*Q2x*Q3y*pow(sp,3.0) - 90*P3x*P0y*Q3x*Q2y*pow(sp,3.0) - 72*P3x*P0y*Q3x*Q3y*pow(sp,2.0) - 90*P3x*P1y*Q2x*Q2y*pow(sp,3.0) - 72*P3x*P1y*Q2x*Q3y*pow(sp,2.0) - 72*P3x*P1y*Q3x*Q2y*pow(sp,2.0) -
	240*P0y*P2y*Q2x*Q3x*pow(sp,4.0) - 240*P0y*P3y*Q2x*Q3x*pow(sp,3.0) - 672*P1y*P2y*Q2x*Q3x*pow(sp,3.0) - 528*P1y*P3y*Q2x*Q3x*pow(sp,2.0) + 90*P0x*P2y*Q3x*Q3y*pow(sp,4.0) + 90*P0x*P3y*Q3x*Q3y*pow(sp,3.0) +
	240*P1x*P2x*Q2y*Q3y*pow(sp,4.0) + 240*P1x*P3x*Q2y*Q3y*pow(sp,3.0) - 72*P1x*P1y*Q3x*Q3y*pow(sp,4.0) + 90*P1x*P2y*Q2x*Q3y*pow(sp,4.0) + 90*P1x*P2y*Q3x*Q2y*pow(sp,4.0) + 126*P1x*P2y*Q3x*Q3y*pow(sp,3.0) +
	90*P1x*P3y*Q2x*Q3y*pow(sp,3.0) + 90*P1x*P3y*Q3x*Q2y*pow(sp,3.0) + 72*P1x*P3y*Q3x*Q3y*pow(sp,2.0) + 90*P2x*P0y*Q3x*Q3y*pow(sp,4.0) + 90*P2x*P1y*Q2x*Q3y*pow(sp,4.0) + 90*P2x*P1y*Q3x*Q2y*pow(sp,4.0) +
	126*P2x*P1y*Q3x*Q3y*pow(sp,3.0) + 90*P3x*P0y*Q3x*Q3y*pow(sp,3.0) + 90*P3x*P1y*Q2x*Q3y*pow(sp,3.0) + 90*P3x*P1y*Q3x*Q2y*pow(sp,3.0) + 72*P3x*P1y*Q3x*Q3y*pow(sp,2.0) + 240*P1y*P2y*Q2x*Q3x*pow(sp,4.0) +
	240*P1y*P3y*Q2x*Q3x*pow(sp,3.0) - 90*P1x*P2y*Q3x*Q3y*pow(sp,4.0) - 90*P1x*P3y*Q3x*Q3y*pow(sp,3.0) - 90*P2x*P1y*Q3x*Q3y*pow(sp,4.0) - 90*P3x*P1y*Q3x*Q3y*pow(sp,3.0) - 168*P0y*P1y*Q2y*Q3y*pow(sp,2.0) +
	308*P0y*P1y*Q2y*Q3y*pow(sp,3.0) - 504*P0y*P2y*Q2y*Q3y*pow(sp,2.0) - 126*P0y*P1y*Q2y*Q3y*pow(sp,4.0) + 924*P0y*P2y*Q2y*Q3y*pow(sp,3.0) + 672*P0y*P3y*Q2y*Q3y*pow(sp,2.0) + 504*P1y*P2y*Q2y*Q3y*pow(sp,2.0) -
	420*P0y*P2y*Q2y*Q3y*pow(sp,4.0) - 420*P0y*P3y*Q2y*Q3y*pow(sp,3.0) - 924*P1y*P2y*Q2y*Q3y*pow(sp,3.0) - 672*P1y*P3y*Q2y*Q3y*pow(sp,2.0) + 420*P1y*P2y*Q2y*Q3y*pow(sp,4.0) + 420*P1y*P3y*Q2y*Q3y*pow(sp,3.0) +
	378*P0x*Q1x*Q2x*Q3x*pow(sp,2.0) - 840*P0x*Q1x*Q2x*Q3x*pow(sp,3.0) - 378*P1x*Q1x*Q2x*Q3x*pow(sp,2.0) + 420*P0x*Q1x*Q2x*Q3x*pow(sp,4.0) + 840*P1x*Q1x*Q2x*Q3x*pow(sp,3.0) - 420*P1x*Q1x*Q2x*Q3x*pow(sp,4.0) +
	27*P0x*Q2x*Q1y*Q2y*pow(sp,2.0) + 27*P0y*Q1x*Q2x*Q2y*pow(sp,2.0) + 432*P0x*Q1x*Q2y*Q3y*pow(sp,2.0) + 108*P0x*Q2x*Q1y*Q2y*pow(sp,3.0) - 27*P0x*Q2x*Q1y*Q3y*pow(sp,2.0) - 27*P0x*Q3x*Q1y*Q2y*pow(sp,2.0) -
	27*P1x*Q2x*Q1y*Q2y*pow(sp,2.0) + 108*P0y*Q1x*Q2x*Q2y*pow(sp,3.0) - 27*P0y*Q1x*Q2x*Q3y*pow(sp,2.0) - 27*P0y*Q1x*Q3x*Q2y*pow(sp,2.0) + 432*P0y*Q2x*Q3x*Q1y*pow(sp,2.0) - 27*P1y*Q1x*Q2x*Q2y*pow(sp,2.0) -
	624*P0x*Q1x*Q2y*Q3y*pow(sp,3.0) - 90*P0x*Q2x*Q1y*Q2y*pow(sp,4.0) - 108*P0x*Q2x*Q1y*Q3y*pow(sp,3.0) + 45*P0x*Q2x*Q2y*Q3y*pow(sp,2.0) - 108*P0x*Q3x*Q1y*Q2y*pow(sp,3.0) + 27*P0x*Q3x*Q1y*Q3y*pow(sp,2.0) -
	432*P1x*Q1x*Q2y*Q3y*pow(sp,2.0) - 108*P1x*Q2x*Q1y*Q2y*pow(sp,3.0) + 27*P1x*Q2x*Q1y*Q3y*pow(sp,2.0) + 27*P1x*Q3x*Q1y*Q2y*pow(sp,2.0) - 90*P0y*Q1x*Q2x*Q2y*pow(sp,4.0) - 108*P0y*Q1x*Q2x*Q3y*pow(sp,3.0) -
	108*P0y*Q1x*Q3x*Q2y*pow(sp,3.0) + 27*P0y*Q1x*Q3x*Q3y*pow(sp,2.0) - 624*P0y*Q2x*Q3x*Q1y*pow(sp,3.0) + 45*P0y*Q2x*Q3x*Q2y*pow(sp,2.0) - 108*P1y*Q1x*Q2x*Q2y*pow(sp,3.0) + 27*P1y*Q1x*Q2x*Q3y*pow(sp,2.0) +
	27*P1y*Q1x*Q3x*Q2y*pow(sp,2.0) - 432*P1y*Q2x*Q3x*Q1y*pow(sp,2.0) + 240*P0x*Q1x*Q2y*Q3y*pow(sp,4.0) + 90*P0x*Q2x*Q1y*Q3y*pow(sp,4.0) - 240*P0x*Q2x*Q2y*Q3y*pow(sp,3.0) + 90*P0x*Q3x*Q1y*Q2y*pow(sp,4.0) +
	108*P0x*Q3x*Q1y*Q3y*pow(sp,3.0) + 3*P0x*Q3x*Q2y*Q3y*pow(sp,2.0) + 624*P1x*Q1x*Q2y*Q3y*pow(sp,3.0) + 90*P1x*Q2x*Q1y*Q2y*pow(sp,4.0) + 108*P1x*Q2x*Q1y*Q3y*pow(sp,3.0) - 45*P1x*Q2x*Q2y*Q3y*pow(sp,2.0) +
	108*P1x*Q3x*Q1y*Q2y*pow(sp,3.0) - 27*P1x*Q3x*Q1y*Q3y*pow(sp,2.0) + 90*P0y*Q1x*Q2x*Q3y*pow(sp,4.0) + 90*P0y*Q1x*Q3x*Q2y*pow(sp,4.0) + 108*P0y*Q1x*Q3x*Q3y*pow(sp,3.0) + 240*P0y*Q2x*Q3x*Q1y*pow(sp,4.0) -
	240*P0y*Q2x*Q3x*Q2y*pow(sp,3.0) + 3*P0y*Q2x*Q3x*Q3y*pow(sp,2.0) + 90*P1y*Q1x*Q2x*Q2y*pow(sp,4.0) + 108*P1y*Q1x*Q2x*Q3y*pow(sp,3.0) + 108*P1y*Q1x*Q3x*Q2y*pow(sp,3.0) - 27*P1y*Q1x*Q3x*Q3y*pow(sp,2.0) +
	624*P1y*Q2x*Q3x*Q1y*pow(sp,3.0) - 45*P1y*Q2x*Q3x*Q2y*pow(sp,2.0) + 123*P0x*Q2x*Q2y*Q3y*pow(sp,4.0) - 90*P0x*Q3x*Q1y*Q3y*pow(sp,4.0) - 16*P0x*Q3x*Q2y*Q3y*pow(sp,3.0) - 240*P1x*Q1x*Q2y*Q3y*pow(sp,4.0) -
	90*P1x*Q2x*Q1y*Q3y*pow(sp,4.0) + 240*P1x*Q2x*Q2y*Q3y*pow(sp,3.0) - 90*P1x*Q3x*Q1y*Q2y*pow(sp,4.0) - 108*P1x*Q3x*Q1y*Q3y*pow(sp,3.0) - 3*P1x*Q3x*Q2y*Q3y*pow(sp,2.0) - 90*P0y*Q1x*Q3x*Q3y*pow(sp,4.0) +
	123*P0y*Q2x*Q3x*Q2y*pow(sp,4.0) - 16*P0y*Q2x*Q3x*Q3y*pow(sp,3.0) - 90*P1y*Q1x*Q2x*Q3y*pow(sp,4.0) - 90*P1y*Q1x*Q3x*Q2y*pow(sp,4.0) - 108*P1y*Q1x*Q3x*Q3y*pow(sp,3.0) - 240*P1y*Q2x*Q3x*Q1y*pow(sp,4.0) +
	240*P1y*Q2x*Q3x*Q2y*pow(sp,3.0) - 3*P1y*Q2x*Q3x*Q3y*pow(sp,2.0) - 3*P0x*Q3x*Q2y*Q3y*pow(sp,4.0) - 123*P1x*Q2x*Q2y*Q3y*pow(sp,4.0) + 90*P1x*Q3x*Q1y*Q3y*pow(sp,4.0) + 16*P1x*Q3x*Q2y*Q3y*pow(sp,3.0) -
	3*P0y*Q2x*Q3x*Q3y*pow(sp,4.0) + 90*P1y*Q1x*Q3x*Q3y*pow(sp,4.0) - 123*P1y*Q2x*Q3x*Q2y*pow(sp,4.0) + 16*P1y*Q2x*Q3x*Q3y*pow(sp,3.0) + 3*P1x*Q3x*Q2y*Q3y*pow(sp,4.0) + 3*P1y*Q2x*Q3x*Q3y*pow(sp,4.0) +
	378*P0y*Q1y*Q2y*Q3y*pow(sp,2.0) - 840*P0y*Q1y*Q2y*Q3y*pow(sp,3.0) - 378*P1y*Q1y*Q2y*Q3y*pow(sp,2.0) + 420*P0y*Q1y*Q2y*Q3y*pow(sp,4.0) + 840*P1y*Q1y*Q2y*Q3y*pow(sp,3.0) -
	420*P1y*Q1y*Q2y*Q3y*pow(sp,4.0))/(3*(7*pow(P0x,2.0)*pow(Q2x,2.0) + 7*pow(P0x,2.0)*pow(Q3x,2.0) + 7*pow(P1x,2.0)*pow(Q2x,2.0) + 7*pow(P1x,2.0)*pow(Q3x,2.0) + 16*pow(P0x,2.0)*pow(Q2y,2.0) +
	16*pow(P0y,2.0)*pow(Q2x,2.0) + 16*pow(P0x,2.0)*pow(Q3y,2.0) + 16*pow(P1x,2.0)*pow(Q2y,2.0) + 16*pow(P0y,2.0)*pow(Q3x,2.0) + 16*pow(P1y,2.0)*pow(Q2x,2.0) + 16*pow(P1x,2.0)*pow(Q3y,2.0) +
	16*pow(P1y,2.0)*pow(Q3x,2.0) + 7*pow(P0y,2.0)*pow(Q2y,2.0) + 7*pow(P0y,2.0)*pow(Q3y,2.0) + 7*pow(P1y,2.0)*pow(Q2y,2.0) + 7*pow(P1y,2.0)*pow(Q3y,2.0) - 14*P0x*P1x*pow(Q2x,2.0) - 14*P0x*P1x*pow(Q3x,2.0) -
	32*P0x*P1x*pow(Q2y,2.0) - 32*P0x*P1x*pow(Q3y,2.0) - 32*P0y*P1y*pow(Q2x,2.0) - 32*P0y*P1y*pow(Q3x,2.0) - 14*P0y*P1y*pow(Q2y,2.0) - 14*P0y*P1y*pow(Q3y,2.0) - 14*pow(P0x,2.0)*Q2x*Q3x - 14*pow(P1x,2.0)*Q2x*Q3x -
	32*pow(P0y,2.0)*Q2x*Q3x - 32*pow(P1y,2.0)*Q2x*Q3x - 32*pow(P0x,2.0)*Q2y*Q3y - 32*pow(P1x,2.0)*Q2y*Q3y - 14*pow(P0y,2.0)*Q2y*Q3y - 14*pow(P1y,2.0)*Q2y*Q3y + 28*P0x*P1x*Q2x*Q3x - 18*P0x*P0y*Q2x*Q2y + 18*P0x*P0y*Q2x*Q3y +
	18*P0x*P0y*Q3x*Q2y + 18*P0x*P1y*Q2x*Q2y + 18*P1x*P0y*Q2x*Q2y + 64*P0x*P1x*Q2y*Q3y - 18*P0x*P0y*Q3x*Q3y - 18*P0x*P1y*Q2x*Q3y - 18*P0x*P1y*Q3x*Q2y - 18*P1x*P0y*Q2x*Q3y - 18*P1x*P0y*Q3x*Q2y - 18*P1x*P1y*Q2x*Q2y +
	64*P0y*P1y*Q2x*Q3x + 18*P0x*P1y*Q3x*Q3y + 18*P1x*P0y*Q3x*Q3y + 18*P1x*P1y*Q2x*Q3y + 18*P1x*P1y*Q3x*Q2y - 18*P1x*P1y*Q3x*Q3y + 28*P0y*P1y*Q2y*Q3y));
	
	out_qfactor = (14*pow(P0x,3.0)*Q2x - 14*pow(P0x,3.0)*Q3x + 14*pow(P0y,3.0)*Q2y - 14*pow(P0y,3.0)*Q3y + 21*pow(P0x,2.0)*pow(Q2x,2.0) + 21*pow(P0x,2.0)*pow(Q3x,2.0) + 21*pow(P1x,2.0)*pow(Q2x,2.0) + 21*pow(P1x,2.0)*pow(Q3x,2.0) +
	48*pow(P0x,2.0)*pow(Q2y,2.0) + 48*pow(P0y,2.0)*pow(Q2x,2.0) + 48*pow(P0x,2.0)*pow(Q3y,2.0) + 48*pow(P1x,2.0)*pow(Q2y,2.0) + 48*pow(P0y,2.0)*pow(Q3x,2.0) + 48*pow(P1y,2.0)*pow(Q2x,2.0) + 48*pow(P1x,2.0)*pow(Q3y,2.0) +
	48*pow(P1y,2.0)*pow(Q3x,2.0) + 21*pow(P0y,2.0)*pow(Q2y,2.0) + 21*pow(P0y,2.0)*pow(Q3y,2.0) + 21*pow(P1y,2.0)*pow(Q2y,2.0) + 21*pow(P1y,2.0)*pow(Q3y,2.0) + 21*pow(P0x,2.0)*pow(Q2x,2.0)*sp + 21*pow(P0x,2.0)*pow(Q3x,2.0)*sp -
	63*pow(P0x,3.0)*Q2x*pow(sp,2.0) + 21*pow(P1x,2.0)*pow(Q2x,2.0)*sp + 70*pow(P0x,3.0)*Q2x*pow(sp,3.0) + 63*pow(P0x,3.0)*Q3x*pow(sp,2.0) + 21*pow(P1x,2.0)*pow(Q3x,2.0)*sp - 126*pow(P1x,3.0)*Q2x*pow(sp,2.0) -
	21*pow(P0x,3.0)*Q2x*pow(sp,4.0) - 70*pow(P0x,3.0)*Q3x*pow(sp,3.0) + 210*pow(P1x,3.0)*Q2x*pow(sp,3.0) + 126*pow(P1x,3.0)*Q3x*pow(sp,2.0) + 21*pow(P0x,3.0)*Q3x*pow(sp,4.0) - 84*pow(P1x,3.0)*Q2x*pow(sp,4.0) -
	210*pow(P1x,3.0)*Q3x*pow(sp,3.0) + 84*pow(P1x,3.0)*Q3x*pow(sp,4.0) - 24*pow(P0x,2.0)*pow(Q2y,2.0)*sp - 24*pow(P0y,2.0)*pow(Q2x,2.0)*sp + 48*pow(P0x,2.0)*pow(Q3y,2.0)*sp - 24*pow(P1x,2.0)*pow(Q2y,2.0)*sp +
	48*pow(P0y,2.0)*pow(Q3x,2.0)*sp - 24*pow(P1y,2.0)*pow(Q2x,2.0)*sp + 48*pow(P1x,2.0)*pow(Q3y,2.0)*sp + 48*pow(P1y,2.0)*pow(Q3x,2.0)*sp + 21*pow(P0y,2.0)*pow(Q2y,2.0)*sp + 21*pow(P0y,2.0)*pow(Q3y,2.0)*sp -
	63*pow(P0y,3.0)*Q2y*pow(sp,2.0) + 21*pow(P1y,2.0)*pow(Q2y,2.0)*sp + 70*pow(P0y,3.0)*Q2y*pow(sp,3.0) + 63*pow(P0y,3.0)*Q3y*pow(sp,2.0) + 21*pow(P1y,2.0)*pow(Q3y,2.0)*sp - 126*pow(P1y,3.0)*Q2y*pow(sp,2.0) -
	21*pow(P0y,3.0)*Q2y*pow(sp,4.0) - 70*pow(P0y,3.0)*Q3y*pow(sp,3.0) + 210*pow(P1y,3.0)*Q2y*pow(sp,3.0) + 126*pow(P1y,3.0)*Q3y*pow(sp,2.0) + 21*pow(P0y,3.0)*Q3y*pow(sp,4.0) - 84*pow(P1y,3.0)*Q2y*pow(sp,4.0) -
	210*pow(P1y,3.0)*Q3y*pow(sp,3.0) + 84*pow(P1y,3.0)*Q3y*pow(sp,4.0) - 21*pow(P0x,2.0)*pow(Q2x,2.0)*pow(sp,2.0) - 105*pow(P0x,2.0)*pow(Q2x,2.0)*pow(sp,3.0) + 21*pow(P0x,2.0)*pow(Q3x,2.0)*pow(sp,2.0) -
	21*pow(P1x,2.0)*pow(Q2x,2.0)*pow(sp,2.0) + 84*pow(P0x,2.0)*pow(Q2x,2.0)*pow(sp,4.0) + 7*pow(P0x,2.0)*pow(Q3x,2.0)*pow(sp,3.0) - 105*pow(P1x,2.0)*pow(Q2x,2.0)*pow(sp,3.0) + 21*pow(P1x,2.0)*pow(Q3x,2.0)*pow(sp,2.0) -
	21*pow(P0x,2.0)*pow(Q3x,2.0)*pow(sp,4.0) + 84*pow(P1x,2.0)*pow(Q2x,2.0)*pow(sp,4.0) + 7*pow(P1x,2.0)*pow(Q3x,2.0)*pow(sp,3.0) - 21*pow(P1x,2.0)*pow(Q3x,2.0)*pow(sp,4.0) - 48*pow(P0x,2.0)*pow(Q2y,2.0)*pow(sp,2.0) -
	48*pow(P0y,2.0)*pow(Q2x,2.0)*pow(sp,2.0) - 24*pow(P0x,2.0)*pow(Q2y,2.0)*pow(sp,3.0) + 12*pow(P0x,2.0)*pow(Q3y,2.0)*pow(sp,2.0) - 48*pow(P1x,2.0)*pow(Q2y,2.0)*pow(sp,2.0) - 24*pow(P0y,2.0)*pow(Q2x,2.0)*pow(sp,3.0) +
	12*pow(P0y,2.0)*pow(Q3x,2.0)*pow(sp,2.0) - 48*pow(P1y,2.0)*pow(Q2x,2.0)*pow(sp,2.0) + 48*pow(P0x,2.0)*pow(Q2y,2.0)*pow(sp,4.0) - 8*pow(P0x,2.0)*pow(Q3y,2.0)*pow(sp,3.0) - 24*pow(P1x,2.0)*pow(Q2y,2.0)*pow(sp,3.0) +
	12*pow(P1x,2.0)*pow(Q3y,2.0)*pow(sp,2.0) + 48*pow(P0y,2.0)*pow(Q2x,2.0)*pow(sp,4.0) - 8*pow(P0y,2.0)*pow(Q3x,2.0)*pow(sp,3.0) - 24*pow(P1y,2.0)*pow(Q2x,2.0)*pow(sp,3.0) + 12*pow(P1y,2.0)*pow(Q3x,2.0)*pow(sp,2.0) -
	12*pow(P0x,2.0)*pow(Q3y,2.0)*pow(sp,4.0) + 48*pow(P1x,2.0)*pow(Q2y,2.0)*pow(sp,4.0) - 8*pow(P1x,2.0)*pow(Q3y,2.0)*pow(sp,3.0) - 12*pow(P0y,2.0)*pow(Q3x,2.0)*pow(sp,4.0) + 48*pow(P1y,2.0)*pow(Q2x,2.0)*pow(sp,4.0) -
	8*pow(P1y,2.0)*pow(Q3x,2.0)*pow(sp,3.0) - 12*pow(P1x,2.0)*pow(Q3y,2.0)*pow(sp,4.0) - 12*pow(P1y,2.0)*pow(Q3x,2.0)*pow(sp,4.0) - 21*pow(P0y,2.0)*pow(Q2y,2.0)*pow(sp,2.0) - 105*pow(P0y,2.0)*pow(Q2y,2.0)*pow(sp,3.0) +
	21*pow(P0y,2.0)*pow(Q3y,2.0)*pow(sp,2.0) - 21*pow(P1y,2.0)*pow(Q2y,2.0)*pow(sp,2.0) + 84*pow(P0y,2.0)*pow(Q2y,2.0)*pow(sp,4.0) + 7*pow(P0y,2.0)*pow(Q3y,2.0)*pow(sp,3.0) - 105*pow(P1y,2.0)*pow(Q2y,2.0)*pow(sp,3.0) +
	21*pow(P1y,2.0)*pow(Q3y,2.0)*pow(sp,2.0) - 21*pow(P0y,2.0)*pow(Q3y,2.0)*pow(sp,4.0) + 84*pow(P1y,2.0)*pow(Q2y,2.0)*pow(sp,4.0) + 7*pow(P1y,2.0)*pow(Q3y,2.0)*pow(sp,3.0) - 21*pow(P1y,2.0)*pow(Q3y,2.0)*pow(sp,4.0) -
	42*P0x*P1x*pow(Q2x,2.0) + 14*P0x*pow(P1x,2.0)*Q2x - 28*pow(P0x,2.0)*P1x*Q2x - 42*P0x*P1x*pow(Q3x,2.0) - 14*P0x*pow(P1x,2.0)*Q3x + 28*pow(P0x,2.0)*P1x*Q3x - 14*pow(P0x,2.0)*P3x*Q2x + 14*pow(P0x,2.0)*P3x*Q3x -
	14*pow(P1x,2.0)*P3x*Q2x + 14*pow(P1x,2.0)*P3x*Q3x + 14*P0x*pow(P0y,2.0)*Q2x - 96*P0x*P1x*pow(Q2y,2.0) - 14*P0x*pow(P0y,2.0)*Q3x - 52*P0x*pow(P1y,2.0)*Q2x - 66*P1x*pow(P0y,2.0)*Q2x - 96*P0x*P1x*pow(Q3y,2.0) +
	52*P0x*pow(P1y,2.0)*Q3x + 66*P1x*pow(P0y,2.0)*Q3x + 16*P3x*pow(P0y,2.0)*Q2x - 16*P3x*pow(P0y,2.0)*Q3x + 16*P3x*pow(P1y,2.0)*Q2x - 16*P3x*pow(P1y,2.0)*Q3x + 14*pow(P0x,2.0)*P0y*Q2y - 96*P0y*P1y*pow(Q2x,2.0) -
	14*pow(P0x,2.0)*P0y*Q3y - 66*pow(P0x,2.0)*P1y*Q2y - 52*pow(P1x,2.0)*P0y*Q2y - 96*P0y*P1y*pow(Q3x,2.0) + 66*pow(P0x,2.0)*P1y*Q3y + 52*pow(P1x,2.0)*P0y*Q3y + 16*pow(P0x,2.0)*P3y*Q2y - 16*pow(P0x,2.0)*P3y*Q3y +
	16*pow(P1x,2.0)*P3y*Q2y - 16*pow(P1x,2.0)*P3y*Q3y - 42*P0y*P1y*pow(Q2y,2.0) + 14*P0y*pow(P1y,2.0)*Q2y - 28*pow(P0y,2.0)*P1y*Q2y - 42*P0y*P1y*pow(Q3y,2.0) - 14*P0y*pow(P1y,2.0)*Q3y + 28*pow(P0y,2.0)*P1y*Q3y -
	14*pow(P0y,2.0)*P3y*Q2y + 14*pow(P0y,2.0)*P3y*Q3y - 14*pow(P1y,2.0)*P3y*Q2y + 14*pow(P1y,2.0)*P3y*Q3y - 42*pow(P0x,2.0)*Q2x*Q3x - 42*pow(P1x,2.0)*Q2x*Q3x + 36*pow(P0y,2.0)*Q1x*Q2x - 36*pow(P0y,2.0)*Q1x*Q3x +
	36*pow(P1y,2.0)*Q1x*Q2x - 96*pow(P0y,2.0)*Q2x*Q3x - 36*pow(P1y,2.0)*Q1x*Q3x - 96*pow(P1y,2.0)*Q2x*Q3x + 36*pow(P0x,2.0)*Q1y*Q2y - 36*pow(P0x,2.0)*Q1y*Q3y + 36*pow(P1x,2.0)*Q1y*Q2y - 96*pow(P0x,2.0)*Q2y*Q3y -
	36*pow(P1x,2.0)*Q1y*Q3y - 96*pow(P1x,2.0)*Q2y*Q3y - 42*pow(P0y,2.0)*Q2y*Q3y - 42*pow(P1y,2.0)*Q2y*Q3y - 42*P0x*P1x*pow(Q2x,2.0)*sp - 42*P0x*P1x*pow(Q3x,2.0)*sp - 42*pow(P0x,2.0)*P3x*Q2x*sp + 42*pow(P0x,2.0)*P3x*Q3x*sp -
	42*pow(P1x,2.0)*P3x*Q2x*sp + 42*pow(P1x,2.0)*P3x*Q3x*sp + 48*P0x*P1x*pow(Q2y,2.0)*sp - 96*P0x*P1x*pow(Q3y,2.0)*sp + 48*P3x*pow(P0y,2.0)*Q2x*sp - 48*P3x*pow(P0y,2.0)*Q3x*sp + 48*P3x*pow(P1y,2.0)*Q2x*sp -
	48*P3x*pow(P1y,2.0)*Q3x*sp + 48*P0y*P1y*pow(Q2x,2.0)*sp - 96*P0y*P1y*pow(Q3x,2.0)*sp + 48*pow(P0x,2.0)*P3y*Q2y*sp - 48*pow(P0x,2.0)*P3y*Q3y*sp + 48*pow(P1x,2.0)*P3y*Q2y*sp - 48*pow(P1x,2.0)*P3y*Q3y*sp -
	42*P0y*P1y*pow(Q2y,2.0)*sp - 42*P0y*P1y*pow(Q3y,2.0)*sp - 42*pow(P0y,2.0)*P3y*Q2y*sp + 42*pow(P0y,2.0)*P3y*Q3y*sp - 42*pow(P1y,2.0)*P3y*Q2y*sp + 42*pow(P1y,2.0)*P3y*Q3y*sp + 42*pow(P0x,2.0)*Q1x*Q2x*sp -
	42*pow(P0x,2.0)*Q1x*Q3x*sp + 42*pow(P1x,2.0)*Q1x*Q2x*sp - 42*pow(P0x,2.0)*Q2x*Q3x*sp - 42*pow(P1x,2.0)*Q1x*Q3x*sp - 42*pow(P1x,2.0)*Q2x*Q3x*sp + 24*pow(P0y,2.0)*Q1x*Q2x*sp - 24*pow(P0y,2.0)*Q1x*Q3x*sp +
	24*pow(P1y,2.0)*Q1x*Q2x*sp - 24*pow(P0y,2.0)*Q2x*Q3x*sp - 24*pow(P1y,2.0)*Q1x*Q3x*sp - 24*pow(P1y,2.0)*Q2x*Q3x*sp + 24*pow(P0x,2.0)*Q1y*Q2y*sp - 24*pow(P0x,2.0)*Q1y*Q3y*sp + 24*pow(P1x,2.0)*Q1y*Q2y*sp -
	24*pow(P0x,2.0)*Q2y*Q3y*sp - 24*pow(P1x,2.0)*Q1y*Q3y*sp - 24*pow(P1x,2.0)*Q2y*Q3y*sp + 42*pow(P0y,2.0)*Q1y*Q2y*sp - 42*pow(P0y,2.0)*Q1y*Q3y*sp + 42*pow(P1y,2.0)*Q1y*Q2y*sp - 42*pow(P0y,2.0)*Q2y*Q3y*sp -
	42*pow(P1y,2.0)*Q1y*Q3y*sp - 42*pow(P1y,2.0)*Q2y*Q3y*sp + 42*P0x*P1x*pow(Q2x,2.0)*pow(sp,2.0) + 189*P0x*pow(P1x,2.0)*Q2x*pow(sp,2.0) + 210*P0x*P1x*pow(Q2x,2.0)*pow(sp,3.0) - 42*P0x*P1x*pow(Q3x,2.0)*pow(sp,2.0) -
	350*P0x*pow(P1x,2.0)*Q2x*pow(sp,3.0) - 189*P0x*pow(P1x,2.0)*Q3x*pow(sp,2.0) + 70*pow(P0x,2.0)*P1x*Q2x*pow(sp,3.0) - 189*pow(P0x,2.0)*P2x*Q2x*pow(sp,2.0) - 168*P0x*P1x*pow(Q2x,2.0)*pow(sp,4.0) -
	14*P0x*P1x*pow(Q3x,2.0)*pow(sp,3.0) + 147*P0x*pow(P1x,2.0)*Q2x*pow(sp,4.0) + 350*P0x*pow(P1x,2.0)*Q3x*pow(sp,3.0) - 42*pow(P0x,2.0)*P1x*Q2x*pow(sp,4.0) - 70*pow(P0x,2.0)*P1x*Q3x*pow(sp,3.0) +
	420*pow(P0x,2.0)*P2x*Q2x*pow(sp,3.0) + 189*pow(P0x,2.0)*P2x*Q3x*pow(sp,2.0) + 294*pow(P0x,2.0)*P3x*Q2x*pow(sp,2.0) - 189*pow(P1x,2.0)*P2x*Q2x*pow(sp,2.0) + 42*P0x*P1x*pow(Q3x,2.0)*pow(sp,4.0) -
	147*P0x*pow(P1x,2.0)*Q3x*pow(sp,4.0) + 42*pow(P0x,2.0)*P1x*Q3x*pow(sp,4.0) - 210*pow(P0x,2.0)*P2x*Q2x*pow(sp,4.0) - 420*pow(P0x,2.0)*P2x*Q3x*pow(sp,3.0) - 210*pow(P0x,2.0)*P3x*Q2x*pow(sp,3.0) -
	294*pow(P0x,2.0)*P3x*Q3x*pow(sp,2.0) + 420*pow(P1x,2.0)*P2x*Q2x*pow(sp,3.0) + 189*pow(P1x,2.0)*P2x*Q3x*pow(sp,2.0) + 294*pow(P1x,2.0)*P3x*Q2x*pow(sp,2.0) + 210*pow(P0x,2.0)*P2x*Q3x*pow(sp,4.0) +
	210*pow(P0x,2.0)*P3x*Q3x*pow(sp,3.0) - 210*pow(P1x,2.0)*P2x*Q2x*pow(sp,4.0) - 420*pow(P1x,2.0)*P2x*Q3x*pow(sp,3.0) - 210*pow(P1x,2.0)*P3x*Q2x*pow(sp,3.0) - 294*pow(P1x,2.0)*P3x*Q3x*pow(sp,2.0) +
	210*pow(P1x,2.0)*P2x*Q3x*pow(sp,4.0) + 210*pow(P1x,2.0)*P3x*Q3x*pow(sp,3.0) - 63*P0x*pow(P0y,2.0)*Q2x*pow(sp,2.0) + 96*P0x*P1x*pow(Q2y,2.0)*pow(sp,2.0) + 70*P0x*pow(P0y,2.0)*Q2x*pow(sp,3.0) +
	63*P0x*pow(P0y,2.0)*Q3x*pow(sp,2.0) + 126*P0x*pow(P1y,2.0)*Q2x*pow(sp,2.0) + 63*P1x*pow(P0y,2.0)*Q2x*pow(sp,2.0) + 48*P0x*P1x*pow(Q2y,2.0)*pow(sp,3.0) - 24*P0x*P1x*pow(Q3y,2.0)*pow(sp,2.0) -
	21*P0x*pow(P0y,2.0)*Q2x*pow(sp,4.0) - 70*P0x*pow(P0y,2.0)*Q3x*pow(sp,3.0) - 98*P0x*pow(P1y,2.0)*Q2x*pow(sp,3.0) - 126*P0x*pow(P1y,2.0)*Q3x*pow(sp,2.0) + 42*P1x*pow(P0y,2.0)*Q2x*pow(sp,3.0) -
	63*P1x*pow(P0y,2.0)*Q3x*pow(sp,2.0) - 126*P1x*pow(P1y,2.0)*Q2x*pow(sp,2.0) - 96*P0x*P1x*pow(Q2y,2.0)*pow(sp,4.0) + 16*P0x*P1x*pow(Q3y,2.0)*pow(sp,3.0) + 21*P0x*pow(P0y,2.0)*Q3x*pow(sp,4.0) +
	24*P0x*pow(P1y,2.0)*Q2x*pow(sp,4.0) + 98*P0x*pow(P1y,2.0)*Q3x*pow(sp,3.0) - 39*P1x*pow(P0y,2.0)*Q2x*pow(sp,4.0) - 42*P1x*pow(P0y,2.0)*Q3x*pow(sp,3.0) + 210*P1x*pow(P1y,2.0)*Q2x*pow(sp,3.0) +
	126*P1x*pow(P1y,2.0)*Q3x*pow(sp,2.0) + 168*P2x*pow(P0y,2.0)*Q2x*pow(sp,3.0) + 96*P3x*pow(P0y,2.0)*Q2x*pow(sp,2.0) + 24*P0x*P1x*pow(Q3y,2.0)*pow(sp,4.0) - 24*P0x*pow(P1y,2.0)*Q3x*pow(sp,4.0) +
	39*P1x*pow(P0y,2.0)*Q3x*pow(sp,4.0) - 84*P1x*pow(P1y,2.0)*Q2x*pow(sp,4.0) - 210*P1x*pow(P1y,2.0)*Q3x*pow(sp,3.0) - 120*P2x*pow(P0y,2.0)*Q2x*pow(sp,4.0) - 168*P2x*pow(P0y,2.0)*Q3x*pow(sp,3.0) +
	168*P2x*pow(P1y,2.0)*Q2x*pow(sp,3.0) - 120*P3x*pow(P0y,2.0)*Q2x*pow(sp,3.0) - 96*P3x*pow(P0y,2.0)*Q3x*pow(sp,2.0) + 96*P3x*pow(P1y,2.0)*Q2x*pow(sp,2.0) + 84*P1x*pow(P1y,2.0)*Q3x*pow(sp,4.0) +
	120*P2x*pow(P0y,2.0)*Q3x*pow(sp,4.0) - 120*P2x*pow(P1y,2.0)*Q2x*pow(sp,4.0) - 168*P2x*pow(P1y,2.0)*Q3x*pow(sp,3.0) + 120*P3x*pow(P0y,2.0)*Q3x*pow(sp,3.0) - 120*P3x*pow(P1y,2.0)*Q2x*pow(sp,3.0) -
	96*P3x*pow(P1y,2.0)*Q3x*pow(sp,2.0) + 120*P2x*pow(P1y,2.0)*Q3x*pow(sp,4.0) + 120*P3x*pow(P1y,2.0)*Q3x*pow(sp,3.0) - 63*pow(P0x,2.0)*P0y*Q2y*pow(sp,2.0) + 96*P0y*P1y*pow(Q2x,2.0)*pow(sp,2.0) +
	70*pow(P0x,2.0)*P0y*Q2y*pow(sp,3.0) + 63*pow(P0x,2.0)*P0y*Q3y*pow(sp,2.0) + 63*pow(P0x,2.0)*P1y*Q2y*pow(sp,2.0) + 126*pow(P1x,2.0)*P0y*Q2y*pow(sp,2.0) + 48*P0y*P1y*pow(Q2x,2.0)*pow(sp,3.0) -
	24*P0y*P1y*pow(Q3x,2.0)*pow(sp,2.0) - 21*pow(P0x,2.0)*P0y*Q2y*pow(sp,4.0) - 70*pow(P0x,2.0)*P0y*Q3y*pow(sp,3.0) + 42*pow(P0x,2.0)*P1y*Q2y*pow(sp,3.0) - 63*pow(P0x,2.0)*P1y*Q3y*pow(sp,2.0) -
	98*pow(P1x,2.0)*P0y*Q2y*pow(sp,3.0) - 126*pow(P1x,2.0)*P0y*Q3y*pow(sp,2.0) - 126*pow(P1x,2.0)*P1y*Q2y*pow(sp,2.0) - 96*P0y*P1y*pow(Q2x,2.0)*pow(sp,4.0) + 16*P0y*P1y*pow(Q3x,2.0)*pow(sp,3.0) +
	21*pow(P0x,2.0)*P0y*Q3y*pow(sp,4.0) - 39*pow(P0x,2.0)*P1y*Q2y*pow(sp,4.0) - 42*pow(P0x,2.0)*P1y*Q3y*pow(sp,3.0) + 168*pow(P0x,2.0)*P2y*Q2y*pow(sp,3.0) + 96*pow(P0x,2.0)*P3y*Q2y*pow(sp,2.0) +
	24*pow(P1x,2.0)*P0y*Q2y*pow(sp,4.0) + 98*pow(P1x,2.0)*P0y*Q3y*pow(sp,3.0) + 210*pow(P1x,2.0)*P1y*Q2y*pow(sp,3.0) + 126*pow(P1x,2.0)*P1y*Q3y*pow(sp,2.0) + 24*P0y*P1y*pow(Q3x,2.0)*pow(sp,4.0) +
	39*pow(P0x,2.0)*P1y*Q3y*pow(sp,4.0) - 120*pow(P0x,2.0)*P2y*Q2y*pow(sp,4.0) - 168*pow(P0x,2.0)*P2y*Q3y*pow(sp,3.0) - 120*pow(P0x,2.0)*P3y*Q2y*pow(sp,3.0) - 96*pow(P0x,2.0)*P3y*Q3y*pow(sp,2.0) -
	24*pow(P1x,2.0)*P0y*Q3y*pow(sp,4.0) - 84*pow(P1x,2.0)*P1y*Q2y*pow(sp,4.0) - 210*pow(P1x,2.0)*P1y*Q3y*pow(sp,3.0) + 168*pow(P1x,2.0)*P2y*Q2y*pow(sp,3.0) + 96*pow(P1x,2.0)*P3y*Q2y*pow(sp,2.0) +
	120*pow(P0x,2.0)*P2y*Q3y*pow(sp,4.0) + 120*pow(P0x,2.0)*P3y*Q3y*pow(sp,3.0) + 84*pow(P1x,2.0)*P1y*Q3y*pow(sp,4.0) - 120*pow(P1x,2.0)*P2y*Q2y*pow(sp,4.0) - 168*pow(P1x,2.0)*P2y*Q3y*pow(sp,3.0) -
	120*pow(P1x,2.0)*P3y*Q2y*pow(sp,3.0) - 96*pow(P1x,2.0)*P3y*Q3y*pow(sp,2.0) + 120*pow(P1x,2.0)*P2y*Q3y*pow(sp,4.0) + 120*pow(P1x,2.0)*P3y*Q3y*pow(sp,3.0) + 42*P0y*P1y*pow(Q2y,2.0)*pow(sp,2.0) +
	189*P0y*pow(P1y,2.0)*Q2y*pow(sp,2.0) + 210*P0y*P1y*pow(Q2y,2.0)*pow(sp,3.0) - 42*P0y*P1y*pow(Q3y,2.0)*pow(sp,2.0) - 350*P0y*pow(P1y,2.0)*Q2y*pow(sp,3.0) - 189*P0y*pow(P1y,2.0)*Q3y*pow(sp,2.0) +
	70*pow(P0y,2.0)*P1y*Q2y*pow(sp,3.0) - 189*pow(P0y,2.0)*P2y*Q2y*pow(sp,2.0) - 168*P0y*P1y*pow(Q2y,2.0)*pow(sp,4.0) - 14*P0y*P1y*pow(Q3y,2.0)*pow(sp,3.0) + 147*P0y*pow(P1y,2.0)*Q2y*pow(sp,4.0) +
	350*P0y*pow(P1y,2.0)*Q3y*pow(sp,3.0) - 42*pow(P0y,2.0)*P1y*Q2y*pow(sp,4.0) - 70*pow(P0y,2.0)*P1y*Q3y*pow(sp,3.0) + 420*pow(P0y,2.0)*P2y*Q2y*pow(sp,3.0) + 189*pow(P0y,2.0)*P2y*Q3y*pow(sp,2.0) +
	294*pow(P0y,2.0)*P3y*Q2y*pow(sp,2.0) - 189*pow(P1y,2.0)*P2y*Q2y*pow(sp,2.0) + 42*P0y*P1y*pow(Q3y,2.0)*pow(sp,4.0) - 147*P0y*pow(P1y,2.0)*Q3y*pow(sp,4.0) + 42*pow(P0y,2.0)*P1y*Q3y*pow(sp,4.0) -
	210*pow(P0y,2.0)*P2y*Q2y*pow(sp,4.0) - 420*pow(P0y,2.0)*P2y*Q3y*pow(sp,3.0) - 210*pow(P0y,2.0)*P3y*Q2y*pow(sp,3.0) - 294*pow(P0y,2.0)*P3y*Q3y*pow(sp,2.0) + 420*pow(P1y,2.0)*P2y*Q2y*pow(sp,3.0) +
	189*pow(P1y,2.0)*P2y*Q3y*pow(sp,2.0) + 294*pow(P1y,2.0)*P3y*Q2y*pow(sp,2.0) + 210*pow(P0y,2.0)*P2y*Q3y*pow(sp,4.0) + 210*pow(P0y,2.0)*P3y*Q3y*pow(sp,3.0) - 210*pow(P1y,2.0)*P2y*Q2y*pow(sp,4.0) -
	420*pow(P1y,2.0)*P2y*Q3y*pow(sp,3.0) - 210*pow(P1y,2.0)*P3y*Q2y*pow(sp,3.0) - 294*pow(P1y,2.0)*P3y*Q3y*pow(sp,2.0) + 210*pow(P1y,2.0)*P2y*Q3y*pow(sp,4.0) + 210*pow(P1y,2.0)*P3y*Q3y*pow(sp,3.0) +
	126*pow(P0x,2.0)*Q1x*Q2x*pow(sp,2.0) - 378*pow(P0x,2.0)*Q1x*Q2x*pow(sp,3.0) - 126*pow(P0x,2.0)*Q1x*Q3x*pow(sp,2.0) + 126*pow(P1x,2.0)*Q1x*Q2x*pow(sp,2.0) + 210*pow(P0x,2.0)*Q1x*Q2x*pow(sp,4.0) +
	378*pow(P0x,2.0)*Q1x*Q3x*pow(sp,3.0) - 378*pow(P1x,2.0)*Q1x*Q2x*pow(sp,3.0) - 126*pow(P1x,2.0)*Q1x*Q3x*pow(sp,2.0) - 210*pow(P0x,2.0)*Q1x*Q3x*pow(sp,4.0) + 98*pow(P0x,2.0)*Q2x*Q3x*pow(sp,3.0) +
	210*pow(P1x,2.0)*Q1x*Q2x*pow(sp,4.0) + 378*pow(P1x,2.0)*Q1x*Q3x*pow(sp,3.0) - 63*pow(P0x,2.0)*Q2x*Q3x*pow(sp,4.0) - 210*pow(P1x,2.0)*Q1x*Q3x*pow(sp,4.0) + 98*pow(P1x,2.0)*Q2x*Q3x*pow(sp,3.0) -
	63*pow(P1x,2.0)*Q2x*Q3x*pow(sp,4.0) - 36*pow(P0y,2.0)*Q1x*Q2x*pow(sp,2.0) - 144*pow(P0y,2.0)*Q1x*Q2x*pow(sp,3.0) + 36*pow(P0y,2.0)*Q1x*Q3x*pow(sp,2.0) - 36*pow(P1y,2.0)*Q1x*Q2x*pow(sp,2.0) +
	120*pow(P0y,2.0)*Q1x*Q2x*pow(sp,4.0) + 144*pow(P0y,2.0)*Q1x*Q3x*pow(sp,3.0) + 36*pow(P0y,2.0)*Q2x*Q3x*pow(sp,2.0) - 144*pow(P1y,2.0)*Q1x*Q2x*pow(sp,3.0) + 36*pow(P1y,2.0)*Q1x*Q3x*pow(sp,2.0) -
	120*pow(P0y,2.0)*Q1x*Q3x*pow(sp,4.0) + 32*pow(P0y,2.0)*Q2x*Q3x*pow(sp,3.0) + 120*pow(P1y,2.0)*Q1x*Q2x*pow(sp,4.0) + 144*pow(P1y,2.0)*Q1x*Q3x*pow(sp,3.0) + 36*pow(P1y,2.0)*Q2x*Q3x*pow(sp,2.0) -
	36*pow(P0y,2.0)*Q2x*Q3x*pow(sp,4.0) - 120*pow(P1y,2.0)*Q1x*Q3x*pow(sp,4.0) + 32*pow(P1y,2.0)*Q2x*Q3x*pow(sp,3.0) - 36*pow(P1y,2.0)*Q2x*Q3x*pow(sp,4.0) - 36*pow(P0x,2.0)*Q1y*Q2y*pow(sp,2.0) -
	144*pow(P0x,2.0)*Q1y*Q2y*pow(sp,3.0) + 36*pow(P0x,2.0)*Q1y*Q3y*pow(sp,2.0) - 36*pow(P1x,2.0)*Q1y*Q2y*pow(sp,2.0) + 120*pow(P0x,2.0)*Q1y*Q2y*pow(sp,4.0) + 144*pow(P0x,2.0)*Q1y*Q3y*pow(sp,3.0) +
	36*pow(P0x,2.0)*Q2y*Q3y*pow(sp,2.0) - 144*pow(P1x,2.0)*Q1y*Q2y*pow(sp,3.0) + 36*pow(P1x,2.0)*Q1y*Q3y*pow(sp,2.0) - 120*pow(P0x,2.0)*Q1y*Q3y*pow(sp,4.0) + 32*pow(P0x,2.0)*Q2y*Q3y*pow(sp,3.0) +
	120*pow(P1x,2.0)*Q1y*Q2y*pow(sp,4.0) + 144*pow(P1x,2.0)*Q1y*Q3y*pow(sp,3.0) + 36*pow(P1x,2.0)*Q2y*Q3y*pow(sp,2.0) - 36*pow(P0x,2.0)*Q2y*Q3y*pow(sp,4.0) - 120*pow(P1x,2.0)*Q1y*Q3y*pow(sp,4.0) +
	32*pow(P1x,2.0)*Q2y*Q3y*pow(sp,3.0) - 36*pow(P1x,2.0)*Q2y*Q3y*pow(sp,4.0) + 126*pow(P0y,2.0)*Q1y*Q2y*pow(sp,2.0) - 378*pow(P0y,2.0)*Q1y*Q2y*pow(sp,3.0) - 126*pow(P0y,2.0)*Q1y*Q3y*pow(sp,2.0) +
	126*pow(P1y,2.0)*Q1y*Q2y*pow(sp,2.0) + 210*pow(P0y,2.0)*Q1y*Q2y*pow(sp,4.0) + 378*pow(P0y,2.0)*Q1y*Q3y*pow(sp,3.0) - 378*pow(P1y,2.0)*Q1y*Q2y*pow(sp,3.0) - 126*pow(P1y,2.0)*Q1y*Q3y*pow(sp,2.0) -
	210*pow(P0y,2.0)*Q1y*Q3y*pow(sp,4.0) + 98*pow(P0y,2.0)*Q2y*Q3y*pow(sp,3.0) + 210*pow(P1y,2.0)*Q1y*Q2y*pow(sp,4.0) + 378*pow(P1y,2.0)*Q1y*Q3y*pow(sp,3.0) - 63*pow(P0y,2.0)*Q2y*Q3y*pow(sp,4.0) -
	210*pow(P1y,2.0)*Q1y*Q3y*pow(sp,4.0) + 98*pow(P1y,2.0)*Q2y*Q3y*pow(sp,3.0) - 63*pow(P1y,2.0)*Q2y*Q3y*pow(sp,4.0) + 28*P0x*P1x*P3x*Q2x - 28*P0x*P1x*P3x*Q3x + 38*P0x*P1x*P0y*Q2y + 38*P0x*P0y*P1y*Q2x -
	38*P0x*P1x*P0y*Q3y + 66*P0x*P1x*P1y*Q2y - 38*P0x*P0y*P1y*Q3x + 66*P1x*P0y*P1y*Q2x - 66*P0x*P1x*P1y*Q3y - 30*P0x*P3x*P0y*Q2y - 30*P0x*P0y*P3y*Q2x - 66*P1x*P0y*P1y*Q3x - 32*P0x*P1x*P3y*Q2y + 30*P0x*P3x*P0y*Q3y +
	30*P0x*P3x*P1y*Q2y + 30*P0x*P0y*P3y*Q3x + 30*P0x*P1y*P3y*Q2x + 30*P1x*P3x*P0y*Q2y + 30*P1x*P0y*P3y*Q2x - 32*P3x*P0y*P1y*Q2x + 32*P0x*P1x*P3y*Q3y - 30*P0x*P3x*P1y*Q3y - 30*P0x*P1y*P3y*Q3x - 30*P1x*P3x*P0y*Q3y -
	30*P1x*P3x*P1y*Q2y - 30*P1x*P0y*P3y*Q3x - 30*P1x*P1y*P3y*Q2x + 32*P3x*P0y*P1y*Q3x + 30*P1x*P3x*P1y*Q3y + 30*P1x*P1y*P3y*Q3x + 28*P0y*P1y*P3y*Q2y - 28*P0y*P1y*P3y*Q3y + 84*P0x*P1x*Q2x*Q3x - 36*P0x*P0y*Q1x*Q2y -
	36*P0x*P0y*Q2x*Q1y - 72*P0x*P1x*Q1y*Q2y + 36*P0x*P0y*Q1x*Q3y - 54*P0x*P0y*Q2x*Q2y + 36*P0x*P0y*Q3x*Q1y + 36*P0x*P1y*Q1x*Q2y + 36*P0x*P1y*Q2x*Q1y + 36*P1x*P0y*Q1x*Q2y + 36*P1x*P0y*Q2x*Q1y - 72*P0y*P1y*Q1x*Q2x +
	72*P0x*P1x*Q1y*Q3y + 54*P0x*P0y*Q2x*Q3y + 54*P0x*P0y*Q3x*Q2y - 36*P0x*P1y*Q1x*Q3y + 54*P0x*P1y*Q2x*Q2y - 36*P0x*P1y*Q3x*Q1y - 36*P1x*P0y*Q1x*Q3y + 54*P1x*P0y*Q2x*Q2y - 36*P1x*P0y*Q3x*Q1y - 36*P1x*P1y*Q1x*Q2y -
	36*P1x*P1y*Q2x*Q1y + 72*P0y*P1y*Q1x*Q3x + 192*P0x*P1x*Q2y*Q3y - 54*P0x*P0y*Q3x*Q3y - 54*P0x*P1y*Q2x*Q3y - 54*P0x*P1y*Q3x*Q2y - 54*P1x*P0y*Q2x*Q3y - 54*P1x*P0y*Q3x*Q2y + 36*P1x*P1y*Q1x*Q3y - 54*P1x*P1y*Q2x*Q2y +
	36*P1x*P1y*Q3x*Q1y + 192*P0y*P1y*Q2x*Q3x + 54*P0x*P1y*Q3x*Q3y + 54*P1x*P0y*Q3x*Q3y + 54*P1x*P1y*Q2x*Q3y + 54*P1x*P1y*Q3x*Q2y - 54*P1x*P1y*Q3x*Q3y + 84*P0y*P1y*Q2y*Q3y + 84*P0x*P1x*P3x*Q2x*sp - 84*P0x*P1x*P3x*Q3x*sp -
	90*P0x*P3x*P0y*Q2y*sp - 90*P0x*P0y*P3y*Q2x*sp - 96*P0x*P1x*P3y*Q2y*sp + 90*P0x*P3x*P0y*Q3y*sp + 90*P0x*P3x*P1y*Q2y*sp + 90*P0x*P0y*P3y*Q3x*sp + 90*P0x*P1y*P3y*Q2x*sp + 90*P1x*P3x*P0y*Q2y*sp + 90*P1x*P0y*P3y*Q2x*sp -
	96*P3x*P0y*P1y*Q2x*sp + 96*P0x*P1x*P3y*Q3y*sp - 90*P0x*P3x*P1y*Q3y*sp - 90*P0x*P1y*P3y*Q3x*sp - 90*P1x*P3x*P0y*Q3y*sp - 90*P1x*P3x*P1y*Q2y*sp - 90*P1x*P0y*P3y*Q3x*sp - 90*P1x*P1y*P3y*Q2x*sp + 96*P3x*P0y*P1y*Q3x*sp +
	90*P1x*P3x*P1y*Q3y*sp + 90*P1x*P1y*P3y*Q3x*sp + 84*P0y*P1y*P3y*Q2y*sp - 84*P0y*P1y*P3y*Q3y*sp - 84*P0x*P1x*Q1x*Q2x*sp + 84*P0x*P1x*Q1x*Q3x*sp + 84*P0x*P1x*Q2x*Q3x*sp + 18*P0x*P0y*Q1x*Q2y*sp + 18*P0x*P0y*Q2x*Q1y*sp -
	48*P0x*P1x*Q1y*Q2y*sp - 18*P0x*P0y*Q1x*Q3y*sp + 90*P0x*P0y*Q2x*Q2y*sp - 18*P0x*P0y*Q3x*Q1y*sp - 18*P0x*P1y*Q1x*Q2y*sp - 18*P0x*P1y*Q2x*Q1y*sp - 18*P1x*P0y*Q1x*Q2y*sp - 18*P1x*P0y*Q2x*Q1y*sp - 48*P0y*P1y*Q1x*Q2x*sp +
	48*P0x*P1x*Q1y*Q3y*sp - 18*P0x*P0y*Q2x*Q3y*sp - 18*P0x*P0y*Q3x*Q2y*sp + 18*P0x*P1y*Q1x*Q3y*sp - 90*P0x*P1y*Q2x*Q2y*sp + 18*P0x*P1y*Q3x*Q1y*sp + 18*P1x*P0y*Q1x*Q3y*sp - 90*P1x*P0y*Q2x*Q2y*sp + 18*P1x*P0y*Q3x*Q1y*sp +
	18*P1x*P1y*Q1x*Q2y*sp + 18*P1x*P1y*Q2x*Q1y*sp + 48*P0y*P1y*Q1x*Q3x*sp + 48*P0x*P1x*Q2y*Q3y*sp - 54*P0x*P0y*Q3x*Q3y*sp + 18*P0x*P1y*Q2x*Q3y*sp + 18*P0x*P1y*Q3x*Q2y*sp + 18*P1x*P0y*Q2x*Q3y*sp + 18*P1x*P0y*Q3x*Q2y*sp -
	18*P1x*P1y*Q1x*Q3y*sp + 90*P1x*P1y*Q2x*Q2y*sp - 18*P1x*P1y*Q3x*Q1y*sp + 48*P0y*P1y*Q2x*Q3x*sp + 54*P0x*P1y*Q3x*Q3y*sp + 54*P1x*P0y*Q3x*Q3y*sp - 18*P1x*P1y*Q2x*Q3y*sp - 18*P1x*P1y*Q3x*Q2y*sp - 54*P1x*P1y*Q3x*Q3y*sp -
	84*P0y*P1y*Q1y*Q2y*sp + 84*P0y*P1y*Q1y*Q3y*sp + 84*P0y*P1y*Q2y*Q3y*sp + 378*P0x*P1x*P2x*Q2x*pow(sp,2.0) - 840*P0x*P1x*P2x*Q2x*pow(sp,3.0) - 378*P0x*P1x*P2x*Q3x*pow(sp,2.0) - 588*P0x*P1x*P3x*Q2x*pow(sp,2.0) +
	420*P0x*P1x*P2x*Q2x*pow(sp,4.0) + 840*P0x*P1x*P2x*Q3x*pow(sp,3.0) + 420*P0x*P1x*P3x*Q2x*pow(sp,3.0) + 588*P0x*P1x*P3x*Q3x*pow(sp,2.0) - 420*P0x*P1x*P2x*Q3x*pow(sp,4.0) - 420*P0x*P1x*P3x*Q3x*pow(sp,3.0) -
	63*P0x*P1x*P0y*Q2y*pow(sp,2.0) - 63*P0x*P0y*P1y*Q2x*pow(sp,2.0) + 28*P0x*P1x*P0y*Q2y*pow(sp,3.0) + 63*P0x*P1x*P0y*Q3y*pow(sp,2.0) + 63*P0x*P1x*P1y*Q2y*pow(sp,2.0) - 189*P0x*P2x*P0y*Q2y*pow(sp,2.0) +
	28*P0x*P0y*P1y*Q2x*pow(sp,3.0) + 63*P0x*P0y*P1y*Q3x*pow(sp,2.0) - 189*P0x*P0y*P2y*Q2x*pow(sp,2.0) + 63*P1x*P0y*P1y*Q2x*pow(sp,2.0) - 3*P0x*P1x*P0y*Q2y*pow(sp,4.0) - 28*P0x*P1x*P0y*Q3y*pow(sp,3.0) -
	252*P0x*P1x*P1y*Q2y*pow(sp,3.0) - 63*P0x*P1x*P1y*Q3y*pow(sp,2.0) + 252*P0x*P2x*P0y*Q2y*pow(sp,3.0) + 189*P0x*P2x*P0y*Q3y*pow(sp,2.0) + 189*P0x*P2x*P1y*Q2y*pow(sp,2.0) + 198*P0x*P3x*P0y*Q2y*pow(sp,2.0) -
	3*P0x*P0y*P1y*Q2x*pow(sp,4.0) - 28*P0x*P0y*P1y*Q3x*pow(sp,3.0) + 252*P0x*P0y*P2y*Q2x*pow(sp,3.0) + 189*P0x*P0y*P2y*Q3x*pow(sp,2.0) + 198*P0x*P0y*P3y*Q2x*pow(sp,2.0) + 189*P0x*P1y*P2y*Q2x*pow(sp,2.0) +
	189*P1x*P2x*P0y*Q2y*pow(sp,2.0) - 252*P1x*P0y*P1y*Q2x*pow(sp,3.0) - 63*P1x*P0y*P1y*Q3x*pow(sp,2.0) + 189*P1x*P0y*P2y*Q2x*pow(sp,2.0) + 3*P0x*P1x*P0y*Q3y*pow(sp,4.0) + 123*P0x*P1x*P1y*Q2y*pow(sp,4.0) +
	252*P0x*P1x*P1y*Q3y*pow(sp,3.0) - 336*P0x*P1x*P2y*Q2y*pow(sp,3.0) - 192*P0x*P1x*P3y*Q2y*pow(sp,2.0) - 90*P0x*P2x*P0y*Q2y*pow(sp,4.0) - 252*P0x*P2x*P0y*Q3y*pow(sp,3.0) - 252*P0x*P2x*P1y*Q2y*pow(sp,3.0) -
	189*P0x*P2x*P1y*Q3y*pow(sp,2.0) - 90*P0x*P3x*P0y*Q2y*pow(sp,3.0) - 198*P0x*P3x*P0y*Q3y*pow(sp,2.0) - 198*P0x*P3x*P1y*Q2y*pow(sp,2.0) + 3*P0x*P0y*P1y*Q3x*pow(sp,4.0) - 90*P0x*P0y*P2y*Q2x*pow(sp,4.0) -
	252*P0x*P0y*P2y*Q3x*pow(sp,3.0) - 90*P0x*P0y*P3y*Q2x*pow(sp,3.0) - 198*P0x*P0y*P3y*Q3x*pow(sp,2.0) - 252*P0x*P1y*P2y*Q2x*pow(sp,3.0) - 189*P0x*P1y*P2y*Q3x*pow(sp,2.0) - 198*P0x*P1y*P3y*Q2x*pow(sp,2.0) -
	252*P1x*P2x*P0y*Q2y*pow(sp,3.0) - 189*P1x*P2x*P0y*Q3y*pow(sp,2.0) - 189*P1x*P2x*P1y*Q2y*pow(sp,2.0) - 198*P1x*P3x*P0y*Q2y*pow(sp,2.0) + 123*P1x*P0y*P1y*Q2x*pow(sp,4.0) + 252*P1x*P0y*P1y*Q3x*pow(sp,3.0) -
	252*P1x*P0y*P2y*Q2x*pow(sp,3.0) - 189*P1x*P0y*P2y*Q3x*pow(sp,2.0) - 198*P1x*P0y*P3y*Q2x*pow(sp,2.0) - 189*P1x*P1y*P2y*Q2x*pow(sp,2.0) - 336*P2x*P0y*P1y*Q2x*pow(sp,3.0) - 192*P3x*P0y*P1y*Q2x*pow(sp,2.0) -
	123*P0x*P1x*P1y*Q3y*pow(sp,4.0) + 240*P0x*P1x*P2y*Q2y*pow(sp,4.0) + 336*P0x*P1x*P2y*Q3y*pow(sp,3.0) + 240*P0x*P1x*P3y*Q2y*pow(sp,3.0) + 192*P0x*P1x*P3y*Q3y*pow(sp,2.0) + 90*P0x*P2x*P0y*Q3y*pow(sp,4.0) +
	90*P0x*P2x*P1y*Q2y*pow(sp,4.0) + 252*P0x*P2x*P1y*Q3y*pow(sp,3.0) + 90*P0x*P3x*P0y*Q3y*pow(sp,3.0) + 90*P0x*P3x*P1y*Q2y*pow(sp,3.0) + 198*P0x*P3x*P1y*Q3y*pow(sp,2.0) + 90*P0x*P0y*P2y*Q3x*pow(sp,4.0) +
	90*P0x*P0y*P3y*Q3x*pow(sp,3.0) + 90*P0x*P1y*P2y*Q2x*pow(sp,4.0) + 252*P0x*P1y*P2y*Q3x*pow(sp,3.0) + 90*P0x*P1y*P3y*Q2x*pow(sp,3.0) + 198*P0x*P1y*P3y*Q3x*pow(sp,2.0) + 90*P1x*P2x*P0y*Q2y*pow(sp,4.0) +
	252*P1x*P2x*P0y*Q3y*pow(sp,3.0) + 252*P1x*P2x*P1y*Q2y*pow(sp,3.0) + 189*P1x*P2x*P1y*Q3y*pow(sp,2.0) + 90*P1x*P3x*P0y*Q2y*pow(sp,3.0) + 198*P1x*P3x*P0y*Q3y*pow(sp,2.0) + 198*P1x*P3x*P1y*Q2y*pow(sp,2.0) -
	123*P1x*P0y*P1y*Q3x*pow(sp,4.0) + 90*P1x*P0y*P2y*Q2x*pow(sp,4.0) + 252*P1x*P0y*P2y*Q3x*pow(sp,3.0) + 90*P1x*P0y*P3y*Q2x*pow(sp,3.0) + 198*P1x*P0y*P3y*Q3x*pow(sp,2.0) + 252*P1x*P1y*P2y*Q2x*pow(sp,3.0) +
	189*P1x*P1y*P2y*Q3x*pow(sp,2.0) + 198*P1x*P1y*P3y*Q2x*pow(sp,2.0) + 240*P2x*P0y*P1y*Q2x*pow(sp,4.0) + 336*P2x*P0y*P1y*Q3x*pow(sp,3.0) + 240*P3x*P0y*P1y*Q2x*pow(sp,3.0) + 192*P3x*P0y*P1y*Q3x*pow(sp,2.0) -
	240*P0x*P1x*P2y*Q3y*pow(sp,4.0) - 240*P0x*P1x*P3y*Q3y*pow(sp,3.0) - 90*P0x*P2x*P1y*Q3y*pow(sp,4.0) - 90*P0x*P3x*P1y*Q3y*pow(sp,3.0) - 90*P0x*P1y*P2y*Q3x*pow(sp,4.0) - 90*P0x*P1y*P3y*Q3x*pow(sp,3.0) -
	90*P1x*P2x*P0y*Q3y*pow(sp,4.0) - 90*P1x*P2x*P1y*Q2y*pow(sp,4.0) - 252*P1x*P2x*P1y*Q3y*pow(sp,3.0) - 90*P1x*P3x*P0y*Q3y*pow(sp,3.0) - 90*P1x*P3x*P1y*Q2y*pow(sp,3.0) - 198*P1x*P3x*P1y*Q3y*pow(sp,2.0) -
	90*P1x*P0y*P2y*Q3x*pow(sp,4.0) - 90*P1x*P0y*P3y*Q3x*pow(sp,3.0) - 90*P1x*P1y*P2y*Q2x*pow(sp,4.0) - 252*P1x*P1y*P2y*Q3x*pow(sp,3.0) - 90*P1x*P1y*P3y*Q2x*pow(sp,3.0) - 198*P1x*P1y*P3y*Q3x*pow(sp,2.0) -
	240*P2x*P0y*P1y*Q3x*pow(sp,4.0) - 240*P3x*P0y*P1y*Q3x*pow(sp,3.0) + 90*P1x*P2x*P1y*Q3y*pow(sp,4.0) + 90*P1x*P3x*P1y*Q3y*pow(sp,3.0) + 90*P1x*P1y*P2y*Q3x*pow(sp,4.0) + 90*P1x*P1y*P3y*Q3x*pow(sp,3.0) +
	378*P0y*P1y*P2y*Q2y*pow(sp,2.0) - 840*P0y*P1y*P2y*Q2y*pow(sp,3.0) - 378*P0y*P1y*P2y*Q3y*pow(sp,2.0) - 588*P0y*P1y*P3y*Q2y*pow(sp,2.0) + 420*P0y*P1y*P2y*Q2y*pow(sp,4.0) + 840*P0y*P1y*P2y*Q3y*pow(sp,3.0) +
	420*P0y*P1y*P3y*Q2y*pow(sp,3.0) + 588*P0y*P1y*P3y*Q3y*pow(sp,2.0) - 420*P0y*P1y*P2y*Q3y*pow(sp,4.0) - 420*P0y*P1y*P3y*Q3y*pow(sp,3.0) - 252*P0x*P1x*Q1x*Q2x*pow(sp,2.0) + 756*P0x*P1x*Q1x*Q2x*pow(sp,3.0) +
	252*P0x*P1x*Q1x*Q3x*pow(sp,2.0) - 420*P0x*P1x*Q1x*Q2x*pow(sp,4.0) - 756*P0x*P1x*Q1x*Q3x*pow(sp,3.0) + 420*P0x*P1x*Q1x*Q3x*pow(sp,4.0) - 196*P0x*P1x*Q2x*Q3x*pow(sp,3.0) + 126*P0x*P1x*Q2x*Q3x*pow(sp,4.0) +
	162*P0x*P0y*Q1x*Q2y*pow(sp,2.0) + 162*P0x*P0y*Q2x*Q1y*pow(sp,2.0) + 72*P0x*P1x*Q1y*Q2y*pow(sp,2.0) - 234*P0x*P0y*Q1x*Q2y*pow(sp,3.0) - 162*P0x*P0y*Q1x*Q3y*pow(sp,2.0) - 234*P0x*P0y*Q2x*Q1y*pow(sp,3.0) +
	54*P0x*P0y*Q2x*Q2y*pow(sp,2.0) - 162*P0x*P0y*Q3x*Q1y*pow(sp,2.0) - 162*P0x*P1y*Q1x*Q2y*pow(sp,2.0) - 162*P0x*P1y*Q2x*Q1y*pow(sp,2.0) - 162*P1x*P0y*Q1x*Q2y*pow(sp,2.0) - 162*P1x*P0y*Q2x*Q1y*pow(sp,2.0) +
	72*P0y*P1y*Q1x*Q2x*pow(sp,2.0) + 288*P0x*P1x*Q1y*Q2y*pow(sp,3.0) - 72*P0x*P1x*Q1y*Q3y*pow(sp,2.0) + 90*P0x*P0y*Q1x*Q2y*pow(sp,4.0) + 234*P0x*P0y*Q1x*Q3y*pow(sp,3.0) + 90*P0x*P0y*Q2x*Q1y*pow(sp,4.0) -
	162*P0x*P0y*Q2x*Q2y*pow(sp,3.0) - 36*P0x*P0y*Q2x*Q3y*pow(sp,2.0) + 234*P0x*P0y*Q3x*Q1y*pow(sp,3.0) - 36*P0x*P0y*Q3x*Q2y*pow(sp,2.0) + 234*P0x*P1y*Q1x*Q2y*pow(sp,3.0) + 162*P0x*P1y*Q1x*Q3y*pow(sp,2.0) +
	234*P0x*P1y*Q2x*Q1y*pow(sp,3.0) - 54*P0x*P1y*Q2x*Q2y*pow(sp,2.0) + 162*P0x*P1y*Q3x*Q1y*pow(sp,2.0) + 234*P1x*P0y*Q1x*Q2y*pow(sp,3.0) + 162*P1x*P0y*Q1x*Q3y*pow(sp,2.0) + 234*P1x*P0y*Q2x*Q1y*pow(sp,3.0) -
	54*P1x*P0y*Q2x*Q2y*pow(sp,2.0) + 162*P1x*P0y*Q3x*Q1y*pow(sp,2.0) + 162*P1x*P1y*Q1x*Q2y*pow(sp,2.0) + 162*P1x*P1y*Q2x*Q1y*pow(sp,2.0) + 288*P0y*P1y*Q1x*Q2x*pow(sp,3.0) - 72*P0y*P1y*Q1x*Q3x*pow(sp,2.0) -
	240*P0x*P1x*Q1y*Q2y*pow(sp,4.0) - 288*P0x*P1x*Q1y*Q3y*pow(sp,3.0) - 72*P0x*P1x*Q2y*Q3y*pow(sp,2.0) - 90*P0x*P0y*Q1x*Q3y*pow(sp,4.0) + 72*P0x*P0y*Q2x*Q2y*pow(sp,4.0) + 66*P0x*P0y*Q2x*Q3y*pow(sp,3.0) -
	90*P0x*P0y*Q3x*Q1y*pow(sp,4.0) + 66*P0x*P0y*Q3x*Q2y*pow(sp,3.0) + 18*P0x*P0y*Q3x*Q3y*pow(sp,2.0) - 90*P0x*P1y*Q1x*Q2y*pow(sp,4.0) - 234*P0x*P1y*Q1x*Q3y*pow(sp,3.0) - 90*P0x*P1y*Q2x*Q1y*pow(sp,4.0) +
	162*P0x*P1y*Q2x*Q2y*pow(sp,3.0) + 36*P0x*P1y*Q2x*Q3y*pow(sp,2.0) - 234*P0x*P1y*Q3x*Q1y*pow(sp,3.0) + 36*P0x*P1y*Q3x*Q2y*pow(sp,2.0) - 90*P1x*P0y*Q1x*Q2y*pow(sp,4.0) - 234*P1x*P0y*Q1x*Q3y*pow(sp,3.0) -
	90*P1x*P0y*Q2x*Q1y*pow(sp,4.0) + 162*P1x*P0y*Q2x*Q2y*pow(sp,3.0) + 36*P1x*P0y*Q2x*Q3y*pow(sp,2.0) - 234*P1x*P0y*Q3x*Q1y*pow(sp,3.0) + 36*P1x*P0y*Q3x*Q2y*pow(sp,2.0) - 234*P1x*P1y*Q1x*Q2y*pow(sp,3.0) -
	162*P1x*P1y*Q1x*Q3y*pow(sp,2.0) - 234*P1x*P1y*Q2x*Q1y*pow(sp,3.0) + 54*P1x*P1y*Q2x*Q2y*pow(sp,2.0) - 162*P1x*P1y*Q3x*Q1y*pow(sp,2.0) - 240*P0y*P1y*Q1x*Q2x*pow(sp,4.0) - 288*P0y*P1y*Q1x*Q3x*pow(sp,3.0) -
	72*P0y*P1y*Q2x*Q3x*pow(sp,2.0) + 240*P0x*P1x*Q1y*Q3y*pow(sp,4.0) - 64*P0x*P1x*Q2y*Q3y*pow(sp,3.0) - 27*P0x*P0y*Q2x*Q3y*pow(sp,4.0) - 27*P0x*P0y*Q3x*Q2y*pow(sp,4.0) + 30*P0x*P0y*Q3x*Q3y*pow(sp,3.0) +
	90*P0x*P1y*Q1x*Q3y*pow(sp,4.0) - 72*P0x*P1y*Q2x*Q2y*pow(sp,4.0) - 66*P0x*P1y*Q2x*Q3y*pow(sp,3.0) + 90*P0x*P1y*Q3x*Q1y*pow(sp,4.0) - 66*P0x*P1y*Q3x*Q2y*pow(sp,3.0) - 18*P0x*P1y*Q3x*Q3y*pow(sp,2.0) +
	90*P1x*P0y*Q1x*Q3y*pow(sp,4.0) - 72*P1x*P0y*Q2x*Q2y*pow(sp,4.0) - 66*P1x*P0y*Q2x*Q3y*pow(sp,3.0) + 90*P1x*P0y*Q3x*Q1y*pow(sp,4.0) - 66*P1x*P0y*Q3x*Q2y*pow(sp,3.0) - 18*P1x*P0y*Q3x*Q3y*pow(sp,2.0) +
	90*P1x*P1y*Q1x*Q2y*pow(sp,4.0) + 234*P1x*P1y*Q1x*Q3y*pow(sp,3.0) + 90*P1x*P1y*Q2x*Q1y*pow(sp,4.0) - 162*P1x*P1y*Q2x*Q2y*pow(sp,3.0) - 36*P1x*P1y*Q2x*Q3y*pow(sp,2.0) + 234*P1x*P1y*Q3x*Q1y*pow(sp,3.0) -
	36*P1x*P1y*Q3x*Q2y*pow(sp,2.0) + 240*P0y*P1y*Q1x*Q3x*pow(sp,4.0) - 64*P0y*P1y*Q2x*Q3x*pow(sp,3.0) + 72*P0x*P1x*Q2y*Q3y*pow(sp,4.0) - 18*P0x*P0y*Q3x*Q3y*pow(sp,4.0) + 27*P0x*P1y*Q2x*Q3y*pow(sp,4.0) +
	27*P0x*P1y*Q3x*Q2y*pow(sp,4.0) - 30*P0x*P1y*Q3x*Q3y*pow(sp,3.0) + 27*P1x*P0y*Q2x*Q3y*pow(sp,4.0) + 27*P1x*P0y*Q3x*Q2y*pow(sp,4.0) - 30*P1x*P0y*Q3x*Q3y*pow(sp,3.0) - 90*P1x*P1y*Q1x*Q3y*pow(sp,4.0) +
	72*P1x*P1y*Q2x*Q2y*pow(sp,4.0) + 66*P1x*P1y*Q2x*Q3y*pow(sp,3.0) - 90*P1x*P1y*Q3x*Q1y*pow(sp,4.0) + 66*P1x*P1y*Q3x*Q2y*pow(sp,3.0) + 18*P1x*P1y*Q3x*Q3y*pow(sp,2.0) + 72*P0y*P1y*Q2x*Q3x*pow(sp,4.0) +
	18*P0x*P1y*Q3x*Q3y*pow(sp,4.0) + 18*P1x*P0y*Q3x*Q3y*pow(sp,4.0) - 27*P1x*P1y*Q2x*Q3y*pow(sp,4.0) - 27*P1x*P1y*Q3x*Q2y*pow(sp,4.0) + 30*P1x*P1y*Q3x*Q3y*pow(sp,3.0) - 18*P1x*P1y*Q3x*Q3y*pow(sp,4.0) -
	252*P0y*P1y*Q1y*Q2y*pow(sp,2.0) + 756*P0y*P1y*Q1y*Q2y*pow(sp,3.0) + 252*P0y*P1y*Q1y*Q3y*pow(sp,2.0) - 420*P0y*P1y*Q1y*Q2y*pow(sp,4.0) - 756*P0y*P1y*Q1y*Q3y*pow(sp,3.0) + 420*P0y*P1y*Q1y*Q3y*pow(sp,4.0) -
	196*P0y*P1y*Q2y*Q3y*pow(sp,3.0) + 126*P0y*P1y*Q2y*Q3y*pow(sp,4.0))/(3*(7*pow(P0x,2.0)*pow(Q2x,2.0) + 7*pow(P0x,2.0)*pow(Q3x,2.0) + 7*pow(P1x,2.0)*pow(Q2x,2.0) + 7*pow(P1x,2.0)*pow(Q3x,2.0) +
	16*pow(P0x,2.0)*pow(Q2y,2.0) + 16*pow(P0y,2.0)*pow(Q2x,2.0) + 16*pow(P0x,2.0)*pow(Q3y,2.0) + 16*pow(P1x,2.0)*pow(Q2y,2.0) + 16*pow(P0y,2.0)*pow(Q3x,2.0) + 16*pow(P1y,2.0)*pow(Q2x,2.0) + 16*pow(P1x,2.0)*pow(Q3y,2.0) +
	16*pow(P1y,2.0)*pow(Q3x,2.0) + 7*pow(P0y,2.0)*pow(Q2y,2.0) + 7*pow(P0y,2.0)*pow(Q3y,2.0) + 7*pow(P1y,2.0)*pow(Q2y,2.0) + 7*pow(P1y,2.0)*pow(Q3y,2.0) - 14*P0x*P1x*pow(Q2x,2.0) - 14*P0x*P1x*pow(Q3x,2.0) -
	32*P0x*P1x*pow(Q2y,2.0) - 32*P0x*P1x*pow(Q3y,2.0) - 32*P0y*P1y*pow(Q2x,2.0) - 32*P0y*P1y*pow(Q3x,2.0) - 14*P0y*P1y*pow(Q2y,2.0) - 14*P0y*P1y*pow(Q3y,2.0) - 14*pow(P0x,2.0)*Q2x*Q3x - 14*pow(P1x,2.0)*Q2x*Q3x -
	32*pow(P0y,2.0)*Q2x*Q3x - 32*pow(P1y,2.0)*Q2x*Q3x - 32*pow(P0x,2.0)*Q2y*Q3y - 32*pow(P1x,2.0)*Q2y*Q3y - 14*pow(P0y,2.0)*Q2y*Q3y - 14*pow(P1y,2.0)*Q2y*Q3y + 28*P0x*P1x*Q2x*Q3x - 18*P0x*P0y*Q2x*Q2y + 18*P0x*P0y*Q2x*Q3y +
	18*P0x*P0y*Q3x*Q2y + 18*P0x*P1y*Q2x*Q2y + 18*P1x*P0y*Q2x*Q2y + 64*P0x*P1x*Q2y*Q3y - 18*P0x*P0y*Q3x*Q3y - 18*P0x*P1y*Q2x*Q3y - 18*P0x*P1y*Q3x*Q2y - 18*P1x*P0y*Q2x*Q3y - 18*P1x*P0y*Q3x*Q2y - 18*P1x*P1y*Q2x*Q2y +
	64*P0y*P1y*Q2x*Q3x + 18*P0x*P1y*Q3x*Q3y + 18*P1x*P0y*Q3x*Q3y + 18*P1x*P1y*Q2x*Q3y + 18*P1x*P1y*Q3x*Q2y - 18*P1x*P1y*Q3x*Q3y + 28*P0y*P1y*Q2y*Q3y));
	
	// Just an arbitrary correction heuristic for negative factors ...
	if (out_pfactor < 0)
		out_pfactor = -0.1 * out_pfactor;
	if (out_qfactor < 0)
		out_qfactor = -0.1 * out_qfactor;
}

float PathObject::calcBezierPointDeletionRetainingShapeCost(MapCoord p0, MapCoordF p1, MapCoordF p2, MapCoord p3, PathObject* reference)
{
	const int num_test_points = 20;
	QBezier curve = QBezier::fromPoints(QPointF(p0), QPointF(p1), QPointF(p2), QPointF(p3));
	
	float cost = 0;
	for (int i = 0; i < num_test_points; ++i)
	{
		auto point = MapCoordF { curve.pointAt((i + 1) / (float)(num_test_points + 1)) };
		float distance_sq;
		PathCoord path_coord;
		reference->calcClosestPointOnPath(MapCoordF(point), distance_sq, path_coord);
		cost += distance_sq;
	}
	// Just some random scaling to pretend that we have 50 sample points
	return cost * (50 / 20.0f);
}

void PathObject::calcBezierPointDeletionRetainingShapeOptimization(MapCoord p0, MapCoord p1, MapCoord p2, MapCoord q0, MapCoord q1, MapCoord q2, MapCoord q3, double& out_pfactor, double& out_qfactor)
{
	const float gradient_abort_threshold = 0.05f;	// if the gradient magnitude is lower than this over num_abort_steps step, the optimization is aborted
	const float decrease_abort_threshold = 0.004f;	// if the cost descrease if lower than this over num_abort_steps step, the optimization is aborted
	const int num_abort_steps = 2;
	const double derivative_delta = 0.05;
	const int num_tested_step_sizes = 5;
	const int max_num_line_search_iterations = 5;
	float step_size_stepping_base = 0.001f;
	
	static LineSymbol line_symbol;
	PathObject old_curve(&line_symbol);
	p0.setCurveStart(true);
	old_curve.addCoordinate(p0);
	old_curve.addCoordinate(p1);
	old_curve.addCoordinate(p2);
	q0.setCurveStart(true);
	old_curve.addCoordinate(q0);
	old_curve.addCoordinate(q1);
	old_curve.addCoordinate(q2);
	q3.setCurveStart(false);
	q3.setClosePoint(false);
	q3.setHolePoint(false);
	old_curve.addCoordinate(q3);
	old_curve.update();
	
	float cur_cost = 0;
	int num_no_improvement_iterations = 0;
	float old_gradient[2];
	float old_step_dir[2];
	for (int i = 0; i < 30; ++i)
	{
		MapCoordF p_direction = MapCoordF(p1) - MapCoordF(p0);
		MapCoordF r1 = MapCoordF(p0) + out_pfactor * p_direction;
		MapCoordF q_direction = MapCoordF(q2) - MapCoordF(q3);
		MapCoordF r2 = MapCoordF(q3) + out_qfactor * q_direction;
		
		// Calculate gradient and cost (if first iteration)
		if (i == 0)
			cur_cost = calcBezierPointDeletionRetainingShapeCost(p0, r1, r2, q3, &old_curve);
		//if (i == 0)
		//	qDebug() << "\nStart cost: " << cur_cost;
		
		float gradient[2];
		gradient[0] = (calcBezierPointDeletionRetainingShapeCost(p0, r1 + derivative_delta * p_direction, r2, q3, &old_curve) -
		               calcBezierPointDeletionRetainingShapeCost(p0, r1 - derivative_delta * p_direction, r2, q3, &old_curve)) / (2 * derivative_delta);
		gradient[1] = (calcBezierPointDeletionRetainingShapeCost(p0, r1, r2 + derivative_delta * q_direction, q3, &old_curve) -
		               calcBezierPointDeletionRetainingShapeCost(p0, r1, r2 - derivative_delta * q_direction, q3, &old_curve)) / (2 * derivative_delta);
		
		// Calculate step direction
		float step_dir_p;
		float step_dir_q;
		float conjugate_gradient_factor = 0;
		if (i == 0)
		{
			// Steepest descent
			step_dir_p = -gradient[0];
			step_dir_q = -gradient[1];
		}
		else
		{
			// Conjugate gradient
			// Fletcher - Reeves:
			//conjugate_gradient_factor = (pow(gradient[0], 2.0) + pow(gradient[1], 2.0)) / (pow(old_gradient[0], 2.0) + pow(old_gradient[1], 2.0));
			// Polak – Ribiere:
			conjugate_gradient_factor = qMax(0.0, ( gradient[0] * (gradient[0] - old_gradient[0]) + gradient[1] * (gradient[1] - old_gradient[1]) ) /
			                                      ( pow(old_gradient[0], 2.0) + pow(old_gradient[1], 2.0) ));
			//qDebug() << "Factor: " << conjugate_gradient_factor;
			step_dir_p = -gradient[0] + conjugate_gradient_factor * old_step_dir[0];
			step_dir_q = -gradient[1] + conjugate_gradient_factor * old_step_dir[1];
		}
		
		// Line search in step direction for lowest cost
		float best_step_size = 0;
		float best_step_size_cost = cur_cost;
		int best_step_factor = 0;
		float adjusted_step_size_stepping_base = step_size_stepping_base;
		
		for (int iteration = 0; iteration < max_num_line_search_iterations; ++iteration)
		{
			const float step_size_stepping = adjusted_step_size_stepping_base * (out_pfactor + out_qfactor) / 2; // * qSqrt(step_dir_p*step_dir_p + step_dir_q*step_dir_q); // qSqrt(cur_cost);
			for (int step_test = 1; step_test <= num_tested_step_sizes; ++step_test)
			{
				float step_size = step_test * step_size_stepping;
				float test_p_step = step_size * step_dir_p;
				float test_q_step = step_size * step_dir_q;
				
				MapCoordF test_r1 = r1 + test_p_step * p_direction;
				MapCoordF test_r2 = r2 + test_q_step * q_direction;
				float test_cost = calcBezierPointDeletionRetainingShapeCost(p0, test_r1, test_r2, q3, &old_curve);
				if (test_cost < best_step_size_cost)
				{
					best_step_size_cost = test_cost;
					best_step_size = step_size;
					best_step_factor = step_test;
				}
			}
			//qDebug() << best_step_factor;
			if (best_step_factor == num_tested_step_sizes)
				adjusted_step_size_stepping_base *= num_tested_step_sizes;
			else if (best_step_factor == 0)
				adjusted_step_size_stepping_base *= (1 / (float)num_tested_step_sizes);
			else
				break;
			if (iteration < 3)
				step_size_stepping_base = adjusted_step_size_stepping_base;
		}
		if (best_step_factor == 0 && conjugate_gradient_factor == 0)
			return;
		
		// Update optimized parameters and constrain them to non-negative values
		out_pfactor += best_step_size * step_dir_p;
		if (out_pfactor < 0)
			out_pfactor = 0;
		out_qfactor += best_step_size * step_dir_q;
		if (out_qfactor < 0)
			out_qfactor = 0;
		
		// Abort if gradient is really low for a number of steps
		float gradient_magnitude = qSqrt(gradient[0]*gradient[0] + gradient[1]*gradient[1]);
		//qDebug() << "Gradient magnitude: " << gradient_magnitude;
		if (gradient_magnitude < gradient_abort_threshold || cur_cost - best_step_size_cost < decrease_abort_threshold)
		{
			++num_no_improvement_iterations;
			if (num_no_improvement_iterations == num_abort_steps)
				break;
		}
		else
			num_no_improvement_iterations = 0;
		
		cur_cost = best_step_size_cost;
		old_gradient[0] = gradient[0];
		old_gradient[1] = gradient[1];
		old_step_dir[0] = step_dir_p;
		old_step_dir[1] = step_dir_q;
		//qDebug() << "Cost: " << cur_cost;
	}
}

void PathObject::appendPath(const PathObject* other)
{
	coords.reserve(coords.size() + other->coords.size());
	coords.insert(coords.end(), other->coords.begin(), other->coords.end());
	
	recalculateParts();
	setOutputDirty();
}

void PathObject::appendPathPart(const PathPart &part)
{
	coords.reserve(coords.size() + part.size());
	for (std::size_t i = 0; i < part.size(); ++i)
		coords.emplace_back(part.path->coords[part.first_index + i]);
	
	recalculateParts();
	setOutputDirty();
}

void PathObject::reverse()
{
	for (auto& part : path_parts)
		part.reverse();
		
	Q_ASSERT(isOutputDirty());
}

void PathObject::closeAllParts()
{
	for (auto& part : path_parts)
		part.setClosed(true, true);
}

bool PathObject::convertToCurves(PathObject** undo_duplicate)
{
	bool converted_a_range = false;
	for (const auto& part : path_parts)
	{
		for (auto index = part.first_index; index < part.last_index; index += 3)
		{
			if (!coords[index].isCurveStart())
			{
				auto end_index = index + 1;
				while (end_index < part.last_index && !coords[end_index].isCurveStart())
					++end_index;
				
				if (!converted_a_range)
				{
					if (undo_duplicate)
						*undo_duplicate = duplicate()->asPath();
					converted_a_range = true;
				}
				
				index = convertRangeToCurves(part, index, end_index);
			}
		}
	}
	
	Q_ASSERT(!converted_a_range || isOutputDirty());
	return converted_a_range;
}

int PathObject::convertRangeToCurves(const PathPart& part, MapCoordVector::size_type start_index, MapCoordVector::size_type end_index)
{
	Q_ASSERT(end_index > start_index);
	
	Q_ASSERT(!coords[start_index].isCurveStart());
	coords[start_index].setCurveStart(true);
	
	// Special case: last coordinate
	MapCoord direction;
	if (end_index != part.last_index)
	{
		// Use direction of next segment
		direction = coords[end_index + 1] - coords[end_index];
	}
	else if (part.isClosed())
	{
		// Use average direction at close point
		direction = coords[part.first_index + 1] - coords[end_index - 1];
	}
	else
	{
		// Use direction of last segment
		direction = coords[end_index] - coords[end_index - 1];
	}
	
	MapCoordF tangent = MapCoordF(direction);
	tangent.normalize();
	
	MapCoord end_handle = coords[end_index];
	end_handle.setFlags(0);
	auto baseline = (coords[end_index] - coords[end_index - 1]).length() * BEZIER_HANDLE_DISTANCE;
	end_handle = end_handle - MapCoord(tangent * baseline);
	
	// Special case: first coordinate
	if (start_index != part.first_index)
	{
		// Use direction of previous segment
		direction = coords[start_index] - coords[start_index - 1];
	}
	else if (part.isClosed())
	{
		// Use average direction at close point
		direction = coords[start_index + 1] - coords[part.last_index - 1];
	}
	else
	{
		// Use direction of first segment
		direction = coords[start_index + 1] - coords[start_index];
	}
	
	tangent = MapCoordF(direction);
	tangent.normalize();
	
	MapCoord handle = coords[start_index];
	handle.setFlags(0);
	baseline = (coords[start_index + 1] - coords[start_index]).length() * BEZIER_HANDLE_DISTANCE;
	handle = handle + MapCoord(tangent * baseline);
	addCoordinate(start_index + 1, handle);
	++end_index;
	
	// In-between coordinates
	for (MapCoordVector::size_type c = start_index + 2; c < end_index; ++c)
	{
		direction = coords[c + 1] - coords[c - 2];
		tangent = MapCoordF(direction);
		tangent.normalize();
		
		// Add previous handle
		handle = coords[c];
		handle.setFlags(0);
		baseline = (coords[c] - coords[c - 2]).length() * BEZIER_HANDLE_DISTANCE;
		handle = handle - MapCoord(tangent * baseline);
		
		addCoordinate(c, handle);
		++c;
		++end_index;
		
		Q_ASSERT(!coords[c].isCurveStart());
		// Set curve start flag on point
		coords[c].setCurveStart(true);
		
		// Add next handle
		handle = coords[c];
		handle.setFlags(0);
		baseline = (coords[c + 1] - coords[c]).length() * BEZIER_HANDLE_DISTANCE;
		handle = handle + MapCoord(tangent * baseline);
		
		addCoordinate(c + 1, handle);
		++c;
		++end_index;
	}
	
	// Add last handle
	addCoordinate(end_index, end_handle);
	++end_index;
	
	return end_index;
}

bool PathObject::simplify(PathObject** undo_duplicate, double threshold)
{
	// A copy for reference and undo while this is modified.
	QScopedPointer<PathObject> original(new PathObject(*this));
	
	// original_indices provides a mapping from this object's indices to the
	// equivalent original object indices in order to extract the relevant
	// parts of the original object later for cost calculation.
	// Note: curve handle indices may become incorrect, we don't need them.
	std::vector<MapCoordVector::size_type> original_indices;
	original_indices.resize(coords.size());
	for (std::size_t i = 0, end = coords.size(); i < end; ++i)
		original_indices[i] = i;
	
	// A high value indicating an cost of deletion which is unknown.
	auto const undetermined_cost = std::numeric_limits<double>::max();
	
	// This vector provides the costs of deleting individual nodes.
	std::vector<double> costs;
	costs.resize(coords.size(), undetermined_cost);
	
	// The empty LineSymbol will not generate any renderables,
	// thus reducing the cost of update().
	// The temp object will be reused (but not reallocated) many times.
	LineSymbol empty_symbol;
	PathObject temp { &empty_symbol };
	temp.coords.reserve(10); // enough for two bezier edges.
	
	for (auto part = path_parts.rbegin(); part != path_parts.rend(); ++part)
	{
		// Don't simplify parts which would get deleted.
		MapCoordVector::size_type minimum_part_size = part->isClosed() ? 4 : 3;
		auto minimumPartSizeReached = [&part, minimum_part_size]() -> bool
		{
			return (part->size() <= 7 && part->countRegularNodes() < minimum_part_size);
		};
		
		auto minimum_cost_step = threshold / qMax(std::log2(part->size() / 64.0), 4.0);
		auto minimum_cost = 0.0;
		
		// Delete nodes in max n runs (where n = max. part.end_index - part.start_index)
		for (auto run = part->last_index; run > part->first_index; --run)
		{
			if (minimumPartSizeReached())
				break;
			
			// Determine the costs of node deletion
			{
				auto extract_start = part->first_index;
				auto index = part->nextCoordIndex(extract_start);
				while (index < part->last_index)
				{
					auto extract_end = part->nextCoordIndex(index);
					if (costs[index] == undetermined_cost)
					{
						temp.assignCoordinates(*this, extract_start, extract_end);
						temp.deleteCoordinate(index - extract_start, true, Settings::DeleteBezierPoint_RetainExistingShape);
						// Debug check: start and end coords of the extracts should be at the same position
						Q_ASSERT(coords[extract_start].isPositionEqualTo(temp.coords.front()));
						Q_ASSERT(coords[extract_end].isPositionEqualTo(temp.coords.back()));
						costs[index] = original->calcMaximumDistanceTo(original_indices[extract_start],
						                                               original_indices[extract_end],
						                                               &temp,
						                                               0,
						                                               temp.coords.size()-1);
					}
					extract_start = index;
					index = extract_end;
				}
				
				if (costs[part->first_index] == undetermined_cost && extract_start < part->last_index)
				{
					if (part->isClosed())
					{
						auto extract_end = part->nextCoordIndex(part->first_index);
						temp.assignCoordinates(*this, extract_start, extract_end);
						temp.deleteCoordinate(part->last_index - extract_start, true, Settings::DeleteBezierPoint_RetainExistingShape);
						// Debug check: start and end coords of the extracts should be at the same position
						Q_ASSERT(coords[extract_start].isPositionEqualTo(temp.coords.front()));
						Q_ASSERT(coords[extract_end].isPositionEqualTo(temp.coords.back()));
						costs[part->first_index] = original->calcMaximumDistanceTo(original_indices[extract_start],
						                                                           original_indices[extract_end],
						                                                           &temp,
						                                                           0,
						                                                           temp.coords.size()-1);
					}
					else
					{
						costs[part->first_index] = threshold * 2;
					}
				}
				
				costs[part->last_index] = undetermined_cost;
			}
			
			// Find an upper bound for the acceptable cost in the current run
			auto cost_bound = threshold * 2;
			for (auto index = part->first_index; index < part->last_index; ++index)
			{
				if (costs[index] <= cost_bound)
				{
					cost_bound = costs[index];
					if (cost_bound <= minimum_cost)
					{
						// short-cut on minimal costs
						cost_bound = minimum_cost;
						break;
					}
				}
			}
			
			if (cost_bound > threshold)
				break;
			
			while (minimum_cost <= cost_bound)
				minimum_cost += minimum_cost_step;
			if (minimum_cost > threshold)
				minimum_cost = threshold;
			
			// Start at the end, in order to shorten the vector quickly
			auto index = part->last_index;
			do
			{
				if (costs[index] <= cost_bound)
				{
					auto extract_start = part->prevCoordIndex(index);
					auto extract_end   = part->nextCoordIndex(index);
					Q_ASSERT(original->coords[original_indices[extract_start]].isPositionEqualTo(coords[extract_start]));
					Q_ASSERT(original->coords[original_indices[extract_end]].isPositionEqualTo(coords[extract_end]));
					deleteCoordinate(index, true, Settings::DeleteBezierPoint_RetainExistingShape);
					
					if (index > part->first_index)
					{
						// Deleted inner point
						auto new_extract_end = part->nextCoordIndex(extract_start);
						auto original_start = begin(original_indices) + extract_start;
						original_indices.erase(original_start + 1,
						                       original_start + 1 + (extract_end - new_extract_end));
						Q_ASSERT(original_indices.size() == coords.size());
						Q_ASSERT(original->coords[original_indices[extract_start]].isPositionEqualTo(coords[extract_start]));
						Q_ASSERT(original->coords[original_indices[new_extract_end]].isPositionEqualTo(coords[new_extract_end]));
						
						costs.erase(begin(costs) + extract_start + 1,
						            begin(costs) + extract_start + 1 + (extract_end - new_extract_end));
						Q_ASSERT(costs.size() == coords.size());
						
						costs[extract_start] = undetermined_cost;
						
						if (new_extract_end == part->last_index)
						{
							costs[part->first_index] = undetermined_cost;
						}
						else
						{
							costs[new_extract_end] = undetermined_cost;
						}
						Q_ASSERT(costs[part->last_index] > threshold);
						
						index = part->prevCoordIndex(extract_start);
						
						if (minimumPartSizeReached())
							break;
					}
					else if (part->isClosed())
					{
						Q_ASSERT(index == part->first_index);
						
						// Deleted start point of closed path
						// This must match PathObject::deleteCoordinate
						extract_start = part->prevCoordIndex(part->last_index);
						auto original_begin = begin(original_indices);
						original_indices.erase(original_begin + part->first_index,
						                       original_begin + extract_end);
						original_indices.resize(coords.size());
						original_indices[part->last_index] = original_indices[part->first_index];
						Q_ASSERT(original->coords[original_indices[extract_start]].isPositionEqualTo(coords[extract_start]));
						Q_ASSERT(original->coords[original_indices[part->last_index]].isPositionEqualTo(coords[part->last_index]));
						Q_ASSERT(original->coords[original_indices[part->first_index]].isPositionEqualTo(coords[part->first_index]));
						
						costs.erase(begin(costs) + part->first_index,
						            begin(costs) + extract_end);
						costs.resize(coords.size(), undetermined_cost);
						costs[extract_start] = undetermined_cost;
						costs[part->first_index] = undetermined_cost;
						Q_ASSERT(costs[part->last_index] > threshold);
					}
					else
					{
						Q_ASSERT(index == part->first_index);
					}
				}
				else if (index != part->first_index)
				{
					--index;
				}
			}
			while (index != part->first_index);
		}
	}
	
	bool removed_a_point = (coords.size() != original->coords.size());
	if (removed_a_point && undo_duplicate)
	{
		*undo_duplicate = original.take();
	}
	
	return removed_a_point;
}

int PathObject::isPointOnPath(MapCoordF coord, float tolerance, bool treat_areas_as_paths, bool extended_selection) const
{
	float side_tolerance = tolerance;
	if (extended_selection && map && (symbol->getType() == Symbol::Line || symbol->getType() == Symbol::Combined))
	{
		// TODO: precalculate largest line extent for all symbols to move it out of this time critical method?
		side_tolerance = qMax(side_tolerance, float(symbol->calculateLargestLineExtent()));
	}
	
	auto contained_types = symbol->getContainedTypes();
	if ((contained_types & Symbol::Line || treat_areas_as_paths) && tolerance > 0)
	{
		update();
		for (const auto& part : path_parts)
		{
			const auto& path_coords = part.path_coords;
			auto size = path_coords.size();
			for (PathCoordVector::size_type i = 0; i < size - 1; ++i)
			{
				Q_ASSERT(path_coords[i].index < coords.size());
				if (coords[path_coords[i].index].isHolePoint())
					continue;
				
				MapCoordF to_coord = coord - path_coords[i].pos;
				MapCoordF to_next = path_coords[i+1].pos - path_coords[i].pos;
				MapCoordF tangent = to_next;
				tangent.normalize();
				
				float dist_along_line = MapCoordF::dotProduct(to_coord, tangent);
				if (dist_along_line < -tolerance)
					continue;
				else if (dist_along_line < 0 && to_coord.lengthSquared() <= tolerance*tolerance)
					return Symbol::Line;
				
				float line_length = path_coords[i+1].clen - path_coords[i].clen;
				if (line_length < 1e-7)
					continue;
				if (dist_along_line > line_length + tolerance)
					continue;
				else if (dist_along_line > line_length && coord.distanceSquaredTo(path_coords[i+1].pos) <= tolerance*tolerance)
					return Symbol::Line;
				
				auto right = tangent.perpRight();
				
				float dist_from_line = qAbs(MapCoordF::dotProduct(right, to_coord));
				if (dist_from_line <= side_tolerance)
					return Symbol::Line;
			}
		}
	}
	
	// Check for area selection
	if ((contained_types & Symbol::Area) && !treat_areas_as_paths)
	{
		if (isPointInsideArea(coord))
			return Symbol::Area;
	}
	
	return Symbol::NoSymbol;
}

bool PathObject::isPointInsideArea(const MapCoordF& coord) const
{
	update();
	bool inside = false;
	for (const auto& part : path_parts)
	{
		if (part.isPointInside(coord))
			inside = !inside;
	}
	return inside;
}

double PathObject::calcMaximumDistanceTo(
        MapCoordVector::size_type start_index,
        MapCoordVector::size_type end_index,
        const PathObject* other,
        MapCoordVector::size_type other_start_index,
        MapCoordVector::size_type other_end_index) const
{
	update();
	
	Q_ASSERT(other_start_index == 0);
	Q_ASSERT(other_end_index == other->coords.size()-1);
	
	if (end_index < start_index)
	{
		const auto part = findPartForIndex(start_index);
		Q_ASSERT(part->isClosed());
		Q_ASSERT(end_index >= part->first_index);
		Q_ASSERT(end_index <= part->last_index);
		auto d1 = calcMaximumDistanceTo(start_index, part->last_index, other, other_start_index, other_end_index);
		auto d2 = calcMaximumDistanceTo(part->first_index, end_index, other, other_start_index, other_end_index);
		return qMax(d1, d2);
	}
	
	const float test_points_per_mm = 2;
	
	float max_distance_sq = 0.0;
	for (const auto& part : path_parts)
	{
		if (part.first_index <= end_index && part.last_index >= start_index )
		{
			const auto& path_coords = part.path_coords;
			
			auto pc_start = std::lower_bound(begin(path_coords), end(path_coords), start_index, PathCoord::indexLessThanValue);
			auto pc_end = std::lower_bound(pc_start, end(path_coords), end_index, PathCoord::indexLessThanValue);
			if (pc_end == end(path_coords))
			{
				if (pc_end != pc_start)
					--pc_end;
			}
			
			PathCoord path_coord;
			float distance_sq = 0.0;
			other->calcClosestPointOnPath(pc_start->pos, distance_sq, path_coord, other_start_index, other_end_index);
			max_distance_sq = qMax(max_distance_sq, distance_sq);
			
			for (auto pc = pc_start; pc != pc_end; ++pc)
			{
				auto next_pc = pc + 1;
				double len = next_pc->clen - pc->clen;
				MapCoordF direction = next_pc->pos - pc->pos;
				
				int num_test_points = qMax(1, qRound(len * test_points_per_mm));
				for (int p = 1; p <= num_test_points; ++p)
				{
					MapCoordF point = pc->pos + direction * ((float)p / num_test_points);
					other->calcClosestPointOnPath(point, distance_sq, path_coord, other_start_index, other_end_index);
					max_distance_sq = qMax(max_distance_sq, distance_sq);
				}
			}
		}
	}
		
	return sqrt(max_distance_sq);
}

PathObject::Intersection PathObject::Intersection::makeIntersectionAt(double a, double b, const PathCoord& a0, const PathCoord& a1, const PathCoord& b0, const PathCoord& b1, PathPartVector::size_type part_index, PathPartVector::size_type other_part_index)
{
	PathObject::Intersection new_intersection;
	new_intersection.coord = MapCoordF(a0.pos.x() + a * (a1.pos.x() - a0.pos.x()), a0.pos.y() + a * (a1.pos.y() - a0.pos.y()));
	new_intersection.part_index = part_index;
	new_intersection.length = a0.clen + a * (a1.clen - a0.clen);
	new_intersection.other_part_index = other_part_index;
	new_intersection.other_length = b0.clen + b * (b1.clen - b0.clen);
	return new_intersection;
}

void PathObject::Intersections::normalize()
{
	std::sort(begin(), end());
	erase(std::unique(begin(), end()), end());
}

void PathObject::calcAllIntersectionsWith(const PathObject* other, PathObject::Intersections& out) const
{
	update();
	other->update();
	
	const double epsilon = 1e-10;
	const double zero_minus_epsilon = 0 - epsilon;
	const double one_plus_epsilon = 1 + epsilon;
	
	for (size_t part_index = 0; part_index < path_parts.size(); ++part_index)
	{
		const PathPart& part = path_parts[part_index];
		auto path_coord_end_index = part.path_coords.size() - 1;
		for (auto i = PathCoordVector::size_type { 1 }; i <= path_coord_end_index; ++i)
		{
			// Get information about this path coord
			bool has_segment_before = (i > 1) || part.isClosed();
			MapCoordF ingoing_direction;
			if (has_segment_before && i == 1)
			{
				Q_ASSERT(path_coord_end_index >= 1);
				ingoing_direction = part.path_coords[path_coord_end_index].pos - part.path_coords[path_coord_end_index - 1].pos;
				ingoing_direction.normalize();
			}
			else if (has_segment_before)
			{
				Q_ASSERT(i >= 1 && i < part.path_coords.size());
				ingoing_direction = part.path_coords[i-1].pos - part.path_coords[i-2].pos;
				ingoing_direction.normalize();
			}
			
			bool has_segment_after = (i < path_coord_end_index) || part.isClosed();
			MapCoordF outgoing_direction;
			if (has_segment_after && i == path_coord_end_index)
			{
				Q_ASSERT(part.path_coords.size() > 1);
				outgoing_direction = part.path_coords[1].pos - part.path_coords[0].pos;
				outgoing_direction.normalize();
			}
			else if (has_segment_after)
			{
				Q_ASSERT(i < path_coord_end_index);
				outgoing_direction = part.path_coords[i+1].pos - part.path_coords[i].pos;
				outgoing_direction.normalize();
			}
			
			// Collision state with other object at current other path coord
			bool colliding = false;
			// Last known intersecting point.
			// This is valid as long as colliding == true and entered as intersection
			// when the next segment suddenly is not colliding anymore.
			Intersection last_intersection;
			
			for (size_t other_part_index = 0; other_part_index < other->path_parts.size(); ++other_part_index)
			{
				const PathPart& other_part = other->path_parts[part_index]; /// \todo FIXME: part_index or other_part_index ???
				auto other_path_coord_end_index = other_part.path_coords.size() - 1;
				for (auto k = PathCoordVector::size_type { 1 }; k <= other_path_coord_end_index; ++k)
				{
					// Test the two line segments against each other.
					// Naming: segment in this path is a, segment in other path is b
					const PathCoord& a0 = part.path_coords[i-1];
					const PathCoord& a1 = part.path_coords[i];
					const PathCoord& b0 = other_part.path_coords[k-1];
					const PathCoord& b1 = other_part.path_coords[k];
					MapCoordF b_direction = b1.pos - b0.pos;
					b_direction.normalize();
					
					bool first_other_segment = (k == 1);
					if (first_other_segment)
					{
						colliding = isPointOnSegment(a0.pos, a1.pos, b0.pos);
						if (colliding && !other_part.isClosed())
						{
							// Enter intersection at start of other segment
							bool ok;
							double a = parameterOfPointOnLine(a0.pos.x(), a0.pos.y(), a1.pos.x() - a0.pos.x(), a1.pos.y() - a0.pos.y(), b0.pos.x(), b0.pos.y(), ok);
							Q_ASSERT(ok);
							out.push_back(Intersection::makeIntersectionAt(a, 0, a0, a1, b0, b1, part_index, other_part_index));
						}
					}
					
					bool last_other_segment = (k == other_path_coord_end_index);
					if (last_other_segment)
					{
						bool collision_at_end = isPointOnSegment(a0.pos, a1.pos, b1.pos);
						if (collision_at_end && !other_part.isClosed())
						{
							// Enter intersection at end of other segment
							bool ok;
							double a = parameterOfPointOnLine(a0.pos.x(), a0.pos.y(), a1.pos.x() - a0.pos.x(), a1.pos.y() - a0.pos.y(), b1.pos.x(), b1.pos.y(), ok);
							Q_ASSERT(ok);
							out.push_back(Intersection::makeIntersectionAt(a, 1, a0, a1, b0, b1, part_index, other_part_index));
						}
					}
					
					double denominator = a0.pos.x()*b0.pos.y() - a0.pos.y()*b0.pos.x() - a0.pos.x()*b1.pos.y() - a1.pos.x()*b0.pos.y() + a0.pos.y()*b1.pos.x() + a1.pos.y()*b0.pos.x() + a1.pos.x()*b1.pos.y() - a1.pos.y()*b1.pos.x();
					if (denominator == 0)
					{
						// Parallel lines, calculate parameters for b's start and end points in a and b.
						// This also checks whether the lines are actually on the same level.
						bool ok;
						double b_start = 0;
						double a_start = parameterOfPointOnLine(a0.pos.x(), a0.pos.y(), a1.pos.x() - a0.pos.x(), a1.pos.y() - a0.pos.y(), b0.pos.x(), b0.pos.y(), ok);
						if (!ok)
						{
							if (colliding) out.push_back(last_intersection);
							colliding = false;
							continue;
						}
						double b_end = 1;
						double a_end = parameterOfPointOnLine(a0.pos.x(), a0.pos.y(), a1.pos.x() - a0.pos.x(), a1.pos.y() - a0.pos.y(), b1.pos.x(), b1.pos.y(), ok);
						if (!ok)
						{
							if (colliding) out.push_back(last_intersection);
							colliding = false;
							continue;
						}
						
						// Cull ranges
						if (a_start < zero_minus_epsilon && a_end < zero_minus_epsilon)
						{
							if (colliding) out.push_back(last_intersection);
							colliding = false;
							continue;
						}
						if (a_start > one_plus_epsilon && a_end > one_plus_epsilon)
						{
							if (colliding) out.push_back(last_intersection);
							colliding = false;
							continue;
						}
						
						// b overlaps somehow with a, check if we have to enter one or two collisions
						// (provided the incoming / outgoing tangents are not parallel!)
						if (!colliding)
						{
							if (a_start <= 0)
							{
								// b comes in over the start of a
								Q_ASSERT(a_end >= 0);
								
								// Check for parallel tangent case
								if (!has_segment_before || MapCoordF::dotProduct(b_direction, ingoing_direction) < 1 - epsilon)
								{
									// Enter intersection at a=0
									double b = b_start + (0 - a_start) / (a_end - a_start) * (b_end - b_start);
									out.push_back(Intersection::makeIntersectionAt(0, b, a0, a1, b0, b1, part_index, other_part_index));
								}
								
								colliding = true;
							}
							else if (a_start >= 1)
							{
								// b comes in over the end of a
								Q_ASSERT(a_end <= 1);
								
								// Check for parallel tangent case
								if (!has_segment_after || -1 * MapCoordF::dotProduct(b_direction, outgoing_direction) < 1 - epsilon)
								{
									// Enter intersection at a=1
									double b = b_start + (1 - a_start) / (a_end - a_start) * (b_end - b_start);
									out.push_back(Intersection::makeIntersectionAt(1, b, a0, a1, b0, b1, part_index, other_part_index));
								}
								
								colliding = true;
							}
							else
								Q_ASSERT(false);
						}
						if (colliding)
						{
							if (a_end > 1)
							{
								// b goes out over the end of a
								Q_ASSERT(a_start <= 1);
								
								// Check for parallel tangent case
								if (!has_segment_after || MapCoordF::dotProduct(b_direction, outgoing_direction) < 1 - epsilon)
								{
									// Enter intersection at a=1
									double b = b_start + (1 - a_start) / (a_end - a_start) * (b_end - b_start);
									out.push_back(Intersection::makeIntersectionAt(1, b, a0, a1, b0, b1, part_index, other_part_index));
								}
								
								colliding = false;
							}
							else if (a_end < 0)
							{
								// b goes out over the start of a
								Q_ASSERT(a_start >= 1);
								
								// Check for parallel tangent case
								if (!has_segment_before || -1 * MapCoordF::dotProduct(b_direction, ingoing_direction) < 1 - epsilon)
								{
									// Enter intersection at a=0
									double b = b_start + (0 - a_start) / (a_end - a_start) * (b_end - b_start);
									out.push_back(Intersection::makeIntersectionAt(1, b, a0, a1, b0, b1, part_index, other_part_index));
								}
								
								colliding = false;
							}
							else
							{
								// b stops in the middle of a.
								// Remember last known colliding point, the endpoint of b.
								last_intersection = Intersection::makeIntersectionAt(a_end, 1, a0, a1, b0, b1, part_index, other_part_index);
							}
						}
						
						// Check if there is a collision at the endpoint
						Q_ASSERT(colliding == (a_end >= 0 && a_end <= 1));
					}
					else
					{
						// Non-parallel lines, calculate intersection parameters and check if in range
						double a = +(a0.pos.x()*b0.pos.y() - a0.pos.y()*b0.pos.x() - a0.pos.x()*b1.pos.y() + a0.pos.y()*b1.pos.x() + b0.pos.x()*b1.pos.y() - b1.pos.x()*b0.pos.y()) / denominator;
						if (a < zero_minus_epsilon || a > one_plus_epsilon)
						{
							if (colliding) out.push_back(last_intersection);
							colliding = false;
							continue;
						}
						
						double b = -(a0.pos.x()*a1.pos.y() - a1.pos.x()*a0.pos.y() - a0.pos.x()*b0.pos.y() + a0.pos.y()*b0.pos.x() + a1.pos.x()*b0.pos.y() - a1.pos.y()*b0.pos.x()) / denominator;
						if (b < zero_minus_epsilon || b > one_plus_epsilon)
						{
							if (colliding) out.push_back(last_intersection);
							colliding = false;
							continue;
						}
						
						// Special case for overlapping (cloned / traced) polylines: check if b is parallel to adjacent direction.
						// If so, set colliding to true / false without entering an intersection because the other line
						// simply continues along / comes from the path of both polylines instead of intersecting.
						double dot = -1;
						if (has_segment_before && a <= 0 + epsilon)
						{
							// Ingoing direction
							dot = MapCoordF::dotProduct(b_direction, ingoing_direction);
							if (b <= 0 + epsilon)
								dot = -1 * dot;
						}
						else if (has_segment_after && a >= 1 - epsilon)
						{
							// Outgoing direction
							dot = MapCoordF::dotProduct(b_direction, outgoing_direction);
							if (b >= 1 - epsilon)
								dot = -1 * dot;
						}
						if (dot >= 1 - epsilon)
						{
							colliding = (b > 0.5);
							continue;
						}
						
						// Enter the intersection
						last_intersection = Intersection::makeIntersectionAt(a, b, a0, a1, b0, b1, part_index, other_part_index);
						out.push_back(last_intersection);
						colliding = (b == 1);
					}
				}
			}
		}
	}
}

void PathObject::setCoordinate(MapCoordVector::size_type pos, const MapCoord& c)
{
	Q_ASSERT(pos < getCoordinateCount());
	
	const PathPart& part = *findPartForIndex(pos);
	if (part.isClosed() && pos == part.last_index)
		pos = part.first_index;
	coords[pos] = c;
	if (part.isClosed() && pos == part.first_index)
		setClosingPoint(part.last_index, c);
	
	setOutputDirty();
}

void PathObject::addCoordinate(MapCoordVector::size_type pos, const MapCoord& c)
{
	Q_ASSERT(pos <= coords.size());
	
	if (coords.empty())
	{
		coords.emplace_back(c);
		
		path_parts.clear();
		path_parts.emplace_back(*this, 0, 0);
	}
	else
	{
		auto part = findPartForIndex(qMin(pos, coords.size() - 1));
		coords.insert(coords.begin() + pos, c);
		partSizeChanged(part, +1);
		
		if (pos == part->first_index)
		{
			if (part->isClosed())
				setClosingPoint(part->last_index, c);
		}
		else if (pos == part->last_index)
		{
			Q_ASSERT(pos > part->first_index);
			coords[pos-1].setClosePoint(false);
			coords[pos-1].setHolePoint(false);
		}
	}
	setOutputDirty();
}

void PathObject::addCoordinate(const MapCoord& c, bool start_new_part)
{
	if (coords.empty())
	{
		addCoordinate(0, c);
	}
	else if (!start_new_part)
	{
		addCoordinate(coords.size(), c);
	}
	else
	{
		auto index = coords.size();
		coords.push_back(c);
		path_parts.emplace_back(*this, index, index);
		setOutputDirty();
	}
}

void PathObject::deleteCoordinate(MapCoordVector::size_type pos, bool adjust_other_coords, int delete_bezier_point_action)
{
	const auto part = findPartForIndex(pos);
	const auto coords_begin = begin(coords);
	
	auto num_coords = part->size();
	if (num_coords <= 7)
	{
		auto num_regular_points = part->countRegularNodes();
		
		if (num_regular_points <= 2 && (pos == part->first_index || pos == part->last_index))
		{
			// Too small, must delete
			deletePart(findPartIndexForIndex(pos));
			return;
		}
		
		if (num_regular_points == 3 && num_coords == 4)
		{
			// A closed path of three straight segments, to be opened
			Q_ASSERT(part->isClosed());
			if (pos == part->last_index)
				pos = part->first_index;
			
			// Move pos to end_index-1, than erase last two coords
			std::rotate(coords_begin + part->first_index, coords_begin + pos + 1, coords_begin + part->last_index);
			coords.erase(coords_begin + part->last_index - 1, coords_begin + part->last_index+1);
			partSizeChanged(part, -2);
			coords[part->last_index].setHolePoint(true);
			return;
		}
	}
	
	auto prev_index = part->prevCoordIndex(pos);
	if (prev_index < pos && coords[prev_index].isCurveStart() && prev_index + 3 > pos)
	{
		// Delete curve handles
		coords[prev_index].setCurveStart(false);
		coords.erase(coords_begin + prev_index + 1, coords_begin + prev_index + 3);
		partSizeChanged(part, -2);
		return;
	}
	
	if (pos == part->first_index || pos == part->last_index)
	{
		if (part->isClosed())
		{
			// Make start/end point an inner point before removing
			auto middle_offset = coords[part->first_index].isCurveStart() ? 3 : 1;
			std::rotate(coords_begin + part->first_index, coords_begin + part->first_index + middle_offset, coords_begin + part->last_index);
			setClosingPoint(part->last_index, coords[part->first_index]);
			pos = part->last_index - middle_offset;
			prev_index = part->prevCoordIndex(pos);
		}
		else
		{
			auto start = std::min<VirtualPath::size_type>(pos, prev_index + 1);
			auto end   = std::max<VirtualPath::size_type>(pos + 1, part->nextCoordIndex(pos));
			coords.erase(coords_begin + start, coords_begin + end);
			partSizeChanged(part, start - end);
			coords[part->last_index].setHolePoint(true);
			coords[part->last_index].setCurveStart(false);
			return;
		}
	}
	
	// Delete inner point
	Q_ASSERT(pos > part->first_index);
	Q_ASSERT(pos < part->last_index);
	if (!(coords[prev_index].isCurveStart() && coords[pos].isCurveStart()))
	{
		// Bezier curve on single side
		if (coords[pos].isCurveStart())
			coords[prev_index].setCurveStart(true);
		
		coords.erase(coords_begin + pos);
		partSizeChanged(part, -1);
	}
	else
	{
		// Bezier curves on both sides
		if (adjust_other_coords)
		{
			// Adjust handle positions to preserve the original shape somewhat
			// (of course, in general it is impossible to do this)
			prepareDeleteBezierPoint(pos, delete_bezier_point_action);
		}
		
		coords.erase(begin(coords) + pos - 1, begin(coords) + pos + 2);
		partSizeChanged(part, -3);
	}
}

void PathObject::prepareDeleteBezierPoint(MapCoordVector::size_type pos, int delete_bezier_point_action)
{
	const MapCoord& p0 = coords[pos - 3];
	MapCoord& p1 = coords[pos - 2];
	const MapCoord& p2 = coords[pos - 1];
	const MapCoord& q0 = coords[pos];
	const MapCoord& q1 = coords[pos + 1];
	MapCoord& q2 = coords[pos + 2];
	const MapCoord& q3 = coords[pos + 3];
	
	double pfactor, qfactor;
	if (delete_bezier_point_action == Settings::DeleteBezierPoint_ResetHandles)
	{
		double target_length = BEZIER_HANDLE_DISTANCE * p0.distanceTo(q3);
		pfactor = target_length / qMax(p0.distanceTo(p1), 0.01);
		qfactor = target_length / qMax(q3.distanceTo(q2), 0.01);
	}
	else if (delete_bezier_point_action == Settings::DeleteBezierPoint_RetainExistingShape)
	{
		calcBezierPointDeletionRetainingShapeFactors(p0, p1, p2, q0, q1, q2, q3, pfactor, qfactor);
		
		if (qIsInf(pfactor))
			pfactor = 1;
		if (qIsInf(qfactor))
			qfactor = 1;
		
		calcBezierPointDeletionRetainingShapeOptimization(p0, p1, p2, q0, q1, q2, q3, pfactor, qfactor);
		
		double minimum_length = 0.01 * p0.distanceTo(q3);
		pfactor = qMax(minimum_length / qMax(p0.distanceTo(p1), 0.01), pfactor);
		qfactor = qMax(minimum_length / qMax(q3.distanceTo(q2), 0.01), qfactor);
	}
	else
	{
		Q_ASSERT(delete_bezier_point_action == Settings::DeleteBezierPoint_KeepHandles);
		pfactor = 1;
		qfactor = 1;
	}
	
	MapCoordF p0p1 = MapCoordF(p1) - MapCoordF(p0);
	p1 = MapCoord(p0.x() + pfactor * p0p1.x(), p0.y() + pfactor * p0p1.y());
	
	MapCoordF q3q2 = MapCoordF(q2) - MapCoordF(q3);
	q2 = MapCoord(q3.x() + qfactor * q3q2.x(), q3.y() + qfactor * q3q2.y());
}

void PathObject::clearCoordinates()
{
	coords.clear();
	path_parts.clear();
	setOutputDirty();
}

void PathObject::assignCoordinates(const PathObject& proto, MapCoordVector::size_type first, MapCoordVector::size_type last)
{
	Q_ASSERT(last < proto.coords.size());
	
	auto part = proto.findPartForIndex(first);
	Q_ASSERT(part == proto.findPartForIndex(last));
	
	coords.clear();
	if (last >= first)
		coords.reserve(last - first + 1);
	else
		coords.reserve(last - part->first_index + part->last_index - first + 2);
	
	for (auto i = first; i != last; )
	{
		MapCoord new_coord = proto.coords[i];
		new_coord.setClosePoint(false);
		new_coord.setHolePoint(false);
		coords.push_back(new_coord);
		
		++i;
		if (i > part->last_index)
		{
			i = part->first_index;
			if (part->isClosed() && i != last)
				coords.erase(coords.end()-1);
		}
		
	}
	// Add last point
	MapCoord new_coord = proto.coords[last];
	new_coord.setCurveStart(false);
	new_coord.setClosePoint(false);
	new_coord.setHolePoint(true);
	coords.push_back(new_coord);
	
	path_parts.clear();
	path_parts.emplace_back(*this, 0, coords.size()-1);
	
	setOutputDirty();
}

void PathObject::updatePathCoords() const
{
	auto part_start = MapCoordVector::size_type { 0 };
	for (auto& part : path_parts)
	{
		part.first_index = part_start;
		part.last_index  = part.path_coords.update(part_start);
		part_start = part.last_index+1;
	}
}

void PathObject::recalculateParts()
{
	setOutputDirty();
	
	path_parts.clear();
	if (!coords.empty())
	{
		MapCoordVector::size_type start_index = 0;
		auto last_index = coords.size()-1;
		
		for (MapCoordVector::size_type i = 0; i <= last_index; ++i)
		{
			if (coords[i].isHolePoint())
			{
				path_parts.emplace_back(*this, start_index, i);
				start_index = i+1;
			}
			else if (coords[i].isCurveStart())
			{
				i += 2;
			}
		}
		
		if (start_index <= last_index)
			path_parts.emplace_back(*this, start_index, last_index);
	}
}

void PathObject::setClosingPoint(MapCoordVector::size_type index, const MapCoord& coord)
{
	auto& out_coord = coords[index];
	out_coord = coord;
	out_coord.setCurveStart(false);
	out_coord.setHolePoint(true);
	out_coord.setClosePoint(true);
}

void PathObject::updateEvent() const
{
	updatePathCoords();
}

void PathObject::createRenderables(ObjectRenderables& output, Symbol::RenderableOptions options) const
{
	symbol->createRenderables(this, path_parts, output, options);
}


// ### PointObject ###

PointObject::PointObject(const Symbol* symbol)
 : Object(Object::Point, symbol)
 , rotation(0)
{
	Q_ASSERT(!symbol || (symbol->getType() == Symbol::Point));
	coords.push_back(MapCoord(0, 0));
}

PointObject::PointObject(const PointObject& proto)
 : Object(proto)
 , rotation(proto.rotation)
{
	// nothing
}

PointObject* PointObject::duplicate() const
{
	return new PointObject(*this);
}

void PointObject::copyFrom(const Object& other)
{
	if (&other == this)
		return;
	
	Object::copyFrom(other);
	const PointObject* point_other = other.asPoint();
	const PointSymbol* point_symbol = getSymbol()->asPoint();
	if (point_symbol && point_symbol->isRotatable())
		setRotation(point_other->getRotation());
}


void PointObject::setPosition(qint32 x, qint32 y)
{
	coords[0].setNativeX(x);
	coords[0].setNativeY(y);
	setOutputDirty();
}

void PointObject::setPosition(const MapCoord& coord)
{
	coords[0] = coord;
}

void PointObject::setPosition(const MapCoordF& coord)
{
	coords[0].setX(coord.x());
	coords[0].setY(coord.y());
	setOutputDirty();
}

MapCoordF PointObject::getCoordF() const
{
	return MapCoordF(coords.front());
}

MapCoord PointObject::getCoord() const
{
	return coords.front();
}

void PointObject::transform(const QTransform& t)
{
	if (t.isIdentity())
		return;
	
	auto& coord = coords.front();
	const auto p = t.map(MapCoordF{coord});
	coord.setX(p.x());
	coord.setY(p.y());
	setOutputDirty();
}


void PointObject::setRotation(qreal new_rotation)
{
	Q_ASSERT(symbol->asPoint()->isRotatable() || qIsNull(new_rotation));
	if (!qIsNaN(new_rotation) && symbol->asPoint()->isRotatable())
	{
		rotation = new_rotation;
		setOutputDirty();
	}
}

void PointObject::setRotation(const MapCoordF& vector)
{
	setRotation(atan2(vector.x(), vector.y()));
}

bool PointObject::intersectsBox(const QRectF& box) const
{
	return box.contains(QPointF(coords.front()));
}


}  // namespace OpenOrienteering
