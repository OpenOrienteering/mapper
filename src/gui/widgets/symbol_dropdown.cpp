/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012, 2013, 2014, 2017 Kai Pastor
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


#include "symbol_dropdown.h"

#include <Qt>
#include <QAbstractItemModel>
#include <QImage>
#include <QLatin1Char>
#include <QList>
#include <QMetaType>
#include <QModelIndex>
#include <QPixmap>
#include <QVariant>
#include <QWidget>

#include "core/map.h"
#include "core/symbols/combined_symbol.h"
#include "core/symbols/symbol.h"
#include "gui/util_gui.h"
#include "util/backports.h"


// allow explicit use of Symbol pointers in QVariant
Q_DECLARE_METATYPE(OpenOrienteering::Symbol*)



namespace OpenOrienteering {

// ### SymbolDropDown ###


SymbolDropDown::SymbolDropDown(const Map* map, int filter, const Symbol* initial_symbol, const Symbol* excluded_symbol, QWidget* parent)
 : QComboBox(parent)
{
	num_custom_items = 0;
	addItem(tr("- none -"), QVariant::fromValue<const Symbol*>(nullptr));
	
	int size = map->getNumSymbols();
	for (int i = 0; i < size; ++i)
	{
		const auto symbol = map->getSymbol(i);
		if (!(symbol->getType() & filter))
			continue;
		if (symbol == excluded_symbol)
			continue;
		if (symbol->getType() == Symbol::Combined)	// TODO: if point objects start to be able to contain objects of other ordinary symbols, add a check for these here, too, to prevent circular references
		{
			auto combined_symbol = static_cast<const CombinedSymbol*>(symbol);
			if (combined_symbol->containsSymbol(excluded_symbol))
				continue;
		}
		
		auto symbol_name = QString{symbol->getNumberAsString() + QLatin1Char(' ')
		                           + Util::plainText(map->translate(symbol->getName()))};
		addItem(QPixmap::fromImage(symbol->getIcon(map)), symbol_name, QVariant::fromValue<const Symbol*>(symbol));
	}
	setSymbol(initial_symbol);
}


SymbolDropDown::~SymbolDropDown() = default;



const Symbol* SymbolDropDown::symbol() const
{
	auto data = itemData(currentIndex());
	if (data.canConvert<const Symbol*>())
		return data.value<const Symbol*>();
	else
		return nullptr;
}

void SymbolDropDown::setSymbol(const Symbol* symbol)
{
	setCurrentIndex(findData(QVariant::fromValue<const Symbol*>(symbol)));
}

void SymbolDropDown::addCustomItem(const QString& text, int id)
{
	insertItem(1 + num_custom_items, text, QVariant{id});
	++num_custom_items;
}

int SymbolDropDown::customID() const
{
	auto data = itemData(currentIndex());
	if (data.canConvert<int>())
		return data.value<int>();
	else
		return -1;
}

void SymbolDropDown::setCustomItem(int id)
{
	setCurrentIndex(findData(QVariant{id}));
}



// ### SymbolDropDownDelegate ###

SymbolDropDownDelegate::SymbolDropDownDelegate(int symbol_type_filter, QObject* parent)
: QItemDelegate{parent}
, symbol_type_filter{symbol_type_filter}
{
	// nothing else
}


SymbolDropDownDelegate::~SymbolDropDownDelegate() = default;



QWidget* SymbolDropDownDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const
{
	auto list = index.data(Qt::UserRole).toList();
	auto widget	= new SymbolDropDown(list.at(0).value<const Map*>(), symbol_type_filter,
	                                 list.at(1).value<const Symbol*>(), nullptr, parent);
	
	connect(widget, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SymbolDropDownDelegate::emitCommitData);
	return widget;
}

void SymbolDropDownDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	auto widget = static_cast<SymbolDropDown*>(editor);
	auto symbol = index.data(Qt::UserRole).toList().at(1).value<const Symbol*>();
	widget->setSymbol(symbol);
}

void SymbolDropDownDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	auto widget = static_cast<SymbolDropDown*>(editor);
	auto symbol = widget->symbol();
	auto list = index.data(Qt::UserRole).toList();
	list[1] = qVariantFromValue<const Symbol*>(symbol);
	model->setData(index, list, Qt::UserRole);
	
	if (symbol)
	{
		auto map = list.at(0).value<const Map*>();
		auto text = QString{symbol->getNumberAsString() + QLatin1Char(' ')
		                    + Util::plainText(map->translate(symbol->getName()))};
		model->setData(index, QVariant{text}, Qt::EditRole);
		model->setData(index, symbol->getIcon(list[0].value<const Map*>()), Qt::DecorationRole);
	}
	else
	{
		model->setData(index, tr("- None -"), Qt::EditRole);
		model->setData(index, {}, Qt::DecorationRole);
	}
}

void SymbolDropDownDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const
{
	editor->setGeometry(option.rect);
}

void SymbolDropDownDelegate::emitCommitData()
{
	emit commitData(qobject_cast<QWidget*>(sender()));
}


}  // namespace OpenOrienteering
