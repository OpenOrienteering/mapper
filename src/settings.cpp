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

#include <assert.h>

#include <QVariant>
#include <QSettings>
#include <QLocale>

Settings::Settings(): QObject()
{
	registerSetting(MapDisplay_Antialiasing, "MapDisplay/antialiasing", true);
	registerSetting(MapEditor_ClickTolerance, "MapEditor/click_tolerance", 5);
	registerSetting(MapEditor_ChangeSymbolWhenSelecting, "MapEditor/change_symbol_when_selecting", false);
	registerSetting(MapEditor_ZoomOutAwayFromCursor, "MapEditor/zoom_out_away_from_cursor", true);
	
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

void Settings::applySettings()
{
	QSettings().sync();
	
	// Invalidate cache as settings could be changed
	settings_cache.clear();
	emit settingsChanged();
}
