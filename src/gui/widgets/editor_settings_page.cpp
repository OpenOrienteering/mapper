/*
 *    Copyright 2012, 2013 Jan Dalheimer
 *    Copyright 2012-2016  Kai Pastor
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

#include "editor_settings_page.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSpacerItem>
#include <QSpinBox>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

#include "settings.h"
#include "gui/map/map_editor.h"
#include "gui/modifier_key.h"
#include "gui/util_gui.h"
#include "gui/widgets/settings_page.h"


namespace OpenOrienteering {

namespace {

class MobileToolbarConfigDialog final : public QDialog
{
public:
	explicit MobileToolbarConfigDialog(QWidget* parent = nullptr)
	: QDialog(parent)
	, toolbar_box(new QComboBox(this))
	, selected_list(new QListWidget(this))
	, available_list(new QListWidget(this))
	{
		setWindowTitle(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Mobile toolbar icons"));

		auto* layout = new QVBoxLayout(this);

		auto* note = new QLabel(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage",
			"Only regular toolbar icons can be changed here. Fixed buttons stay in place, and actions keep their left/right toolbar section in this first version."));
		note->setWordWrap(true);
		layout->addWidget(note);

		toolbar_box->addItem(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Top toolbar"), true);
		toolbar_box->addItem(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Bottom toolbar"), false);
		layout->addWidget(toolbar_box);

		auto* lists_layout = new QHBoxLayout();
		layout->addLayout(lists_layout, 1);

		auto* selected_layout = new QVBoxLayout();
		selected_layout->addWidget(new QLabel(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Displayed actions")));
		selected_layout->addWidget(selected_list, 1);
		lists_layout->addLayout(selected_layout, 1);

		auto* buttons_layout = new QVBoxLayout();
		auto* add_button = new QPushButton(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Add ->"), this);
		auto* remove_button = new QPushButton(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "<- Remove"), this);
		auto* up_button = new QPushButton(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Up"), this);
		auto* down_button = new QPushButton(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Down"), this);
		auto* reset_button = new QPushButton(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Reset"), this);
		buttons_layout->addStretch(1);
		buttons_layout->addWidget(add_button);
		buttons_layout->addWidget(remove_button);
		buttons_layout->addSpacing(12);
		buttons_layout->addWidget(up_button);
		buttons_layout->addWidget(down_button);
		buttons_layout->addSpacing(12);
		buttons_layout->addWidget(reset_button);
		buttons_layout->addStretch(1);
		lists_layout->addLayout(buttons_layout);

		auto* available_layout = new QVBoxLayout();
		available_layout->addWidget(new QLabel(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Available actions")));
		available_layout->addWidget(available_list, 1);
		lists_layout->addLayout(available_layout, 1);

		auto* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
		layout->addWidget(button_box);

		connect(toolbar_box, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
			refillLists(QString{});
		});
		connect(add_button, &QPushButton::clicked, this, [this]() {
			moveAction(true);
		});
		connect(remove_button, &QPushButton::clicked, this, [this]() {
			moveAction(false);
		});
		connect(up_button, &QPushButton::clicked, this, [this]() {
			moveSelectedAction(-1);
		});
		connect(down_button, &QPushButton::clicked, this, [this]() {
			moveSelectedAction(1);
		});
		connect(reset_button, &QPushButton::clicked, this, [this]() {
			currentActions() = MapEditorController::defaultMobileToolbarActionIds(currentTopBar());
			refillLists(QString{});
		});
		connect(selected_list, &QListWidget::itemDoubleClicked, this, [this]() {
			moveAction(false);
		});
		connect(available_list, &QListWidget::itemDoubleClicked, this, [this]() {
			moveAction(true);
		});
		connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
		connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	}

	void setSelectedActions(bool top_bar, const QStringList& actions)
	{
		if (top_bar)
			top_actions = MapEditorController::sanitizeMobileToolbarActionIds(true, actions);
		else
			bottom_actions = MapEditorController::sanitizeMobileToolbarActionIds(false, actions);
		refillLists(QString{});
	}

	QStringList selectedActions(bool top_bar) const
	{
		return top_bar ? top_actions : bottom_actions;
	}

private:
	bool currentTopBar() const
	{
		return toolbar_box->currentData().toBool();
	}

	QStringList& currentActions()
	{
		return currentTopBar() ? top_actions : bottom_actions;
	}

	QStringList candidateActions() const
	{
		return MapEditorController::editableMobileToolbarActionIds(currentTopBar());
	}

	static QListWidgetItem* makeItem(const QString& id)
	{
		auto* item = new QListWidgetItem(MapEditorController::mobileToolbarActionLabel(id));
		item->setData(Qt::UserRole, id);
		return item;
	}

	void refillLists(const QString& selected_id)
	{
		selected_list->clear();
		available_list->clear();

		auto const candidates = candidateActions();
		auto const actions = currentActions();
		for (const auto& id : actions)
		{
			if (candidates.contains(id))
				selected_list->addItem(makeItem(id));
		}
		for (const auto& id : candidates)
		{
			if (!actions.contains(id))
				available_list->addItem(makeItem(id));
		}

		auto select_item = [&selected_id](QListWidget* list) {
			if (selected_id.isEmpty())
				return;
			for (int i = 0; i < list->count(); ++i)
			{
				if (list->item(i)->data(Qt::UserRole).toString() == selected_id)
				{
					list->setCurrentRow(i);
					break;
				}
			}
		};
		select_item(selected_list);
		select_item(available_list);
	}

	void moveAction(bool add_to_selected)
	{
		auto* source = add_to_selected ? available_list : selected_list;
		auto* item = source->currentItem();
		if (!item)
			return;

		auto const id = item->data(Qt::UserRole).toString();
		auto& actions = currentActions();
		if (add_to_selected)
		{
			if (!actions.contains(id))
			{
				if (MapEditorController::mobileToolbarActionOnLeadingSide(id))
				{
					auto insert_pos = actions.size();
					for (int i = 0; i < actions.size(); ++i)
					{
						if (!MapEditorController::mobileToolbarActionOnLeadingSide(actions.at(i)))
						{
							insert_pos = i;
							break;
						}
					}
					actions.insert(insert_pos, id);
				}
				else
				{
					actions << id;
				}
			}
		}
		else
		{
			actions.removeAll(id);
		}
		refillLists(id);
	}

	void moveSelectedAction(int delta)
	{
		auto* item = selected_list->currentItem();
		if (!item)
			return;

		auto& actions = currentActions();
		auto const id = item->data(Qt::UserRole).toString();
		auto const index = actions.indexOf(id);
		auto const new_index = index + delta;
		if (index < 0 || new_index < 0 || new_index >= actions.size())
			return;
		if (MapEditorController::mobileToolbarActionOnLeadingSide(id)
		    != MapEditorController::mobileToolbarActionOnLeadingSide(actions.at(new_index)))
			return;

		actions.move(index, new_index);
		refillLists(id);
	}

	QComboBox* toolbar_box;
	QListWidget* selected_list;
	QListWidget* available_list;
	QStringList top_actions = Settings::defaultMobileTopToolbarActions();
	QStringList bottom_actions = Settings::defaultMobileBottomToolbarActions();
};

}  // namespace

EditorSettingsPage::EditorSettingsPage(QWidget* parent)
 : SettingsPage(parent)
{
	auto layout = new QFormLayout(this);
	
	if (Settings::getInstance().touchModeEnabled())
	{
		button_size = Util::SpinBox::create(1, 3.0, 26.0, tr("mm", "millimeters"), 0.1);
		layout->addRow(tr("Action button size:"), button_size);

		mobile_toolbar_button = new QPushButton(tr("Configure mobile toolbars..."), this);
		layout->addRow(tr("Toolbar icons:"), mobile_toolbar_button);
	}

	icon_size = Util::SpinBox::create(1, 25, tr("mm", "millimeters"));
	layout->addRow(tr("Symbol icon size:"), icon_size);
	
	antialiasing = new QCheckBox(tr("High quality map display (antialiasing)"), this);
	antialiasing->setToolTip(tr("Antialiasing makes the map look much better, but also slows down the map display"));
	layout->addRow(antialiasing);
	
	text_antialiasing = new QCheckBox(tr("High quality text display in map (antialiasing), slow"), this);
	text_antialiasing->setToolTip(tr("Antialiasing makes the map look much better, but also slows down the map display"));
	layout->addRow(text_antialiasing);
	
	tolerance = Util::SpinBox::create(0, 50, tr("mm", "millimeters"));
	layout->addRow(tr("Click tolerance:"), tolerance);
	
	snap_distance = Util::SpinBox::create(0, 100, tr("mm", "millimeters"));
	layout->addRow(tr("Snap distance (%1):").arg(ModifierKey::shift()), snap_distance);
	
	fixed_angle_stepping = Util::SpinBox::create<Util::RotationalDegrees>();
	fixed_angle_stepping->setDecimals(1);
	fixed_angle_stepping->setRange(0.1, 180.0);
	layout->addRow(tr("Stepping of fixed angle mode (%1):").arg(ModifierKey::control()), fixed_angle_stepping);

	select_symbol_of_objects = new QCheckBox(tr("When selecting an object, automatically select its symbol, too"));
	layout->addRow(select_symbol_of_objects);
	
	zoom_out_away_from_cursor = new QCheckBox(tr("Zoom away from cursor when zooming out"));
	layout->addRow(zoom_out_away_from_cursor);
	
	draw_last_point_on_right_click = new QCheckBox(tr("Drawing tools: set last point on finishing with right click"));
	layout->addRow(draw_last_point_on_right_click);
	
	keep_settings_of_closed_templates = new QCheckBox(tr("Templates: keep settings of closed templates"));
	layout->addRow(keep_settings_of_closed_templates);
	
	ignore_touch_input = new QCheckBox(tr("User input: Ignore display touch"));
	layout->addRow(ignore_touch_input);
	
	
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(Util::Headline::create(tr("Edit tool:")));
	
	edit_tool_delete_bezier_point_action = new QComboBox();
	edit_tool_delete_bezier_point_action->addItem(tr("Retain old shape"), (int)Settings::DeleteBezierPoint_RetainExistingShape);
	edit_tool_delete_bezier_point_action->addItem(tr("Reset outer curve handles"), (int)Settings::DeleteBezierPoint_ResetHandles);
	edit_tool_delete_bezier_point_action->addItem(tr("Keep outer curve handles"), (int)Settings::DeleteBezierPoint_KeepHandles);
	layout->addRow(tr("Action on deleting a curve point with %1:").arg(ModifierKey::control()), edit_tool_delete_bezier_point_action);
	
	edit_tool_delete_bezier_point_action_alternative = new QComboBox();
	edit_tool_delete_bezier_point_action_alternative->addItem(tr("Retain old shape"), (int)Settings::DeleteBezierPoint_RetainExistingShape);
	edit_tool_delete_bezier_point_action_alternative->addItem(tr("Reset outer curve handles"), (int)Settings::DeleteBezierPoint_ResetHandles);
	edit_tool_delete_bezier_point_action_alternative->addItem(tr("Keep outer curve handles"), (int)Settings::DeleteBezierPoint_KeepHandles);
	layout->addRow(tr("Action on deleting a curve point with %1:").arg(ModifierKey::controlShift()), edit_tool_delete_bezier_point_action_alternative);
	
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(Util::Headline::create(tr("Rectangle tool:")));
	
	rectangle_helper_cross_radius = Util::SpinBox::create(0, 999999, tr("mm", "millimeters"));
	layout->addRow(tr("Radius of helper cross:"), rectangle_helper_cross_radius);
	
	rectangle_preview_line_width = new QCheckBox(tr("Preview the width of lines with helper cross"));
	layout->addRow(rectangle_preview_line_width);
	
	
	connect(antialiasing, &QAbstractButton::toggled, text_antialiasing, &QCheckBox::setEnabled);
	if (mobile_toolbar_button)
		connect(mobile_toolbar_button, &QPushButton::clicked, this, &EditorSettingsPage::configureMobileToolbars);
	
	updateWidgets();
}

EditorSettingsPage::~EditorSettingsPage()
{
	// nothing, not inlined
}

QString EditorSettingsPage::title() const
{
	return tr("Editor");
}

void EditorSettingsPage::apply()
{
	if (button_size != nullptr)
		setSetting(Settings::ActionGridBar_ButtonSizeMM, button_size->value());
	setSetting(Settings::SymbolWidget_IconSizeMM, icon_size->value());
	setSetting(Settings::MapDisplay_Antialiasing, antialiasing->isChecked());
	setSetting(Settings::MapDisplay_TextAntialiasing, text_antialiasing->isChecked());
	setSetting(Settings::MapEditor_ClickToleranceMM, tolerance->value());
	setSetting(Settings::MapEditor_SnapDistanceMM, snap_distance->value());
	setSetting(Settings::MapEditor_FixedAngleStepping, fixed_angle_stepping->value());
	setSetting(Settings::MapEditor_ChangeSymbolWhenSelecting, select_symbol_of_objects->isChecked());
	setSetting(Settings::MapEditor_ZoomOutAwayFromCursor, zoom_out_away_from_cursor->isChecked());
	setSetting(Settings::MapEditor_DrawLastPointOnRightClick, draw_last_point_on_right_click->isChecked());
	setSetting(Settings::Templates_KeepSettingsOfClosed, keep_settings_of_closed_templates->isChecked());
	setSetting(Settings::MapEditor_IgnoreTouchInput, ignore_touch_input->isChecked());
	setSetting(Settings::MobileToolbar_TopActions, mobile_top_toolbar_actions);
	setSetting(Settings::MobileToolbar_BottomActions, mobile_bottom_toolbar_actions);
	setSetting(Settings::EditTool_DeleteBezierPointAction, edit_tool_delete_bezier_point_action->currentData());
	setSetting(Settings::EditTool_DeleteBezierPointActionAlternative, edit_tool_delete_bezier_point_action_alternative->currentData());
	setSetting(Settings::RectangleTool_HelperCrossRadiusMM, rectangle_helper_cross_radius->value());
	setSetting(Settings::RectangleTool_PreviewLineWidth, rectangle_preview_line_width->isChecked());
}

void EditorSettingsPage::reset()
{
	updateWidgets();
}

void EditorSettingsPage::updateWidgets()
{
	if (button_size != nullptr)
		button_size->setValue(getSetting(Settings::ActionGridBar_ButtonSizeMM).toDouble());
	icon_size->setValue(getSetting(Settings::SymbolWidget_IconSizeMM).toInt());
	antialiasing->setChecked(getSetting(Settings::MapDisplay_Antialiasing).toBool());
	text_antialiasing->setEnabled(antialiasing->isChecked());
	text_antialiasing->setChecked(getSetting(Settings::MapDisplay_TextAntialiasing).toBool());
	tolerance->setValue(getSetting(Settings::MapEditor_ClickToleranceMM).toInt());
	snap_distance->setValue(getSetting(Settings::MapEditor_SnapDistanceMM).toInt());
	fixed_angle_stepping->setValue(getSetting(Settings::MapEditor_FixedAngleStepping).toInt());
	select_symbol_of_objects->setChecked(getSetting(Settings::MapEditor_ChangeSymbolWhenSelecting).toBool());
	zoom_out_away_from_cursor->setChecked(getSetting(Settings::MapEditor_ZoomOutAwayFromCursor).toBool());
	draw_last_point_on_right_click->setChecked(getSetting(Settings::MapEditor_DrawLastPointOnRightClick).toBool());
	keep_settings_of_closed_templates->setChecked(getSetting(Settings::Templates_KeepSettingsOfClosed).toBool());
	ignore_touch_input->setChecked(getSetting(Settings::MapEditor_IgnoreTouchInput).toBool());
	mobile_top_toolbar_actions = MapEditorController::sanitizeMobileToolbarActionIds(true, getSetting(Settings::MobileToolbar_TopActions).toStringList());
	mobile_bottom_toolbar_actions = MapEditorController::sanitizeMobileToolbarActionIds(false, getSetting(Settings::MobileToolbar_BottomActions).toStringList());
	
	edit_tool_delete_bezier_point_action->setCurrentIndex(edit_tool_delete_bezier_point_action->findData(getSetting(Settings::EditTool_DeleteBezierPointAction).toInt()));
	edit_tool_delete_bezier_point_action_alternative->setCurrentIndex(edit_tool_delete_bezier_point_action_alternative->findData(getSetting(Settings::EditTool_DeleteBezierPointActionAlternative).toInt()));
	
	rectangle_helper_cross_radius->setValue(getSetting(Settings::RectangleTool_HelperCrossRadiusMM).toInt());
	rectangle_preview_line_width->setChecked(getSetting(Settings::RectangleTool_PreviewLineWidth).toBool());
}

void EditorSettingsPage::configureMobileToolbars()
{
	MobileToolbarConfigDialog dialog(window());
	dialog.setSelectedActions(true, mobile_top_toolbar_actions);
	dialog.setSelectedActions(false, mobile_bottom_toolbar_actions);
	if (dialog.exec() != QDialog::Accepted)
		return;

	mobile_top_toolbar_actions = dialog.selectedActions(true);
	mobile_bottom_toolbar_actions = dialog.selectedActions(false);
}


}  // namespace OpenOrienteering
