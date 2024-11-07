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

#ifndef OPENORIENTEERING_MAP_INFORMATION_DIALOG_H
#define OPENORIENTEERING_MAP_INFORMATION_DIALOG_H

#include <array>
#include <vector>

#include <QDialog>
#include <QObject>
#include <QString>

class QTreeWidget;
class QTreeWidgetItem;
class QWidget;

namespace OpenOrienteering {

class Map;
class MainWindow;
class Symbol;

/**
 * A class for providing information about the current map.
 * 
 * Information is given about the number of objects in total and per map part,
 * the number of symbols, templates and undo/redo steps.
 * For each color a list of symbols using that color is shown.
 * All fonts (and their substitutions) being used by symbols are shown.
 * For objects there is a hierarchical view:
 * - the number of objects per symbol class (e.g., Point symbols, Line symbols etc.)
 *   - for each symbol class the symbols in use and the related number of objects
 *     - for each symbol the colors used by it
 */
class MapInformationDialog : public QDialog
{
Q_OBJECT
public:
	/**
	 * Creates a new MapInformationDialog object.
	 */
	MapInformationDialog(MainWindow* parent, Map* map);
	
	~MapInformationDialog() override;
	
private slots:
	void save();
	
private:
	struct ItemNum {
		QString name;
		int num = 0;
	};

	struct FontNum {
		QString name;
		QString name_substitute;	// the substituted font name, can be the same as 'name'
		int num = 0;
	};
	
	struct SymbolNum {
		const Symbol* symbol;
		QString name;
		int num;
		std::vector<QString> colors;
	};
	
	struct ObjectCategory {
		ItemNum category;
		std::vector<SymbolNum> objects;
	};
	
	struct ColorUsage {
		QString name;
		std::vector<QString> symbols;
	};
	
	struct MapInfo {
		int map_objects_num = 0;
		int map_scale;
		QString map_crs;
		int map_parts_num;
		std::vector<ItemNum> map_parts;
		int symbols_num;
		int undo_steps_num;
		int redo_steps_num;
		int templates_num;
		int colors_num;
		std::vector<ColorUsage> colors;
		int fonts_num;
		std::vector<FontNum> font_names;
		std::array<ObjectCategory, 6> map_objects;
	} map_information;
	
	struct TreeItem {
		int level;
		QString item;
		QString value;
	};
	
	/**
	 * Retrieves the map information and stores it in the map_information structure.
	 */
	void retrieveInformation();
	
	/**
	 * For a given symbol:
	 * - Retrieves the symbol class and counts its objects.
	 * - Counts the objects.
	 * - Retrieves all colors.
	 */
	void addSymbol(const Symbol* symbol);
	
	/**
	 * Retrieves font (and its substitution) for a given text symbol and counts
	 * the font occurrence.
	 */
	void addFont(const Symbol* symbol);
	
	/**
	 * Creates the textual information from the stored information and
	 * puts it in the tree widget.
	 */
	void buildTree();
	
	/**
	 * Adds an item, its level and (in most cases) its value to the tree_items list and
	 * determines the maximum length of all (indented) items for creating a text report.
	 */
	void addTreeItem(const int level, const QString& item, const QString& value = QString());
	
	/**
	 * Processes tree_items list to hierarchically setup the tree widget.
	 */
	void setupTreeWidget();
	
	/**
	 * Processes tree_items list to create a text report.
	 */
	QString makeTextReport() const;
	
	
	const Map* map;
	const MainWindow* main_window;
	
	std::vector<TreeItem> tree_items;
	QTreeWidget* map_info_tree = nullptr;
	std::vector<QTreeWidgetItem*> tree_item_hierarchy;
	
	int max_item_length;
	const int text_report_indent;
};

}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_MAP_INFORMATION_DIALOG_H
