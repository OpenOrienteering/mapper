/*
 *    Copyright 2019 Libor Pecháček
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

#include <QDebug>
#include <QLatin1String>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/map.h"
#include "core/symbols/symbol.h"
#include "core/symbols/symbol.h"
#include "symbol_undo.h"
#include "util/xml_stream_util.h"


namespace literal
{
	const QLatin1String type("type");
	const QLatin1String id("id");
	const QLatin1String add("add");
	const QLatin1String change("change");
	const QLatin1String remove("remove");
}  // namespace literal


namespace OpenOrienteering {

SymbolUndoStep::SymbolUndoStep(Map* map, SymbolUndoStep::SymbolChange change_type, Symbol* symbol, Symbol* new_symbol)
    : UndoStep(UndoStep::SymbolUndoStepType, map)
    , change_type(change_type)
    , symbol_ptr(symbol)
    , new_symbol_ptr(new_symbol)
    , symbol_pos(0)
{
	// nothing
}


SymbolUndoStep::SymbolUndoStep(Map* map, SymbolUndoStep::SymbolChange change_type, Symbol* symbol, int pos)
    : SymbolUndoStep(map, change_type, symbol, nullptr)
{
	symbol_pos = pos;
}


SymbolUndoStep::SymbolUndoStep(Map* map)
    : SymbolUndoStep(map, UndefinedChange, nullptr, nullptr)
{
	// nothing
}


UndoStep* SymbolUndoStep::undo()
{
	UndoStep* redo_step = nullptr;
	switch (change_type)
	{
	case AddSymbol:
		map->addSymbol(symbol_ptr, symbol_pos);
		redo_step = new SymbolUndoStep(map, RemoveSymbol, symbol_ptr, symbol_pos);
		break;
	case RemoveSymbol:
		{
			const auto current_symbol_pos = map->findSymbolIndex(symbol_ptr);
			Q_ASSERT(current_symbol_pos >= 0);
			redo_step = new SymbolUndoStep(map, AddSymbol, map->releaseSymbol(current_symbol_pos), current_symbol_pos);
		}
		break;
	case ChangeSymbol:
		{
			const auto current_symbol_pos = map->findSymbolIndex(symbol_ptr);
			Q_ASSERT(current_symbol_pos >= 0);
			auto old_symbol_ptr = map->setSymbol(new_symbol_ptr, current_symbol_pos);
			redo_step = new SymbolUndoStep(map, SymbolUndoStep::ChangeSymbol,
			                               map->getSymbol(current_symbol_pos),
			                               old_symbol_ptr.release());
		}
		break;
	case UndefinedChange:
		Q_ASSERT(false);
		redo_step = new NoOpUndoStep(map, true);
		break;
	}

	Q_ASSERT(redo_step);
	return redo_step;
}

bool SymbolUndoStep::isValid() const
{
	return change_type != UndefinedChange;
}

// TODO - incomplete, leverage unique symbol id
void SymbolUndoStep::saveImpl(QXmlStreamWriter &xml) const
{
	UndoStep::saveImpl(xml);
	XmlElementWriter change_element(xml, literal::change);

	switch (change_type)
	{
	case AddSymbol:
		change_element.writeAttribute(literal::type, literal::add);
		change_element.writeAttribute(literal::id, symbol_pos);
		symbol_ptr->save(xml, *map);
		break;
	case RemoveSymbol:
		{
			change_element.writeAttribute(literal::type, literal::remove);
			change_element.writeAttribute(literal::id, symbol_pos);
		}
		break;
	case ChangeSymbol:
		{
			change_element.writeAttribute(literal::type, literal::change);
			change_element.writeAttribute(literal::id, symbol_pos);
			new_symbol_ptr->save(xml, *map);
		}
		break;
	case UndefinedChange:
		qWarning() << this << " Attempt to save invalid undo change";
		break;
	}
}

void SymbolUndoStep::loadImpl(QXmlStreamReader &xml, SymbolDictionary &)
{
	if (xml.name() == literal::change)
	{
		XmlElementReader change_element(xml);
		const auto type = change_element.attribute<QStringRef>(literal::type);
		if (type == literal::add)
		{
			change_type = AddSymbol;
			symbol_pos = change_element.attribute<int>(literal::id);
			SymbolDictionary fake_symbol_dict;
			symbol_ptr = Symbol::load(xml, *map, fake_symbol_dict).release();
		}
		else if (type == literal::remove)
		{
			change_type = RemoveSymbol;
			symbol_pos = change_element.attribute<int>(literal::id);
		}
		else if (type == literal::change)
		{
			change_type = ChangeSymbol;
			symbol_pos = change_element.attribute<int>(literal::id);
			SymbolDictionary fake_symbol_dict;
			symbol_ptr = Symbol::load(xml, *map, fake_symbol_dict).release();
		}
		else
		{
			qWarning() << "Unknown symbol change type \"" << type << "\"";
		}
	}
}

}  // namespace OpenOrienteering
