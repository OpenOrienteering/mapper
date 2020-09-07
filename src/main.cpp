/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2020 Kai Pastor
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


#include <clocale>
#include <memory>
#include <utility>
// IWYU pragma: no_include <type_traits>

#include <Qt>
#include <QtGlobal>
#include <QtPlugin>  // IWYU pragma: keep
#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QLatin1String>
#include <QList>
#include <QLocale>
#include <QObject>
#include <QPointer>
#include <QSettings>
#include <QStaticPlugin>  // IWYU pragma: keep
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QTranslator>
#include <QWidget>

#ifdef MAPPER_USE_QTSINGLEAPPLICATION
#include <QtSingleApplication>
#include <QFileInfo>
#endif

#include "global.h"
#include "mapper_config.h"
#include "mapper_resource.h"
#include "gui/home_screen_controller.h"
#include "gui/main_window.h"
#include "gui/widgets/mapper_proxystyle.h"
#include "util/recording_translator.h"  // IWYU pragma: keep
#include "util/translation_util.h"

// IWYU pragma: no_forward_declare QTranslator

using namespace OpenOrienteering;


#if defined(MAPPER_USE_FAKE_POSITION_PLUGIN)
Q_IMPORT_PLUGIN(FakePositionPlugin)
#endif

#if defined(Q_OS_WIN) && defined(MAPPER_USE_POWERSHELL_POSITION_PLUGIN)
Q_IMPORT_PLUGIN(PowershellPositionPlugin)
#endif

#if (defined(Q_OS_LINUX) || defined(Q_OS_MACOS)) && defined(MAPPER_USE_NMEA_POSITION_PLUGIN)
Q_IMPORT_PLUGIN(NmeaPositionPlugin)
#endif

#if defined(SCALING_ICON_ENGINE_PLUGIN)
Q_IMPORT_PLUGIN(ScalingIconEnginePlugin)
#endif


// From map.h

namespace OpenOrienteering {

extern QPointer<QTranslator> map_symbol_translator;

}  // namespace OpenOrienteering


QStringList firstRemoved(QStringList&& input)
{
	if (!input.empty())
		input.removeFirst();
	return std::move(input);
}


#ifdef MAPPER_USE_QTSINGLEAPPLICATION

void resetActivationWindow(QtSingleApplication& app)
{
	const auto* const old_window = app.activationWindow();
	app.setActivationWindow(nullptr);
	
	if (!QCoreApplication::closingDown())
	{
		const auto top_level_widgets = QApplication::topLevelWidgets();
		for (auto* widget : top_level_widgets)
		{	
			auto* const new_window = qobject_cast<MainWindow*>(widget);
			if (new_window && new_window != old_window)
			{
				app.setActivationWindow(new_window);
				QObject::connect(new_window, &QObject::destroyed, &app, [&app]() { resetActivationWindow(app); });
				QObject::connect(&app, &QtSingleApplication::messageReceived, new_window, &MainWindow::openPathLater);
				break;
			}
		}
	}
}

#endif


int main(int argc, char** argv)
{
#ifdef MAPPER_USE_QTSINGLEAPPLICATION
	// Create single-instance application.
	// Use "oo-mapper" instead of the executable as identifier, in case we launch from different paths.
	QtSingleApplication qapp(QString::fromLatin1("oo-mapper"), argc, argv);
	if (qapp.isRunning()) {
		// Send a message to activate the running app, and optionally open a file
		auto const arguments = firstRemoved(QCoreApplication::arguments());
		for (auto const& arg : arguments)
			qapp.sendMessage(QFileInfo(arg).absoluteFilePath());
		return 0;
	}
#else
	QApplication qapp(argc, argv);
#endif
	
#ifdef Q_OS_ANDROID
	qputenv("QT_USE_ANDROID_NATIVE_STYLE", "1");
#endif
	
	// Load resources
	Q_INIT_RESOURCE(resources);
	
	// QSettings on OS X benefits from using an internet domain here.
	QCoreApplication::setOrganizationName(QString::fromLatin1("OpenOrienteering.org"));
	QCoreApplication::setApplicationName(QString::fromLatin1("Mapper"));
	QGuiApplication::setApplicationDisplayName(APP_NAME + QString::fromUtf8(" " APP_VERSION));
	
#ifdef WIN32
	// Load plugins on Windows
	QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() + QLatin1String("/plugins"));
#endif
	
	MapperResource::setSeachPaths();
	
	// Localization
	QSettings settings;
	TranslationUtil::setBaseName(QLatin1String("OpenOrienteering"));
	TranslationUtil translation(settings);
	QLocale::setDefault(QLocale(translation.code()));
#if defined(Q_OS_MACOS)
	// Normally this is done in Settings::apply() because it is too late here.
	// But Mapper 0.6.2/0.6.3 accidentally wrote a string instead of a list. This
	// error caused crashes when opening native dialogs (i.e. the open-file dialog!).
	settings.setValue(QString::fromLatin1("AppleLanguages"), QStringList{ translation.code() });
#endif
#if defined(Mapper_DEBUG_TRANSLATIONS)
	if (!translation.getAppTranslator().isEmpty())
	{
		// Debug translation only if there is a Mapper translation, i.e. not for English.
		QCoreApplication::installTranslator(new RecordingTranslator());
	}
#endif
	QCoreApplication::installTranslator(&translation.getQtTranslator());
	QCoreApplication::installTranslator(&translation.getAppTranslator());
	map_symbol_translator = translation.load(QString::fromLatin1("map_symbols")).release();
	if (map_symbol_translator)
		map_symbol_translator->setParent(&qapp);
	
	// Avoid numeric issues in libraries such as GDAL
	setlocale(LC_NUMERIC, "C");
	
	// Initialize static things like the file format registry.
	doStaticInitializations();
	
	// Some style settings (in particular the menu item font) are not
	// applied correctly before the app runs. So we postpone these steps
	// via the event loop.
	// OTOH the app crashes on Android if we don't set style early enough.
	QTimer::singleShot(0, qApp, [&qapp]() {
#ifndef __clang_analyzer__
		// No leak: QApplication takes ownership.
		QApplication::setStyle(new MapperProxyStyle());
#endif
		
		// Create first main window
		auto first_window = new MainWindow();
		Q_ASSERT(first_window->testAttribute(Qt::WA_DeleteOnClose));
		first_window->setController(new HomeScreenController());
		
		// Open given files later, i.e. after the initial home screen has been
		// displayed. In this way, error messages for missing files will show on 
		// top of a regular main window (home screen or other file).
		
		// Treat all program parameters as files to be opened
		auto const arguments = firstRemoved(QCoreApplication::arguments());
		for (auto const& arg : arguments)
			first_window->openPathLater(arg);
		
		first_window->applicationStateChanged();
		
#ifdef MAPPER_USE_QTSINGLEAPPLICATION
		resetActivationWindow(qapp);
#endif
		
		first_window->setVisible(true);
		first_window->raise();
	});
	
	// Let application run
	return QApplication::exec();
}
