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


#include "mapper_service_proxy.h"

#include <QWidget>

#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QtAndroidExtras/QAndroidJniObject>
#endif

#include "gui/main_window.h"


namespace OpenOrienteering {

#ifdef Q_OS_ANDROID

namespace {

auto const foreground_service = QStringLiteral("android.permission.FOREGROUND_SERVICE");

/**
 * A callback which handles Android permission request results.
 * 
 * In addition to the MapperServiceProxy and it is member function, this unit
 * takes a QPointer to the target window which also serves as an indicator that
 * the proxy object still exists at the time the functor is called.
 */
struct MapperServiceProxyCallback
{
	MapperServiceProxy* proxy;
	void (MapperServiceProxy::*start_service)();
	QPointer<QWidget> window;
	
	void operator()(const QtAndroid::PermissionResultMap &results)
	{
		if (!window || window != proxy->activeWindow())
			return;
		if (results[foreground_service] == QtAndroid::PermissionResult::Granted)
			(proxy->*start_service)();
	}
};

}

#endif


MapperServiceProxy::~MapperServiceProxy()
{
	setActiveWindow(nullptr);
}


void MapperServiceProxy::setActiveWindow(QWidget* window)
{
	if (active_window == window)
		return;
	
	if (active_window != nullptr)
		stopService();
		
	active_window = window;
		
	if (active_window == nullptr)
		return;
	
#ifdef Q_OS_ANDROID
	if (QtAndroid::androidSdkVersion() >= 28)
	{
		if (QtAndroid::checkPermission(foreground_service) != QtAndroid::PermissionResult::Granted)
		{
			auto callback = MapperServiceProxyCallback{this, &MapperServiceProxy::startService, window};
			QtAndroid::requestPermissions({foreground_service}, callback);
			return;
		}
	}
#endif
	
	startService();
}


void MapperServiceProxy::startService()
{
	Q_ASSERT(active_window);
	
#ifdef Q_OS_ANDROID
	auto const file_path = active_window->windowFilePath();
	auto const prefix_length = file_path.lastIndexOf(QLatin1Char('/')) + 1;
	QAndroidJniObject java_string = QAndroidJniObject::fromString(file_path.mid(prefix_length));
	QAndroidJniObject::callStaticMethod<void>("org/openorienteering/mapper/MapperActivity",
	                                          "startService",
	                                          "(Ljava/lang/String;)V",
	                                          java_string.object<jstring>());
#endif
}


void MapperServiceProxy::stopService()
{
	Q_ASSERT(active_window);
	
#ifdef Q_OS_ANDROID
	QAndroidJniObject::callStaticMethod<void>("org/openorienteering/mapper/MapperActivity",
	                                          "stopService",
	                                          "()V");
#endif
}

}  // namespace OpenOrienteering
