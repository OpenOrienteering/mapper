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


#include "import_map_dialog.h"

#include <algorithm>

#include <Qt>
#include <QAbstractButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLatin1String>
#include <QLineEdit>
#include <QRadioButton>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>

#include "core/map.h"
#include "core/map_part.h"
#include "gui/util_gui.h"

namespace OpenOrienteering {

ImportMapDialog::ImportMapDialog(QWidget* parent, Map* map, Map* other)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
, map(map)
, other(other)
{
	setWindowTitle(tr("Import map"));
	
	auto* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
	
	auto* part_selection_layout = new QHBoxLayout();
	import_part = new QCheckBox(tr("Import part"));
	previous_button = Util::ToolButton::create(QIcon(QLatin1String(":/images/arrow-left.png")), QString{}, nullptr);
	part_name = new QLabel();
	part_name->setTextFormat(Qt::PlainText);
	next_button = Util::ToolButton::create(QIcon(QLatin1String(":/images/arrow-right.png")), QString{}, nullptr);
	part_num_objects = new QLabel();
	
	part_selection_layout->addWidget(import_part);
	part_selection_layout->addWidget(previous_button);
	part_selection_layout->addWidget(part_name);
	part_selection_layout->addWidget(next_button);
	part_selection_layout->addWidget(part_num_objects);
	
	auto* object_import_layout = new QHBoxLayout();
	auto* button_group = new QButtonGroup();
	import_all_objects = new QRadioButton(tr("Import all objects into part"));
	import_selected_objects = new QRadioButton(tr("Import selected objects into part"));

	button_group->addButton(import_all_objects, 0);
	button_group->addButton(import_selected_objects, 1);
	
	auto* radio_layout = new QVBoxLayout();
	radio_layout->addWidget(import_all_objects);
	radio_layout->addWidget(import_selected_objects);
	
	object_import_layout->addLayout(radio_layout);
	part_selection = new QComboBox();
	object_import_layout->addWidget(part_selection);
	add_part_button = Util::ToolButton::create(QIcon(QLatin1String(":/images/plus.png")), QString{}, nullptr);
	object_import_layout->addWidget(add_part_button);
	
	object_selection_box = new QGroupBox();
//	object_selection_box->setStyleSheet(QLatin1String("QGroupBox { border: none; margin-top: 1px; }"));
	object_selection_box->setFlat(true);
	auto* object_selection_layout = new QHBoxLayout(object_selection_box);
	auto* equality_button_group = new QButtonGroup();
	import_equal_objects = new QRadioButton(tr("that are equal to those of part"));
	import_unequal_objects = new QRadioButton(tr("that are unequal to those of part"));
	equality_button_group->addButton(import_equal_objects, 0);
	equality_button_group->addButton(import_unequal_objects, 1);
	auto* equality_radio_layout = new QVBoxLayout();
	equality_radio_layout->addWidget(import_equal_objects);
	equality_radio_layout->addWidget(import_unequal_objects);
	object_selection_layout->addLayout(equality_radio_layout);
	object_comparison_part = new QComboBox();
	for (int i = 0; i < map->getNumParts(); ++i)
	{
		if (map->getPart(i)->getNumObjects())
			object_comparison_part->addItem(map->getPart(i)->getName());
	}
	object_selection_layout->addWidget(object_comparison_part);
	
	optional_tags_box = new QGroupBox(tr("Optional: Set tags for imported objects"));
	auto* optional_tags_layout = new QHBoxLayout(optional_tags_box);
	optional_tags_layout->addWidget(new QLabel(tr("Key:")));
	objects_tag_key = new QLineEdit();
	optional_tags_layout->addWidget(objects_tag_key);
	optional_tags_layout->addWidget(new QLabel(tr("Value:")));
	objects_tag_value = new QLineEdit();
	optional_tags_layout->addWidget(objects_tag_value);
	
	auto* box_layout = new QVBoxLayout();
	box_layout->addLayout(part_selection_layout);
	box_layout->addItem(Util::SpacerItem::create(this));
	box_layout->addLayout(object_import_layout);
	box_layout->addWidget(object_selection_box);
	box_layout->addItem(Util::SpacerItem::create(this));
	box_layout->addWidget(optional_tags_box);
	box_layout->addItem(Util::SpacerItem::create(this));
	box_layout->addStretch();
	box_layout->addWidget(button_box);
	
	setLayout(box_layout);
	
	connect(import_part, &QAbstractButton::clicked, this, &ImportMapDialog::importChanged);
	connect(previous_button, &QAbstractButton::clicked, this, &ImportMapDialog::previousClicked);
	connect(next_button, &QAbstractButton::clicked, this, &ImportMapDialog::nextClicked);
	connect(button_group, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &ImportMapDialog::objectSelectionChanged);
	connect(part_selection, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, &ImportMapDialog::partSelectionChanged);
	connect(add_part_button, &QAbstractButton::clicked, this, &ImportMapDialog::addPartClicked);
	connect(equality_button_group, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &ImportMapDialog::equalitySelectionChanged);
	connect(object_comparison_part, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, &ImportMapDialog::objectComparisonPartChanged);
	connect(objects_tag_key, &QLineEdit::textEdited, this, &ImportMapDialog::tag_key_edited);
	connect(objects_tag_value, &QLineEdit::textEdited, this, &ImportMapDialog::tag_value_edited);
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	
	setupPartConfig(other);
	showCurrentPart();
}

ImportMapDialog::~ImportMapDialog() = default;


void ImportMapDialog::setupPartConfig(const Map* other)
{
	for (int i = 0; i < other->getNumParts(); ++i)
	{
		const auto part = other->getPart(i);
		PartConfigItem configItem;
		configItem.name = part->getName();
		configItem.target_name = determineTargetName(i);
		configItem.num_objects = part->getNumObjects();
		configItem.import = true;
		configItem.import_selected = false;
		configItem.import_equal_objects = false;
		configItem.compare_map_part = determineComparePartName(i);
		part_config.emplace_back(configItem);
	}
	part_config_index = 0;
}

void ImportMapDialog::showCurrentPart()
{
	Q_ASSERT(part_config_index < (int)part_config.size());
	
	const auto configItem = part_config.at(part_config_index);
	const auto import = configItem.import;
	const auto hasObjects = configItem.num_objects > 0;
	
	part_name->setText(configItem.name);
	part_name->setEnabled(import);
	import_part->setChecked(import);
	previous_button->setEnabled(part_config_index > 0);
	next_button->setEnabled(part_config_index < (int)part_config.size()-1);
	part_num_objects->setText(tr("with %n object(s)", nullptr, configItem.num_objects));
	(configItem.import_selected ? import_selected_objects : import_all_objects)->setChecked(true);
	import_selected_objects->setEnabled(import && hasObjects);
	import_all_objects->setEnabled(import && hasObjects);
	add_part_button->setEnabled(import && hasObjects);
	optional_tags_box->setEnabled(import && hasObjects);
	
	part_selection->setEnabled(import);
	QSignalBlocker block(part_selection);
	part_selection->clear();
	// first add all existing part names in destination map and check whether one of them would
	// match the import part name
	auto match = false;
	for (int i = 0; i < map->getNumParts(); ++i)
	{
		part_selection->addItem(map->getPart(i)->getName());
		if (map->getPart(i)->getName().compare(other->getPart(part_config_index)->getName(), Qt::CaseInsensitive) == 0)
			match = true;
	}
	// if there is no match, add the import part name
	if (!match)
		part_selection->addItem(other->getPart(part_config_index)->getName());
	// add additional map parts
	for (const auto& name : qAsConst(additional_map_parts))
	{
		if (part_selection->findText(name, Qt::MatchExactly) == -1)
			part_selection->addItem(name);
	}
	
	auto index = part_selection->findText(configItem.target_name, Qt::MatchExactly);
	part_selection->setCurrentIndex(index != -1 ? index : 0);
	
	objects_tag_key->setText(configItem.object_tag_key);
	objects_tag_value->setText(configItem.object_tag_value);
	
	object_selection_box->setVisible(configItem.import_selected);
	if (configItem.import_selected)
	{
		(configItem.import_equal_objects ? import_equal_objects : import_unequal_objects)->setChecked(true);
		object_comparison_part->setCurrentIndex(object_comparison_part->findText(configItem.compare_map_part, Qt::MatchExactly));
	}
}

//slot
void ImportMapDialog::previousClicked()
{
	Q_ASSERT(part_config_index > 0);
	--part_config_index;
	showCurrentPart();
}

//slot
void ImportMapDialog::nextClicked()
{
	Q_ASSERT(part_config_index < (int)part_config.size()-1);
	++part_config_index;
	showCurrentPart();
}

//slot
void ImportMapDialog::importChanged()
{
	part_config.at(part_config_index).import = import_part->isChecked();
	showCurrentPart();
}

//slot
void ImportMapDialog::objectSelectionChanged(int id)
{
	part_config.at(part_config_index).import_selected = id == 1;
	showCurrentPart();
}

//slot
void ImportMapDialog::partSelectionChanged(const QString &text)
{
	part_config.at(part_config_index).target_name = text;
}

//slot
void ImportMapDialog::addPartClicked()
{
	bool accepted = false;
	QString name = QInputDialog::getText(
	                   this,
	                   tr("Add new part..."),
	                   tr("Enter the name of the map part:"),
	                   QLineEdit::Normal,
	                   QString(),
	                   &accepted );
	if (accepted && !name.isEmpty())
	{
		if (!additional_map_parts.contains(name))
		{
			additional_map_parts.append(name);
			showCurrentPart();
		}
	}
}

//slot
void ImportMapDialog::tag_key_edited()
{
	part_config.at(part_config_index).object_tag_key = objects_tag_key->text();
}

//slot
void ImportMapDialog::tag_value_edited()
{
	part_config.at(part_config_index).object_tag_value = objects_tag_value->text();
}

//slot
void ImportMapDialog::equalitySelectionChanged(int id)
{
	part_config.at(part_config_index).import_equal_objects = id == 0;
	showCurrentPart();
}

//slot
void ImportMapDialog::objectComparisonPartChanged(const QString &text)
{
	part_config.at(part_config_index).compare_map_part = text;
}


bool ImportMapDialog::isNothingToImport() const
{
	return std::none_of(begin(part_config), end(part_config), [](const auto& item) {return item.import;});
}

QString ImportMapDialog::determineTargetName(int part_number) const
{
	// align to Mapper's current behavior to determine part names
	if (other->getNumParts() == 1)
		return map->getCurrentPart()->getName();
	
	for (int i = 0; i < map->getNumParts(); ++i)
	{
		if (map->getPart(i)->getName().compare(other->getPart(part_number)->getName(), Qt::CaseInsensitive) == 0)
		{
			return map->getPart(i)->getName();
		}
	}
	return other->getPart(part_number)->getName();
}

QString ImportMapDialog::determineComparePartName(int part_number) const
{
	for (int i = 0; i < map->getNumParts(); ++i)
	{
		if (map->getPart(i)->getName().compare(other->getPart(part_number)->getName(), Qt::CaseInsensitive) == 0)
		{
			return map->getPart(i)->getName();
		}
	}
	return map->getCurrentPart()->getName();
}

}  // namespace OpenOrienteering
