/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2019 Kai Pastor
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
#include <QLatin1String>
#include <QList>
#include <QLocale>
#include <QObject>
#include <QPalette>
#include <QPointer>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTranslator>
#include <QWidget>

#include <mapper_config.h>

#if defined(QT_NETWORK_LIB)
#  define MAPPER_USE_QTSINGLEAPPLICATION 1
#  include <QtSingleApplication>
#  include <QFileInfo>
#else
#  define MAPPER_USE_QTSINGLEAPPLICATION 0
#endif

#include "global.h"
#include "mapper_resource.h"
#include "gui/home_screen_controller.h"
#include "gui/main_window.h"
#include "gui/widgets/mapper_proxystyle.h"
#include "util/recording_translator.h"  // IWYU pragma: keep
#include "util/translation_util.h"

// IWYU pragma: no_forward_declare QTranslator

using namespace OpenOrienteering;


#if defined(Q_OS_WIN32) && defined(MAPPER_USE_POWERSHELL_POSITION_PLUGIN)
Q_IMPORT_PLUGIN(PowershellPositionPlugin)
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


#if MAPPER_USE_QTSINGLEAPPLICATION

void resetActivationWindow()
{
	auto app = qobject_cast<QtSingleApplication*>(qApp);
	app->setActivationWindow(nullptr);
	
	if (!app->closingDown())
	{
		const auto* old_window = app->activationWindow();
		const auto top_level_widgets = app->topLevelWidgets();
		for (auto* widget : top_level_widgets)
		{	
			auto new_window = qobject_cast<MainWindow*>(widget);
			if (new_window && new_window != old_window)
			{
				app->setActivationWindow(new_window);
				QObject::connect(new_window, &QObject::destroyed, &resetActivationWindow);
				QObject::connect(app, &QtSingleApplication::messageReceived, new_window, &MainWindow::openPathLater);
				break;
			}
		}
	}
}

#endif


int main(int argc, char** argv)
{
#if MAPPER_USE_QTSINGLEAPPLICATION
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
	// Workaround for Mapper issue GH-1373, QTBUG-72408
	/// \todo Remove QTBUG-72408 workaround once solved in Qt.
	qputenv("QT_QPA_NO_TEXT_HANDLES", "1");
#endif
	
	// Load resources
	Q_INIT_RESOURCE(resources);
	
	// QSettings on OS X benefits from using an internet domain here.
	QApplication::setOrganizationName(QString::fromLatin1("OpenOrienteering.org"));
	QApplication::setApplicationName(QString::fromLatin1("Mapper"));
	qapp.setApplicationDisplayName(APP_NAME + QString::fromUtf8(" " APP_VERSION));
	
#ifdef WIN32
	// Load plugins on Windows
	qapp.addLibraryPath(QCoreApplication::applicationDirPath() + QLatin1String("/plugins"));
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
		qapp.installTranslator(new RecordingTranslator());
	}
#endif
	qapp.installTranslator(&translation.getQtTranslator());
	qapp.installTranslator(&translation.getAppTranslator());
	map_symbol_translator = translation.load(QString::fromLatin1("map_symbols")).release();
	if (map_symbol_translator)
		map_symbol_translator->setParent(&qapp);
	
	// Avoid numeric issues in libraries such as GDAL
	setlocale(LC_NUMERIC, "C");
	
	// Initialize static things like the file format registry.
	doStaticInitializations();
	
	auto const palette = QApplication::palette();
	QApplication::setStyle(new MapperProxyStyle());
	QApplication::setPalette(palette);
	
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
	
#if MAPPER_USE_QTSINGLEAPPLICATION
	resetActivationWindow();
#endif
	
	// Let application run
	first_window->setVisible(true);
	first_window->raise();
	return qapp.exec();
}
