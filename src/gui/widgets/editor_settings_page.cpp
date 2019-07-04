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
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QSpinBox>
#include <QVariant>
#include <QWidget>

#include "settings.h"
#include "gui/modifier_key.h"
#include "gui/util_gui.h"
#include "gui/widgets/settings_page.h"


namespace OpenOrienteering {

EditorSettingsPage::EditorSettingsPage(QWidget* parent)
 : SettingsPage(parent)
{
	auto layout = new QFormLayout(this);
	
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
	
	edit_tool_delete_bezier_point_action->setCurrentIndex(edit_tool_delete_bezier_point_action->findData(getSetting(Settings::EditTool_DeleteBezierPointAction).toInt()));
	edit_tool_delete_bezier_point_action_alternative->setCurrentIndex(edit_tool_delete_bezier_point_action_alternative->findData(getSetting(Settings::EditTool_DeleteBezierPointActionAlternative).toInt()));
	
	rectangle_helper_cross_radius->setValue(getSetting(Settings::RectangleTool_HelperCrossRadiusMM).toInt());
	rectangle_preview_line_width->setChecked(getSetting(Settings::RectangleTool_PreviewLineWidth).toBool());
}


}  // namespace OpenOrienteering
