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


#include "combined_symbol.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <numeric>

#include <QtGlobal>
#include <QFlags>
#include <QLatin1String>
#include <QString>
#include <QStringRef>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/map.h"
#include "core/map_color.h"
#include "core/objects/object.h"
#include "core/symbols/symbol.h"


namespace OpenOrienteering {

CombinedSymbol::CombinedSymbol()
: Symbol { Symbol::Combined }
, private_parts { false, false }
, parts { private_parts.size(), nullptr }
{
	Q_ASSERT(private_parts.size() == parts.size());
	// nothing else
}


CombinedSymbol::CombinedSymbol(const CombinedSymbol& proto)
: Symbol { proto }
, private_parts { proto.private_parts }
, parts { proto.parts }
{
	std::transform(begin(parts), end(parts), begin(private_parts), begin(parts), [this](auto part, auto is_private) {
		return (part && is_private) ? Symbol::duplicate(*part).release() : part;
	});
}


CombinedSymbol::~CombinedSymbol()
{
	std::transform(begin(parts), end(parts), begin(private_parts), begin(parts), [](auto part, auto is_private) {
		if (part && is_private)
			delete part;
		return nullptr;
	});
}


CombinedSymbol* CombinedSymbol::duplicate() const
{
	return new CombinedSymbol(*this);
}



bool CombinedSymbol::validate() const
{
	return std::all_of(begin(parts), end(parts), [](auto& symbol) { return symbol->validate(); });
}



void CombinedSymbol::createRenderables(
        const Object *object,
        const VirtualCoordVector &coords,
        ObjectRenderables &output,
        Symbol::RenderableOptions options) const        
{
	auto path = static_cast<const PathObject*>(object);
	PathPartVector path_parts = PathPart::calculatePathParts(coords);
	createRenderables(path, path_parts, output, options);
}

void CombinedSymbol::createRenderables(
        const PathObject* object,
        const PathPartVector& path_parts,
        ObjectRenderables &output,
        Symbol::RenderableOptions options) const
{
	for (auto subsymbol : parts)
	{
		if (subsymbol)
			subsymbol->createRenderables(object, path_parts, output, options);
	}
}

void CombinedSymbol::colorDeletedEvent(const MapColor* color)
{
	if (containsColor(color))
		resetIcon();
	
	auto is_private = begin(private_parts);
	for (auto subsymbol : parts)
	{
		if (*is_private)
			const_cast<Symbol*>(subsymbol)->colorDeletedEvent(color);
		++is_private;
	};
}

bool CombinedSymbol::containsColor(const MapColor* color) const
{
	return std::any_of(begin(parts), end(parts), [color](const auto& part) {
		return part && part->containsColor(color);
	});
}

const MapColor* CombinedSymbol::guessDominantColor() const
{
	// Speculative heuristic. Prefers areas and non-white colors.
	const MapColor* dominant_color = nullptr;
	for (auto subsymbol : parts)
	{
		if (subsymbol && subsymbol->getContainedTypes() & Symbol::Area)
		{
			dominant_color = subsymbol->guessDominantColor();
			if (dominant_color && !dominant_color->isWhite())
				return dominant_color;
		}
	}
	
	if (dominant_color)
		return dominant_color;
	
	for (auto subsymbol : parts)
	{
		if (subsymbol && !(subsymbol->getContainedTypes() & Symbol::Area))
		{
			dominant_color = subsymbol->guessDominantColor();
			if (dominant_color && !dominant_color->isWhite())
				return dominant_color;
		}
	}
	
	return dominant_color;
}


void CombinedSymbol::replaceColors(const MapColorMap& color_map)
{
	std::transform(begin(parts), end(parts), begin(private_parts), begin(parts), [&color_map](auto part, auto is_private) {
		if (part && is_private)
			const_cast<Symbol*>(part)->replaceColors(color_map);
		return part;
	});
}



bool CombinedSymbol::symbolChangedEvent(const Symbol* old_symbol, const Symbol* new_symbol)
{
	bool have_symbol = false;
	for (auto& subsymbol : parts)
	{
		if (subsymbol == old_symbol)
		{
			have_symbol = true;
			subsymbol = new_symbol;
		}
	}
	
	// always invalidate the icon, since the parts might have changed.
	resetIcon();
	
	return have_symbol;
}

bool CombinedSymbol::containsSymbol(const Symbol* symbol) const
{
	for (auto subsymbol : parts)
	{
		if (subsymbol == symbol)
			return true;
		
		if (subsymbol == nullptr)
			continue;
		
		if (subsymbol->getType() == Symbol::Combined)	// TODO: see TODO in SymbolDropDown constructor.
		{
			const CombinedSymbol* combined_symbol = reinterpret_cast<const CombinedSymbol*>(subsymbol);
			if (combined_symbol->containsSymbol(symbol))
				return true;
		}
	}
	return false;
}

void CombinedSymbol::scale(double factor)
{
	auto is_private = begin(private_parts);
	for (auto subsymbol : parts)
	{
		if (*is_private)
			const_cast<Symbol*>(subsymbol)->scale(factor);
		++is_private;
	};
	
	resetIcon();
}

Symbol::TypeCombination CombinedSymbol::getContainedTypes() const
{
	auto combination = TypeCombination(getType());
	
	for (auto subsymbol : parts)
	{
		if (subsymbol)
			combination |= subsymbol->getContainedTypes();
	}
	
	return combination;
}



void CombinedSymbol::saveImpl(QXmlStreamWriter& xml, const Map& map) const
{
	xml.writeStartElement(QString::fromLatin1("combined_symbol"));
	int num_parts = int(parts.size());
	xml.writeAttribute(QString::fromLatin1("parts"), QString::number(num_parts));
	
	auto is_private = begin(private_parts);
	for (const auto* subsymbol : parts)
	{
		xml.writeStartElement(QString::fromLatin1("part"));
		if (*is_private)
		{
			xml.writeAttribute(QString::fromLatin1("private"), QString::fromLatin1("true"));
			subsymbol->save(xml, map);
		}
		else
		{
			auto index = map.findSymbolIndex(subsymbol);
			xml.writeAttribute(QString::fromLatin1("symbol"), QString::number(index));
		}
		xml.writeEndElement(/*part*/);
		++is_private;
	}
	xml.writeEndElement(/*combined_symbol*/);
}

bool CombinedSymbol::loadImpl(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict)
{
	if (xml.name() != QLatin1String("combined_symbol"))
		return false;
	
	int num_parts = xml.attributes().value(QLatin1String("parts")).toInt();
	temp_part_indices.reserve(num_parts % 10); // 10 is not the limit
	private_parts.clear();
	private_parts.reserve(num_parts % 10);
	parts.clear();
	parts.reserve(num_parts % 10);
	while (xml.readNextStartElement())
	{
		if (xml.name() == QLatin1String("part"))
		{
			bool is_private = (xml.attributes().value(QLatin1String("private")) == QLatin1String("true"));
			private_parts.push_back(is_private);
			if (is_private)
			{
				xml.readNextStartElement();
				parts.push_back(Symbol::load(xml, map, symbol_dict).release());
				temp_part_indices.push_back(-1);
			}
			else
			{
				int temp = xml.attributes().value(QLatin1String("symbol")).toInt();
				temp_part_indices.push_back(temp);
				parts.push_back(nullptr);
			}
			xml.skipCurrentElement();
		}
		else
			xml.skipCurrentElement(); // unknown
	}
	
	return true;
}

bool CombinedSymbol::equalsImpl(const Symbol* other, Qt::CaseSensitivity case_sensitivity) const
{
	const CombinedSymbol* combination = static_cast<const CombinedSymbol*>(other);
	return parts.size() == combination->parts.size()
	       && std::equal(begin(private_parts), end(private_parts), begin(combination->private_parts))
	       && std::equal(begin(parts), end(parts), begin(combination->parts),
	                      [case_sensitivity](const auto lhs, const auto rhs)
	{
		return (!lhs && !rhs)
		       || (lhs && rhs && lhs->equals(rhs, case_sensitivity));
	});
}

bool CombinedSymbol::loadingFinishedEvent(Map* map)
{
	const auto num_symbols = map->getNumSymbols();
	const auto last = std::find_if(begin(temp_part_indices), end(temp_part_indices),
	                               [num_symbols](const auto index)
	{
		return index >= num_symbols;
	});
	
	std::transform(begin(temp_part_indices), last, begin(parts), begin(parts),
	               [map](const auto index, const auto& subsymbol) -> const Symbol*
	{
		return (index < 0) ? subsymbol : map->getSymbol(index);
	});
	
	return last == end(temp_part_indices);
}



qreal CombinedSymbol::dimensionForIcon() const
{
	return std::accumulate(begin(parts), end(parts), qreal(0), [](qreal value, auto subsymbol)
	{
		return subsymbol ? qMax(value, subsymbol->dimensionForIcon()) : value;
	});
}


qreal CombinedSymbol::calculateLargestLineExtent() const
{
	return std::accumulate(begin(parts), end(parts), qreal(0), [](qreal value, auto subsymbol)
	{
		return subsymbol ? qMax(value, subsymbol->calculateLargestLineExtent()) : value;
	});
}



void CombinedSymbol::setPart(int i, const Symbol* symbol, bool is_private)
{
	const auto index = std::size_t(i);
	
	if (private_parts[index])
		delete parts[index];
	
	parts[index] = symbol;
	private_parts[index] = symbol ? is_private : false;
}


}  // namespace OpenOrienteering
