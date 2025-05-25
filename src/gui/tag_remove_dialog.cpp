/*
 *    Copyright 2025 Matthias Kühlewein
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

#include "tag_remove_dialog.h"

#include <algorithm>
#include <functional>
#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QAbstractButton>
#include <QChar>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLatin1String>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "core/map.h"
#include "core/objects/object.h"
#include "gui/util_gui.h"
#include "undo/object_undo.h"


namespace {

struct CompOpStruct {
	const QString op;
	const std::function<bool (const QString&, const QString&)> fn;
};

static const CompOpStruct compare_operations[4] = {
	{ QCoreApplication::translate("OpenOrienteering::TagRemoveDialog", "is"), [](const QString& key, const QString& pattern) { return key == pattern; } },
	{ QCoreApplication::translate("OpenOrienteering::TagRemoveDialog", "is not"), [](const QString& key, const QString& pattern) { return key != pattern; } },
	{ QCoreApplication::translate("OpenOrienteering::TagRemoveDialog", "contains"), [](const QString& key, const QString& pattern) { return key.contains(pattern); } },
	{ QCoreApplication::translate("OpenOrienteering::TagRemoveDialog", "contains not"), [](const QString& key, const QString& pattern) { return !key.contains(pattern); } }
};

}  // namespace


namespace OpenOrienteering {

TagRemoveDialog::TagRemoveDialog(QWidget* parent, Map* map)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
, map { map }
{
	setWindowTitle(tr("Remove Tags"));
	
	auto* search_operation_layout = new QHBoxLayout();
	search_operation_layout->addWidget(new QLabel(tr("Key")));
	compare_op = new QComboBox();
	for (auto& compare_operation : compare_operations)
	{
		compare_op->addItem(compare_operation.op);
	}
	search_operation_layout->addWidget(compare_op);
	pattern_edit = new QLineEdit();
	search_operation_layout->addWidget(pattern_edit);
	
	undo_check = new QCheckBox(tr("Add undo step"));
	number_matching_objects = new QLabel();
	number_matching_keys = new QLabel();
	matching_keys_details = new QPlainTextEdit();
	matching_keys_details->setReadOnly(true);

	auto* button_box = new QDialogButtonBox();
	find_button = new QPushButton(tr("Find"));
	find_button->setEnabled(false);
	button_box->addButton(find_button, QDialogButtonBox::ActionRole);
	button_box->addButton(QDialogButtonBox::Cancel);
	remove_button = new QPushButton(QIcon(QLatin1String(":/images/delete.png")), tr("Remove"));
	remove_button->setEnabled(false);
	button_box->addButton(remove_button, QDialogButtonBox::ActionRole);
	
	auto* layout = new QVBoxLayout();
	layout->addWidget(new QLabel(tr("Remove tags from %n selected object(s)", nullptr, map->getNumSelectedObjects())));
	layout->addLayout(search_operation_layout);
	layout->addWidget(undo_check);
	layout->addItem(Util::SpacerItem::create(this));
	layout->addWidget(number_matching_objects);
	layout->addWidget(number_matching_keys);
	layout->addWidget(matching_keys_details);
	layout->addWidget(button_box);
	setLayout(layout);
	
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(find_button, &QAbstractButton::clicked, this, &TagRemoveDialog::findClicked);
	connect(remove_button, &QAbstractButton::clicked, this, &TagRemoveDialog::removeClicked);
	connect(pattern_edit, &QLineEdit::textChanged, this, &TagRemoveDialog::textChanged);
	connect(compare_op, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TagRemoveDialog::comboBoxChanged);
}

TagRemoveDialog::~TagRemoveDialog() = default;


// slot
void TagRemoveDialog::textChanged(const QString& text)
{
	find_button->setEnabled(!text.trimmed().isEmpty());
	remove_button->setEnabled(false);
}

// slot
void TagRemoveDialog::comboBoxChanged()
{
	remove_button->setEnabled(false);
}

// slot
void TagRemoveDialog::findClicked()
{
	std::set<QString> matching_keys;
	const auto objects_count = findMatchingTags(map, pattern_edit->text(), compare_op->currentIndex(), matching_keys);
	
	number_matching_objects->setText(tr("Number of matching objects: %1").arg(objects_count));
	number_matching_keys->setText(tr("%n matching keys:", nullptr, matching_keys.size()));
	matching_keys_details->clear();
	remove_button->setEnabled(!matching_keys.empty());
	
	if (!matching_keys.empty())
	{
		QString details = std::accumulate(begin(matching_keys), 
		                                  end(matching_keys), 
		                                  QString(),
		                                  [](const QString& a, const QString& b) -> QString { return a.isEmpty() ? b : a + QChar::LineFeed + b; }
		                                 );
		matching_keys_details->insertPlainText(details);
	}
}

// static
int TagRemoveDialog::findMatchingTags(const Map *map, const QString& pattern, int op, std::set<QString>& matching_keys)
{
	int objects_count = 0;
	matching_keys.clear();
	
	for (const auto& object : map->selectedObjects())
	{
		auto object_matched = false;
		for (const auto& tag : object->tags())
		{
			if ((compare_operations[op].fn)(tag.key, pattern))
			{
				matching_keys.insert(tag.key);
				object_matched = true;
			}
		}
		if (object_matched)
			++objects_count;
	}
	return objects_count;
}

// slot
void TagRemoveDialog::removeClicked()
{
	const auto add_undo = undo_check->isChecked();
	
	auto question = QString(tr("Do you really want to remove the found object tags?"));
	if (!add_undo)
		question += QChar::LineFeed + QString(tr("This cannot be undone."));
	if (QMessageBox::question(this, tr("Remove object tags"), question, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
	{
		removeMatchingTags(map, pattern_edit->text(), compare_op->currentIndex(), add_undo);
		accept();
	}
}

// static
void TagRemoveDialog::removeMatchingTags(Map *map, const QString& pattern, int op, bool add_undo)
{
	CombinedUndoStep* combined_step;
	if (add_undo)
		combined_step = new CombinedUndoStep(map);
	
	std::vector<QString> matching_keys;
	for (const auto& object : map->selectedObjects())
	{
		matching_keys.clear();
		for (const auto& tag : object->tags())
		{
			if ((compare_operations[op].fn)(tag.key, pattern))
			{
				matching_keys.push_back(tag.key);
			}
		}
		if (add_undo && !matching_keys.empty())
		{
			auto undo_step = new ObjectTagsUndoStep(map);
			undo_step->addObject(map->getCurrentPart()->findObjectIndex(object));
			combined_step->push(undo_step);
		}
		for (const auto& key : matching_keys)
		{
			object->removeTag(key);
		}
	}
	if (add_undo)
		map->push(combined_step);
}

}  // namespace OpenOrienteering
