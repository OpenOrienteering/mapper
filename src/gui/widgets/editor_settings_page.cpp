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
#include <QGridLayout>
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
	, available_list(new QListWidget(this))
	, top_left_list(new QListWidget(this))
	, top_right_list(new QListWidget(this))
	, bottom_left_list(new QListWidget(this))
	, bottom_right_list(new QListWidget(this))
	, move_top_left_button(new QPushButton(this))
	, move_top_right_button(new QPushButton(this))
	, move_bottom_left_button(new QPushButton(this))
	, move_bottom_right_button(new QPushButton(this))
	, remove_button(new QPushButton(this))
	, up_button(new QPushButton(this))
	, down_button(new QPushButton(this))
	{
		setWindowTitle(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Mobile toolbar icons"));

		auto* layout = new QVBoxLayout(this);

		auto* note = new QLabel(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage",
			"Move actions between the four toolbar areas. Save, Close, and Overflow cannot be removed. Overflow stays visible even when the toolbar becomes crowded."));
		note->setWordWrap(true);
		layout->addWidget(note);

		auto* lists_layout = new QHBoxLayout();
		layout->addLayout(lists_layout, 1);

		auto* available_layout = new QVBoxLayout();
		available_layout->addWidget(new QLabel(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Available actions")));
		available_layout->addWidget(available_list, 1);
		lists_layout->addLayout(available_layout, 1);

		auto* buttons_layout = new QVBoxLayout();
		move_top_left_button->setText(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "To top left"));
		move_top_right_button->setText(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "To top right"));
		move_bottom_left_button->setText(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "To bottom left"));
		move_bottom_right_button->setText(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "To bottom right"));
		remove_button->setText(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Remove"));
		up_button->setText(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Up"));
		down_button->setText(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Down"));
		auto* reset_button = new QPushButton(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Reset"), this);
		buttons_layout->addStretch(1);
		buttons_layout->addWidget(move_top_left_button);
		buttons_layout->addWidget(move_top_right_button);
		buttons_layout->addWidget(move_bottom_left_button);
		buttons_layout->addWidget(move_bottom_right_button);
		buttons_layout->addSpacing(12);
		buttons_layout->addWidget(remove_button);
		buttons_layout->addSpacing(12);
		buttons_layout->addWidget(this->up_button);
		buttons_layout->addWidget(this->down_button);
		buttons_layout->addSpacing(12);
		buttons_layout->addWidget(reset_button);
		buttons_layout->addStretch(1);
		lists_layout->addLayout(buttons_layout);

		auto* zones_layout = new QGridLayout();
		zones_layout->addWidget(new QLabel(MapEditorController::mobileToolbarZoneLabel(MapEditorController::MobileToolbarTopLeft)), 0, 0);
		zones_layout->addWidget(new QLabel(MapEditorController::mobileToolbarZoneLabel(MapEditorController::MobileToolbarTopRight)), 0, 1);
		zones_layout->addWidget(top_left_list, 1, 0);
		zones_layout->addWidget(top_right_list, 1, 1);
		zones_layout->addWidget(new QLabel(MapEditorController::mobileToolbarZoneLabel(MapEditorController::MobileToolbarBottomLeft)), 2, 0);
		zones_layout->addWidget(new QLabel(MapEditorController::mobileToolbarZoneLabel(MapEditorController::MobileToolbarBottomRight)), 2, 1);
		zones_layout->addWidget(bottom_left_list, 3, 0);
		zones_layout->addWidget(bottom_right_list, 3, 1);
		lists_layout->addLayout(zones_layout, 2);

		auto* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
		layout->addWidget(button_box);

		for (auto* list : { available_list, top_left_list, top_right_list, bottom_left_list, bottom_right_list })
			connect(list, &QListWidget::itemSelectionChanged, this, [this, list]() { activateList(list); });
		connect(move_top_left_button, &QPushButton::clicked, this, [this]() {
			moveSelectedAction(MapEditorController::MobileToolbarTopLeft);
		});
		connect(move_top_right_button, &QPushButton::clicked, this, [this]() {
			moveSelectedAction(MapEditorController::MobileToolbarTopRight);
		});
		connect(move_bottom_left_button, &QPushButton::clicked, this, [this]() {
			moveSelectedAction(MapEditorController::MobileToolbarBottomLeft);
		});
		connect(move_bottom_right_button, &QPushButton::clicked, this, [this]() {
			moveSelectedAction(MapEditorController::MobileToolbarBottomRight);
		});
		connect(remove_button, &QPushButton::clicked, this, &MobileToolbarConfigDialog::removeSelectedAction);
		connect(up_button, &QPushButton::clicked, this, [this]() {
			moveSelectedAction(-1);
		});
		connect(this->down_button, &QPushButton::clicked, this, [this]() {
			moveSelectedAction(1);
		});
		connect(reset_button, &QPushButton::clicked, this, [this]() {
			top_left_actions = MapEditorController::defaultMobileToolbarActionIds(MapEditorController::MobileToolbarTopLeft);
			top_right_actions = MapEditorController::defaultMobileToolbarActionIds(MapEditorController::MobileToolbarTopRight);
			bottom_left_actions = MapEditorController::defaultMobileToolbarActionIds(MapEditorController::MobileToolbarBottomLeft);
			bottom_right_actions = MapEditorController::defaultMobileToolbarActionIds(MapEditorController::MobileToolbarBottomRight);
			refillLists(QString{});
		});
		for (auto zone : { MapEditorController::MobileToolbarTopLeft, MapEditorController::MobileToolbarTopRight, MapEditorController::MobileToolbarBottomLeft, MapEditorController::MobileToolbarBottomRight })
			connect(listForZone(zone), &QListWidget::itemDoubleClicked, this, [this]() { removeSelectedAction(); });
		connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
		connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
		updateButtons();
	}

	void setSelectedActions(
	        const QStringList& new_top_left_actions,
	        const QStringList& new_top_right_actions,
	        const QStringList& new_bottom_left_actions,
	        const QStringList& new_bottom_right_actions
	)
	{
		top_left_actions = new_top_left_actions;
		top_right_actions = new_top_right_actions;
		bottom_left_actions = new_bottom_left_actions;
		bottom_right_actions = new_bottom_right_actions;
		MapEditorController::sanitizeMobileToolbarConfiguration(top_left_actions, top_right_actions, bottom_left_actions, bottom_right_actions);
		refillLists(QString{});
	}

	QStringList selectedActions(MapEditorController::MobileToolbarZone zone) const
	{
		switch (zone)
		{
		case MapEditorController::MobileToolbarTopLeft:
			return top_left_actions;
		case MapEditorController::MobileToolbarTopRight:
			return top_right_actions;
		case MapEditorController::MobileToolbarBottomLeft:
			return bottom_left_actions;
		case MapEditorController::MobileToolbarBottomRight:
			return bottom_right_actions;
		}
		Q_UNREACHABLE();
		return {};
	}

private:
	QStringList& actionsForZone(MapEditorController::MobileToolbarZone zone)
	{
		switch (zone)
		{
		case MapEditorController::MobileToolbarTopLeft:
			return top_left_actions;
		case MapEditorController::MobileToolbarTopRight:
			return top_right_actions;
		case MapEditorController::MobileToolbarBottomLeft:
			return bottom_left_actions;
		case MapEditorController::MobileToolbarBottomRight:
			return bottom_right_actions;
		}
		Q_UNREACHABLE();
		return top_left_actions;
	}

	QListWidget* listForZone(MapEditorController::MobileToolbarZone zone) const
	{
		switch (zone)
		{
		case MapEditorController::MobileToolbarTopLeft:
			return top_left_list;
		case MapEditorController::MobileToolbarTopRight:
			return top_right_list;
		case MapEditorController::MobileToolbarBottomLeft:
			return bottom_left_list;
		case MapEditorController::MobileToolbarBottomRight:
			return bottom_right_list;
		}
		Q_UNREACHABLE();
		return top_left_list;
	}

	static QString displayLabel(const QString& id)
	{
		auto label = MapEditorController::mobileToolbarActionLabel(id);
		if (!MapEditorController::mobileToolbarActionRemovable(id))
			label += QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", " (required)");
		return label;
	}

	static QListWidgetItem* makeItem(const QString& id)
	{
		auto* item = new QListWidgetItem(displayLabel(id));
		item->setData(Qt::UserRole, id);
		return item;
	}

	void activateList(QListWidget* active_list)
	{
		if (selection_update_in_progress)
			return;
		if (!active_list->currentItem())
		{
			updateButtons();
			return;
		}

		selection_update_in_progress = true;
		for (auto* list : { available_list, top_left_list, top_right_list, bottom_left_list, bottom_right_list })
		{
			if (list != active_list)
				list->clearSelection();
		}
		selection_update_in_progress = false;
		updateButtons();
	}

	void refillLists(const QString& selected_id)
	{
		available_list->clear();
		top_left_list->clear();
		top_right_list->clear();
		bottom_left_list->clear();
		bottom_right_list->clear();

		for (const auto& id : top_left_actions)
			top_left_list->addItem(makeItem(id));
		for (const auto& id : top_right_actions)
			top_right_list->addItem(makeItem(id));
		for (const auto& id : bottom_left_actions)
			bottom_left_list->addItem(makeItem(id));
		for (const auto& id : bottom_right_actions)
			bottom_right_list->addItem(makeItem(id));

		auto const configured_actions = top_left_actions + top_right_actions + bottom_left_actions + bottom_right_actions;
		for (const auto& id : MapEditorController::editableMobileToolbarActionIds())
		{
			if (!configured_actions.contains(id))
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
		select_item(top_left_list);
		select_item(top_right_list);
		select_item(bottom_left_list);
		select_item(bottom_right_list);
		select_item(available_list);
		updateButtons();
	}

	bool selectedAction(QString& id, MapEditorController::MobileToolbarZone* zone = nullptr) const
	{
		for (auto candidate_zone : { MapEditorController::MobileToolbarTopLeft, MapEditorController::MobileToolbarTopRight, MapEditorController::MobileToolbarBottomLeft, MapEditorController::MobileToolbarBottomRight })
		{
			if (auto* item = listForZone(candidate_zone)->currentItem())
			{
				id = item->data(Qt::UserRole).toString();
				if (zone)
					*zone = candidate_zone;
				return true;
			}
		}
		if (auto* item = available_list->currentItem())
		{
			id = item->data(Qt::UserRole).toString();
			return true;
		}
		return false;
	}

	void moveSelectedAction(MapEditorController::MobileToolbarZone target_zone)
	{
		QString id;
		MapEditorController::MobileToolbarZone source_zone = MapEditorController::MobileToolbarTopLeft;
		auto const in_zone = selectedAction(id, &source_zone);
		if (!in_zone)
			return;

		if (!available_list->currentItem() && source_zone == target_zone)
			return;

		if (!available_list->currentItem())
			actionsForZone(source_zone).removeAll(id);
		auto& target_actions = actionsForZone(target_zone);
		if (!target_actions.contains(id))
			target_actions << id;
		refillLists(id);
	}

	void removeSelectedAction()
	{
		QString id;
		MapEditorController::MobileToolbarZone source_zone = MapEditorController::MobileToolbarTopLeft;
		if (!selectedAction(id, &source_zone) || available_list->currentItem())
			return;
		if (!MapEditorController::mobileToolbarActionRemovable(id))
			return;

		actionsForZone(source_zone).removeAll(id);
		refillLists(QString{});
	}

	void moveSelectedAction(int delta)
	{
		QString id;
		MapEditorController::MobileToolbarZone source_zone = MapEditorController::MobileToolbarTopLeft;
		if (!selectedAction(id, &source_zone) || available_list->currentItem())
			return;

		auto& actions = actionsForZone(source_zone);
		auto const index = actions.indexOf(id);
		auto const new_index = index + delta;
		if (index < 0 || new_index < 0 || new_index >= actions.size())
			return;

		actions.move(index, new_index);
		refillLists(id);
	}

	void updateButtons()
	{
		QString id;
		MapEditorController::MobileToolbarZone source_zone = MapEditorController::MobileToolbarTopLeft;
		auto const has_selection = selectedAction(id, &source_zone);
		auto const selected_in_available = (available_list->currentItem() != nullptr);
		auto const selected_in_zone = has_selection && !selected_in_available;

		move_top_left_button->setEnabled(has_selection);
		move_top_right_button->setEnabled(has_selection);
		move_bottom_left_button->setEnabled(has_selection);
		move_bottom_right_button->setEnabled(has_selection);
		remove_button->setEnabled(selected_in_zone && MapEditorController::mobileToolbarActionRemovable(id));
		if (selected_in_zone)
		{
			auto const& actions = selectedActions(source_zone);
			auto const index = actions.indexOf(id);
			up_button->setEnabled(index > 0);
			down_button->setEnabled(index >= 0 && index + 1 < actions.size());
		}
		else
		{
			up_button->setEnabled(false);
			down_button->setEnabled(false);
		}
	}

	QListWidget* available_list;
	QListWidget* top_left_list;
	QListWidget* top_right_list;
	QListWidget* bottom_left_list;
	QListWidget* bottom_right_list;
	QPushButton* move_top_left_button;
	QPushButton* move_top_right_button;
	QPushButton* move_bottom_left_button;
	QPushButton* move_bottom_right_button;
	QPushButton* remove_button;
	QPushButton* up_button;
	QPushButton* down_button;
	QStringList top_left_actions = Settings::defaultMobileTopLeftToolbarActions();
	QStringList top_right_actions = Settings::defaultMobileTopRightToolbarActions();
	QStringList bottom_left_actions = Settings::defaultMobileBottomLeftToolbarActions();
	QStringList bottom_right_actions = Settings::defaultMobileBottomRightToolbarActions();
	bool selection_update_in_progress = false;
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
	setSetting(Settings::MobileToolbar_TopLeftActions, mobile_top_left_toolbar_actions);
	setSetting(Settings::MobileToolbar_TopRightActions, mobile_top_right_toolbar_actions);
	setSetting(Settings::MobileToolbar_BottomLeftActions, mobile_bottom_left_toolbar_actions);
	setSetting(Settings::MobileToolbar_BottomRightActions, mobile_bottom_right_toolbar_actions);
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
	mobile_top_left_toolbar_actions = getSetting(Settings::MobileToolbar_TopLeftActions).toStringList();
	mobile_top_right_toolbar_actions = getSetting(Settings::MobileToolbar_TopRightActions).toStringList();
	mobile_bottom_left_toolbar_actions = getSetting(Settings::MobileToolbar_BottomLeftActions).toStringList();
	mobile_bottom_right_toolbar_actions = getSetting(Settings::MobileToolbar_BottomRightActions).toStringList();
	MapEditorController::sanitizeMobileToolbarConfiguration(
	        mobile_top_left_toolbar_actions,
	        mobile_top_right_toolbar_actions,
	        mobile_bottom_left_toolbar_actions,
	        mobile_bottom_right_toolbar_actions
	);
	
	edit_tool_delete_bezier_point_action->setCurrentIndex(edit_tool_delete_bezier_point_action->findData(getSetting(Settings::EditTool_DeleteBezierPointAction).toInt()));
	edit_tool_delete_bezier_point_action_alternative->setCurrentIndex(edit_tool_delete_bezier_point_action_alternative->findData(getSetting(Settings::EditTool_DeleteBezierPointActionAlternative).toInt()));
	
	rectangle_helper_cross_radius->setValue(getSetting(Settings::RectangleTool_HelperCrossRadiusMM).toInt());
	rectangle_preview_line_width->setChecked(getSetting(Settings::RectangleTool_PreviewLineWidth).toBool());
}

void EditorSettingsPage::configureMobileToolbars()
{
	MobileToolbarConfigDialog dialog(window());
	dialog.setSelectedActions(
	        mobile_top_left_toolbar_actions,
	        mobile_top_right_toolbar_actions,
	        mobile_bottom_left_toolbar_actions,
	        mobile_bottom_right_toolbar_actions
	);
	if (dialog.exec() != QDialog::Accepted)
		return;

	mobile_top_left_toolbar_actions = dialog.selectedActions(MapEditorController::MobileToolbarTopLeft);
	mobile_top_right_toolbar_actions = dialog.selectedActions(MapEditorController::MobileToolbarTopRight);
	mobile_bottom_left_toolbar_actions = dialog.selectedActions(MapEditorController::MobileToolbarBottomLeft);
	mobile_bottom_right_toolbar_actions = dialog.selectedActions(MapEditorController::MobileToolbarBottomRight);
}


}  // namespace OpenOrienteering
