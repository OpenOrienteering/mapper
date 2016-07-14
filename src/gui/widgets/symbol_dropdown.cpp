/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012, 2013, 2014 Kai Pastor
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

#include <qmath.h>
#include <QDebug>
#include <QFile>
#include <QPainter>
#include <QTextDocument>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "../../symbol_combined.h"
#include "../../map.h"

// ### SymbolDropDown ###

// allow explicit use of Symbol pointers in QVariant
Q_DECLARE_METATYPE(Symbol*)

SymbolDropDown::SymbolDropDown(const Map* map, int filter, const Symbol* initial_symbol, const Symbol* excluded_symbol, QWidget* parent)
 : QComboBox(parent)
{
	num_custom_items = 0;
	addItem(tr("- none -"), QVariant::fromValue<Symbol*>(NULL));
	
	int size = map->getNumSymbols();
	for (int i = 0; i < size; ++i)
	{
		const Symbol* symbol = map->getSymbol(i);
		if (!(symbol->getType() & filter))
			continue;
		if (symbol == excluded_symbol)
			continue;
		if (symbol->getType() == Symbol::Combined)	// TODO: if point objects start to be able to contain objects of other ordinary symbols, add a check for these here, too, to prevent circular references
		{
			const CombinedSymbol* combined_symbol = reinterpret_cast<const CombinedSymbol*>(symbol);
			if (combined_symbol->containsSymbol(excluded_symbol))
				continue;
		}
		
		QString symbol_name = symbol->getNumberAsString() + QLatin1Char(' ') + symbol->getPlainTextName();
		addItem(QPixmap::fromImage(symbol->getIcon(map)), symbol_name, QVariant::fromValue<const Symbol*>(symbol));
	}
	setSymbol(initial_symbol);
}

const Symbol* SymbolDropDown::symbol() const
{
	QVariant data = itemData(currentIndex());
	if (data.canConvert<const Symbol*>())
		return data.value<const Symbol*>();
	else
		return NULL;
}

void SymbolDropDown::setSymbol(const Symbol* symbol)
{
	setCurrentIndex(findData(QVariant::fromValue<const Symbol*>(symbol)));
}

void SymbolDropDown::addCustomItem(const QString& text, int id)
{
	insertItem(1 + num_custom_items, text, QVariant(id));
	++num_custom_items;
}

int SymbolDropDown::customID() const
{
	QVariant data = itemData(currentIndex());
	if (data.canConvert<int>())
		return data.value<int>();
	else
		return -1;
}

void SymbolDropDown::setCustomItem(int id)
{
	setCurrentIndex(findData(QVariant(id)));
}


// ### SymbolDropDownDelegate ###

SymbolDropDownDelegate::SymbolDropDownDelegate(int symbol_type_filter, QObject* parent)
 : QItemDelegate(parent), symbol_type_filter(symbol_type_filter)
{
}

QWidget* SymbolDropDownDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	Q_UNUSED(option);
	QVariantList list = index.data(Qt::UserRole).toList();
	SymbolDropDown* widget
		= new SymbolDropDown(list.at(0).value<const Map*>(), symbol_type_filter,
							 list.at(1).value<const Symbol*>(), NULL, parent);
	
	connect(widget, SIGNAL(currentIndexChanged(int)), this, SLOT(emitCommitData()));
	return widget;
}

void SymbolDropDownDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	SymbolDropDown* widget = static_cast<SymbolDropDown*>(editor);
	const Symbol* symbol = index.data(Qt::UserRole).toList().at(1).value<const Symbol*>();
	widget->setSymbol(symbol);
}

void SymbolDropDownDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	SymbolDropDown* widget = static_cast<SymbolDropDown*>(editor);
	const Symbol* symbol = widget->symbol();
	QVariantList list = index.data(Qt::UserRole).toList();
	list[1] = qVariantFromValue<const Symbol*>(symbol);
	model->setData(index, list, Qt::UserRole);
	
	if (symbol)
	{
		model->setData(index, QVariant(symbol->getNumberAsString() + QLatin1Char(' ') + symbol->getPlainTextName()), Qt::EditRole);
		model->setData(index, symbol->getIcon(list[0].value<const Map*>()), Qt::DecorationRole);
	}
	else
	{
		model->setData(index, tr("- None -"), Qt::EditRole);
		model->setData(index, QVariant(), Qt::DecorationRole);
	}
}

void SymbolDropDownDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	Q_UNUSED(index);
	editor->setGeometry(option.rect);
}

void SymbolDropDownDelegate::emitCommitData()
{
	emit commitData(qobject_cast<QWidget*>(sender()));
}
