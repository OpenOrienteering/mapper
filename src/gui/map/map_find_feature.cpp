/*
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


#include "map_find_feature.h"


#include <Qt>
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFlags>
#include <QGridLayout>
#include <QKeySequence>  // IWYU pragma: keep
#include <QPushButton>
#include <QString>
#include <QTextEdit>

#include "core/map.h"
#include "core/map_part.h"
#include "core/objects/object.h"
#include "core/objects/object_query.h"
#include "core/objects/text_object.h"
#include "gui/main_window.h"
#include "gui/map/map_editor.h"
#include "util/util.h"


MapFindFeature::MapFindFeature(MainWindow& window, MapEditorController& controller)
: window{window}
, controller{controller}
, text_edit{nullptr}
{
	show_action.reset(new QAction(tr("&Find..."), &window));
	// QKeySequence::Find may be Ctrl+F, which conflicts with "Fill / Create Border"
	//show_action->setShortcut(QKeySequence::Find);
	//action->setStatusTip(tr_tip);
	show_action->setWhatsThis(Util::makeWhatThis("edit_menu.html"));
	connect(&*show_action, &QAction::triggered, this, &MapFindFeature::showDialog);
	
	find_next_action.reset(new QAction(tr("Find &next"), &window));
	// QKeySequence::FindNext may be F3, which conflicts with "Baseline view"
	//find_next_action->setShortcut(QKeySequence::FindNext);
	//action->setStatusTip(tr_tip);
	find_next_action->setWhatsThis(Util::makeWhatThis("edit_menu.html"));
	connect(&*find_next_action, &QAction::triggered, this, &MapFindFeature::findNext);
}


MapFindFeature::~MapFindFeature() = default; // not inlined



void MapFindFeature::setEnabled(bool enabled)
{
	show_action->setEnabled(enabled);
	find_next_action->setEnabled(enabled);
}



QAction* MapFindFeature::showAction()
{
	return show_action.get();
}


QAction* MapFindFeature::findNextAction()
{
	return find_next_action.get();
}



void MapFindFeature::showDialog()
{
	if (!find_dialog)
	{
		find_dialog.reset(new QDialog(&window));
		find_dialog->setWindowTitle(tr("Find objects"));
		
		text_edit = new QTextEdit;
		text_edit->setLineWrapMode(QTextEdit::WidgetWidth);
		
		auto find_next = new QPushButton(tr("&Find next"));
		connect(find_next, &QPushButton::clicked, this, &MapFindFeature::findNext);
		
		auto find_all = new QPushButton(tr("Find &all"));
		connect(find_all, &QPushButton::clicked, this, &MapFindFeature::findAll);
		
		auto button_box = new QDialogButtonBox(QDialogButtonBox::Close | QDialogButtonBox::Help);
		connect(button_box, &QDialogButtonBox::rejected, &*find_dialog, &QDialog::hide);
		connect(button_box->button(QDialogButtonBox::Help), &QPushButton::clicked, this, &MapFindFeature::showHelp);
		
		auto layout = new QGridLayout;
		layout->addWidget(text_edit, 0, 0, 3, 1);
		layout->addWidget(find_next, 0, 1, 1, 1);
		layout->addWidget(find_all, 1, 1, 1, 1);
		layout->addWidget(button_box, 3, 0, 1, 2);
		
		find_dialog->setLayout(layout);
	}
	
	find_dialog->show();
	find_dialog->raise();
	find_dialog->activateWindow();
}


void MapFindFeature::findNext()
{
	if (text_edit)
	{
		auto text = text_edit->toPlainText().trimmed();
		auto query = ObjectQueryParser().parse(text);
		if (!query)
			query = ObjectQuery(ObjectQuery::OperatorSearch, text);
		
		auto map = controller.getMap();
		auto first_object = map->getFirstSelectedObject();
		Object* next_object = nullptr;
		auto search = [&first_object, &next_object, &query, &text](Object* object) {
			if (!next_object)
			{
				if (first_object)
				{
					if (object == first_object)
						first_object = nullptr;
				}
				else if (query(object)
				        || (!text.isEmpty()
				            && object->getType() == Object::Text
				            && static_cast<const TextObject*>(object)->getText().contains(text, Qt::CaseInsensitive)))
				{
					next_object = object;
				}
			}
		};
		
		if (first_object)
			// Start from selected object
			map->getCurrentPart()->applyOnAllObjects(search);
		if (!next_object)
			// Start from first object
			map->getCurrentPart()->applyOnAllObjects(search);
		
		map->clearObjectSelection(false);
		if (next_object)
			map->addObjectToSelection(next_object, false);
		map->emitSelectionChanged();
	}
}


void MapFindFeature::findAll()
{
	if (text_edit)
	{
		auto text = text_edit->toPlainText().trimmed();
		auto query = ObjectQueryParser().parse(text);
		if (!query)
			query = ObjectQuery(ObjectQuery::OperatorSearch, text);
		
		auto map = controller.getMap();
		map->clearObjectSelection(false);
		map->getCurrentPart()->applyOnAllObjects([map, &query, &text](Object* object) {
			if (query(object)
			    || (!text.isEmpty()
			        && object->getType() == Object::Text
			        && static_cast<const TextObject*>(object)->getText().contains(text, Qt::CaseInsensitive)))
			{
				map->addObjectToSelection(object, false);
			}
		});
		map->emitSelectionChanged();
	}
}


void MapFindFeature::showHelp()
{
	Util::showHelp(&window, "search_dialog.html");
}

