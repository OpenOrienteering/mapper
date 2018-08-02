/*
 *    Copyright 2017, 2018 Kai Pastor
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

#include <functional>

#include <QAction>
#include <QAbstractButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFlags>
#include <QGridLayout>
#include <QKeySequence>  // IWYU pragma: keep
#include <QPushButton>
#include <QStackedLayout>
#include <QString>
#include <QTextEdit>
#include <QWidget>

#include "core/map.h"
#include "core/map_part.h"
#include "core/objects/object_query.h"
#include "gui/main_window.h"
#include "gui/util_gui.h"
#include "gui/map/map_editor.h"
#include "gui/widgets/tag_select_widget.h"


namespace OpenOrienteering {

class Object;

MapFindFeature::MapFindFeature(MapEditorController& controller)
: QObject{nullptr}
, controller{controller}
{
	show_action = new QAction(tr("&Find..."), this);
	show_action->setMenuRole(QAction::NoRole);
	// QKeySequence::Find may be Ctrl+F, which conflicts with "Fill / Create Border"
	//show_action->setShortcut(QKeySequence::Find);
	//action->setStatusTip(tr_tip);
	show_action->setWhatsThis(Util::makeWhatThis("edit_menu.html"));
	connect(show_action, &QAction::triggered, this, &MapFindFeature::showDialog);
	
	find_next_action = new QAction(tr("Find &next"), this);
	find_next_action->setMenuRole(QAction::NoRole);
	// QKeySequence::FindNext may be F3, which conflicts with "Baseline view"
	//find_next_action->setShortcut(QKeySequence::FindNext);
	//action->setStatusTip(tr_tip);
	find_next_action->setWhatsThis(Util::makeWhatThis("edit_menu.html"));
	connect(find_next_action, &QAction::triggered, this, &MapFindFeature::findNext);
}


MapFindFeature::~MapFindFeature() = default; // not inlined



void MapFindFeature::setEnabled(bool enabled)
{
	show_action->setEnabled(enabled);
	find_next_action->setEnabled(enabled);
}



void MapFindFeature::showDialog()
{
	auto window = controller.getWindow();
	if (!window)
		return;
	
	if (!find_dialog)
	{
		find_dialog = new QDialog(window);
		find_dialog->setWindowTitle(tr("Find objects"));
		
		text_edit = new QTextEdit;
		text_edit->setLineWrapMode(QTextEdit::WidgetWidth);
		
		tag_selector = new TagSelectWidget;
		
		auto find_next = new QPushButton(tr("&Find next"));
		connect(find_next, &QPushButton::clicked, this, &MapFindFeature::findNext);
		
		auto find_all = new QPushButton(tr("Find &all"));
		connect(find_all, &QPushButton::clicked, this, &MapFindFeature::findAll);
		
		auto tags_button = new QPushButton(tr("Query editor"));
		tags_button->setCheckable(true);
		
		tag_selector_buttons = tag_selector->makeButtons();
		tag_selector_buttons->setEnabled(false);
		
		auto button_box = new QDialogButtonBox(QDialogButtonBox::Close | QDialogButtonBox::Help);
		connect(button_box, &QDialogButtonBox::rejected, &*find_dialog, &QDialog::hide);
		connect(button_box->button(QDialogButtonBox::Help), &QPushButton::clicked, this, &MapFindFeature::showHelp);
		
		editor_stack = new QStackedLayout();
		editor_stack->addWidget(text_edit);
		editor_stack->addWidget(tag_selector);
		
		connect(tags_button, &QAbstractButton::toggled, this, &MapFindFeature::tagSelectorToggled);
		
		auto layout = new QGridLayout;
		layout->addLayout(editor_stack, 0, 0, 6, 1);
		layout->addWidget(find_next, 0, 1, 1, 1);
		layout->addWidget(find_all, 1, 1, 1, 1);
		layout->addWidget(tags_button, 3, 1, 1, 1);
		layout->addWidget(tag_selector_buttons, 5, 1, 1, 1);
		layout->addWidget(button_box, 6, 0, 1, 2);
		
		find_dialog->setLayout(layout);
	}
	
	find_dialog->show();
	find_dialog->raise();
	find_dialog->activateWindow();
}



ObjectQuery MapFindFeature::makeQuery() const
{
	auto query = ObjectQuery{};
	if (find_dialog)
	{
		if (editor_stack->currentIndex() == 0)
		{
			auto text = text_edit->toPlainText().trimmed();
			if (!text.isEmpty())
			{
				query = ObjectQueryParser().parse(text);
				if (!query || query.getOperator() == ObjectQuery::OperatorSearch)
					query = ObjectQuery{ ObjectQuery(ObjectQuery::OperatorSearch, text),
					        ObjectQuery::OperatorOr,
					        ObjectQuery(ObjectQuery::OperatorObjectText, text) };
			
			}
		}
		else
		{
			query = tag_selector->makeQuery();
		}
	}
	return query;
}


void MapFindFeature::findNext()
{
	auto map = controller.getMap();
	auto first_object = map->getFirstSelectedObject();
	map->clearObjectSelection(false);
	
	Object* next_object = nullptr;
	auto query = makeQuery();
	if (!query)
	{
		if (auto window = controller.getWindow())
			window->showStatusBarMessage(OpenOrienteering::TagSelectWidget::tr("Invalid query"), 2000);
		return;
	}
		
	auto search = [&first_object, &next_object, &query](Object* object) {
		if (!next_object)
		{
			if (first_object)
			{
				if (object == first_object)
					first_object = nullptr;
			}
			else if (query(object))
			{
				next_object = object;
			}
		}
	};
	
	// Start from selected object
	map->getCurrentPart()->applyOnAllObjects(search);
	if (!next_object)
	{
		// Start from first object
		first_object = nullptr;
		map->getCurrentPart()->applyOnAllObjects(search);
	}
	
	map->clearObjectSelection(false);
	if (next_object)
		map->addObjectToSelection(next_object, false);
	map->emitSelectionChanged();
	
	if (!map->selectedObjects().empty())
		controller.setEditTool();
}


void MapFindFeature::findAll()
{
	auto map = controller.getMap();
	map->clearObjectSelection(false);
	
	auto query = makeQuery();
	if (!query)
	{
		controller.getWindow()->showStatusBarMessage(OpenOrienteering::TagSelectWidget::tr("Invalid query"), 2000);
		return;
	}
	
	map->getCurrentPart()->applyOnMatchingObjects([map](Object* object) {
		map->addObjectToSelection(object, false);
	}, std::cref(query));
	map->emitSelectionChanged();
	controller.getWindow()->showStatusBarMessage(OpenOrienteering::TagSelectWidget::tr("%n object(s) selected", nullptr, map->getNumSelectedObjects()), 2000);
	
	if (!map->selectedObjects().empty())
		controller.setEditTool();
}



void MapFindFeature::showHelp() const
{
	Util::showHelp(controller.getWindow(), "find_objects.html");
}



// slot
void MapFindFeature::tagSelectorToggled(bool active)
{
	editor_stack->setCurrentIndex(active ? 1 : 0);
	tag_selector_buttons->setEnabled(active);
	if (!active)
	{
		auto text = tag_selector->makeQuery().toString();
		if (!text.isEmpty())
			text_edit->setText(text);
	}
}


}  // namespace OpenOrienteering
