/*
 *    Copyright 2012 Thomas Schöps
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


#include "settings.h"

#include <cassert>

#include <QVariant>
#include <QSettings>
#include <QLocale>

Settings::Settings(): QObject()
{
	registerSetting(MapDisplay_Antialiasing, "MapDisplay/antialiasing", true);
	registerSetting(MapDisplay_TextAntialiasing, "MapDisplay/text_antialiasing", false);
	registerSetting(MapEditor_ClickTolerance, "MapEditor/click_tolerance", 5);
	registerSetting(MapEditor_SnapDistance, "MapEditor/snap_distance", 20);
	registerSetting(MapEditor_FixedAngleStepping, "MapEditor/fixed_angle_stepping", 15);
	registerSetting(MapEditor_ChangeSymbolWhenSelecting, "MapEditor/change_symbol_when_selecting", true);
	registerSetting(MapEditor_ZoomOutAwayFromCursor, "MapEditor/zoom_out_away_from_cursor", true);
	registerSetting(MapEditor_DrawLastPointOnRightClick, "MapEditor/draw_last_point_on_right_click", true);
	
	registerSetting(EditTool_DeleteBezierPointAction, "EditTool/delete_bezier_point_action", (int)DeleteBezierPoint_RetainExistingShape);
	registerSetting(EditTool_DeleteBezierPointActionAlternative, "EditTool/delete_bezier_point_action_alternative", (int)DeleteBezierPoint_ResetHandles);
	
	registerSetting(RectangleTool_HelperCrossRadius, "RectangleTool/helper_cross_radius", 300);
	registerSetting(RectangleTool_PreviewLineWidth, "RectangleTool/preview_line_with", true);
	
	registerSetting(Templates_KeepSettingsOfClosed, "Templates/keep_settings_of_closed_templates", true);
	
	registerSetting(General_Language, "General/language", QVariant((int)QLocale::system().language()));
}
void Settings::registerSetting(Settings::SettingsEnum id, const QString& path, const QVariant& default_value)
{
	setting_paths[id] = path;
	setting_defaults[id] = default_value;
}

QVariant Settings::getSetting(Settings::SettingsEnum setting) const
{
	QSettings settings;
	return settings.value(getSettingPath(setting), getDefaultValue(setting));
}
QVariant Settings::getSettingCached(Settings::SettingsEnum setting)
{
	if (settings_cache.contains(setting))
		return settings_cache.value(setting);
	
	QSettings settings;
	QVariant value = settings.value(getSettingPath(setting), getDefaultValue(setting));
	settings_cache.insert(setting, value);
	return value;
}

void Settings::setSettingInCache(Settings::SettingsEnum setting, QVariant value)
{
	settings_cache.insert(setting, value);
}

void Settings::applySettings()
{
	QSettings().sync();
	
	// Invalidate cache as settings could be changed
	settings_cache.clear();
	emit settingsChanged();
}
