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
		MapGeoreferencing_ControlScaleFactor,
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
	
	static Settings& getInstance();
	
	// Methods related to specific settings
	
	int getSymbolWidgetIconSizePx();
	qreal getMapEditorClickTolerancePx();
	qreal getMapEditorSnapDistancePx();
	qreal getRectangleToolHelperCrossRadiusPx();
	int getStartDragDistancePx();
	
	
#ifdef Q_OS_ANDROID
	constexpr bool touchModeEnabled() const noexcept { return true; }
	void setTouchModeEnabled(bool /* ignored */) {};
	constexpr static bool mobileModeEnforced() noexcept { return true; }
#else
	/**
	 * Returns true if the user wants to use a touch device.
	 * 
	 * Main window controllers may use this property to adjust their user
	 * interface, e.g. to select child widget types or to hide menubar and
	 * statusbar.
	 * 
	 * On PCs, the setting may be changed while multiple windows are opened.
	 * For consistent behaviour, main window controllers and widgets are
	 * expected to not adapt instantaneously to a change, but to capture the
	 * current setting at construction time, and to tear down accordingly on
	 * destruction.
	 * 
	 * On Android, or with enforced mobile mode, touch mode is always active
	 * (constexpr true) and cannot be disabled.
	 */
	bool touchModeEnabled() const noexcept { return touch_mode_enabled; }
	
	/**
	 * Enables or disables touch mode on PCs.
	 * 
	 * On Android, or with enforced mobile mode, this function does nothing.
	 */
	void setTouchModeEnabled(bool enabled);
	
	/**
	 * Returns true if the developer wants a PC user experience most closely to
	 * mobile devices.
	 * 
	 * This is intended as a utility for developers wanting to test or to debug
	 * Android features without taking the slow deployment path to a real device.
	 * 
	 * The property does not change during execution. On Android, it is constexpr
	 * true, giving the compile the chance for extra optimizations. On PCs, it
	 * is enabled by setting the environment variable MAPPER_MOBILE_GUI to a
	 * value different from '0'.
	 * 
	 * Controllers and widgets shall use this property to enable at run-time
	 * what is otherwise enabled by compile-time macros for Android.
	 * 
	 * Enabling this property enforces touch mode, too.
	 */
	static bool mobileModeEnforced() noexcept;
#endif
	
	
	/**
	 * Returns the name of the position source to be used.
	 * 
	 * If this is empty, the system's default source shall be used.
	 * 
	 * \see QGeoPositionInfoSource::createSource
	 * \see QGeoPositionInfoSource::createDefaultSource
	 */
	QString positionSource() const { return sensors.position_source; }
	
	/**
	 * Changes the name of the position source.
	 * 
	 * Setting this to an empty string selects the system's default source.
	 */
	void setPositionSource(const QString& name);
	
	
	/**
	 * Returns the name of the serial port for reading NMEA data.
	 * 
	 * If this is empty, port selection is left to Qt which either reads the
	 * port name from the environment variable QT_NMEA_SERIAL_PORT, or
	 * recognizes a few GPS chipsets by serial port vendor IDs.
	 * 
	 * \see qtlocation/src/plugins/position/serialnmea/qgeopositioninfosourcefactory_serialnmea.cpp
	 */
	QString nmeaSerialPort() const { return sensors.nmea_serialport; }
	
	/**
	 * Changes the name of the serial port for reading NMEA data.
	 * 
	 * Setting this to an empty string activates Qt's default port selection
	 * behaviour.
	 */
	void setNmeaSerialPort(const QString& name);
	
	
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
	
	struct {
		QString position_source = {};
		QString nmea_serialport = {};
	} sensors;
	
#ifndef Q_OS_ANDROID
	bool touch_mode_enabled = false;
#endif
};


}  // namespace OpenOrienteering

#endif
