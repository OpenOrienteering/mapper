/*
 *    Copyright 2012-2014 Thomas Schöps
 *    Copyright 2013-2017 Kai Pastor
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

#include <initializer_list>

#include <QtGlobal>
#if defined(QT_WIDGETS_LIB)
#include <QApplication>
#endif
#include <QByteArray>
#include <QColor>
#include <QGuiApplication>
#include <QLatin1Char>
#include <QLatin1String>
#include <QLocale>
#include <QRgb>
#include <QScreen>
#include <QSettings>
#include <QStringList>
#include <QStringRef>
#include <QVector>


namespace OpenOrienteering {

/*
 * Implementation for some functions which need a Settings instance.
 * 
 * Settings has Qt dependencies only. The Util function are declared in
 * gui/util_gui.h. By moving the definitions here, we facilitate the
 * compilation of unit tests which depend on Settings.
 */
namespace Util
{

	qreal mmToPixelPhysical(qreal millimeters)
	{
		auto ppi = Settings::getInstance().getSettingCached(Settings::General_PixelsPerInch).toReal();
		return millimeters * ppi / 25.4;
	}
	
	qreal pixelToMMPhysical(qreal pixels)
	{
		auto ppi = Settings::getInstance().getSettingCached(Settings::General_PixelsPerInch).toReal();
		return pixels * 25.4 / ppi;
	}

	
	qreal mmToPixelLogical(qreal millimeters)
	{
		auto ppi = QGuiApplication::primaryScreen()->logicalDotsPerInch();
		return millimeters * ppi / qreal(25.4);
	}
	
	qreal pixelToMMLogical(qreal pixels)
	{
		auto ppi = QGuiApplication::primaryScreen()->logicalDotsPerInch();
		return pixels * qreal(25.4) / ppi;
	}
	
	
	bool isAntialiasingRequired(qreal ppi)
	{
		return ppi < 200;
	}
	
	bool isAntialiasingRequired()
	{
		return isAntialiasingRequired(Settings::getInstance().getSettingCached(Settings::General_PixelsPerInch).toReal());
	}
	
}



Settings& Settings::getInstance()
{
	static Settings instance;
	return instance;
}

QStringList Settings::defaultMobileTopToolbarActions()
{
	return {
		QStringLiteral("compassdisplay"),
		QStringLiteral("gpsdisplay"),
		QStringLiteral("gpsdistancerings"),
		QStringLiteral("alignmapwithnorth"),
		QStringLiteral("showgrid"),
		QStringLiteral("showall"),
		QStringLiteral("redo"),
		QStringLiteral("undo"),
		QStringLiteral("touchcursor"),
		QStringLiteral("templatewindow"),
		QStringLiteral("editobjects"),
		QStringLiteral("editlines"),
		QStringLiteral("delete"),
		QStringLiteral("duplicate"),
		QStringLiteral("switchsymbol"),
		QStringLiteral("fillborder"),
		QStringLiteral("switchdashes"),
		QStringLiteral("booleanunion"),
		QStringLiteral("cutobject"),
		QStringLiteral("connectpaths"),
		QStringLiteral("rotateobjects"),
		QStringLiteral("cuthole"),
		QStringLiteral("scaleobjects"),
		QStringLiteral("rotatepatterns"),
		QStringLiteral("converttocurves"),
		QStringLiteral("simplify"),
		QStringLiteral("distributepoints"),
		QStringLiteral("booleandifference"),
		QStringLiteral("measure"),
		QStringLiteral("booleanmergeholes"),
	};
}

QStringList Settings::defaultMobileBottomToolbarActions()
{
	return {
		QStringLiteral("zoomin"),
		QStringLiteral("panmap"),
		QStringLiteral("zoomout"),
		QStringLiteral("movegps"),
		QStringLiteral("hatchareasview"),
		QStringLiteral("baselineview"),
		QStringLiteral("gpstemporarypath"),
		QStringLiteral("gpstemporarypoint"),
		QStringLiteral("drawpoint"),
		QStringLiteral("drawpointgps"),
		QStringLiteral("drawpath"),
		QStringLiteral("drawfreehand"),
		QStringLiteral("drawrectangle"),
		QStringLiteral("drawcircle"),
		QStringLiteral("drawtext"),
	};
}

QStringList Settings::defaultMobileTopLeftToolbarActions()
{
	return {
		QStringLiteral("hidetopbar"),
		QStringLiteral("save"),
		QStringLiteral("compassdisplay"),
		QStringLiteral("gpsdisplay"),
		QStringLiteral("gpsdistancerings"),
		QStringLiteral("alignmapwithnorth"),
		QStringLiteral("showgrid"),
		QStringLiteral("showall"),
	};
}

QStringList Settings::defaultMobileTopRightToolbarActions()
{
	return {
		QStringLiteral("close"),
		QStringLiteral("overflow"),
		QStringLiteral("redo"),
		QStringLiteral("undo"),
		QStringLiteral("touchcursor"),
		QStringLiteral("templatewindow"),
		QStringLiteral("editobjects"),
		QStringLiteral("editlines"),
		QStringLiteral("delete"),
		QStringLiteral("duplicate"),
		QStringLiteral("switchsymbol"),
		QStringLiteral("fillborder"),
		QStringLiteral("switchdashes"),
		QStringLiteral("booleanunion"),
		QStringLiteral("cutobject"),
		QStringLiteral("connectpaths"),
		QStringLiteral("rotateobjects"),
		QStringLiteral("cuthole"),
		QStringLiteral("scaleobjects"),
		QStringLiteral("rotatepatterns"),
		QStringLiteral("converttocurves"),
		QStringLiteral("simplify"),
		QStringLiteral("distributepoints"),
		QStringLiteral("booleandifference"),
		QStringLiteral("measure"),
		QStringLiteral("booleanmergeholes"),
		QStringLiteral("mapparts"),
	};
}

QStringList Settings::defaultMobileBottomLeftToolbarActions()
{
	return {
		QStringLiteral("zoomin"),
		QStringLiteral("panmap"),
		QStringLiteral("zoomout"),
		QStringLiteral("movegps"),
		QStringLiteral("hatchareasview"),
		QStringLiteral("baselineview"),
		QStringLiteral("gpstemporarypath"),
		QStringLiteral("gpstemporarypoint"),
		QStringLiteral("paint"),
	};
}

QStringList Settings::defaultMobileBottomRightToolbarActions()
{
	return {
		QStringLiteral("symbolselector"),
		QStringLiteral("drawpoint"),
		QStringLiteral("drawpointgps"),
		QStringLiteral("drawpath"),
		QStringLiteral("drawfreehand"),
		QStringLiteral("drawrectangle"),
		QStringLiteral("drawcircle"),
		QStringLiteral("drawtext"),
	};
}

QStringList Settings::defaultPieMenuActions()
{
	return {
		QStringLiteral("editobjects"),
		QStringLiteral("drawpoint"),
		QStringLiteral("drawpath"),
		QStringLiteral("drawrectangle"),
		QStringLiteral("cutobject"),
		QStringLiteral("cuthole"),
		QStringLiteral("switchdashes"),
		QStringLiteral("connectpaths"),
	};
}


Settings::Settings()
 : QObject()
{
	const float touch_button_minimum_size_default = 6.5;
	float symbol_widget_icon_size_mm_default;
	float map_editor_click_tolerance_default;
	float map_editor_snap_distance_default;
	int start_drag_distance_default;
	
	// Platform-specific settings defaults
#if defined(ANDROID) || !defined(QT_WIDGETS_LIB)
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
	
	qreal ppi = QGuiApplication::primaryScreen()->physicalDotsPerInch();
	// Beware of https://bugreports.qt-project.org/browse/QTBUG-35701
	if (ppi > 2048.0 || ppi < 16.0)
		ppi = QGuiApplication::primaryScreen()->logicalDotsPerInch();
	
	registerSetting(MapDisplay_TextAntialiasing, "MapDisplay/text_antialiasing", false);
	registerSetting(MapEditor_ClickToleranceMM, "MapEditor/click_tolerance_mm", map_editor_click_tolerance_default);
	registerSetting(MapEditor_SnapDistanceMM, "MapEditor/snap_distance_mm", map_editor_snap_distance_default);
	registerSetting(MapEditor_FixedAngleStepping, "MapEditor/fixed_angle_stepping", 15);
	registerSetting(MapEditor_ChangeSymbolWhenSelecting, "MapEditor/change_symbol_when_selecting", true);
	registerSetting(MapEditor_ZoomOutAwayFromCursor, "MapEditor/zoom_out_away_from_cursor", true);
	registerSetting(MapEditor_DrawLastPointOnRightClick, "MapEditor/draw_last_point_on_right_click", true);
	registerSetting(MapEditor_IgnoreTouchInput, "MapEditor/ignore_touch_input", false);
	registerSetting(MapGeoreferencing_ControlScaleFactor, "MapGeoreferencing/control_scale_factor", false);
	
	registerSetting(EditTool_DeleteBezierPointAction, "EditTool/delete_bezier_point_action", int(DeleteBezierPoint_RetainExistingShape));
	registerSetting(EditTool_DeleteBezierPointActionAlternative, "EditTool/delete_bezier_point_action_alternative", int(DeleteBezierPoint_ResetHandles));
	
	registerSetting(RectangleTool_HelperCrossRadiusMM, "RectangleTool/helper_cross_radius_mm", 100.0f);
	registerSetting(RectangleTool_PreviewLineWidth, "RectangleTool/preview_line_with", true);
	
	registerSetting(Templates_KeepSettingsOfClosed, "Templates/keep_settings_of_closed_templates", true);
	
	registerSetting(ActionGridBar_ButtonSizeMM, "ActionGridBar/button_size_mm", touch_button_minimum_size_default);
	registerSetting(MobileToolbar_TopActions, "ActionGridBar/mobile_top_actions", defaultMobileTopToolbarActions());
	registerSetting(MobileToolbar_BottomActions, "ActionGridBar/mobile_bottom_actions", defaultMobileBottomToolbarActions());
	registerSetting(MobileToolbar_TopLeftActions, "ActionGridBar/mobile_top_left_actions", defaultMobileTopLeftToolbarActions());
	registerSetting(MobileToolbar_TopRightActions, "ActionGridBar/mobile_top_right_actions", defaultMobileTopRightToolbarActions());
	registerSetting(MobileToolbar_BottomLeftActions, "ActionGridBar/mobile_bottom_left_actions", defaultMobileBottomLeftToolbarActions());
	registerSetting(MobileToolbar_BottomRightActions, "ActionGridBar/mobile_bottom_right_actions", defaultMobileBottomRightToolbarActions());
	registerSetting(SymbolWidget_IconSizeMM, "SymbolWidget/icon_size_mm", symbol_widget_icon_size_mm_default);
	registerSetting(SymbolWidget_ShowCustomIcons, "SymbolWidget/show_custom_icons", true);
	
	registerSetting(General_RetainCompatiblity, "retainCompatiblity", false);
	registerSetting(General_SaveUndoRedo, "saveUndoRedo", true);
	registerSetting(General_AutosaveInterval, "autosave", 15); // unit: minutes
	registerSetting(General_Language, "language", QLocale::system().name().left(2));
	registerSetting(General_PixelsPerInch, "pixelsPerInch", ppi);
	registerSetting(General_TranslationFile, "translationFile", QVariant(QString{}));
	registerSetting(General_RecentFilesList, "recentFileList", QVariant(QStringList()));
	registerSetting(General_OpenMRUFile, "openMRUFile", false);
	registerSetting(General_Local8BitEncoding, "local_8bit_encoding", QLatin1String("Default"));
	registerSetting(General_StartDragDistance, "startDragDistance", start_drag_distance_default);
	
	registerSetting(HomeScreen_TipsVisible, "HomeScreen/tipsVisible", true);
	registerSetting(HomeScreen_CurrentTip, "HomeScreen/currentTip", -1);
	
	// Set antialiasing default depending on screen pixels per inch
	registerSetting(MapDisplay_Antialiasing, "MapDisplay/antialiasing", Util::isAntialiasingRequired(getSetting(General_PixelsPerInch).toReal()));
	
	// Gesture uncovered area rendering: 0=Off, 1=Templates only, 2=Full
	registerSetting(MapDisplay_GestureExtraRendering, "MapDisplay/gesture_extra_rendering", 1);
	registerSetting(MapDisplay_AutoRotationFrequency, "MapDisplay/auto_rotation_frequency", 0);
	registerSetting(MapEditor_TouchPanOnly, "MapEditor/touch_pan_only", false);
	registerSetting(MapEditor_SPenButtonAction, "MapEditor/spen_button_action", 0);
	registerSetting(MapEditor_SPenHoverPausesRotation, "MapEditor/spen_hover_pauses_rotation", false);
	registerSetting(PieMenu_Actions, "PieMenu/actions", defaultPieMenuActions());

	// Paint On Template tool settings
	registerSetting(PaintOnTemplateTool_Colors, "PaintOnTemplateTool/colors", QLatin1String("FF0000,FFFF00,00FF00,DB00D9,0000FF,D15C00,000000"));

	QSettings settings;

	auto migrate_legacy_mobile_toolbar_settings = [this, &settings]() {
		auto const has_new_toolbar_settings
		    = settings.contains(getSettingPath(MobileToolbar_TopLeftActions))
		      || settings.contains(getSettingPath(MobileToolbar_TopRightActions))
		      || settings.contains(getSettingPath(MobileToolbar_BottomLeftActions))
		      || settings.contains(getSettingPath(MobileToolbar_BottomRightActions));
		if (has_new_toolbar_settings)
			return;

		auto const has_legacy_toolbar_settings
		    = settings.contains(getSettingPath(MobileToolbar_TopActions))
		      || settings.contains(getSettingPath(MobileToolbar_BottomActions));
		if (!has_legacy_toolbar_settings)
			return;

		auto const legacy_top = settings.value(getSettingPath(MobileToolbar_TopActions), getDefaultValue(MobileToolbar_TopActions)).toStringList();
		auto const legacy_bottom = settings.value(getSettingPath(MobileToolbar_BottomActions), getDefaultValue(MobileToolbar_BottomActions)).toStringList();

		auto top_left = QStringList{ QStringLiteral("hidetopbar"), QStringLiteral("save") };
		auto top_right = QStringList{ QStringLiteral("close"), QStringLiteral("overflow") };
		auto bottom_left = QStringList{};
		auto bottom_right = QStringList{ QStringLiteral("symbolselector") };

		auto const default_top_left = defaultMobileTopLeftToolbarActions();
		auto const default_top_right = defaultMobileTopRightToolbarActions();
		auto const default_bottom_left = defaultMobileBottomLeftToolbarActions();
		auto const default_bottom_right = defaultMobileBottomRightToolbarActions();

		for (const auto& id : legacy_top)
		{
			if (default_top_left.contains(id))
				top_left << id;
			else if (default_top_right.contains(id))
				top_right << id;
		}
		for (const auto& id : legacy_bottom)
		{
			if (default_bottom_left.contains(id))
				bottom_left << id;
			else if (default_bottom_right.contains(id))
				bottom_right << id;
		}

		if (!top_right.contains(QStringLiteral("mapparts")))
			top_right << QStringLiteral("mapparts");
		if (!bottom_left.contains(QStringLiteral("paint")))
			bottom_left << QStringLiteral("paint");

		settings.setValue(getSettingPath(MobileToolbar_TopLeftActions), top_left);
		settings.setValue(getSettingPath(MobileToolbar_TopRightActions), top_right);
		settings.setValue(getSettingPath(MobileToolbar_BottomLeftActions), bottom_left);
		settings.setValue(getSettingPath(MobileToolbar_BottomRightActions), bottom_right);
	};
	migrate_legacy_mobile_toolbar_settings();

#ifndef Q_OS_ANDROID
	// Overwrite default value with actual setting
	touch_mode_enabled = mobileModeEnforced() || settings.value(QLatin1String("General/touch_mode_enabled"), touch_mode_enabled).toBool();
#endif
	
	sensors.position_source = settings.value(QLatin1String("Sensors/position_source"), sensors.position_source).toString();
	sensors.nmea_serialport = settings.value(QLatin1String("Sensors/nmea_serialport"), sensors.nmea_serialport).toString();
	
	// Migrate old settings
	static bool migration_checked = false;
	if (!migration_checked)
	{
		QVariant current_version { QLatin1String("0.8") };
		auto settings_version = settings.value(QLatin1String("version")).toString();
		if (!settings_version.isEmpty() && settings_version != current_version)
		{
			migrateSettings(settings, current_version);
		}
		settings.setValue(QLatin1String("version"), current_version);
		migration_checked = true;
	}
}

void Settings::registerSetting(Settings::SettingsEnum id, const char* path_latin1, const QVariant& default_value)
{
	setting_paths[id] = QString::fromLatin1(path_latin1);
	setting_defaults[id] = default_value;
}

void Settings::migrateSettings(QSettings& settings, const QVariant& version)
{
	migrateValue("General/language", General_Language, settings);
	if (migrateValue("MapEditor/click_tolerance", MapEditor_ClickToleranceMM, settings))
		settings.setValue(getSettingPath(MapEditor_ClickToleranceMM), Util::pixelToMMLogical(settings.value(getSettingPath(MapEditor_ClickToleranceMM)).toReal()));
	if (migrateValue("MapEditor/snap_distance", MapEditor_SnapDistanceMM, settings))
		settings.setValue(getSettingPath(MapEditor_SnapDistanceMM), Util::pixelToMMLogical(settings.value(getSettingPath(MapEditor_SnapDistanceMM)).toReal()));
	if (migrateValue("RectangleTool/helper_cross_radius", RectangleTool_HelperCrossRadiusMM, settings))
		settings.setValue(getSettingPath(RectangleTool_HelperCrossRadiusMM), Util::pixelToMMLogical(settings.value(getSettingPath(RectangleTool_HelperCrossRadiusMM)).toReal()));
	
	if (!settings.value(QLatin1String("MapEditor/units_rectified"), false).toBool())
	{
		const auto factor = QGuiApplication::primaryScreen()->logicalDotsPerInch()
		                    / getSetting(Settings::General_PixelsPerInch).toReal();
		for (auto setting : { SymbolWidget_IconSizeMM, MapEditor_ClickToleranceMM, MapEditor_SnapDistanceMM, RectangleTool_HelperCrossRadiusMM })
		{
			const auto path = getSettingPath(setting);
			const auto value = settings.value(path, getDefaultValue(setting)).toReal();
			settings.setValue(path, qRound(value * factor));
		}
		settings.setValue(QLatin1String("MapEditor/units_rectified"), true);
	}
	
	bool have_language_number;
	auto language_number = getSetting(General_Language).toInt(&have_language_number);
	if (have_language_number)
	{
		// Migrate old numeric language setting
		auto language = QLocale(QLocale::Language(language_number)).name().left(2);
		setSetting(Settings::General_Language, language);
	}
	
	if (!settings.value(QLatin1String("new_ocd8_implementation_v0.6")).isNull())
	{
		// Remove/reset to default
		settings.remove(QLatin1String("new_ocd8_implementation_v0.6"));
		settings.remove(QLatin1String("new_ocd8_implementation"));
	}
	
	if (!version.toByteArray().startsWith("0."))
	{
		// Future cleanup
		settings.remove(QLatin1String("new_ocd8_implementation"));
		settings.remove(QLatin1String("MapEditor/units_rectified"));
	}
}

bool Settings::migrateValue(const char* old_key_latin1, SettingsEnum new_setting, QSettings& settings) const
{
	auto old_key = QString::fromLatin1(old_key_latin1);
	bool value_migrated = false;
	if (settings.contains(old_key))
	{
		const QString new_key = getSettingPath(new_setting);
		Q_ASSERT_X(new_key != old_key, "Settings::migrateValue",
		  qPrintable(QString::fromLatin1("New key \"%1\" equals old key").arg(new_key)) );
		
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

void Settings::setSettingInCache(Settings::SettingsEnum setting, const QVariant& value)
{
	settings_cache.insert(setting, value);
}

void Settings::setSetting(Settings::SettingsEnum setting, const QVariant& value)
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
	return qRound(Util::mmToPixelPhysical(getSettingCached(Settings::SymbolWidget_IconSizeMM).toReal()));
}

qreal Settings::getMapEditorClickTolerancePx()
{
	return Util::mmToPixelPhysical(getSettingCached(Settings::MapEditor_ClickToleranceMM).toReal());
}

qreal Settings::getMapEditorSnapDistancePx()
{
	return Util::mmToPixelPhysical(getSettingCached(Settings::MapEditor_SnapDistanceMM).toReal());
}

qreal Settings::getRectangleToolHelperCrossRadiusPx()
{
	return Util::mmToPixelPhysical(getSettingCached(Settings::RectangleTool_HelperCrossRadiusMM).toReal());
}

int Settings::getStartDragDistancePx()
{
	return getSettingCached(Settings::General_StartDragDistance).toInt();
}


#ifndef Q_OS_ANDROID

void Settings::setTouchModeEnabled(bool enabled)
{
	if (!mobileModeEnforced() && touch_mode_enabled != enabled)
	{
		touch_mode_enabled = enabled;
		QSettings().setValue(QLatin1String("General/touch_mode_enabled"), enabled);
		emit settingsChanged();
	}
}

// static
bool Settings::mobileModeEnforced() noexcept
{
	static bool const mobile_mode_enforced = qEnvironmentVariableIsSet("MAPPER_MOBILE_GUI")
	                                         ? (qgetenv("MAPPER_MOBILE_GUI") != "0")
	                                         : false;
	return mobile_mode_enforced;
}

#endif


void Settings::setPositionSource(const QString& name)
{
	if (name != sensors.position_source)
	{
		sensors.position_source = name;
		QSettings().setValue(QLatin1String("Sensors/position_source"), name);
		emit settingsChanged();
	}
}

void Settings::setNmeaSerialPort(const QString& name)
{
	if (name != sensors.nmea_serialport)
	{
		sensors.nmea_serialport = name;
		QSettings().setValue(QLatin1String("Sensors/nmea_serialport"), name);
		emit settingsChanged();
	}
}

std::vector<QColor> Settings::colorsStringToVector(QString config_string)
{
	auto const color_strings = config_string.splitRef(QLatin1Char(','));

	auto colors = std::vector<QColor>();
	colors.reserve(color_strings.size());

	// we don't bother about malformed strings and accept the resulting zero
	for (const auto& color_string : color_strings)
		colors.emplace_back(QRgb(color_string.toUInt(nullptr, 16)));

	return colors;
}

std::vector<QColor> Settings::paintOnTemplateColors() const
{
	return colorsStringToVector(getSetting(PaintOnTemplateTool_Colors).toString());
}

QString Settings::colorsVectorToString(const std::vector<QColor>& new_colors)
{
	auto strings = QStringList {};
	strings.reserve(new_colors.size());

	for (auto const& color : new_colors)
		strings.append(color.name().right(6).toUpper());

	return strings.join(QLatin1Char(','));
}

void Settings::setPaintOnTemplateColors(const std::vector<QColor>& new_colors)
{
	setSetting(PaintOnTemplateTool_Colors, colorsVectorToString(new_colors));
}


}  // namespace OpenOrienteering
