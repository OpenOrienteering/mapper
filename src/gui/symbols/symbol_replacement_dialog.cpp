/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2017-2020, 2024, 2025 Kai Pastor
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


#include "symbol_replacement_dialog.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>

#include <Qt>
#include <QtGlobal>
#include <QAbstractItemView>
#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QColor>
#include <QDialogButtonBox>
#include <QFlags>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHeaderView>
#include <QIcon>
#include <QImage>
#include <QIODevice>
#include <QLabel>
#include <QLatin1Char>
#include <QLatin1String>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QSpacerItem>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QVariant>
#include <QVariantList>
#include <QVBoxLayout>

#include "settings.h"
#include "core/map.h"
#include "core/map_color.h"
#include "core/objects/object.h"
#include "core/objects/object_query.h"
#include "core/objects/symbol_rule_set.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"
#include "core/symbols/text_symbol.h"
#include "gui/file_dialog.h"
#include "gui/util_gui.h"
#include "gui/symbols/symbol_replacement.h"
#include "gui/widgets/symbol_dropdown.h"
#include "util/backports.h"  // IWYU pragma: keep

// IWYU pragma: no_forward_declare QColor
// IWYU pragma: no_forward_declare QFormLayout
// IWYU pragma: no_forward_declare QLabel
// IWYU pragma: no_forward_declare QTableWidgetItem


namespace OpenOrienteering {


SymbolReplacementDialog::SymbolReplacementDialog(QWidget* parent, Map& object_map, const Map& symbol_set, SymbolRuleSet& symbol_rules, Mode mode)
 : QDialog{ parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint }
 , object_map{ object_map }
 , symbol_set{ symbol_set }
 , symbol_rules{ symbol_rules }
 , mode( &object_map==&symbol_set ? AssignByPattern : mode )
{
	setWindowModality(Qt::WindowModal);
	
	QFormLayout* form_layout = nullptr;
	auto mapping_menu = new QMenu(this);
	
	QStringList horizontal_headers;
	horizontal_headers.reserve(2);
	if (mode == ReplaceSymbolSet)
	{
		setWindowTitle(tr("Replace symbol set"));
		form_layout = new QFormLayout();
		auto* desc_label = new QLabel(tr("Configure how the symbols should be replaced, and which."));
		form_layout->addRow(desc_label);
		form_layout->addItem(Util::SpacerItem::create(this));
		import_all_check = new QCheckBox(tr("Import all new symbols, even if not used as replacement"));
		import_all_check->setChecked(true);
		form_layout->addRow(import_all_check);
		preserve_symbol_states_check = new QCheckBox(tr("Keep the symbols' hidden / protected states of the old symbol set"));
		preserve_symbol_states_check->setChecked(true);
		form_layout->addRow(preserve_symbol_states_check);
		delete_unused_symbols_check = new QCheckBox(tr("Delete original symbols which are unused after the replacement"));
		delete_unused_symbols_check->setChecked(true);
		form_layout->addRow(delete_unused_symbols_check);
		delete_unused_colors_check = new QCheckBox(tr("Delete unused colors after the replacement"));
		delete_unused_colors_check->setChecked(true);
		form_layout->addRow(delete_unused_colors_check);
		id_edit = new QComboBox();
		id_edit->setEditable(true);
		auto new_id = symbol_set.symbolSetId();
		if (!new_id.isEmpty())
			id_edit->addItem(new_id);
		auto old_id = object_map.symbolSetId();
		if (!old_id.isEmpty() && old_id != new_id)
			id_edit->addItem(old_id);
		id_edit->setCurrentIndex(0);
		form_layout->addRow(tr("Edit the symbol set ID:"), id_edit);
		form_layout->addItem(Util::SpacerItem::create(this));
		
		horizontal_headers << tr("Original") << tr("Replacement");
		
		match_by_number_action = mapping_menu->addAction(tr("Match replacement symbols by symbol number"));
		match_by_number_action->setCheckable(true);
		connect(match_by_number_action, &QAction::triggered, this, &SymbolReplacementDialog::matchByNumber);
		match_by_name_action = mapping_menu->addAction(tr("Match by symbol name"));
		match_by_name_action->setCheckable(true);
		match_by_name_action->setChecked(true);
		connect(match_by_name_action, &QAction::triggered, this, &SymbolReplacementDialog::matchByName);
		auto* match_by_group = new QActionGroup(this);
		match_by_group->addAction(match_by_number_action);
		match_by_group->addAction(match_by_name_action);
	}
	else
	{
		setWindowTitle(tr("Assign new symbols"));
		horizontal_headers << tr("Pattern") << tr("Replacement");
	}
	
	mapping_table = new QTableWidget();
	mapping_table->verticalHeader()->setVisible(false);
	mapping_table->setColumnCount(2);
	mapping_table->setHorizontalHeaderLabels(horizontal_headers);
	mapping_table->horizontalHeader()->setSectionsClickable(false);
	mapping_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	mapping_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	mapping_table->setEditTriggers(QAbstractItemView::AllEditTriggers);
	connect(mapping_table, &QTableWidget::cellChanged, this, &SymbolReplacementDialog::clearMatchCheckmarks);
	
	auto action = mapping_menu->addAction(tr("Clear replacements"));
	connect(action, &QAction::triggered, this, &SymbolReplacementDialog::resetReplacements);
	mapping_menu->addSeparator();
	action = mapping_menu->addAction(QIcon{QLatin1String{":/images/open.png"}}, tr("Open CRT file..."));
	connect(action, &QAction::triggered, this, QOverload<>::of(&SymbolReplacementDialog::openCrtFile));
	action = mapping_menu->addAction(QIcon{QLatin1String{":/images/save.png"}}, tr("Save CRT file..."));
	connect(action, &QAction::triggered, this, &SymbolReplacementDialog::saveCrtFile);
	
	auto button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, Qt::Horizontal);
	auto mapping_button = button_box->addButton(tr("Symbol mapping:").replace(QLatin1Char{':'}, QString{}), QDialogButtonBox::ActionRole);
	void(tr("Symbol mapping")); /// \todo Switch translation
	mapping_button->setMenu(mapping_menu);
	
	auto layout = new QVBoxLayout(this);
	if (form_layout)
		layout->addLayout(form_layout);
	layout->addWidget(mapping_table);
	layout->addItem(Util::SpacerItem::create(this));
	layout->addWidget(button_box);
	setLayout(layout);
	
	connect(button_box, &QDialogButtonBox::helpRequested, this, &SymbolReplacementDialog::showHelp);
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	
	updateMappingTable();
}


SymbolReplacementDialog::~SymbolReplacementDialog() = default;


// slot
void SymbolReplacementDialog::showHelp()
{
	Util::showHelp(this, "symbol_replace_dialog.html");
}


// slot
void SymbolReplacementDialog::matchByName()
{
	symbol_rules.matchQuerySymbolName(symbol_set);
	updateMappingTable();
}

// slot
void SymbolReplacementDialog::matchByNumber()
{
	symbol_rules.matchQuerySymbolNumber(symbol_set);
	updateMappingTable();
}

// slot
void SymbolReplacementDialog::resetReplacements()
{
	for (auto& item : symbol_rules)
	{
		item.symbol = nullptr;
		item.type = SymbolRule::NoAssignment;
	}
	clearMatchCheckmarks();
	updateMappingTable();
}

// slot
void SymbolReplacementDialog::clearMatchCheckmarks()
{
	if (react_to_changes)
	{
		if (match_by_number_action)
			match_by_number_action->setChecked(false);
		if (match_by_name_action)
			match_by_name_action->setChecked(false);
	}
}


// slot
void SymbolReplacementDialog::openCrtFile()
{
	auto const dir = QLatin1String{"data:/symbol sets"};
	auto const filter = QString{tr("CRT file") + QLatin1String{" (*.crt)"}};
	auto const filepath = FileDialog::getOpenFileName(this, tr("Open CRT file..."), dir, filter);
	if (!filepath.isEmpty())
	{
		auto new_rules = SymbolReplacement(object_map, symbol_set).loadCrtFile(this, filepath);
		symbol_rules.merge(std::move(new_rules), SymbolRuleSet::UpdateAndAppend);
		updateMappingTable();
	}
}

// slot
bool SymbolReplacementDialog::saveCrtFile()
{
	/// \todo Choose user-writable directory.
	auto const dir = QLatin1String{"data:/symbol sets"};
	auto const filter = QString{tr("CRT file") + QLatin1String{" (*.crt)"}};
	auto const filepath = FileDialog::getSaveFileName(this, tr("Save CRT file..."), dir, filter);
	if (!filepath.isEmpty())
	{
		updateMappingFromTable();
		QSaveFile crt_file(filepath);
		crt_file.open(QIODevice::WriteOnly | QIODevice::Text);
		QTextStream stream{ &crt_file };
		symbol_rules.writeCrt(stream);
		if (crt_file.commit())
			return true;
		
		/// \todo Reused translation, consider generalized context
		QMessageBox::warning(this, ::OpenOrienteering::Map::tr("Error"),
		                     tr("Cannot save file:\n%1\n\n%2")
		                     .arg(filepath, crt_file.errorString()) );
	}
	return false;
}



void SymbolReplacementDialog::done(int r)
{
	updateMappingFromTable();
	if (std::any_of(begin(symbol_rules), end(symbol_rules), [](const auto& item) {
	                return item.type == SymbolRule::ManualAssignment;
	}))
	{
		auto save_crt = QMessageBox::warning(this, qApp->applicationDisplayName(),
		                                     tr("The cross reference table has been modified.\n"
		                                        "Do you want to save your changes?"),
		                                     // QGnomeThema label for Discard: "Close without saving"
		                                     QMessageBox::Save | QMessageBox::No | QMessageBox::Cancel);
		switch (save_crt)
		{
		case QMessageBox::Cancel:
			return;
			
		case QMessageBox::Discard:
		case QMessageBox::No:
			break;
			
		case QMessageBox::Save:
			if (!saveCrtFile())
				return;
			break;
			
		default:
			Q_UNREACHABLE();
		}
	}
	QDialog::done(r);
}



void SymbolReplacementDialog::updateMappingTable()
{
	if (!mapping_table)
		return;
	
	react_to_changes = false;
	mapping_table->setUpdatesEnabled(false);
	
	mapping_table->clearContents();
	mapping_table->setRowCount(int(symbol_rules.size()));
	
	symbol_widget_delegates.clear();
	symbol_widget_delegates.resize(symbol_rules.size());
	
	std::size_t row = 0;
	for (const auto& item : symbol_rules)
	{
		const Symbol* original_symbol = nullptr;
		auto original_icon = QImage{};
		auto original_string = QString{};
		auto compatible_symbols = Symbol::TypeCombination(Symbol::AllSymbols);
		const auto* replacement_symbol = item.symbol;
		
		if (item.query.getOperator() == ObjectQuery::OperatorSymbol
		    && item.query.symbolOperand())
		{
			original_symbol = item.query.symbolOperand();
			original_string = Util::plainText(object_map.translate(original_symbol->getName()));
			original_string.prepend(original_symbol->getNumberAsString() + QLatin1Char(' '));
			original_icon = original_symbol->getIcon(&object_map);
			compatible_symbols = Symbol::getCompatibleTypes(original_symbol->getType());
		}
		else
		{
			if (replacement_symbol)
			{
				switch (replacement_symbol->getType())
				{
				case Symbol::Area:
					original_icon = object_map.getUndefinedLine()->getIcon(&object_map);
					original_string = QGuiApplication::translate("OpenOrienteering::SymbolRenderWidget", "Area");
					compatible_symbols = Symbol::getCompatibleTypes(replacement_symbol->getType());
					break;
				case Symbol::Combined:
					original_icon = object_map.getUndefinedLine()->getIcon(&object_map);
					original_string = QGuiApplication::translate("OpenOrienteering::SymbolRenderWidget", "Combined");
					compatible_symbols = Symbol::getCompatibleTypes(replacement_symbol->getType());
					break;
				case Symbol::Line:
					original_icon = object_map.getUndefinedLine()->getIcon(&object_map);
					original_string = QGuiApplication::translate("OpenOrienteering::SymbolRenderWidget", "Line");
					compatible_symbols = Symbol::getCompatibleTypes(replacement_symbol->getType());
					break;
				case Symbol::Point:
					original_icon = object_map.getUndefinedPoint()->getIcon(&object_map);
					original_string = QGuiApplication::translate("OpenOrienteering::SymbolRenderWidget", "Point");
					compatible_symbols = Symbol::getCompatibleTypes(replacement_symbol->getType());
					break;
				case Symbol::Text:
					original_icon = object_map.getUndefinedText()->getIcon(&object_map);
					original_string = QGuiApplication::translate("OpenOrienteering::SymbolRenderWidget", "Text");
					compatible_symbols = Symbol::getCompatibleTypes(replacement_symbol->getType());
					break;
				case Symbol::AllSymbols:
				case Symbol::NoSymbol:
					; // no icon, no string
				}
			}
			
			const Symbol* unique_symbol = nullptr;
			auto matching_types = Symbol::TypeCombination(Symbol::NoSymbol);
			auto update_matching = [&unique_symbol, &matching_types](Object* object) {
				if (auto symbol = object->getSymbol())
				{
					if (matching_types == Symbol::NoSymbol)
					{
						unique_symbol = symbol;
						matching_types = Symbol::getCompatibleTypes(symbol->getType());
					}
					else
					{
						if (symbol != unique_symbol)
							unique_symbol = nullptr;
						matching_types |= Symbol::getCompatibleTypes(symbol->getType());
					}
				}
			};
			object_map.applyOnMatchingObjects(update_matching, std::cref(item.query));
			if (matching_types != Symbol::NoSymbol)
			{
				compatible_symbols = matching_types;
				if (unique_symbol)
				{
					original_symbol = unique_symbol;
					original_string = Util::plainText(object_map.translate(original_symbol->getName()));
					original_string.prepend(original_symbol->getNumberAsString() + QLatin1Char(' '));
					original_icon = original_symbol->getIcon(&object_map);
				}
			}
			
			if (original_icon.isNull())
			{
				auto side_length = Settings::getInstance().getSymbolWidgetIconSizePx();
				original_icon = QImage{side_length, side_length, QImage::Format_ARGB32_Premultiplied};
				auto color = object_map.getUndefinedColor()->operator const QColor &();
				color.setAlphaF(0.5);
				original_icon.fill(color);
			}
			
			switch (item.query.getOperator())
			{
			case ObjectQuery::OperatorInvalid:
			case ObjectQuery::OperatorSymbol:
				break;
				
			default:
				original_string = item.query.toString();
			}
		}
		
		auto original_item = new QTableWidgetItem(original_string);
		original_item->setFlags(Qt::ItemIsEnabled); // make item non-editable
		QVariantList original_item_data =
			QVariantList() << QVariant::fromValue<const Map*>(&object_map) << QVariant::fromValue<const Symbol*>(original_symbol);
		original_item->setData(Qt::UserRole, QVariant(original_item_data));
		original_item->setData(Qt::DecorationRole, original_icon);
		mapping_table->setItem(int(row), 0, original_item);
		
		// Note on const: this is not a const method.
		QTableWidgetItem* replacement_item;
		if (replacement_symbol)
			replacement_item = new QTableWidgetItem(replacement_symbol->getNumberAsString() + QLatin1Char(' ')
			                                        + Util::plainText(symbol_set.translate(replacement_symbol->getName())));
		else
			replacement_item = new QTableWidgetItem(tr("- None -"));
		QVariantList replacement_item_data =
			QVariantList() << QVariant::fromValue<const Map*>(&symbol_set) << QVariant::fromValue<const Symbol*>(replacement_symbol);
		replacement_item->setData(Qt::UserRole, QVariant(replacement_item_data));
		if (replacement_symbol)
			replacement_item->setData(Qt::DecorationRole, replacement_symbol->getIcon(&symbol_set));
		mapping_table->setItem(int(row), 1, replacement_item);
		
		//auto hide = (mode == ModeCRT && original_symbol->isHidden());
		//mapping_table->setRowHidden(row, hide);
		
		symbol_widget_delegates[row] = std::make_unique<SymbolDropDownDelegate>(compatible_symbols);
		mapping_table->setItemDelegateForRow(int(row), symbol_widget_delegates[row].get());
		
		++row;
	}
	
	mapping_table->setUpdatesEnabled(true);
	react_to_changes = true;
}


void SymbolReplacementDialog::updateMappingFromTable()
{
	Q_ASSERT(int(symbol_rules.size()) == mapping_table->rowCount());
	auto item = begin(symbol_rules);
	for (int row = 0; row < mapping_table->rowCount(); ++row)
	{
		auto replacement_symbol = mapping_table->item(row, 1)->data(Qt::UserRole).toList().at(1).value<const Symbol*>();
		if (item->symbol != replacement_symbol)
		{
			item->symbol = replacement_symbol;
			item->type = SymbolRule::ManualAssignment;
		}
		++item;
	}
}



SymbolRuleSet::Options SymbolReplacementDialog::replacementOptions() const
{
	auto options = SymbolRuleSet::Options{};
	if (mode == ReplaceSymbolSet)
	{
		options.setFlag(SymbolRuleSet::ImportAllSymbols, import_all_check->isChecked());
		options.setFlag(SymbolRuleSet::PreserveSymbolState, preserve_symbol_states_check->isChecked());
		options.setFlag(SymbolRuleSet::RemoveUnusedSymbols, delete_unused_symbols_check->isChecked());
		options.setFlag(SymbolRuleSet::RemoveUnusedColors, delete_unused_colors_check->isChecked());
	}
	return options;
}

QString SymbolReplacementDialog::replacementId() const
{
	auto id = object_map.symbolSetId();
	if (mode == ReplaceSymbolSet)
	{
		id = id_edit->currentText();
	}
	return id;
}


}  // namespace OpenOrienteering
