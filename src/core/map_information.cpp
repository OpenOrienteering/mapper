/*
 *    Copyright 2024 Kai Pastor
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

#include "map_information.h"

#include <algorithm>
#include <iterator>
// IWYU pragma: no_include <memory>
#include <numeric>
#include <vector>

#include <QtGlobal>
#include <QChar>
#include <QCoreApplication>
#include <QFontInfo>
#include <QLatin1Char>
#include <QLatin1String>

#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_color.h"  // IWYU pragma: keep
#include "core/map_part.h"
#include "core/objects/object.h"
#include "core/symbols/symbol.h"
#include "core/symbols/text_symbol.h"
#include "undo/undo_manager.h"


namespace OpenOrienteering {

class MapInformationBuilder
{
public:
	explicit MapInformationBuilder(const Map& map);
	
	/**
	 * Creates the textual information from the stored information and
	 * puts it in the tree widget.
	 */
	void buildTree(std::vector<MapInformation::TreeItem>& tree_items) const;

private:
	struct MapPartUsage {
		QString name;
		int object_count = 0;
	};
	
	struct FontUsage {
		QString name;
		QString name_substitute;	// the substituted font name, can be equal to 'name'
		int symbol_count = 0;
	};
	
	struct SymbolUsage {
		const Symbol* symbol;
		QString name;
		int object_count = 0;
		std::vector<QString> colors = {};
	};
	
	struct SymbolTypeUsage {
		QString name;
		int object_count = 0;
		std::vector<SymbolUsage> symbols = {};
	};
	
	struct ColorUsage {
		QString name;
		std::vector<QString> symbols = {};
	};
	
	QString crs;
	int scale;
	int undo_steps_count;
	int redo_steps_count;
	int templates_count;
	int symbols_count;
	int objects_count;
	int fonts_count;
	
	std::vector<MapPartUsage> map_parts;
	
	SymbolTypeUsage symbol_types[6];
	
	std::vector<ColorUsage> colors;
	
	std::vector<FontUsage> fonts;
	
	/**
	 * Returns the usage record for a given symbol type.
	 */
	SymbolTypeUsage& getSymbolTypeUsage(Symbol::Type type);
};



MapInformationBuilder::SymbolTypeUsage& MapInformationBuilder::getSymbolTypeUsage(Symbol::Type type)
{
	switch (type)
	{
	case Symbol::Point:
		return symbol_types[0];
	case Symbol::Line:
		return symbol_types[1];
	case Symbol::Area:
		return symbol_types[2];
	case Symbol::Combined:
		return symbol_types[3];
	case Symbol::Text:
		return symbol_types[4];
	default:
		return symbol_types[5];
	}
}

// retrieve and store the information
MapInformationBuilder::MapInformationBuilder(const Map& map)
{
	scale = int(map.getScaleDenominator());
	
	const auto& map_crs = map.getGeoreferencing().getProjectedCRSId();
	crs = map.getGeoreferencing().getProjectedCRSName();
	if (map_crs != QLatin1String("Local") && map_crs != QLatin1String("PROJ.4"))
	{
		const auto& projected_crs_parameters = map.getGeoreferencing().getProjectedCRSParameters();
		if (!projected_crs_parameters.empty())
		{
			QString crs_details = QLatin1String(" (") + (map_crs == QLatin1String("EPSG") ? QCoreApplication::translate("OpenOrienteering::MapInformation", "code") : QCoreApplication::translate("OpenOrienteering::MapInformation", "zone")) + QChar::Space + projected_crs_parameters.front() + QLatin1Char(')');
			crs += crs_details;
		}
	}
	
	templates_count = map.getNumTemplates();
	
	colors.reserve(map.getNumColors());
	map.applyOnAllColors([this](const auto* color){
		colors.push_back({color->getName()});
	});
	
	getSymbolTypeUsage(Symbol::Point).name    = QCoreApplication::translate("OpenOrienteering::MapInformation", "Point symbols");
	getSymbolTypeUsage(Symbol::Line).name     = QCoreApplication::translate("OpenOrienteering::MapInformation", "Line symbols");
	getSymbolTypeUsage(Symbol::Area).name     = QCoreApplication::translate("OpenOrienteering::MapInformation", "Area symbols");
	getSymbolTypeUsage(Symbol::Combined).name = QCoreApplication::translate("OpenOrienteering::MapInformation", "Combined symbols");
	getSymbolTypeUsage(Symbol::Text).name     = QCoreApplication::translate("OpenOrienteering::MapInformation", "Text symbols");
	getSymbolTypeUsage(Symbol::NoSymbol).name = QCoreApplication::translate("OpenOrienteering::MapInformation", "Undefined symbols");
	
	symbols_count = map.getNumSymbols();
	map.applyOnAllSymbols([this, &map](const Symbol* symbol){
		auto& category = getSymbolTypeUsage(symbol->getType());
		category.symbols.push_back({symbol, symbol->getNumberAndPlainTextName()});
		
		auto& colors = category.symbols.back().colors;
		colors.reserve(4);
		map.applyOnMatchingColors(
		    [&colors](const MapColor* color) { colors.push_back(color->getName()); },
		    [symbol](const MapColor* color) { return symbol->containsColor(color); }
		    );
	});
	
	auto category_text = getSymbolTypeUsage(Symbol::Text);
	fonts.reserve(category_text.object_count);
	for (const auto& item : category_text.symbols)
	{
		Q_ASSERT(item.symbol->getType() == Symbol::Text);
		const auto* text_symbol = item.symbol->asText();
		const auto& font_family = text_symbol->getFontFamily();
		auto font_family_substituted = QFontInfo(text_symbol->getQFont()).family();
		auto& font_names = fonts;
		auto found = std::find_if(begin(font_names), end(font_names), [&font_family](const FontUsage& font_count) { return font_count.name == font_family; });
		if (found == std::end(font_names))
			font_names.push_back({font_family, font_family_substituted, 1});
		else
			++found->symbol_count;
	}
	fonts_count = static_cast<int>(fonts.size());
	
	map_parts.reserve(map.getNumParts());
	objects_count = 0;
	for (int i = 0; i < map.getNumParts(); ++i)
	{
		const auto* map_part = map.getPart(i);
		const auto map_part_objects = map_part->getNumObjects();
		objects_count += map_part_objects;
		map_parts.push_back({map_part->getName(), map_part_objects});
		
		map_part->applyOnAllObjects([this](const Object* object) {
			const auto* const symbol = object->getSymbol();
			auto& object_category = getSymbolTypeUsage(symbol ? symbol->getType() : Symbol::NoSymbol);
			object_category.object_count++;
			auto s = std::find_if(object_category.symbols.begin(), object_category.symbols.end(), [symbol](const auto& s) { return symbol == s.symbol; } );
			if (s != object_category.symbols.end())
				s->object_count++;
		});
	}
	
	const auto& undo_manager = map.undoManager();
	undo_steps_count = undo_manager.canUndo() ? undo_manager.undoStepCount() : 0;
	redo_steps_count = undo_manager.canRedo() ? undo_manager.redoStepCount() : 0;
}

void MapInformationBuilder::buildTree(std::vector<MapInformation::TreeItem>& tree_items) const
{
	tree_items.clear();

	tree_items.push_back({0, QCoreApplication::translate("OpenOrienteering::MapInformation", "Map"), QCoreApplication::translate("OpenOrienteering::MapInformation", "%n object(s)", nullptr, objects_count)});
	{
		tree_items.push_back({1, QCoreApplication::translate("OpenOrienteering::MapInformation", "Scale"), QString::fromLatin1("1:%1").arg(scale)});
		tree_items.push_back({1, QCoreApplication::translate("OpenOrienteering::MapInformation", "Coordinate reference system"), crs});
		if (undo_steps_count)
			tree_items.push_back({1, QCoreApplication::translate("OpenOrienteering::MapInformation", "Undo steps"), QCoreApplication::translate("OpenOrienteering::MapInformation", "%n step(s)", nullptr, undo_steps_count)});
		if (redo_steps_count)
			tree_items.push_back({1, QCoreApplication::translate("OpenOrienteering::MapInformation", "Redo steps"), QCoreApplication::translate("OpenOrienteering::MapInformation", "%n step(s)", nullptr, redo_steps_count)});
	}
	
	tree_items.push_back({0, QCoreApplication::translate("OpenOrienteering::MapInformation", "Templates"), QCoreApplication::translate("OpenOrienteering::MapInformation", "%n template(s)", nullptr, templates_count)});
	
	tree_items.push_back({0, QCoreApplication::translate("OpenOrienteering::MapInformation", "Map parts"), QCoreApplication::translate("OpenOrienteering::MapInformation", "%n part(s)", nullptr, map_parts.size())});
	for (const auto& map_part : map_parts)
	{
		tree_items.push_back({1, map_part.name, QCoreApplication::translate("OpenOrienteering::MapInformation", "%n object(s)", nullptr, map_part.object_count)});
	}
	
	tree_items.push_back({0, QCoreApplication::translate("OpenOrienteering::MapInformation", "Symbols"), QCoreApplication::translate("OpenOrienteering::MapInformation", "%n symbol(s)", nullptr, symbols_count)});
	for (const auto& map_object : symbol_types)
	{
		tree_items.push_back({1, map_object.name, QCoreApplication::translate("OpenOrienteering::MapInformation", "%n object(s)", nullptr, map_object.object_count)});
		for (const auto& symbol : map_object.symbols)
		{
			tree_items.push_back({2, symbol.name, QCoreApplication::translate("OpenOrienteering::MapInformation", "%n object(s)", nullptr, symbol.object_count)});
			for (const auto& color : symbol.colors)
			{
				tree_items.push_back({3, color});
			}
		}
	}
	
	tree_items.push_back({0, QCoreApplication::translate("OpenOrienteering::MapInformation", "Colors"), QCoreApplication::translate("OpenOrienteering::MapInformation", "%n color(s)", nullptr, colors.size())});
	for (const auto& color : colors)
	{
		tree_items.push_back({1, color.name});
		for (const auto& symbol : color.symbols)
			tree_items.push_back({2, symbol});
	}
	
	tree_items.push_back({0, QCoreApplication::translate("OpenOrienteering::MapInformation", "Fonts"), QCoreApplication::translate("OpenOrienteering::MapInformation", "%n font(s)", nullptr, fonts_count)});
	for (const auto& font_name : fonts)
	{
		auto name = font_name.name;
		if (name != font_name.name_substitute)
			name = QCoreApplication::translate("OpenOrienteering::MapInformation", "%1 (substituted by %2)").arg(name, font_name.name_substitute);
		tree_items.push_back({1, name, QCoreApplication::translate("OpenOrienteering::MapInformation", "%n symbol(s)", nullptr, font_name.symbol_count)});
	}
}


MapInformation::MapInformation(const Map* map)
{
	if (map)
		MapInformationBuilder(*map).buildTree(tree_items);
}

QString MapInformation::makeTextReport(int indent) const
{
	QString text_report;
	
	auto actual_indent = [indent](int level) {
		return level * indent;
	};
	auto max_item_length = std::accumulate(tree_items.begin(), tree_items.end(), 0, [actual_indent](auto acc, const auto& item) {
		return std::max(acc, actual_indent(item.level) + item.label.length());
	});
	
	for (const auto &tree_item : tree_items)
	{
		if (!text_report.isEmpty() && !tree_item.level)	// separate items on topmost level
			text_report += QChar::LineFeed;
		auto item_value = QString(tree_item.level * indent, QChar::Space);
		item_value.append(tree_item.label);
		if (!tree_item.value.isEmpty())
		{
			item_value = item_value.leftJustified(max_item_length + 5, QChar::Space);
			item_value.append(tree_item.value);
		}
		text_report += item_value + QChar::LineFeed;
	}
	return text_report;
}


}  // namespace OpenOrienteering
