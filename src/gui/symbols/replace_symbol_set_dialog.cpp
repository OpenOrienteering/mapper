/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2017 Kai Pastor
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

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>

#include <Qt>
#include <QtGlobal>
#include <QAbstractItemView>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QColor>
#include <QDialogButtonBox>
#include <QFile>
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
#include <QSpacerItem>
#include <QString>
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
#include "fileformats/file_format.h"
#include "fileformats/file_import_export.h"
#include "gui/file_dialog.h"
#include "gui/main_window.h"
#include "gui/util_gui.h"
#include "gui/widgets/symbol_dropdown.h"
#include "util/util.h"
#include "util/backports.h"

// IWYU pragma: no_forward_declare QColor
// IWYU pragma: no_forward_declare QFormLayout
// IWYU pragma: no_forward_declare QLabel
// IWYU pragma: no_forward_declare QTableWidgetItem


namespace OpenOrienteering {

ReplaceSymbolSetDialog::ReplaceSymbolSetDialog(QWidget* parent, Map& object_map, const Map& symbol_set, SymbolRuleSet& replacements, Mode mode)
 : QDialog{ parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint }
 , object_map{ object_map }
 , symbol_set{ symbol_set }
 , replacements{ replacements }
 , import_all_check{ nullptr }
 , delete_unused_symbols_check{ nullptr }
 , delete_unused_colors_check{ nullptr }
 , preserve_symbol_states_check{ nullptr }
 , mode( &object_map==&symbol_set ? AssignByPattern : mode )
{
	QFormLayout* form_layout = nullptr;
	auto mapping_menu = new QMenu(this);
	
	QStringList horizontal_headers;
	horizontal_headers.reserve(2);
	if (mode == ReplaceSymbolSet)
	{
		setWindowTitle(tr("Replace symbol set"));
		form_layout = new QFormLayout();
		QLabel* desc_label = new QLabel(tr("Configure how the symbols should be replaced, and which."));
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
		
		auto action = mapping_menu->addAction(tr("Match replacement symbols by symbol number"));
		connect(action, &QAction::triggered, this, &ReplaceSymbolSetDialog::matchByNumber);
		action = mapping_menu->addAction(tr("Match by symbol name"));
		connect(action, &QAction::triggered, this, &ReplaceSymbolSetDialog::matchByName);
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
	
	auto action = mapping_menu->addAction(tr("Clear replacements"));
	connect(action, &QAction::triggered, this, &ReplaceSymbolSetDialog::resetReplacements);
	mapping_menu->addSeparator();
	action = mapping_menu->addAction(QIcon{QLatin1String{":/images/open.png"}}, tr("Open CRT file..."));
	connect(action, &QAction::triggered, this, QOverload<>::of(&ReplaceSymbolSetDialog::openCrtFile));
	action = mapping_menu->addAction(QIcon{QLatin1String{":/images/save.png"}}, tr("Save CRT file..."));
	connect(action, &QAction::triggered, this, &ReplaceSymbolSetDialog::saveCrtFile);
	
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
	
	connect(button_box, &QDialogButtonBox::helpRequested, this, &ReplaceSymbolSetDialog::showHelp);
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	
	if (replacements.empty() && mode == ReplaceSymbolSet)
	{
		this->replacements = SymbolRuleSet::forOriginalSymbols(object_map);
		this->replacements.matchQuerySymbolName(symbol_set);
	}
	
	updateMappingTable();
}


ReplaceSymbolSetDialog::~ReplaceSymbolSetDialog()
{
	// nothing, not inlined
}



void ReplaceSymbolSetDialog::showHelp()
{
	Util::showHelp(this, "symbol_replace_dialog.html");
}



void ReplaceSymbolSetDialog::matchByName()
{
	replacements.matchQuerySymbolName(symbol_set);
	updateMappingTable();
}


void ReplaceSymbolSetDialog::matchByNumber()
{
	replacements.matchQuerySymbolNumber(symbol_set);
	updateMappingTable();
}


void ReplaceSymbolSetDialog::resetReplacements()
{
	for (auto& item : replacements)
	{
		item.symbol = nullptr;
		item.type = SymbolRule::NoAssignment;
	}
	updateMappingTable();
}



void ReplaceSymbolSetDialog::openCrtFile()
{
	auto dir = QLatin1String{"data:/symbol sets"};
	auto filter = QString{tr("CRT file") + QLatin1String{" (*.crt)"}};
	QString path = FileDialog::getOpenFileName(this, tr("Open CRT file..."), dir, filter);
	if (!path.isEmpty())
		openCrtFile(path);
}
	
void ReplaceSymbolSetDialog::openCrtFile(const QString& path)
{
	QFile crt_file{path};
	crt_file.open(QIODevice::ReadOnly);
	QTextStream stream{ &crt_file };
	auto new_replacements = SymbolRuleSet::loadCrt(stream, symbol_set);
	if (stream.status() == QTextStream::Ok)
	{
		// Postprocess CRT
		for (auto& item : new_replacements)
		{
			if (item.type == SymbolRule::NoAssignment
			    || item.query.getOperator() != ObjectQuery::OperatorSearch)
				continue;
			Q_ASSERT(item.symbol);
			
			auto operands = item.query.tagOperands();
			if (!operands || operands->value.isEmpty())
				continue;
			
			// Find original symbol number matching the pattern
			for (int i = 0; i < object_map.getNumSymbols(); ++i)
			{
				auto symbol = object_map.getSymbol(i);
				if (symbol->getNumberAsString() == operands->value)
				{
					if (Symbol::areTypesCompatible(symbol->getType(), item.symbol->getType()))
						item.query = { symbol };
					break;
				}
			}
			
			// Find inconsistencies
			if (item.query.getOperator() == ObjectQuery::OperatorSymbol)
			{
				auto has_conflict = [&item](const auto& other)->bool {
					return &other != &item
					       && other.type != SymbolRule::NoAssignment
					       && item.query == other.query
					       && item.symbol != other.symbol;
				};
				if (std::any_of(begin(new_replacements), end(new_replacements), has_conflict))
				{
					stream.setStatus(QTextStream::ReadCorruptData);
					auto error_msg = tr("There are multiple replacements for symbol %1.")
					                 .arg(item.query.symbolOperand()->getNumberAsString());
					QMessageBox::warning(this, ::OpenOrienteering::Map::tr("Error"),
					  tr("Cannot open file:\n%1\n\n%2").arg(path, error_msg) );
					return;
				}
			}
		}
		
		// Apply
		for (auto& item : new_replacements)
		{
			for (auto& current : replacements)
			{
				if (item.query == current.query)
				{
					if (item.type != SymbolRule::NoAssignment)
					{
						current.symbol = item.symbol;
						current.type = item.type;
						item = {};
					}
					break;
				}
			}
		}
		if (mode == AssignByPattern)
		{
			for (auto&& item : new_replacements)
			{
				if (item.query.getOperator() != ObjectQuery::OperatorInvalid)
				{
					replacements.emplace_back(std::move(item));
				}
			}
		}
		
		updateMappingTable();
	}
}


bool ReplaceSymbolSetDialog::saveCrtFile()
{
	/// \todo Choose user-writable directory.
	auto dir = QLatin1String{"data:/symbol sets"};
	auto filter = QString{tr("CRT file") + QLatin1String{" (*.crt)"}};
	QString path = FileDialog::getSaveFileName(this, tr("Save CRT file..."), dir, filter);
	if (!path.isEmpty())
	{
		updateMappingFromTable();
		QFile crt_file{path};
		crt_file.open(QIODevice::WriteOnly);
		QTextStream stream{ &crt_file };
		replacements.writeCrt(stream);
		if (stream.pos() != -1)
		{
			return true;
		}
		/// \todo Reused translation, consider generalized context
		QMessageBox::warning(this, ::OpenOrienteering::Map::tr("Error"),
		                     tr("Cannot save file:\n%1\n\n%2")
		                     .arg(path, crt_file.errorString()) );
	}
	return false;
}



void ReplaceSymbolSetDialog::done(int r)
{
	updateMappingFromTable();
	if (std::any_of(begin(replacements), end(replacements), [](const auto& item) {
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



void ReplaceSymbolSetDialog::updateMappingTable()
{
	mapping_table->setUpdatesEnabled(false);
	
	mapping_table->clearContents();
	mapping_table->setRowCount(int(replacements.size()));
	
	symbol_widget_delegates.clear();
	symbol_widget_delegates.resize(replacements.size());
	
	std::size_t row = 0;
	for (const auto& item : replacements)
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
			QVariantList() << qVariantFromValue<const Map*>(&object_map) << qVariantFromValue<const Symbol*>(original_symbol);
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
			QVariantList() << qVariantFromValue<const Map*>(&symbol_set) << qVariantFromValue<const Symbol*>(replacement_symbol);
		replacement_item->setData(Qt::UserRole, QVariant(replacement_item_data));
		if (replacement_symbol)
			replacement_item->setData(Qt::DecorationRole, replacement_symbol->getIcon(&symbol_set));
		mapping_table->setItem(int(row), 1, replacement_item);
		
		//auto hide = (mode == ModeCRT && original_symbol->isHidden());
		//mapping_table->setRowHidden(row, hide);
		
		symbol_widget_delegates[row].reset(new SymbolDropDownDelegate(compatible_symbols));
		mapping_table->setItemDelegateForRow(int(row), symbol_widget_delegates[row].get());
		
		++row;
	}
	
	mapping_table->setUpdatesEnabled(true);
}


void ReplaceSymbolSetDialog::updateMappingFromTable()
{
	Q_ASSERT(int(replacements.size()) == mapping_table->rowCount());
	auto item = begin(replacements);
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



// static
bool ReplaceSymbolSetDialog::showDialog(QWidget* parent, Map& object_map)
{
	auto symbol_set = std::unique_ptr<Map>{};
	while (!symbol_set)
	{
		auto selected = MainWindow::getOpenFileName(
		                    parent,
		                    tr("Choose map file to load symbols from"),
		                    FileFormat::MapFile);
		if (!selected)
		{
			// canceled
			return false;
		}
		
		if (!selected.fileFormat())
		{
			/// \todo More precise message
			QMessageBox::warning(parent, tr("Error"), tr("Cannot load symbol set, aborting."));
			return false;
		}
		
		symbol_set.reset(new Map);
		auto importer = selected.fileFormat()->makeImporter(selected.filePath(), symbol_set.get(), nullptr);
		if (!importer)
		{
			/// \todo More precise message
			QMessageBox::warning(parent, tr("Error"), tr("Cannot load symbol set, aborting."));
			return false;
		}
		
		if (!importer->doImport())
		{
			/// \todo Show error from importer
			QMessageBox::warning(parent, tr("Error"), tr("Cannot load symbol set, aborting."));
			return false;
		}
		
		if (!importer->warnings().empty())
		{
			MainWindow::showMessageBox(parent, tr("Warning"), tr("The symbol set import generated warnings."), importer->warnings());
		}
		
		if (symbol_set->getScaleDenominator() != object_map.getScaleDenominator())
		{
			if (QMessageBox::warning(parent, tr("Warning"),
				tr("The chosen symbol set has a scale of 1:%1, while the map scale is 1:%2. Do you really want to choose this set?").arg(symbol_set->getScaleDenominator()).arg(object_map.getScaleDenominator()),
				QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
			{
				symbol_set.reset(nullptr);
			}
		}
	}
	
	auto replacements = SymbolRuleSet::forOriginalSymbols(object_map);
	replacements.matchQuerySymbolName(*symbol_set);
	
	auto flags_from_dialog = [](const ReplaceSymbolSetDialog& dialog){
		auto options = SymbolRuleSet::Options{};
		if (dialog.import_all_check->isChecked())
			options |= SymbolRuleSet::ImportAllSymbols;
		if (dialog.preserve_symbol_states_check->isChecked())
			options |= SymbolRuleSet::PreserveSymbolState;
		if (!dialog.delete_unused_symbols_check->isChecked())
			options |= SymbolRuleSet::KeepUnusedSymbols;
		if (!dialog.delete_unused_colors_check->isChecked())
			options |= SymbolRuleSet::KeepUnusedColors;
		return options;
	};

	ReplaceSymbolSetDialog dialog(parent, object_map, *symbol_set, replacements, ReplaceSymbolSet);
	dialog.setWindowModality(Qt::WindowModal);
	auto crt_file = discoverCrtFile(object_map.symbolSetId(), symbol_set->symbolSetId());
	if (QFileInfo::exists(crt_file))
	{
		dialog.show();
		dialog.openCrtFile(crt_file);
	}
	auto result = dialog.exec();
	switch (result)
	{
	case QDialog::Accepted:
		replacements.squeezed().apply(object_map, *symbol_set, flags_from_dialog(dialog));
		object_map.setSymbolSetId(dialog.id_edit->currentText());
		return true;
		
	case QDialog::Rejected:
		return false;
	}
	
	Q_UNREACHABLE();
}


// static
bool ReplaceSymbolSetDialog::showDialogForCRT(QWidget* parent, Map& object_map, const Map& symbol_set)
{
	auto replacements = SymbolRuleSet{};
	ReplaceSymbolSetDialog dialog(parent, object_map, symbol_set, replacements, AssignByPattern);
	dialog.setWindowModality(Qt::WindowModal);
	dialog.show();
	dialog.openCrtFile();
	auto result = dialog.exec();
	switch (result)
	{
	case QDialog::Accepted:
		replacements.squeezed().apply(object_map, symbol_set);
		return true;
		
	case QDialog::Rejected:
		return false;
	}
	
	Q_UNREACHABLE();
}


// static
bool ReplaceSymbolSetDialog::showDialogForCRT(QWidget* parent, Map& object_map, const Map& symbol_set, QIODevice& crt_file)
{
	QTextStream stream{ &crt_file };
	auto replacements = SymbolRuleSet::loadCrt(stream, symbol_set);
	if (stream.status() != QTextStream::Ok)
	{
		QMessageBox::warning(parent, tr("Error"), tr("Cannot load CRT file, aborting."));
		return false;
	}
	
	for (auto& item : replacements)
	{
		if (item.query.getOperator() != ObjectQuery::OperatorSearch)
			continue;
		
		auto operands = item.query.tagOperands();
		if (operands && !operands->value.isEmpty())
		{
			// Construct layer queries
			item.query = { QLatin1String("Layer"), ObjectQuery::OperatorIs, operands->value };
		}
	}
		
	/// \todo Collect source symbols for all patterns, for source icon in dialog
	
	ReplaceSymbolSetDialog dialog(parent, object_map, symbol_set, replacements, AssignByPattern);
	dialog.setWindowModality(Qt::WindowModal);
	auto result = dialog.exec();
	switch (result)
	{
	case QDialog::Accepted:
		replacements.squeezed().apply(object_map, symbol_set);
		return true;
		
	case QDialog::Rejected:
		return false;
	}
	
	Q_UNREACHABLE();
}


// static
QString ReplaceSymbolSetDialog::discoverCrtFile(const QString& source_id, const QString& target_id)
{
	QString name = QLatin1String("data:/symbol sets/")
	               + source_id + QLatin1Char('-')
	               + target_id + QLatin1String(".crt");
#ifdef MAPPER_DEVELOPMENT_BUILD
	if (!QFileInfo::exists(name))
		name = QLatin1String("data:/symbol sets/COPY_OF_")
		       + source_id + QLatin1Char('-')
		       + target_id + QLatin1String(".crt");
#endif
	return name;
}


}  // namespace OpenOrienteering
