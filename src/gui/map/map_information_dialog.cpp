/*
 *    Copyright 2024 Matthias Kühlewein
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

#include "map_information_dialog.h"

#include <algorithm>
#include <array>
#include <initializer_list>
#include <iterator>
// IWYU pragma: no_include <memory>
#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QAbstractButton>
#include <QChar>
#include <QFileInfo>
#include <QFlags>
#include <QFontInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QIODevice>
#include <QLatin1Char>
#include <QLatin1String>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVarLengthArray>
#include <QVBoxLayout>

#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_color.h"
#include "core/map_part.h"
#include "core/objects/object.h"
#include "core/symbols/symbol.h"
#include "core/symbols/text_symbol.h"
#include "gui/file_dialog.h"
#include "gui/main_window.h"
#include "undo/undo_manager.h"


namespace OpenOrienteering {

MapInformationDialog::MapInformationDialog(MainWindow* parent, Map* map)
 : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
 , map(map)
 , main_window(parent)
 , text_report_indent{3}
{
	setWindowTitle(tr("Map information"));
	
	auto* save_button = new QPushButton(QIcon(QLatin1String(":/images/save.png")), tr("Save as..."));
	auto* close_button = new QPushButton(QGuiApplication::translate("QPlatformTheme", "Close"));
	close_button->setDefault(true);
	
	auto* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(save_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(close_button);
	
	map_info_tree = new QTreeWidget();
	map_info_tree->setColumnCount(2);
	map_info_tree->setHeaderHidden(true);
	
	retrieveInformation();
	buildTree();
	setupTreeWidget();
	
	map_info_tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	
	auto* layout = new QVBoxLayout();
	layout->addWidget(map_info_tree);
	layout->addLayout(buttons_layout);
	setLayout(layout);
	resize(650, 600);
	
	connect(save_button, &QAbstractButton::clicked, this, &MapInformationDialog::save);
	connect(close_button, &QAbstractButton::clicked, this, &QDialog::accept);
}

MapInformationDialog::~MapInformationDialog() = default;


// retrieve and store the information
void MapInformationDialog::retrieveInformation()
{
	map_information.map_scale = int(map->getScaleDenominator());
	
	const auto& map_crs = map->getGeoreferencing().getProjectedCRSId();
	map_information.map_crs = map->getGeoreferencing().getProjectedCRSName();
	if (map_crs != QLatin1String("Local") && map_crs != QLatin1String("PROJ.4"))
	{
		const auto& projected_crs_parameters = map->getGeoreferencing().getProjectedCRSParameters();
		if (!projected_crs_parameters.empty())
		{
			QString crs_details = QLatin1String(" (") + (map_crs == QLatin1String("EPSG") ? tr("code") : tr("zone")) + QChar::Space + projected_crs_parameters.front() + QLatin1Char(')');
			map_information.map_crs += crs_details;
		}
	}
	
	map_information.map_parts_num = map->getNumParts();
	map_information.map_parts.reserve(map_information.map_parts_num);
	map_information.symbols_num = map->getNumSymbols();
	map_information.templates_num = map->getNumTemplates();
	map_information.colors_num = map->getNumColors();
	
	for (int i = 0; i < map_information.colors_num; ++i)
	{
		const auto* map_color = map->getMapColor(i);
		if (map_color)
		{
			ColorUsage color = {};
			color.name = map_color->getName();
			for (int j = 0; j < map_information.symbols_num; ++j)
			{
				const auto* symbol = map->getSymbol(j);
				if (symbol->getType())
				{
					if (symbol->containsColor(map_color))
					{
						color.symbols.push_back({symbol->getNumberAndPlainTextName()});
					}
				}
			}
			map_information.colors.emplace_back(color);
		}
	}
	
	const auto& undo_manager = map->undoManager();
	map_information.undo_steps_num = undo_manager.canUndo() ? undo_manager.undoStepCount() : 0;
	map_information.redo_steps_num = undo_manager.canRedo() ? undo_manager.redoStepCount() : 0;
	
	int i = 0;
	for (auto& name : {tr("Point symbols"), tr("Line symbols"), tr("Area symbols"), tr("Text symbols"), tr("Combined symbols"), tr("Undefined symbols")})
		map_information.map_objects[i++].category.name = name;
	
	for (i = 0; i < map_information.map_parts_num; ++i)
	{
		const auto* map_part = map->getPart(i);
		const auto map_part_objects = map_part->getNumObjects();
		map_information.map_parts.push_back({map_part->getName(), map_part_objects});
		map_information.map_objects_num += map_part_objects;
		
		for (int j = 0; j < map_part_objects; ++j)
		{
			const auto* symbol = map_part->getObject(j)->getSymbol();
			addSymbol(symbol);
		}
	}
	for (auto& map_object : map_information.map_objects)
	{
		if (map_object.category.num > 1)
		{
			auto& objects_vector = map_object.objects;
			sort(begin(objects_vector), end(objects_vector), [](const SymbolNum& sym1, const SymbolNum& sym2) { return Symbol::lessByNumber(sym1.symbol, sym2.symbol); });
		}
	}
	for (i = 0; i < map_information.symbols_num; ++i)
	{
		const auto* symbol = map->getSymbol(i);
		if (symbol->getType() == Symbol::Text)
		{
			addFont(symbol);
		}
	}
	map_information.fonts_num = static_cast<int>(map_information.font_names.size());
}

void MapInformationDialog::addSymbol(const Symbol* symbol)
{
	const auto symbol_type = symbol->getType();
	int symbol_index;
	for (symbol_index = 0; symbol_index < 5; ++symbol_index)
	{
		if ((symbol_type & (1 << symbol_index)) == symbol_type)
			break;
	}
	const bool undefined_object = map->findSymbolIndex(symbol) < 0;
	auto& object_category = map_information.map_objects[symbol_index];
	auto& objects_vector = object_category.objects;
	auto found = std::find_if(begin(objects_vector), end(objects_vector), [&symbol](const SymbolNum& sym_count) { return sym_count.symbol == symbol; });
	if (found != std::end(objects_vector))
	{
		++found->num;
	}
	else
	{
		std::vector<QString> colors;
		for (int i = 0; i < map_information.colors_num; ++i)
		{
			const auto* map_color = map->getMapColor(i);
			if (map_color && symbol->containsColor(map_color))
			{
				colors.push_back({map_color->getName()});
			}
		}
		objects_vector.push_back({symbol, undefined_object ? tr("<undefined>") : symbol->getNumberAndPlainTextName(), 1, colors});
	}
	++object_category.category.num;
}

void MapInformationDialog::addFont(const Symbol* symbol)
{
	const auto* text_symbol = symbol->asText();
	const auto& font_family = text_symbol->getFontFamily();
	const auto font_family_substituted = QFontInfo(text_symbol->getQFont()).family();
	auto& font_names = map_information.font_names;
	auto found = std::find_if(begin(font_names), end(font_names), [&font_family](const FontNum& font_count) { return font_count.name == font_family; });
	if (found != std::end(font_names))
	{
		++found->num;
	}
	else
	{
		font_names.push_back({font_family, font_family_substituted, 1});
	}
}

// create textual information and put it in the tree widget
void MapInformationDialog::buildTree()
{
	max_item_length = 0;
	
	addTreeItem(0, tr("Map"), tr("%n object(s)", nullptr, map_information.map_objects_num));
	
	addTreeItem(1, tr("Scale"), QString::fromLatin1("1:%1").arg(map_information.map_scale));
	
	addTreeItem(1, tr("Coordinate reference system"), map_information.map_crs);
	
	addTreeItem(0, tr("Map parts"), tr("%n part(s)", nullptr, map_information.map_parts_num));
	for (const auto& map_part : map_information.map_parts)
	{
		addTreeItem(1, map_part.name, tr("%n object(s)", nullptr, map_part.num));
	}
	
	addTreeItem(0, tr("Symbols"), tr("%n symbol(s)", nullptr, map_information.symbols_num));
	
	addTreeItem(0, tr("Templates"), tr("%n template(s)", nullptr, map_information.templates_num));
	
	if (map_information.undo_steps_num || map_information.redo_steps_num)
	{
		QString undo_redo_type = map_information.undo_steps_num ? tr("Undo") : tr("Redo");
		QString undo_redo_num = QString::fromLatin1("%1").arg(map_information.undo_steps_num ? map_information.undo_steps_num : map_information.redo_steps_num);
		if (map_information.undo_steps_num && map_information.redo_steps_num)
		{
			undo_redo_type.append(QLatin1Char('/')).append(tr("Redo"));
			undo_redo_num.append(QString::fromLatin1("/%1").arg(map_information.redo_steps_num));
		}
		undo_redo_type.append(QChar::Space).append(tr("steps"));
		undo_redo_num.append(QChar::Space).append(map_information.undo_steps_num + map_information.redo_steps_num > 1 ? tr("steps") : tr("step"));
		addTreeItem(0, undo_redo_type, undo_redo_num);
	}
	
	addTreeItem(0, tr("Colors"), tr("%n color(s)", nullptr, map_information.colors_num));
	if (map_information.colors_num)
	{
		for (const auto& color : map_information.colors)
		{
			addTreeItem(1, color.name);
			if (color.symbols.size())
			{
				for (const auto& symbol : color.symbols)
				{
					addTreeItem(2, symbol);
				}
			}
		}
	}
	
	addTreeItem(0, tr("Fonts"), tr("%n font(s)", nullptr, map_information.fonts_num));
	for (const auto& font_name : map_information.font_names)
	{
		addTreeItem(1, (font_name.name == font_name.name_substitute ? font_name.name : font_name.name + tr(" (substituted by ") + font_name.name_substitute + QLatin1Char(')')), 
		            tr("%n symbol(s)", nullptr, font_name.num));
	}
	
	addTreeItem(0, tr("Objects"), tr("%n object(s)", nullptr, map_information.map_objects_num));
	for (const auto& map_object : map_information.map_objects)
	{
		if (map_object.category.num)
		{
			addTreeItem(1, map_object.category.name, tr("%n object(s)", nullptr, map_object.category.num));
			for (const auto& object : map_object.objects)
			{
				addTreeItem(2, object.name, tr("%n object(s)", nullptr, object.num));
				if (object.colors.size())
				{
					for (const auto& color : object.colors)
					{
						addTreeItem(3, color);
					}
				}
			}
		}
	}
}

void MapInformationDialog::addTreeItem(const int level, const QString& item, const QString& value)
{
	Q_ASSERT(level >= 0 && level <= 3);
	
	tree_items.push_back({level, item, value});
	max_item_length = qMax(max_item_length, level * text_report_indent + item.length());
}

void MapInformationDialog::setupTreeWidget()
{
	QVarLengthArray<QTreeWidgetItem*, 5> tree_item_hierarchy;
	tree_item_hierarchy.push_back(map_info_tree->invisibleRootItem());
	
	for (const auto &tree_item : tree_items)
	{
		auto const level = qMax(1, tree_item.level + 1);
		if (tree_item_hierarchy.size() > tree_item.level)
			tree_item_hierarchy.resize(level);
	
		auto* tree_widget_item = new QTreeWidgetItem(tree_item_hierarchy.back());
		tree_widget_item->setText(0, tree_item.item);
		tree_widget_item->setText(1, tree_item.value);
		tree_item_hierarchy.push_back(tree_widget_item);
	}
}

QString MapInformationDialog::makeTextReport() const
{
	QString text_report;
	for (const auto &tree_item : tree_items)
	{
		QString item_value;
		if (!text_report.isEmpty() && !tree_item.level)	// separate items on topmost level
			text_report += QChar::LineFeed;
		item_value.fill(QChar::Space, tree_item.level * text_report_indent);
		item_value.append(tree_item.item);
		if (!tree_item.value.isEmpty())
		{
			item_value = item_value.leftJustified(max_item_length + 5, QChar::Space);
			item_value.append(tree_item.value);
		}
		text_report += item_value + QChar::LineFeed;
	}
	return text_report;
}

// slot
void MapInformationDialog::save()
{
	auto filepath = FileDialog::getSaveFileName(
	    this,
	    ::OpenOrienteering::MainWindow::tr("Save file"),
	    QFileInfo(main_window->currentPath()).canonicalPath(),
	    tr("Textfiles (*.txt)") );
	
	if (filepath.isEmpty())
		return;
	if (!filepath.endsWith(QLatin1String(".txt"), Qt::CaseInsensitive))
		filepath.append(QLatin1String(".txt"));
	
	QSaveFile file(filepath);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		file.write(makeTextReport().toUtf8());
		if (file.commit())
			return;
	}
	QMessageBox::warning(this, tr("Error"),
	                     tr("Cannot save file:\n%1\n\n%2")
	                     .arg(filepath, file.errorString()) );
}


}  // namespace OpenOrienteering
