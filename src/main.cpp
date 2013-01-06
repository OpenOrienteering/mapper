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


#include "../3rd-party/qtsingleapplication/src/qtsingleapplication.h"
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

#include <mapper_config.h>

#include "global.h"
#include "main_window.h"
#include "gui/home_screen_controller.h"
#include "settings.h"
#include "util_translation.h"

int main(int argc, char** argv)
{
#if QT_VERSION < 0x050000
	// Set the default graphics system to raster. This can be overwritten by the command line option "-graphicssystem".
	// This must be called before the QApplication constructor is called!
	QApplication::setGraphicsSystem("raster");
#endif
	
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
	
	QCoreApplication::setOrganizationName("Thomas Schoeps");
	QCoreApplication::setApplicationName("OpenOrienteering");
	
	// Set settings defaults
	Settings& settings = Settings::getInstance();
	settings.applySettings();
	
#ifdef WIN32
	// Load plugins on Windows
	qapp.addLibraryPath(QCoreApplication::applicationDirPath() + "/plugins");
#endif
	
	// Localization
	QLocale::Language lang = (QLocale::Language)settings.getSetting(Settings::General_Language).toInt();
	TranslationUtil translation(lang);
	QLocale::setDefault(translation.getLocale());
	qapp.installTranslator(&translation.getQtTranslator());
	qapp.installTranslator(&translation.getAppTranslator());
	
	doStaticInitializations();
	
	// Create first main window
	MainWindow first_window(true);
	first_window.setAttribute(Qt::WA_DeleteOnClose, false);
	first_window.setController(new HomeScreenController());
	
	// Open given files later, i.e. after the initial home screen has been
	// displayed. In this way, error messages for missing files will show on 
	// top of a regular main window (home screen or other file).
	
	// Treat all program parameters as files to be opened
	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] != '-')
			first_window.openPathLater(argv[i]);
	}
	// Optionally open most recently used file on startup
	if (argc <= 1 && settings.getSettingCached(Settings::General_OpenMRUFile).toBool())
	{
		QStringList files = settings.getSettingCached(Settings::General_RecentFilesList).toStringList();
		if (files.size() > 0)
			first_window.openPathLater(files[0]);
	}
	
	// If we need to respond to a second app launch, do so, but also accept a file open request.
	qapp.setActivationWindow(&first_window);
	QObject::connect(&qapp, SIGNAL(messageReceived(const QString&)), &first_window, SLOT(openPath(const QString &)));
	
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
