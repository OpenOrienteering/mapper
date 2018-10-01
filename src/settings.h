/*
 *    Copyright 2012 Thomas Schöps
 *    Copyright 2013, 2014,2017 Thomas Schöps, Kai Pastor
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


#ifndef OPENORIENTEERING_SETTINGS_H
#define OPENORIENTEERING_SETTINGS_H

#include <QtGlobal>
#include <QHash>
#include <QObject>
#include <QString>
#include <QVariant>

class QSettings;


namespace OpenOrienteering {

/** Singleton which handles the global application settings.
 *  If you need to do any action when the application settings are changed, connect to the settingsChanged() signal.
 */
class Settings : public QObject
{
Q_OBJECT
public:
	/// List of all application settings to prevent having the QSettings path strings in multiple places
	enum SettingsEnum
	{
		MapDisplay_Antialiasing = 0,
		MapDisplay_TextAntialiasing,
		MapEditor_ClickToleranceMM,
		MapEditor_SnapDistanceMM,
		MapEditor_FixedAngleStepping,
		MapEditor_ChangeSymbolWhenSelecting,
		MapEditor_ZoomOutAwayFromCursor,
		MapEditor_DrawLastPointOnRightClick,
		EditTool_DeleteBezierPointAction,
		EditTool_DeleteBezierPointActionAlternative,
		RectangleTool_HelperCrossRadiusMM,
		RectangleTool_PreviewLineWidth,
		Templates_KeepSettingsOfClosed,
		SymbolWidget_IconSizeMM,
		SymbolWidget_ShowCustomIcons,
		ActionGridBar_ButtonSizeMM,
		General_RetainCompatiblity,
		General_SaveUndoRedo,
		General_AutosaveInterval,
		General_Language,
		General_PixelsPerInch,
		General_RecentFilesList,
		General_TranslationFile,
		General_OpenMRUFile,
		General_Local8BitEncoding,
		General_StartDragDistance,
		HomeScreen_TipsVisible,
		HomeScreen_CurrentTip,
		END_OF_SETTINGSENUM /* Don't add items below this line. */
	};
	
	enum DeleteBezierPointAction
	{
		DeleteBezierPoint_RetainExistingShape = 0,
		DeleteBezierPoint_ResetHandles,
		DeleteBezierPoint_KeepHandles
	};
	
	/// Retrieve a setting from QSettings without caching
	QVariant getSetting(SettingsEnum setting) const;
	
	/// Can be used to cache and retrieve settings which are only changed in the settings dialog (i.e. applySettings() must be called after they are changed)
	QVariant getSettingCached(SettingsEnum setting);
	
	/// Change a setting, but only in the cache. Do not use this if in doubt.
	void setSettingInCache(Settings::SettingsEnum setting, const QVariant& value);
	
	/// This must be called after cached settings have been changed and on application startup.
	void applySettings();
	
	/// Change a setting immediately.
	void setSetting(Settings::SettingsEnum setting, const QVariant& value);
	
	/// Removes a setting immediately. Next reading will return the default value.
	void remove(Settings::SettingsEnum setting);
	
	// TODO: Methods for settings import / export?
	
	/// Returns the path to use with QSettings
	inline QString getSettingPath(SettingsEnum setting) const { return setting_paths[setting]; }
	
	/// Returns the default value for the setting
	inline QVariant getDefaultValue(SettingsEnum setting) const { return setting_defaults[setting]; }
	
	static Settings& getInstance()
	{
		static Settings instance;
		return instance;
	}
	
	// Methods related to specific settings
	
	int getSymbolWidgetIconSizePx();
	qreal getMapEditorClickTolerancePx();
	qreal getMapEditorSnapDistancePx();
	qreal getRectangleToolHelperCrossRadiusPx();
	int getStartDragDistancePx();
	
signals:
	void settingsChanged();
	
private:
	Settings();
	void registerSetting(SettingsEnum id, const char* path_latin1, const QVariant& default_value);
	
	void migrateSettings(QSettings& settings, const QVariant& version);
	
	/** Migrates a value from an old key to a new key.
	 *  Uses the given or a newly constructed QSettings object.
	 *  Returns true if the value was migrated. */
	bool migrateValue(const char* old_key_latin1, SettingsEnum new_setting, QSettings& settings) const;
	
	QHash<SettingsEnum, QVariant> settings_cache;
	QHash<SettingsEnum, QString> setting_paths;
	QHash<SettingsEnum, QVariant> setting_defaults;
};


}  // namespace OpenOrienteering

#endif
