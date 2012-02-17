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
#include <QMessageBox>
#include <QDebug>

#include "main_window.h"
#include "main_window_home_screen.h"

int main(int argc, char** argv)
{
    // Create single-instance application.
    // Use "oo-mapper" instead of the executable as identifier, in case we launch from different paths.
    QtSingleApplication qapp("oo-mapper", argc, argv);
    if (qapp.isRunning()) {
        // Send a message to activate the running app, and optionally open a file
        qapp.sendMessage((argc > 1) ? argv[1] : "");
        return 0;
    }

	MainWindow* first_window = NULL;
	
	// Set settings defaults
	QCoreApplication::setOrganizationName("Thomas Schoeps");
	QCoreApplication::setApplicationName("OpenOrienteering");
	
	// Create first main window - if a program parameter is specified, open the file
	first_window = new MainWindow(true);
	if (argc > 1)
	{
		if (!first_window->openPath(argv[1]))
		{
			QMessageBox::warning(NULL, MainWindow::tr("Error"), MainWindow::tr("Cannot open file:\n%1\n\nFile format not recognized."));
			first_window->setController(new HomeScreenController());
		}
	}
	else
		first_window->setController(new HomeScreenController());

    // If we need to respond to a second app launch, do so, but also accept a file open request.
    qapp.setActivationWindow(first_window);
    QObject::connect(&qapp, SIGNAL(messageReceived(const QString&)), first_window, SLOT(openPath(const QString &)));

	// Let application run
	first_window->show();
	first_window->raise();
	return qapp.exec();
}

#ifdef WIN32
#include "Windows.h"
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(0, NULL);
}
#endif
