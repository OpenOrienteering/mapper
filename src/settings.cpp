/*
 *    Copyright 2012, 2013 Thomas Schöps
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

#include <QLocale>
#include <QVariant>
#include <QSettings>
#include <QStringList>

Settings::Settings()
 : QObject()
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
	
	registerSetting(General_Language, "language", QVariant((int)QLocale::system().language()));
	registerSetting(General_TranslationFile, "translationFile", QVariant(QString::null));
	registerSetting(General_RecentFilesList, "recentFileList", QVariant(QStringList()));
	registerSetting(General_OpenMRUFile, "openMRUFile", false);
	
	registerSetting(HomeScreen_TipsVisible, "HomeScreen/tipsVisible", true);
	registerSetting(HomeScreen_CurrentTip, "HomeScreen/currentTip", -1);
	
	// Migrate old settings
	static QVariant current_version("0.5");
	QSettings settings;
	if (settings.value("version") != current_version)
	{
		if (!settings.contains("version"))
		{
			// pre-0.5
			QSettings old_settings("Thomas Schoeps", "OpenOrienteering");
			old_settings.setFallbacksEnabled(false);
			Q_FOREACH(QString key, old_settings.allKeys())
				settings.setValue(key, old_settings.value(key));
		}
		migrateValue("General/language", General_Language, settings);
		settings.setValue("version", current_version);
	}
}

void Settings::registerSetting(Settings::SettingsEnum id, const QString& path, const QVariant& default_value)
{
	setting_paths[id] = path;
	setting_defaults[id] = default_value;
}

void Settings::migrateValue(const QString& old_key, SettingsEnum new_setting, QSettings& settings) const
{
	if (settings.contains(old_key))
	{
		const QString new_key = getSettingPath(new_setting);
		Q_ASSERT_X(new_key != old_key, "Settings::migrateValue",
		  QString("New key \"%1\" equals old key").arg(new_key).toLocal8Bit().constData() );
		
		if (!settings.contains(new_key))
		{
			settings.setValue(new_key, settings.value(old_key));
		}
		settings.remove(old_key);
	}
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

void Settings::setSetting(Settings::SettingsEnum setting, QVariant value)
{
	bool setting_changed = true;
	if (settings_cache.contains(setting))
		setting_changed = settings_cache.value(setting) != value;
	else
		setting_changed = QSettings().value(getSettingPath(setting), getDefaultValue(setting)) != value;
	
	if (setting_changed)
	{
		QSettings().setValue(getSettingPath(setting), value);
		settings_cache.clear();
		emit settingsChanged();
	}
}

void Settings::remove(Settings::SettingsEnum setting)
{
	QVariant value = getDefaultValue(setting);
	bool setting_changed = true;
	if (settings_cache.contains(setting))
		setting_changed = settings_cache.value(setting) != value;
	else
		setting_changed = QSettings().value(getSettingPath(setting), value) != value;
	
	QSettings().remove(getSettingPath(setting));
	if (setting_changed)
	{
		settings_cache.clear();
		emit settingsChanged();
	}
}

void Settings::applySettings()
{
	QSettings().sync();
	
	// Invalidate cache as settings could be changed
	settings_cache.clear();
	emit settingsChanged();
}
