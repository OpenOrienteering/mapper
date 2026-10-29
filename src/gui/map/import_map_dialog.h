/*
 *    Copyright 2026 Matthias Kühlewein
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

#ifndef OPENORIENTEERING_IMPORT_MAP_DIALOG_H
#define OPENORIENTEERING_IMPORT_MAP_DIALOG_H

#include <vector>

#include <QDialog>
#include <QObject>
#include <QString>
#include <QStringList>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QRadioButton;
class QToolButton;
class QWidget;

namespace OpenOrienteering {

class Map;

/**
 * Configuration for one part
 */
struct PartConfigItem {
	QString name;				// part name in import map
	QString target_name;		// part name in destination map
	bool import;				// import?
	int num_objects;
	bool import_selected;
	bool import_equal_objects;
	QString compare_map_part;
	QString object_tag_key;
	QString object_tag_value;
};


/**
 * Dialog for detailing the import of a map.
 * 
 * For each part to import, it's possible to configure
 * - if the part shall be imported and into which part
 * - if all objects or only these that either equal or unequal
 *   to those of a selectable part of the destination map shall be imported
 * - Key and Value for object tags that will be set for the imported objects.
 */
class ImportMapDialog : public QDialog
{
Q_OBJECT
public:
	/** Creates a new ImportMapDialog. */
	ImportMapDialog(QWidget* parent, Map* map, Map* other);
	
	~ImportMapDialog() override;
	
	std::vector<PartConfigItem> getImportConfig() const { return part_config; }
	bool isNothingToImport() const;
	
private slots:
	void previousClicked();
	void nextClicked();
	void importChanged();
	void objectSelectionChanged(int id);
	void partSelectionChanged(const QString &text);
	void addPartClicked();
	void tag_key_edited();
	void tag_value_edited();
	void equalitySelectionChanged(int id);
	void objectComparisonPartChanged(const QString &text);
	
private:
	void setupPartConfig(const Map* other);
	void showCurrentPart();
	QString determineTargetName(int part_number) const;
	QString determineComparePartName(int part_number) const;
	
	QCheckBox* import_part;
	QToolButton* previous_button;
	QToolButton* next_button;
	QLabel* part_name;
	QLabel* part_num_objects;
	
	QRadioButton* import_all_objects;
	QRadioButton* import_selected_objects;
	QComboBox* part_selection;
	QToolButton* add_part_button;
	
	QGroupBox* object_selection_box;
	QRadioButton* import_equal_objects;
	QRadioButton* import_unequal_objects;
	QComboBox* object_comparison_part;
	
	QGroupBox* optional_tags_box;
	QLineEdit* objects_tag_key;
	QLineEdit* objects_tag_value;
	
	Map* map;
	Map* other;
	std::vector<PartConfigItem> part_config;
	int part_config_index;
	QStringList additional_map_parts;
};


}  // namespace OpenOrienteering


#endif // OPENORIENTEERING_IMPORT_MAP_DIALOG_H
