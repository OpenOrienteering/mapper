/*
 *    Copyright 2012-2014 Thomas Schöps
 *    Copyright 2013-2015 Kai Pastor
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

#include <QApplication>
#include <QDebug>
#include <QLocale>
#include <QScreen>
#include <QSettings>
#include <QStringList>

#include "util.h"


Settings::Settings()
 : QObject()
{
	const float touch_button_minimum_size_default = 11;
	float symbol_widget_icon_size_mm_default;
	float map_editor_click_tolerance_default;
	float map_editor_snap_distance_default;
	int start_drag_distance_default;
	
	// Platform-specific settings defaults
	#if defined(ANDROID)
		symbol_widget_icon_size_mm_default = touch_button_minimum_size_default;
		map_editor_click_tolerance_default = 4.0f;
		map_editor_snap_distance_default = 15.0f;
		start_drag_distance_default = Util::mmToPixelLogical(3.0f);
	#else
		symbol_widget_icon_size_mm_default = 8;
		map_editor_click_tolerance_default = 3.0f;
		map_editor_snap_distance_default = 10.0f;
		start_drag_distance_default = QApplication::startDragDistance();
	#endif
	
	qreal ppi = QApplication::primaryScreen()->physicalDotsPerInch();
	// Beware of https://bugreports.qt-project.org/browse/QTBUG-35701
	if (ppi > 2048.0 || ppi < 16.0)
		ppi = QApplication::primaryScreen()->logicalDotsPerInch();
	
	registerSetting(MapDisplay_TextAntialiasing, "MapDisplay/text_antialiasing", false);
	registerSetting(MapEditor_ClickToleranceMM, "MapEditor/click_tolerance_mm", map_editor_click_tolerance_default);
	registerSetting(MapEditor_SnapDistanceMM, "MapEditor/snap_distance_mm", map_editor_snap_distance_default);
	registerSetting(MapEditor_FixedAngleStepping, "MapEditor/fixed_angle_stepping", 15);
	registerSetting(MapEditor_ChangeSymbolWhenSelecting, "MapEditor/change_symbol_when_selecting", true);
	registerSetting(MapEditor_ZoomOutAwayFromCursor, "MapEditor/zoom_out_away_from_cursor", true);
	registerSetting(MapEditor_DrawLastPointOnRightClick, "MapEditor/draw_last_point_on_right_click", true);
	
	registerSetting(EditTool_DeleteBezierPointAction, "EditTool/delete_bezier_point_action", (int)DeleteBezierPoint_RetainExistingShape);
	registerSetting(EditTool_DeleteBezierPointActionAlternative, "EditTool/delete_bezier_point_action_alternative", (int)DeleteBezierPoint_ResetHandles);
	
	registerSetting(RectangleTool_HelperCrossRadiusMM, "RectangleTool/helper_cross_radius_mm", 100.0f);
	registerSetting(RectangleTool_PreviewLineWidth, "RectangleTool/preview_line_with", true);
	
	registerSetting(Templates_KeepSettingsOfClosed, "Templates/keep_settings_of_closed_templates", true);
	
	registerSetting(ActionGridBar_ButtonSizeMM, "ActionGridBar/button_size_mm", touch_button_minimum_size_default);
	registerSetting(SymbolWidget_IconSizeMM, "SymbolWidget/icon_size_mm", symbol_widget_icon_size_mm_default);
	
	registerSetting(General_RetainCompatiblity, "retainCompatiblity", true);
	registerSetting(General_AutosaveInterval, "autosave", 15); // unit: minutes
	registerSetting(General_Language, "language", QVariant((int)QLocale::system().language()));
	registerSetting(General_PixelsPerInch, "pixelsPerInch", ppi);
	registerSetting(General_TranslationFile, "translationFile", QVariant(QString::null));
	registerSetting(General_RecentFilesList, "recentFileList", QVariant(QStringList()));
	registerSetting(General_OpenMRUFile, "openMRUFile", false);
	registerSetting(General_Local8BitEncoding, "local_8bit_encoding", "Windows-1252");
	registerSetting(General_NewOcd8Implementation, "new_ocd8_implementation_v0.6", true);
	registerSetting(General_StartDragDistance, "startDragDistance", start_drag_distance_default);
	
	registerSetting(HomeScreen_TipsVisible, "HomeScreen/tipsVisible", true);
	registerSetting(HomeScreen_CurrentTip, "HomeScreen/currentTip", -1);
	
	// Set antialiasing default depending on screen pixels per inch
	registerSetting(MapDisplay_Antialiasing, "MapDisplay/antialiasing", Util::isAntialiasingRequired(getSetting(General_PixelsPerInch).toReal()));
	
	// Migrate old settings
	static bool migration_checked = false;
	if (!migration_checked)
	{
		QVariant current_version { "0.5.9" };
		QSettings settings;
		if (settings.value("version") != current_version)
		{
			migrateSettings(settings, current_version);
		}
		migration_checked = true;
	}
}

void Settings::registerSetting(Settings::SettingsEnum id, const QString& path, const QVariant& default_value)
{
	setting_paths[id] = path;
	setting_defaults[id] = default_value;
}

void Settings::migrateSettings(QSettings& settings, QVariant version)
{
	migrateValue("General/language", General_Language, settings);
	if (migrateValue("MapEditor/click_tolerance", MapEditor_ClickToleranceMM, settings))
		settings.setValue(getSettingPath(MapEditor_ClickToleranceMM), Util::pixelToMMLogical(settings.value(getSettingPath(MapEditor_ClickToleranceMM)).toFloat()));
	if (migrateValue("MapEditor/snap_distance", MapEditor_SnapDistanceMM, settings))
		settings.setValue(getSettingPath(MapEditor_SnapDistanceMM), Util::pixelToMMLogical(settings.value(getSettingPath(MapEditor_SnapDistanceMM)).toFloat()));
	if (migrateValue("RectangleTool/helper_cross_radius", RectangleTool_HelperCrossRadiusMM, settings))
		settings.setValue(getSettingPath(RectangleTool_HelperCrossRadiusMM), Util::pixelToMMLogical(settings.value(getSettingPath(RectangleTool_HelperCrossRadiusMM)).toFloat()));
	
	if (!version.toByteArray().startsWith("0."))
	{
		// Future cleanup
		settings.remove("new_ocd8_implementation");
	}

	settings.setValue("version", version);
}

bool Settings::migrateValue(const QString& old_key, SettingsEnum new_setting, QSettings& settings) const
{
	bool value_migrated = false;
	if (settings.contains(old_key))
	{
		const QString new_key = getSettingPath(new_setting);
		Q_ASSERT_X(new_key != old_key, "Settings::migrateValue",
		  QString("New key \"%1\" equals old key").arg(new_key).toLocal8Bit().constData() );
		
		if (!settings.contains(new_key))
		{
			settings.setValue(new_key, settings.value(old_key));
			value_migrated = true;
		}
		settings.remove(old_key);
	}
	return value_migrated;
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

int Settings::getSymbolWidgetIconSizePx()
{
	return qRound(Util::mmToPixelLogical(getSettingCached(Settings::SymbolWidget_IconSizeMM).toFloat()));
}

float Settings::getMapEditorClickTolerancePx()
{
	return Util::mmToPixelLogical(getSettingCached(Settings::MapEditor_ClickToleranceMM).toFloat());
}

float Settings::getMapEditorSnapDistancePx()
{
	return Util::mmToPixelLogical(getSettingCached(Settings::MapEditor_SnapDistanceMM).toFloat());
}

float Settings::getRectangleToolHelperCrossRadiusPx()
{
	return Util::mmToPixelLogical(getSettingCached(Settings::RectangleTool_HelperCrossRadiusMM).toFloat());
}

int Settings::getStartDragDistancePx()
{
	return getSettingCached(Settings::General_StartDragDistance).toInt();
}
