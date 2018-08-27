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


#include "combined_symbol_settings.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QAbstractButton>
#include <QComboBox>
#include <QDialog>
#include <QFlags>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QString>
#include <QWidget>

#include "core/symbols/combined_symbol.h"
#include "core/symbols/symbol.h"
#include "gui/symbols/symbol_setting_dialog.h"
#include "gui/widgets/symbol_dropdown.h"
#include "util/backports.h"


namespace OpenOrienteering {

// ### CombinedSymbol ###

SymbolPropertiesWidget* CombinedSymbol::createPropertiesWidget(SymbolSettingDialog* dialog)
{
	return new CombinedSymbolSettings(this, dialog);
}



// ### CombinedSymbolSettings ###

struct CombinedSymbolSettings::Widgets
{
	QLabel*         label;
	SymbolDropDown* edit;
	QPushButton*    button;
};



CombinedSymbolSettings::CombinedSymbolSettings(CombinedSymbol* symbol, SymbolSettingDialog* dialog) 
: SymbolPropertiesWidget(symbol, dialog)
, symbol(symbol)
{
	auto source_symbol = static_cast<const CombinedSymbol*>(dialog->getUnmodifiedSymbol());
	auto source_map = dialog->getSourceMap();
	
	auto layout = new QFormLayout();
	
	number_edit = new QSpinBox();
	number_edit->setRange(2, qMax(5, symbol->getNumParts()));
	number_edit->setValue(symbol->getNumParts());
	connect(number_edit, QOverload<int>::of(&QSpinBox::valueChanged), this, &CombinedSymbolSettings::numberChanged);
	layout->addRow(tr("&Number of parts:"), number_edit);
	
	widgets.resize(std::size_t(number_edit->maximum()));
	for (auto i = 0u; i < widgets.size(); ++i)
	{
		auto label = new QLabel(tr("Symbol %1:").arg(i+1));
		
		auto edit = new SymbolDropDown(source_map, Symbol::Line | Symbol::Area | Symbol::Combined, (symbol->parts.size() > i) ? symbol->parts[i] : nullptr, source_symbol);
		edit->addCustomItem(tr("- Private line symbol -"), 1);
		edit->addCustomItem(tr("- Private area symbol -"), 2);
		connect(edit, QOverload<int>::of(&SymbolDropDown::currentIndexChanged), this, &CombinedSymbolSettings::symbolChanged);
		
		auto button = new QPushButton(tr("Edit private symbol..."));
		connect(button, QOverload<bool>::of(&QAbstractButton::clicked), this, QOverload<>::of(&CombinedSymbolSettings::editButtonClicked));
		
		auto row_layout = new QHBoxLayout();
		row_layout->addWidget(edit);
		row_layout->addWidget(button);
		layout->addRow(label, row_layout);
		
		widgets[i] = { label, edit, button };
	}
	updateContents();
	
	auto widget = new QWidget;
	widget->setLayout(layout);
	addPropertiesGroup(tr("Combination settings"), widget);
}


CombinedSymbolSettings::~CombinedSymbolSettings() = default;



void CombinedSymbolSettings::reset(Symbol* symbol)
{
	if (Q_UNLIKELY(symbol->getType() != Symbol::Combined))
	{
		qWarning("Not a combined symbol: %s", symbol ? "nullptr" : qPrintable(symbol->getPlainTextName()));
		return;
	}
	
	SymbolPropertiesWidget::reset(symbol);
	this->symbol = static_cast<CombinedSymbol*>(symbol);
	updateContents();
}



void CombinedSymbolSettings::updateContents()
{
	auto num_parts = symbol->parts.size();
	for (auto i = 0u; i < num_parts; ++i)
	{
		auto& w = widgets[i];
		auto is_private = symbol->isPartPrivate(int(i));
		QSignalBlocker block{w.edit};
		w.label->setVisible(true);
		if (is_private)
			w.edit->setCustomItem((symbol->parts[i]->getType() == Symbol::Line) ? 1 : 2);
		else
			w.edit->setSymbol(symbol->parts[i]);
		w.edit->setVisible(true);
		w.button->setEnabled(is_private);
		w.button->setVisible(true);
	}
	for (auto i = num_parts; i < widgets.size(); ++i)
	{
		auto& w = widgets[i];
		w.label->setVisible(false);
		w.edit->setSymbol(nullptr);
		w.edit->setVisible(false);
		w.button->setVisible(false);
	}
	
	QSignalBlocker block{number_edit};
	number_edit->setValue(int(num_parts));
}



void CombinedSymbolSettings::numberChanged(int value)
{
	int old_num_items = symbol->getNumParts();
	if (old_num_items == value)
		return;
	
	value =	qMin(int(widgets.size()), value);
	symbol->setNumParts(value);
	for (auto i = std::size_t(old_num_items); i < std::size_t(value); ++i)
	{
		// This item appears now.
		auto& w = widgets[i];
		QSignalBlocker block{w.edit};
		w.label->setVisible(true);
		w.edit->setSymbol(nullptr);
		w.edit->setVisible(true);
		w.button->setEnabled(false);
		w.button->setVisible(true);
	}
	for (auto i = std::size_t(value); i < std::size_t(old_num_items); ++i)
	{
		// This item disappears now.
		auto& w = widgets[i];
		w.label->setVisible(false);
		w.edit->setVisible(false);
		w.button->setVisible(false);
	}
	emit propertiesModified();
}



void CombinedSymbolSettings::symbolChanged()
{
	const auto* edit = sender();
	auto widget = std::find_if(begin(widgets), end(widgets), [edit](const auto& w) {
		return w.edit == edit;
	});
	if (widget == end(widgets))
		return;
	
	auto index = int(std::distance(begin(widgets), widget));
	if (widget->edit->symbol())
	{
		symbol->setPart(index, widget->edit->symbol(), false);
	}
	else if (widget->edit->customID() > 0)
	{
		// Changing to a private symbol
		Symbol::Type new_symbol_type;
		if (widget->edit->customID() == 1)
			new_symbol_type = Symbol::Line;
		else // if (symbol_edits[index]->customID() == 2)
			new_symbol_type = Symbol::Area;
		
		std::unique_ptr<Symbol> new_symbol;
		if (symbol->getPart(index) && new_symbol_type == symbol->getPart(index)->getType())
		{
			if (QMessageBox::question(this, tr("Change from public to private symbol"),
				tr("Take the old symbol as template for the private symbol?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				new_symbol = duplicate(*symbol->getPart(index));
			}
		}
		
		if (!new_symbol)
			new_symbol = Symbol::makeSymbolForType(new_symbol_type);
		
		symbol->setPart(index, new_symbol.release(), true);
		showEditDialog(index);
	}
	else
	{
		symbol->setPart(index, nullptr, false);
	}
	
	widget->button->setEnabled(symbol->isPartPrivate(index));
	emit propertiesModified();
}



void CombinedSymbolSettings::editButtonClicked()
{
	const auto* button = sender();
	auto widget = std::find_if(begin(widgets), end(widgets), [button](const auto& w) {
		return w.button == button;
	});
	if (widget != end(widgets))
		showEditDialog(int(std::distance(begin(widgets), widget)));
}


void CombinedSymbolSettings::showEditDialog(int index)
{
	if (Q_UNLIKELY(!symbol->isPartPrivate(index)))
	{
		qWarning("Symbol settings dialog requested for non-private subsymbol %d", index);
		return;
	}
	
	auto part = Symbol::duplicate(*symbol->getPart(index));
	SymbolSettingDialog sub_dialog(part.get(), dialog->getSourceMap(), this);
	sub_dialog.setWindowModality(Qt::WindowModal);
	if (sub_dialog.exec() == QDialog::Accepted)
	{
		symbol->setPart(index, sub_dialog.getNewSymbol().release(), true);
		emit propertiesModified();
	}
}


}  // namespace OpenOrienteering
