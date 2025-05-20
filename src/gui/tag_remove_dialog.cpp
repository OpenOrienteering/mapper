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
#include <set>
#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QChar>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLatin1String>
#include <QLineEdit>
#include <QMessageBox>
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
	{ QCoreApplication::translate("OpenOrienteering::TagRemoveDialog", "equal to"), [](const QString& key, const QString& pattern) { return key == pattern; } },
	{ QCoreApplication::translate("OpenOrienteering::TagRemoveDialog", "not equal to"), [](const QString& key, const QString& pattern) { return key != pattern; } },
	{ QCoreApplication::translate("OpenOrienteering::TagRemoveDialog", "containing"), [](const QString& key, const QString& pattern) { return key.contains(pattern); } },
	{ QCoreApplication::translate("OpenOrienteering::TagRemoveDialog", "not containing"), [](const QString& key, const QString& pattern) { return !key.contains(pattern); } }
};

}  // namespace


namespace OpenOrienteering {

TagRemoveDialog::TagRemoveDialog(QWidget* parent, Map* map)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
, map { map }
{
	setWindowTitle(tr("Remove Tags"));
	
	auto* h_layout = new QHBoxLayout();
	h_layout->addWidget(new QLabel(tr("Remove all tags")));
	
	compare_op = new QComboBox();
	for (auto& compare_operation : compare_operations)
	{
		compare_op->addItem(compare_operation.op);
	}
	h_layout->addWidget(compare_op);
	
	pattern_edit = new QLineEdit();
	
	undo_check = new QCheckBox(tr("Add undo step"));
	
	auto* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	ok_button = button_box->button(QDialogButtonBox::Ok);
	ok_button->setEnabled(false);
	
	auto* layout = new QVBoxLayout();
	layout->addLayout(h_layout);
	layout->addWidget(pattern_edit);
	layout->addWidget(new QLabel(tr("from the selected objects.")));
	layout->addWidget(undo_check);
	layout->addItem(Util::SpacerItem::create(this));
	layout->addStretch();
	layout->addWidget(button_box);
	setLayout(layout);
	
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(button_box, &QDialogButtonBox::accepted, this, &TagRemoveDialog::okClicked);
	connect(pattern_edit, &QLineEdit::textChanged, this, &TagRemoveDialog::textChanged);
}

TagRemoveDialog::~TagRemoveDialog() = default;


// slot
void TagRemoveDialog::textChanged(const QString& text)
{
	ok_button->setEnabled(!text.trimmed().isEmpty());
}

// slot
void TagRemoveDialog::okClicked()
{
	const auto pattern = pattern_edit->text();
	const auto op = compare_op->currentIndex();
	
	int objects_count = 0;
	std::set<QString> matching_keys;
	
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
	if (matching_keys.empty())
	{
		QMessageBox::information(this, tr("Information"), tr("No matching object tags found."), QMessageBox::Ok);
		return;
	}
	else
	{
		QString detailed_text = std::accumulate( begin(matching_keys), 
												 end(matching_keys), 
												 QString(tr("The following object tags will be removed:")),
												 [](const QString& a, const QString& b) -> QString { return a + QChar::LineFeed + b; }
												);
		QMessageBox msgBox;
		msgBox.setWindowTitle(tr("Confirmation"));
		msgBox.setIcon(QMessageBox::Warning);
		QString question = tr("Do you want to remove %n tag(s)", nullptr, matching_keys.size());
		question += QChar::Space + tr("from %n object(s)?", nullptr, objects_count);
		msgBox.setText(question);
		msgBox.setDetailedText(detailed_text);
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		
		if (msgBox.exec() == QMessageBox::Yes)
		{
			const auto add_undo = undo_check->isChecked();
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
	}
	accept();
}


}  // namespace OpenOrienteering
