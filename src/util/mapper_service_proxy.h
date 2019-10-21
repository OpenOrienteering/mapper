/*
 *    Copyright 2019 Kai Pastor
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


#ifndef OPENORIENTEERING_MAPPER_SERVICE_PROXY_H
#define OPENORIENTEERING_MAPPER_SERVICE_PROXY_H

#include <QPointer>

class QWidget;

namespace OpenOrienteering {

/**
 * A class which helps to run a service alongside the application.
 * 
 * The service is started by setting a non-null active window, and stopped
 * by setting the active window to null.
 * 
 * On Android, this utility runs a foregound service with a notification
 * showing the app name and the file name. Running a foreground service
 * increases the app's importance with regard to power management and
 * continuous location access, and it makes the user aware of the app
 * when it is in the background.
 * 
 * On other platforms, this class does nothing at the moment.
 */
class MapperServiceProxy
{
public:
	MapperServiceProxy() = default;
	~MapperServiceProxy();
	
	QWidget* activeWindow() { return active_window.data(); }
	void setActiveWindow(QWidget* widget);
	
private:
	void startService();
	void stopService();
	
	QPointer<QWidget> active_window;
	
	Q_DISABLE_COPY(MapperServiceProxy)
};

}  // namespace OpenOrienteering

#endif
