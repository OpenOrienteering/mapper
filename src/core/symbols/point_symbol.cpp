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


#include "point_symbol.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <memory>

#include <QtMath>
#include <QLatin1String>
#include <QPainterPath>
#include <QRectF>
#include <QString>
#include <QStringRef>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/map.h"
#include "core/map_color.h"
#include "core/map_coord.h"
#include "core/objects/object.h"
#include "core/renderables/renderable.h"
#include "core/renderables/renderable_implementation.h"
#include "core/symbols/symbol.h"
#include "core/virtual_coord_vector.h"
#include "util/util.h"

// IWYU pragma: no_forward_declare QPainterPath
// IWYU pragma: no_forward_declare QXmlStreamReader
// IWYU pragma: no_forward_declare QXmlStreamWriter


namespace OpenOrienteering {

PointSymbol::PointSymbol() noexcept
: Symbol { Symbol::Point }
, inner_color { nullptr }
, outer_color { nullptr }
, inner_radius { 1000 }
, outer_width { 0 }
, rotatable { false }
{
	// nothing else
}


PointSymbol::PointSymbol(const PointSymbol& proto)
: Symbol { proto }
, inner_color { proto.inner_color }
, outer_color { proto.outer_color }
, inner_radius { proto.inner_radius }
, outer_width { proto.outer_width }
, rotatable { proto.rotatable }
{
	elements.reserve(proto.elements.size());
	std::transform(begin(proto.elements), end(proto.elements), std::back_inserter(elements), [this](const auto& element) {
		auto new_element = Element { Symbol::duplicate(*element.symbol),
		                             std::unique_ptr<Object>(element.object->duplicate()) };
		new_element.object->setSymbol(new_element.symbol.get(), true);
		return new_element;
	});
}


PointSymbol::~PointSymbol() = default;


PointSymbol* PointSymbol::duplicate() const
{
	return new PointSymbol(*this);
}



bool PointSymbol::validate() const
{
	return std::all_of(begin(elements), end(elements), [](auto& element) {
		return element.symbol->validate() && element.object->validate();
	});
}



void PointSymbol::createRenderables(
        const Object* object,
        const VirtualCoordVector& coords,
        ObjectRenderables& output,
        RenderableOptions options ) const
{
	auto point = object->asPoint();
	auto rotation = isRotatable() ? -point->getRotation() : qreal(0);
	
	if (options.testFlag(Symbol::RenderBaselines))
	{
		const MapColor* dominant_color = guessDominantColor();
		if (dominant_color)
		{
			PointSymbol* point = Map::getUndefinedPoint();
			const MapColor* temp_color = point->getInnerColor();
			point->setInnerColor(dominant_color);
			
			point->createRenderablesScaled(coords[0], rotation, output);
			
			point->setInnerColor(temp_color);
		}
	}
	else
	{
		createRenderablesScaled(coords[0], rotation, output);
	}
}

void PointSymbol::createRenderablesScaled(const MapCoordF& coord, qreal rotation, ObjectRenderables& output, qreal coord_scale) const
{
	if (inner_color && inner_radius > 0)
		output.insertRenderable(new DotRenderable(this, coord));
	if (outer_color && outer_width > 0)
		output.insertRenderable(new CircleRenderable(this, coord));
	
	if (!elements.empty())
	{
		auto offset_x = coord.x();
		auto offset_y = coord.y();
		auto cosr = 1.0;
		auto sinr = 0.0;
		if (!qIsNull(rotation))
		{
			cosr = cos(rotation);
			sinr = sin(rotation);
		}
		
		for (auto& element : elements)
		{
			// Point symbol elements should not be entered into the map,
			// otherwise map settings like area hatching affect them
			Q_ASSERT(!element.object->getMap());
			
			const MapCoordVector& object_coords = element.object->getRawCoordinateVector();
			
			MapCoordVectorF transformed_coords;
			transformed_coords.reserve(object_coords.size());
			for (auto& coord : object_coords)
			{
				auto ox = coord_scale * coord.x();
				auto oy = coord_scale * coord.y();
				transformed_coords.emplace_back(ox * cosr - oy * sinr + offset_x,
				                                oy * cosr + ox * sinr + offset_y);
			}
			
			// TODO: if this point is rotated, it has to pass it on to its children to make it work that rotatable point objects can be children.
			// But currently only basic, rotationally symmetric points can be children, so it does not matter for now.
			element.symbol->createRenderables(element.object.get(), VirtualCoordVector(object_coords, transformed_coords), output, Symbol::RenderNormal);
		}
	}
}


void PointSymbol::createRenderablesIfCenterInside(const MapCoordF& point_coord, qreal rotation, const QPainterPath* outline, ObjectRenderables& output) const
{
	if (outline->contains(point_coord))
	{
		if (inner_color && inner_radius > 0)
		{
			output.insertRenderable(new DotRenderable(this, point_coord));
		}
		
		if (outer_color && outer_width > 0)
		{
			output.insertRenderable(new CircleRenderable(this, point_coord));
		}
	}
	
	if (!elements.empty())
	{
		auto offset_x = point_coord.x();
		auto offset_y = point_coord.y();
		auto cosr = 1.0;
		auto sinr = 0.0;
		if (!qIsNull(rotation))
		{
			cosr = qCos(rotation);
			sinr = qSin(rotation);
		}
		
		// Add elements which possibly need to be moved and rotated
		for (auto& element : elements)
		{
			// Point symbol elements should not be entered into the map,
			// otherwise map settings like area hatching affect them
			Q_ASSERT(!element.object->getMap());
			
			MapCoordF center{};
			const MapCoordVector& element_coords = element.object->getRawCoordinateVector();
			MapCoordVectorF transformed_coords;
			transformed_coords.reserve(element_coords.size());
			for (auto& coord : element_coords)
			{
				auto ex = coord.x();
				auto ey = coord.y();
				transformed_coords.emplace_back(ex * cosr - ey * sinr + offset_x,
				                                ey * cosr + ex * sinr + offset_y);
				
				center += transformed_coords.back();
			}
			
			if (!transformed_coords.empty() && outline->contains(center/transformed_coords.size()))
			{
				// TODO: if this point is rotated, it has to pass it on to its children to make it work that rotatable point objects can be children.
				// But currently only basic, rotationally symmetric points can be children, so it does not matter for now.
				element.symbol->createRenderables(element.object.get(), VirtualCoordVector(element_coords, transformed_coords), output, Symbol::RenderNormal);
			}
		}
	}
}


void PointSymbol::createPrimitivesIfCompletelyInside(const MapCoordF& point_coord, const QPainterPath* outline, ObjectRenderables& output) const
{
	if (inner_color && inner_radius > 0)
	{
		auto r = inner_radius/1000.0;
		if (outline->contains({point_coord.x()-r, point_coord.y()})
		    && outline->contains({point_coord.x(), point_coord.y()-r})
		    && outline->contains({point_coord.x()+r, point_coord.y()})
		    && outline->contains({point_coord.x(), point_coord.y()+r}) )
		{
			output.insertRenderable(new DotRenderable(this, point_coord));
		}
	}
	
	if (outer_color && outer_width > 0)
	{
		auto r = inner_radius/1000.0 + outer_width/2000.0;
		if (outline->contains({point_coord.x()-r, point_coord.y()})
		    && outline->contains({point_coord.x(), point_coord.y()-r})
		    && outline->contains({point_coord.x()+r, point_coord.y()})
		    && outline->contains({point_coord.x(), point_coord.y()+r}) )
		{
			output.insertRenderable(new CircleRenderable(this, point_coord));
		}
	}
}


void PointSymbol::createRenderablesIfCompletelyInside(const MapCoordF& point_coord, qreal rotation, const QPainterPath* outline, ObjectRenderables& output) const
{
	createPrimitivesIfCompletelyInside(point_coord, outline, output);
	
	if (!elements.empty())
	{
		auto offset_x = point_coord.x();
		auto offset_y = point_coord.y();
		auto cosr = 1.0;
		auto sinr = 0.0;
		if (!qIsNull(rotation))
		{
			cosr = qCos(rotation);
			sinr = qSin(rotation);
		}
		
		// Add elements which possibly need to be moved and rotated
		for (auto& element : elements)
		{
			// Point symbol elements should not be entered into the map,
			// otherwise map settings like area hatching affect them
			Q_ASSERT(!element.object->getMap());
			
			if (element.symbol->getType() == Symbol::Point)
			{
				auto coord = element.object->getRawCoordinateVector().front();
				auto transformed_coord = MapCoordF{ coord.x() * cosr - coord.y() * sinr + offset_x,
				                                    coord.y() * cosr + coord.x() * sinr + offset_y};
				static_cast<const PointSymbol*>(element.symbol.get())->createPrimitivesIfCompletelyInside(transformed_coord, outline, output);
				continue;
			}
			
			const MapCoordVector& element_coords = element.object->getRawCoordinateVector();
			MapCoordVectorF transformed_coords;
			transformed_coords.reserve(element_coords.size());
			for (auto& coord : element_coords)
			{
				auto ex = coord.x();
				auto ey = coord.y();
				transformed_coords.emplace_back(ex * cosr - ey * sinr + offset_x,
				                                ey * cosr + ex * sinr + offset_y);
				if (!outline->contains(transformed_coords.back()))
				{
					transformed_coords.clear();
					break;
				}
			}
			
			if (!transformed_coords.empty())
			{
				// TODO: if this point is rotated, it has to pass it on to its children to make it work that rotatable point objects can be children.
				// But currently only basic, rotationally symmetric points can be children, so it does not matter for now.
				element.symbol->createRenderables(element.object.get(), VirtualCoordVector(element_coords, transformed_coords), output, Symbol::RenderNormal);
			}
		}
	}
}


void PointSymbol::createPrimitivesIfPartiallyInside(const MapCoordF& point_coord, const QPainterPath* outline, ObjectRenderables& output) const
{
	if (inner_color && inner_radius > 0)
	{
		auto r = inner_radius/1000.0;
		if (outline->contains({point_coord.x()-r, point_coord.y()})
		    || outline->contains({point_coord.x(), point_coord.y()-r})
		    || outline->contains({point_coord.x()+r, point_coord.y()})
		    || outline->contains({point_coord.x(), point_coord.y()+r}) )
		{
			output.insertRenderable(new DotRenderable(this, point_coord));
		}
	}
	
	if (outer_color && outer_width > 0)
	{
		auto r = inner_radius/1000.0 + outer_width/2000.0;
		if (outline->contains({point_coord.x()-r, point_coord.y()})
		    || outline->contains({point_coord.x(), point_coord.y()-r})
		    || outline->contains({point_coord.x()+r, point_coord.y()})
		    || outline->contains({point_coord.x(), point_coord.y()+r}) )
		{
			output.insertRenderable(new CircleRenderable(this, point_coord));
		}
	}
}


void PointSymbol::createRenderablesIfPartiallyInside(const MapCoordF& point_coord, qreal rotation, const QPainterPath* outline, ObjectRenderables& output) const
{
	createPrimitivesIfPartiallyInside(point_coord, outline, output);
	
	if (!elements.empty())
	{
		auto offset_x = point_coord.x();
		auto offset_y = point_coord.y();
		auto cosr = 1.0;
		auto sinr = 0.0;
		if (!qIsNull(rotation))
		{
			cosr = qCos(rotation);
			sinr = qSin(rotation);
		}
		
		// Add elements which possibly need to be moved and rotated
		for (auto& element : elements)
		{
			// Point symbol elements should not be entered into the map,
			// otherwise map settings like area hatching affect them
			Q_ASSERT(!element.object->getMap());
			
			if (element.symbol->getType() == Symbol::Point)
			{
				auto coord = element.object->getRawCoordinateVector().front();
				auto transformed_coord = MapCoordF{ coord.x() * cosr - coord.y() * sinr + offset_x,
				                                    coord.y() * cosr + coord.x() * sinr + offset_y};
				static_cast<const PointSymbol*>(element.symbol.get())->createPrimitivesIfPartiallyInside(transformed_coord, outline, output);
				continue;
			}
			
			bool is_partially_inside = false;
			const MapCoordVector& element_coords = element.object->getRawCoordinateVector();
			MapCoordVectorF transformed_coords;
			transformed_coords.reserve(element_coords.size());
			for (auto& coord : element_coords)
			{
				auto ex = coord.x();
				auto ey = coord.y();
				transformed_coords.emplace_back(ex * cosr - ey * sinr + offset_x,
				                                ey * cosr + ex * sinr + offset_y);
				if (!is_partially_inside)
					is_partially_inside = outline->contains(transformed_coords.back());
			}
			
			if (is_partially_inside)
			{
				// TODO: if this point is rotated, it has to pass it on to its children to make it work that rotatable point objects can be children.
				// But currently only basic, rotationally symmetric points can be children, so it does not matter for now.
				element.symbol->createRenderables(element.object.get(), VirtualCoordVector(element_coords, transformed_coords), output, Symbol::RenderNormal);
			}
		}
	}
}



int PointSymbol::getNumElements() const
{
	return int(elements.size());
}

void PointSymbol::addElement(int pos, Object* object, Symbol* symbol)
{
	elements.insert(elements.begin() + pos, { std::unique_ptr<Symbol>(symbol), std::unique_ptr<Object>(object) });
}

Object* PointSymbol::getElementObject(int pos)
{
	return const_cast<Object*>(static_cast<const PointSymbol*>(this)->getElementObject(pos));
}

const Object* PointSymbol::getElementObject(int pos) const
{
	return elements[std::size_t(pos)].object.get();
}

Symbol* PointSymbol::getElementSymbol(int pos)
{
	return const_cast<Symbol*>(static_cast<const PointSymbol*>(this)->getElementSymbol(pos));
}

const Symbol* PointSymbol::getElementSymbol(int pos) const
{
	return elements[std::size_t(pos)].symbol.get();
}

void PointSymbol::deleteElement(int pos)
{
	elements.erase(elements.begin() + pos);
}

bool PointSymbol::isEmpty() const
{
	return getNumElements() == 0
	        && (!inner_color || inner_radius == 0)
	        && (!outer_color || outer_width == 0);
}

bool PointSymbol::isSymmetrical() const
{
	return std::none_of(begin(elements), end(elements), [](auto& element) {
		return element.symbol->getType() != Symbol::Point
		       || static_cast<const PointObject*>(element.object.get())->getCoord().isPositionEqualTo({0,0});
	});
}



void PointSymbol::colorDeletedEvent(const MapColor* color)
{
	bool change = false;
	
	if (color == inner_color)
	{
		inner_color = nullptr;
		change = true;
	}
	if (color == outer_color)
	{
		outer_color = nullptr;
		change = true;
	}
	
	for (auto& element : elements)
	{
		element.symbol->colorDeletedEvent(color);
		change = true;
	}
	
	if (change)
		resetIcon();
}

bool PointSymbol::containsColor(const MapColor* color) const
{
	if (color == inner_color)
		return true;
	if (color == outer_color)
		return true;
	
	return std::any_of(begin(elements), end(elements), [color](const auto& element) {
		return element.symbol->containsColor(color);
	});
}

const MapColor* PointSymbol::guessDominantColor() const
{
	bool have_inner_color = inner_color && inner_radius > 0;
	bool have_outer_color = outer_color && outer_width > 0;
	if (have_inner_color != have_outer_color)
		return have_inner_color ? inner_color : outer_color;
	else if (have_inner_color && have_outer_color)
	{
		if (inner_color->isWhite())
			return outer_color;
		else if (outer_color->isWhite())
			return inner_color;
		else
			return (qPow(inner_radius, 2) * M_PI > qPow(inner_radius + outer_width, 2) * M_PI - qPow(inner_radius, 2) * M_PI) ? inner_color : outer_color;
	}
	else if (!elements.empty())
	{
		// Hope that the first element's color is representative
		return elements.front().symbol->guessDominantColor();
	}
	return nullptr;
}


void PointSymbol::replaceColors(const MapColorMap& color_map)
{
	inner_color = color_map.value(inner_color);
	outer_color = color_map.value(outer_color);
	for (auto& element : elements)
		element.symbol->replaceColors(color_map);
}



void PointSymbol::scale(double factor)
{
	inner_radius = qRound(inner_radius * factor);
	outer_width = qRound(outer_width * factor);
	
	for (auto& element : elements)
	{
		element.symbol->scale(factor);
		element.object->scale(factor, factor);
	}
	
	resetIcon();
}


qreal PointSymbol::dimensionForIcon() const
{
	auto size = qreal(0);
	if (getOuterColor())
		size = 0.002 * (getInnerRadius() + getOuterWidth());
	else if (getInnerColor())
		size = 0.002 * getInnerRadius();
	
	QRectF extent;
	for (int i = 0; i < getNumElements(); ++i)
	{
		auto object = std::unique_ptr<Object>(getElementObject(i)->duplicate());
		object->setSymbol(getElementSymbol(i), true);
		object->update();
		rectIncludeSafe(extent, object->getExtent());
		object->clearRenderables();
	}
	if (extent.isValid())
	{
		auto w = 2 * std::max(std::abs(extent.left()), std::abs(extent.right()));
		auto h = 2 * std::max(std::abs(extent.top()), std::abs(extent.bottom()));
		return std::max(size, std::max(w, h));
	}
	
	return size;
}



void PointSymbol::saveImpl(QXmlStreamWriter& xml, const Map& map) const
{
	xml.writeStartElement(QString::fromLatin1("point_symbol"));
	if (rotatable)
		xml.writeAttribute(QString::fromLatin1("rotatable"), QString::fromLatin1("true"));
	xml.writeAttribute(QString::fromLatin1("inner_radius"), QString::number(inner_radius));
	xml.writeAttribute(QString::fromLatin1("inner_color"), QString::number(map.findColorIndex(inner_color)));
	xml.writeAttribute(QString::fromLatin1("outer_width"), QString::number(outer_width));
	xml.writeAttribute(QString::fromLatin1("outer_color"), QString::number(map.findColorIndex(outer_color)));
	int num_elements = getNumElements();
	xml.writeAttribute(QString::fromLatin1("elements"), QString::number(num_elements));
	for (auto& element : elements)
	{
		xml.writeStartElement(QString::fromLatin1("element"));
		element.symbol->save(xml, map);
		element.object->save(xml);
		xml.writeEndElement(/*element*/);
	}
	xml.writeEndElement(/*point_symbol*/);
}

bool PointSymbol::loadImpl(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict)
{
	if (xml.name() != QLatin1String("point_symbol"))
		return false;
	
	QXmlStreamAttributes attributes(xml.attributes());
	rotatable = (attributes.value(QLatin1String("rotatable")) == QLatin1String("true"));
	inner_radius = attributes.value(QLatin1String("inner_radius")).toInt();
	int temp = attributes.value(QLatin1String("inner_color")).toInt();
	inner_color = map.getColor(temp);
	outer_width = attributes.value(QLatin1String("outer_width")).toInt();
	temp = attributes.value(QLatin1String("outer_color")).toInt();
	outer_color = map.getColor(temp);
	int num_elements = attributes.value(QLatin1String("elements")).toInt();
	
	elements.reserve(qMin(num_elements, 10)); // 10 is not a limit
	for (int i = 0; xml.readNextStartElement(); ++i)
	{
		if (xml.name() == QLatin1String("element"))
		{
			auto symbol = std::unique_ptr<Symbol> {};
			while (xml.readNextStartElement())
			{
				if (xml.name() == QLatin1String("symbol") && !symbol)
				{
					symbol = Symbol::load(xml, map, symbol_dict);
				}
				else if (xml.name() == QLatin1String("object") && symbol)
				{
					auto object = Object::load(xml, nullptr, symbol_dict, symbol.get());
					elements.push_back({std::move(symbol), std::unique_ptr<Object>(object)});
				}
				else
				{
					xml.skipCurrentElement(); // unknown element
				}
			}
		}
		else
			xml.skipCurrentElement(); // unknown element
	}
	return true;
}

bool PointSymbol::equalsImpl(const Symbol* other, Qt::CaseSensitivity case_sensitivity) const
{
	const PointSymbol* point = static_cast<const PointSymbol*>(other);
	
	if (rotatable != point->rotatable)
		return false;
	if (!MapColor::equal(inner_color, point->inner_color))
		return false;
	if (inner_color && inner_radius != point->inner_radius)
		return false;
	if (!MapColor::equal(outer_color, point->outer_color))
		return false;
	if (outer_color && outer_width != point->outer_width)
		return false;
	
	if (elements.size() != point->elements.size())
		return false;
	return std::equal(begin(elements), end(elements), begin(point->elements), [case_sensitivity](auto& lhs, auto& rhs) {
		return lhs.symbol->equals(rhs.symbol.get(), case_sensitivity)
		       && lhs.object->equals(rhs.object.get(), false);
	});
}


}  // namespace OpenOrienteering
