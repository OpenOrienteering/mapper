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

#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalMapper>
#include <QSpinBox>

#include "core/symbols/combined_symbol.h"
#include "gui/widgets/symbol_dropdown.h"
#include "gui/symbols/symbol_setting_dialog.h"


// ### CombinedSymbol ###

SymbolPropertiesWidget* CombinedSymbol::createPropertiesWidget(SymbolSettingDialog* dialog)
{
	return new CombinedSymbolSettings(this, dialog);
}



// ### CombinedSymbolSettings ###

const int CombinedSymbolSettings::max_count = 5;

CombinedSymbolSettings::CombinedSymbolSettings(CombinedSymbol* symbol, SymbolSettingDialog* dialog) 
 : SymbolPropertiesWidget(symbol, dialog), symbol(symbol)
{
	const CombinedSymbol* source_symbol = static_cast<const CombinedSymbol*>(dialog->getUnmodifiedSymbol());
	Map* source_map = dialog->getSourceMap();
	
	QFormLayout* layout = new QFormLayout();
	
	number_edit = new QSpinBox();
	number_edit->setRange(2, qMax<int>(max_count, symbol->getNumParts()));
	number_edit->setValue(symbol->getNumParts());
	connect(number_edit, SIGNAL(valueChanged(int)), this, SLOT(numberChanged(int)));
	layout->addRow(tr("&Number of parts:"), number_edit);
	
	QSignalMapper* button_signal_mapper = new QSignalMapper(this);
	connect(button_signal_mapper, SIGNAL(mapped(int)), this, SLOT(editClicked(int)));
	QSignalMapper* symbol_signal_mapper = new QSignalMapper(this);
	connect(symbol_signal_mapper, SIGNAL(mapped(int)), this, SLOT(symbolChanged(int)));
	
	symbol_labels = new QLabel*[max_count];
	symbol_edits = new SymbolDropDown*[max_count];
	edit_buttons = new QPushButton*[max_count];
	for (int i = 0; i < max_count; ++i)
	{
		symbol_labels[i] = new QLabel(tr("Symbol %1:").arg(i+1));
		
		symbol_edits[i] = new SymbolDropDown(source_map, Symbol::Line | Symbol::Area | Symbol::Combined, ((int)symbol->parts.size() > i) ? symbol->parts[i] : nullptr, source_symbol);
		symbol_edits[i]->addCustomItem(tr("- Private line symbol -"), 1);
		symbol_edits[i]->addCustomItem(tr("- Private area symbol -"), 2);
		if (((int)symbol->parts.size() > i) && symbol->isPartPrivate(i))
			symbol_edits[i]->setCustomItem((symbol->getPart(i)->getType() == Symbol::Line) ? 1 : 2);
		connect(symbol_edits[i], SIGNAL(currentIndexChanged(int)), symbol_signal_mapper, SLOT(map()));
		symbol_signal_mapper->setMapping(symbol_edits[i], i);
		
		edit_buttons[i] = new QPushButton(tr("Edit private symbol..."));
		edit_buttons[i]->setEnabled((int)symbol->parts.size() > i && symbol->private_parts[i]);
		connect(edit_buttons[i], SIGNAL(clicked()), button_signal_mapper, SLOT(map()));
		button_signal_mapper->setMapping(edit_buttons[i], i);
		
		QHBoxLayout* row_layout = new QHBoxLayout();
		row_layout->addWidget(symbol_edits[i]);
		row_layout->addWidget(edit_buttons[i]);
		layout->addRow(symbol_labels[i], row_layout);
		
		if (i >= symbol->getNumParts())
		{
			symbol_labels[i]->hide();
			symbol_edits[i]->hide();
			edit_buttons[i]->hide();
		}
	}
	
	QWidget* widget = new QWidget;
	widget->setLayout(layout);
	addPropertiesGroup(tr("Combination settings"), widget);
}

CombinedSymbolSettings::~CombinedSymbolSettings()
{
	delete[] symbol_labels;
	delete[] symbol_edits;
	delete[] edit_buttons;
}

void CombinedSymbolSettings::numberChanged(int value)
{
	int old_num_items = symbol->getNumParts();
	if (old_num_items == value)
		return;
	
	int num_items = value;
	symbol->setNumParts(num_items);
	for (int i = 0; i < max_count; ++i)
	{
		symbol_labels[i]->setVisible(i < num_items);
		symbol_edits[i]->setVisible(i < num_items);
		edit_buttons[i]->setVisible(i < num_items);
		
		if (i >= old_num_items && i < num_items)
		{
			// This item appears now
			symbol->setPart(i, nullptr, false);
			symbol_edits[i]->blockSignals(true);
			symbol_edits[i]->setSymbol(nullptr);
			symbol_edits[i]->blockSignals(false);
			edit_buttons[i]->setEnabled(symbol->isPartPrivate(i));
		}
	}
	emit propertiesModified();
}

void CombinedSymbolSettings::symbolChanged(int index)
{
	if (symbol_edits[index]->symbol())
		symbol->setPart(index, symbol_edits[index]->symbol(), false);
	else if (symbol_edits[index]->customID() > 0)
	{
		// Changing to a private symbol
		Symbol::Type new_symbol_type;
		if (symbol_edits[index]->customID() == 1)
			new_symbol_type = Symbol::Line;
		else // if (symbol_edits[index]->customID() == 2)
			new_symbol_type = Symbol::Area;
		
		Symbol* new_symbol = nullptr;
		if (symbol->getPart(index) && new_symbol_type == symbol->getPart(index)->getType())
		{
			if (QMessageBox::question(this, tr("Change from public to private symbol"),
				tr("Take the old symbol as template for the private symbol?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				new_symbol = symbol->getPart(index)->duplicate();
			}
		}
		
		if (!new_symbol)
			new_symbol = Symbol::getSymbolForType(new_symbol_type);
		
		symbol->setPart(index, new_symbol, true);
		editClicked(index);
	}
	else
		symbol->setPart(index, nullptr, false);
	
	edit_buttons[index]->setEnabled(symbol->isPartPrivate(index));
	emit propertiesModified();
}

void CombinedSymbolSettings::editClicked(int index)
{
	Q_ASSERT(symbol->isPartPrivate(index));
	if (!symbol->isPartPrivate(index))
		return;
	
	QScopedPointer<Symbol> part(symbol->getPart(index)->duplicate());
	SymbolSettingDialog sub_dialog(part.data(), dialog->getSourceMap(), this);
	sub_dialog.setWindowModality(Qt::WindowModal);
	if (sub_dialog.exec() == QDialog::Accepted)
	{
		symbol->setPart(index, sub_dialog.getNewSymbol().release(), true);
		emit propertiesModified();
	}
}

void CombinedSymbolSettings::reset(Symbol* symbol)
{
	Q_ASSERT(symbol->getType() == Symbol::Combined);
	
	SymbolPropertiesWidget::reset(symbol);
	
	this->symbol = symbol->asCombined();
	updateContents();
}

void CombinedSymbolSettings::updateContents()
{
	int num_parts = symbol->getNumParts();
	for (int i = 0; i < max_count; ++i)
	{
		symbol_edits[i]->blockSignals(true);
		if (i < num_parts)
		{
			if (symbol->isPartPrivate(i))
				symbol_edits[i]->setCustomItem((symbol->getPart(i)->getType() == Symbol::Line) ? 1 : 2);
			else
				symbol_edits[i]->setSymbol(symbol->parts[i]);
			symbol_edits[i]->show();
			symbol_labels[i]->show();
			edit_buttons[i]->setEnabled(symbol->isPartPrivate(i));
			edit_buttons[i]->show();
		}
		else
		{
			symbol_edits[i]->setSymbol(nullptr);
			symbol_edits[i]->hide();
			symbol_labels[i]->hide();
			edit_buttons[i]->hide();
		}
		symbol_edits[i]->blockSignals(false);
	}
	
	number_edit->blockSignals(true);
	number_edit->setValue(num_parts);
	number_edit->blockSignals(false);
}
