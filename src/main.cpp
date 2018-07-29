/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2017 Kai Pastor
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

#include <Qt>
#include <QtGlobal>
#include <QApplication>
#include <QLatin1String>
#include <QLocale>
#include <QObject>
#include <QPointer>
#include <QSettings>  // IWYU pragma: keep
#include <QString>
#include <QStringList>
#include <QStyle>
#include <QStyleFactory>
#include <QTranslator>

#include <mapper_config.h>

#if defined(QT_NETWORK_LIB)
#  define MAPPER_USE_QTSINGLEAPPLICATION 1
#  include <QtSingleApplication>  // IWYU pragma: keep
#else
#  define MAPPER_USE_QTSINGLEAPPLICATION 0
#endif

#include "global.h"
#include "mapper_resource.h"
#include "gui/home_screen_controller.h"
#include "gui/main_window.h"
#include "gui/widgets/mapper_proxystyle.h"
#include "util/backports.h"
#include "util/recording_translator.h"  // IWYU pragma: keep
#include "util/translation_util.h"

// IWYU pragma: no_forward_declare QTranslator

using namespace OpenOrienteering;


// From map.h

namespace OpenOrienteering {

extern QPointer<QTranslator> map_symbol_translator;

}  // namespace OpenOrienteering



#if MAPPER_USE_QTSINGLEAPPLICATION

void resetActivationWindow()
{
	auto app = qobject_cast<QtSingleApplication*>(qApp);
	app->setActivationWindow(nullptr);
	
	if (!app->closingDown())
	{
		const auto old_window = app->activationWindow();
		const auto top_level_widgets = app->topLevelWidgets();
		for (const auto widget : top_level_widgets)
		{	
			auto new_window = qobject_cast<MainWindow*>(widget);
			if (new_window && new_window != old_window)
			{
				app->setActivationWindow(new_window);
				QObject::connect(new_window, &QObject::destroyed, &resetActivationWindow);
				QObject::connect(app, &QtSingleApplication::messageReceived, new_window, QOverload<const QString&>::of(&MainWindow::openPath));
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
		qapp.sendMessage(QString::fromUtf8((argc > 1) ? argv[1] : ""));
		return 0;
	}
#else
	QApplication qapp(argc, argv);
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
	// But Mapper 0.6.2/0.6.3 accidently wrote a string instead of a list. This
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
	
	QStyle* base_style = nullptr;
#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
	if (QApplication::platformName() == QLatin1String("xcb"))
	{
		// Use the modern 'fusion' style instead of the 
		// default "windows" style on X11.
		base_style = QStyleFactory::create(QString::fromLatin1("fusion"));
	}
#endif
	QApplication::setStyle(new MapperProxyStyle(base_style));
#if !defined(Q_OS_MACOS)
	QApplication::setPalette(QApplication::style()->standardPalette());
#endif
	
	// Create first main window
	auto first_window = new MainWindow();
	Q_ASSERT(first_window->testAttribute(Qt::WA_DeleteOnClose));
	first_window->setController(new HomeScreenController());
	
	// Open given files later, i.e. after the initial home screen has been
	// displayed. In this way, error messages for missing files will show on 
	// top of a regular main window (home screen or other file).
	
	// Treat all program parameters as files to be opened
	QStringList args(qapp.arguments());
	args.removeFirst(); // the program name
	for (const auto& arg : qAsConst(args))
	{
		first_window->openPathLater(arg);
	}
	
	first_window->applicationStateChanged();
	
#if MAPPER_USE_QTSINGLEAPPLICATION
	resetActivationWindow();
#endif
	
	// Let application run
	first_window->setVisible(true);
	first_window->raise();
	return qapp.exec();
}
