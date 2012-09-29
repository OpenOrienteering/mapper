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


#include "../qtsingleapplication/qtsingleapplication.h"
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

#include <mapper_config.h>

#include "global.h"
#include "main_window.h"
#include "main_window_home_screen.h"
#include "settings.h"

int main(int argc, char** argv)
{
	// Set the default graphics system to raster. This can be overwritten by the command line option "-graphicssystem".
	// This must be called before the QApplication constructor is called!
	QApplication::setGraphicsSystem("raster");
	
	// Create single-instance application.
	// Use "oo-mapper" instead of the executable as identifier, in case we launch from different paths.
	QtSingleApplication qapp("oo-mapper", argc, argv);
	if (qapp.isRunning()) {
		// Send a message to activate the running app, and optionally open a file
		qapp.sendMessage((argc > 1) ? argv[1] : "");
		return 0;
	}
	
	// Load resources
	Q_INIT_RESOURCE(resources);
	
	// Set settings defaults
	QCoreApplication::setOrganizationName("Thomas Schoeps");
	QCoreApplication::setApplicationName("OpenOrienteering");
	Settings::getInstance().applySettings();
	
#ifdef WIN32
	// Load plugins on Windows
	qapp.addLibraryPath(QCoreApplication::applicationDirPath() + "/plugins");
#endif
	
	// Localization
	QLocale locale = QLocale((QLocale::Language)Settings::getInstance().getSetting(Settings::General_Language).toInt());
	QLocale::setDefault(locale);
	QString locale_name = locale.name();
	
	// Load Qt translation
	QTranslator qtTranslator;
#ifdef WIN32
	qtTranslator.load("qt_" + locale_name, QCoreApplication::applicationDirPath() + "/translations/");
#else
	qtTranslator.load("qt_" + locale_name, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
#endif
	qapp.installTranslator(&qtTranslator);

	// Load application translation
	QTranslator translator;
	QString translation_name = "OpenOrienteering_" + locale_name;
	bool translation_ok = false;
#ifdef Mapper_TRANSLATIONS_EMBEDDED
	Q_INIT_RESOURCE(translations);
	translation_ok = translator.load(translation_name, QString(":/translations"));
#endif
	if (!translation_ok)
		translation_ok = translator.load(translation_name, QCoreApplication::applicationDirPath() + "/translations");
#ifdef MAPPER_DEBIAN_PACKAGE_NAME
	if (!translation_ok)
		translation_ok = translator.load(translation_name, QString("/usr/share/") + MAPPER_DEBIAN_PACKAGE_NAME + "/translations");
#endif
	qapp.installTranslator(&translator);
	
	doStaticInitializations();
	
	// Create first main window
	MainWindow* first_window = new MainWindow(true);
	first_window->setController(new HomeScreenController());
	
	// Treat all program parameters as files to be opened
	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] != '-')
			first_window->openPath(argv[i]);
	}
	
	// If we need to respond to a second app launch, do so, but also accept a file open request.
	qapp.setActivationWindow(first_window);
	QObject::connect(&qapp, SIGNAL(messageReceived(const QString&)), first_window, SLOT(openPath(const QString &)));
	
	// Let application run
	first_window->show();
	first_window->raise();
	return qapp.exec();
}

#ifdef _MSC_VER
#include "Windows.h"
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(__argc, __argv);
}
#endif
