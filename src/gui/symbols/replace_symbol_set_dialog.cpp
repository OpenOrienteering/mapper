/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#include "replace_symbol_set_dialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QSet>
#include <QTableWidget>
#include <QVBoxLayout>

#include "fileformats/file_format.h"
#include "gui/main_window.h"
#include "gui/widgets/symbol_dropdown.h"
#include "core/map.h"
#include "core/objects/object.h"
#include "undo_manager.h"
#include "util.h"


ReplaceSymbolSetDialog::ReplaceSymbolSetDialog(QWidget* parent, Map* map, Map* symbol_map)
 : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint), map(map), symbol_map(symbol_map)
{
	setWindowTitle(tr("Replace symbol set"));
	
	QLabel* desc_label = new QLabel(tr("Configure how the symbols should be replaced, and which."));
	
	import_all_check = new QCheckBox(tr("Import all new symbols, even if not used as replacement"));
	import_all_check->setChecked(true);
	delete_unused_symbols_check = new QCheckBox(tr("Delete original symbols which are unused after the replacement"));
	delete_unused_symbols_check->setChecked(true);
	delete_unused_colors_check = new QCheckBox(tr("Delete unused colors after the replacement"));
	delete_unused_colors_check->setChecked(true);
	
	QLabel* mapping_label = new QLabel(tr("Symbol mapping:"));
	preserve_symbol_states_check = new QCheckBox(tr("Keep the symbols' hidden / protected states of the old symbol set"));
	preserve_symbol_states_check->setChecked(true);
	match_by_number_check = new QCheckBox(tr("Match replacement symbols by symbol number"));
	match_by_number_check->setChecked(true);
	
	mapping_table = new QTableWidget();
	mapping_table->verticalHeader()->setVisible(false);
	mapping_table->setColumnCount(2);
	mapping_table->setHorizontalHeaderLabels(QStringList() << tr("Original") << tr("Replacement"));
	mapping_table->horizontalHeader()->setSectionsClickable(false);
	mapping_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	mapping_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	
	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, Qt::Horizontal);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(desc_label);
	layout->addSpacing(16);
	layout->addWidget(import_all_check);
	layout->addWidget(delete_unused_symbols_check);
	layout->addWidget(delete_unused_colors_check);
	layout->addSpacing(16);
	layout->addWidget(mapping_label);
	layout->addWidget(preserve_symbol_states_check);
	layout->addWidget(match_by_number_check);
	layout->addWidget(mapping_table);
	layout->addSpacing(16);
	layout->addWidget(button_box);
	setLayout(layout);
	
	connect(match_by_number_check, SIGNAL(clicked(bool)), this, SLOT(matchByNumberClicked(bool)));
	connect(button_box, SIGNAL(helpRequested()), this, SLOT(showHelp()));
	connect(button_box, SIGNAL(accepted()), this, SLOT(apply()));
	connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));
	
	matchByNumberClicked(true);
}

ReplaceSymbolSetDialog::~ReplaceSymbolSetDialog()
{
	for (int i = 0, end = (int)symbol_widget_delegates.size(); i < end; ++i)
		delete symbol_widget_delegates[i];
}

void ReplaceSymbolSetDialog::matchByNumberClicked(bool checked)
{
	if (checked)
	{
		calculateNumberMatchMapping();
		updateMappingTable();
	}
	
	mapping_table->setEditTriggers(checked ?
		QAbstractItemView::NoEditTriggers : QAbstractItemView::AllEditTriggers);
	for (int row = 0; row < mapping_table->rowCount(); ++row)
	{
		mapping_table->item(row, 1)->setFlags(checked ?
			Qt::NoItemFlags : (Qt::ItemIsEnabled | Qt::ItemIsEditable));
	}
}

void ReplaceSymbolSetDialog::showHelp()
{
	Util::showHelp(this, "symbol_replace_dialog.html");
}

struct ReplaceSymbolSetOperation
{
	inline ReplaceSymbolSetOperation(QHash<const Symbol*, const Symbol*>* mapping, QHash<const Symbol*, Symbol*>* import_symbol_map)
	 : mapping(mapping), import_symbol_map(import_symbol_map)
	{
	}
	inline bool operator()(Object* object, MapPart* part, int object_index) const
	{
		Q_UNUSED(part);
		Q_UNUSED(object_index);
		if (mapping->contains(object->getSymbol()))
		{
			const Symbol* target_symbol = import_symbol_map->value(mapping->value(object->getSymbol()));
			object->setSymbol(target_symbol, true);
		}
		return true;
	}
private:
	QHash<const Symbol*, const Symbol*>* mapping;
	QHash<const Symbol*, Symbol*>* import_symbol_map;
};

void ReplaceSymbolSetDialog::apply()
{
	QHash<const Symbol*, Symbol*> import_symbol_map;
	
	updateMappingFromTable();
	
	QSet<const Symbol*> old_symbols;
	if (delete_unused_symbols_check->isChecked())
	{
		for (int i = 0; i < map->getNumSymbols(); ++i)
			old_symbols.insert(map->getSymbol(i));
	}
	
	// Import new symbols
	std::vector<bool>* symbol_filter = NULL;
	if (!import_all_check->isChecked())
	{
		// Import only symbols which are chosen as replacement symbols
		symbol_filter = new std::vector<bool>();
		symbol_filter->resize(symbol_map->getNumSymbols(), false);
		for (int i = 0; i < symbol_map->getNumSymbols(); ++i)
		{
			for (QHash<const Symbol*, const Symbol*>::iterator it = mapping.begin(), end = mapping.end(); it != end; ++it)
			{
				if (it.value() == symbol_map->getSymbol(i))
				{
					symbol_filter->at(i) = true;
					break;
				}
			}
		}
	}
	map->importMap(symbol_map, Map::MinimalSymbolImport, this, symbol_filter, -1, false, &import_symbol_map);
	delete symbol_filter;
	
	// Take over hidden / protected states from old symbol set?
	if (preserve_symbol_states_check->isChecked())
	{
		for (QHash<const Symbol*, const Symbol*>::iterator it = mapping.begin(), end = mapping.end(); it != end; ++it)
		{
			Symbol* target_symbol = import_symbol_map.value(it.value());
			target_symbol->setHidden(it.key()->isHidden());
			target_symbol->setProtected(it.key()->isProtected());
		}
	}
	
	// Change symbols for all objects
	map->applyOnAllObjects(ReplaceSymbolSetOperation(&mapping, &import_symbol_map));
	
	// Delete unused old symbols
	if (delete_unused_symbols_check->isChecked())
	{
		std::vector<bool> symbols_in_use;
		map->determineSymbolsInUse(symbols_in_use);
		
		for (int i = map->getNumSymbols() - 1; i >= 0; --i)
		{
			Symbol* symbol = map->getSymbol(i);
			if (!old_symbols.contains(symbol) || symbols_in_use[i])
				continue;
			
			map->deleteSymbol(i);
		}
	}
	
	// Delete unused colors
	if (delete_unused_colors_check->isChecked())
	{
		std::vector<bool> all_symbols;
		all_symbols.assign(map->getNumSymbols(), true);
		std::vector<bool> colors_in_use;
		map->determineColorsInUse(all_symbols, colors_in_use);
		if (colors_in_use.empty())
			colors_in_use.assign(map->getNumColors(), false);
		
		for (int i = map->getNumColors() - 1; i >= 0; --i)
		{
			if (colors_in_use[i])
				continue;
			map->deleteColor(i);
		}
	}
	
	// Finish
	map->updateAllObjects();
	map->setObjectsDirty();
	map->setSymbolsDirty();
	map->undoManager().clear();
	accept();
}

void ReplaceSymbolSetDialog::calculateNumberMatchMapping()
{
	mapping.clear();
	
	for (int i = 0; i < map->getNumSymbols(); ++i)
	{
		Symbol* original = map->getSymbol(i);
		Symbol* replacement = findNumberMatch(original, false);
		if (replacement)
			mapping.insert(original, replacement);
		else
		{
			// No match found. Do second pass which ignores trailing zeros
			replacement = findNumberMatch(original, true);
			if (replacement)
				mapping.insert(original, replacement);
		}
	}
}

Symbol* ReplaceSymbolSetDialog::findNumberMatch(Symbol* original, bool ignore_trailing_zeros)
{
	for (int k = 0; k < symbol_map->getNumSymbols(); ++k)
	{
		Symbol* replacement = symbol_map->getSymbol(k);
		if (original->numberEquals(replacement, ignore_trailing_zeros) &&
			Symbol::areTypesCompatible(original->getType(), replacement->getType()))
		{
			return replacement;
		}
	}
	return NULL;
}

void ReplaceSymbolSetDialog::updateMappingTable()
{
	mapping_table->clearContents();
	mapping_table->setRowCount(map->getNumSymbols());
	
	for (int i = 0, end = (int)symbol_widget_delegates.size(); i < end; ++i)
		delete symbol_widget_delegates[i];
	symbol_widget_delegates.resize(map->getNumSymbols());
	
	for (int row = 0; row < map->getNumSymbols(); ++row)
	{
		Symbol* original_symbol = map->getSymbol(row);
		QTableWidgetItem* original_item = new QTableWidgetItem(original_symbol->getNumberAsString() + QLatin1Char(' ') + original_symbol->getPlainTextName());
		original_item->setFlags(Qt::ItemIsEnabled); // make item non-editable
		QVariantList original_item_data =
			QVariantList() << qVariantFromValue<const Map*>(map) << qVariantFromValue<const Symbol*>(original_symbol);
		original_item->setData(Qt::UserRole, QVariant(original_item_data));
		original_item->setData(Qt::DecorationRole, original_symbol->getIcon(map));
		mapping_table->setItem(row, 0, original_item);
		
		// Note on const: this is not a const method.
		const Symbol* replacement_symbol = mapping.contains(original_symbol) ? mapping.value(original_symbol) : NULL;
		QTableWidgetItem* replacement_item;
		if (replacement_symbol)
			replacement_item = new QTableWidgetItem(replacement_symbol->getNumberAsString() + QLatin1Char(' ') + replacement_symbol->getPlainTextName());
		else
			replacement_item = new QTableWidgetItem(tr("- None -"));
		QVariantList replacement_item_data =
			QVariantList() << qVariantFromValue<const Map*>(symbol_map) << qVariantFromValue<const Symbol*>(replacement_symbol);
		replacement_item->setData(Qt::UserRole, QVariant(replacement_item_data));
		if (replacement_symbol)
			replacement_item->setData(Qt::DecorationRole, replacement_symbol->getIcon(symbol_map));
		mapping_table->setItem(row, 1, replacement_item);
		
		symbol_widget_delegates[row] = new SymbolDropDownDelegate(Symbol::getCompatibleTypes(original_symbol->getType()));
		mapping_table->setItemDelegateForRow(row, symbol_widget_delegates[row]);
	}
}

void ReplaceSymbolSetDialog::updateMappingFromTable()
{
	mapping.clear();
	
	for (int row = 0; row < map->getNumSymbols(); ++row)
	{
		const Symbol* replacement_symbol = mapping_table->item(row, 1)->data(Qt::UserRole).toList().at(1).value<const Symbol*>();
		if (!replacement_symbol)
			continue;
		
		mapping.insert(map->getSymbol(row), replacement_symbol);
	}
}

bool ReplaceSymbolSetDialog::showDialog(QWidget* parent, Map* map)
{
	Map* symbol_map = NULL;
	while (true)
	{
		QString path = MainWindow::getOpenFileName(parent, tr("Choose map file to load symbols from"),
													FileFormat::MapFile);
		if (path.isEmpty())
			return false;
		
		symbol_map = new Map();
		if (!symbol_map->loadFrom(path, parent, NULL, true))
		{
			QMessageBox::warning(parent, tr("Error"), tr("Cannot load map file, aborting."));
			return false;
		}
		
		if (symbol_map->getScaleDenominator() != map->getScaleDenominator())
		{
			if (QMessageBox::warning(parent, tr("Warning"),
				tr("The chosen symbol set has a scale of 1:%1, while the map scale is 1:%2. Do you really want to choose this set?").arg(symbol_map->getScaleDenominator()).arg(map->getScaleDenominator()),
				QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
			{
				delete symbol_map;
				continue;
			}
		}
		
		break;
	}
	
	ReplaceSymbolSetDialog dialog(parent, map, symbol_map);
	dialog.setWindowModality(Qt::WindowModal);
	bool accepted = dialog.exec() == QDialog::Accepted;
	
	delete symbol_map;
	return accepted;
}
