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


#include <QDebug>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>
#include <QStyleFactory>
#include <qstyle.h>

#include <mapper_config.h>

#if defined(QT_NETWORK_LIB)
#define MAPPER_USE_QTSINGLEAPPLICATION 1
#include <QtSingleApplication>
#else
#define MAPPER_USE_QTSINGLEAPPLICATION 0
#include <QApplication>
#endif

#include "global.h"
#include "gui/home_screen_controller.h"
#include "gui/main_window.h"
#include "gui/widgets/mapper_proxystyle.h"
#include "settings.h"
#include "util_translation.h"
#include "util/recording_translator.h"

int main(int argc, char** argv)
{
#ifdef Q_OS_ANDROID
	// Android native style is activated later.
	qputenv("QT_USE_ANDROID_NATIVE_STYLE", "0");
#endif
	
#if MAPPER_USE_QTSINGLEAPPLICATION
	// Create single-instance application.
	// Use "oo-mapper" instead of the executable as identifier, in case we launch from different paths.
	QtSingleApplication qapp("oo-mapper", argc, argv);
	if (qapp.isRunning()) {
		// Send a message to activate the running app, and optionally open a file
		qapp.sendMessage((argc > 1) ? argv[1] : "");
		return 0;
	}
#else
	QApplication qapp(argc, argv);
#endif
	
	// Load resources
#ifdef MAPPER_USE_QT_CONF_QRC
	Q_INIT_RESOURCE(qt);
#endif
	Q_INIT_RESOURCE(resources);
	Q_INIT_RESOURCE(licensing);
	
	// QSettings on OS X benefits from using an internet domain here.
	QCoreApplication::setOrganizationName("OpenOrienteering.org");
	QCoreApplication::setApplicationName("Mapper");
	
	// Set settings defaults
	Settings& settings = Settings::getInstance();
	settings.applySettings();
	
#ifdef WIN32
	// Load plugins on Windows
	qapp.addLibraryPath(QCoreApplication::applicationDirPath() + "/plugins");
#endif
	
	// Localization
	TranslationUtil::setBaseName(QString("OpenOrienteering"));
	QLocale::Language lang = (QLocale::Language)settings.getSetting(Settings::General_Language).toInt();
	QString translation_file = settings.getSetting(Settings::General_TranslationFile).toString();
	TranslationUtil translation(lang, translation_file);
	QLocale::setDefault(translation.getLocale());
#if defined(Mapper_DEBUG_TRANSLATIONS)
	if (!translation.getAppTranslator().isEmpty())
	{
		// Debug translation only if there is a Mapper translation, i.e. not for English.
		qapp.installTranslator(new RecordingTranslator());
	}
#endif
	qapp.installTranslator(&translation.getQtTranslator());
	qapp.installTranslator(&translation.getAppTranslator());
	
	// Initialize static things like the file format registry.
	doStaticInitializations();
	
	QStyle* base_style = nullptr;
#if defined(Q_OS_ANDROID)
	base_style = QStyleFactory::create("android");
#elif !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
	if (QGuiApplication::platformName() == QLatin1String("xcb"))
	{
		// Use the modern 'fusion' style instead of the 
		// default "windows" style on X11.
		base_style = QStyleFactory::create("fusion");
	}
#endif
	QApplication::setStyle(new MapperProxyStyle(base_style));
	QApplication::setPalette(QApplication::style()->standardPalette());
	
	// Create first main window
	MainWindow first_window(true);
	first_window.setAttribute(Qt::WA_DeleteOnClose, false);
	first_window.setController(new HomeScreenController());
	
	// Open given files later, i.e. after the initial home screen has been
	// displayed. In this way, error messages for missing files will show on 
	// top of a regular main window (home screen or other file).
	
	// Treat all program parameters as files to be opened
	bool no_files_given = true;
	QStringList args(qapp.arguments());
	args.removeFirst(); // the program name
	Q_FOREACH(QString arg, args)
	{
		if (arg[0] != '-')
		{
			first_window.openPathLater(arg);
			no_files_given = false;
		}
	}
	
	// Optionally open most recently used file on startup
	if (no_files_given && settings.getSettingCached(Settings::General_OpenMRUFile).toBool())
	{
		QStringList files(settings.getSettingCached(Settings::General_RecentFilesList).toStringList());
		if (!files.isEmpty())
			first_window.openPathLater(files[0]);
	}
	
#if MAPPER_USE_QTSINGLEAPPLICATION
	// If we need to respond to a second app launch, do so, but also accept a file open request.
	qapp.setActivationWindow(&first_window);
	QObject::connect(&qapp, SIGNAL(messageReceived(const QString&)), &first_window, SLOT(openPath(const QString &)));
#endif
	
	// Let application run
	first_window.show();
	first_window.raise();
	return qapp.exec();
}

#ifdef _MSC_VER
#include "Windows.h"
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(__argc, __argv);
}
#endif
