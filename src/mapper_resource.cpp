/*
 *    Copyright 2012-2015 Kai Pastor
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


#include "mapper_resource.h"

#include <mapper_config.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStringList>



/**
 * Private MapperResource utilities
 */
namespace MapperResource
{
	/** 
	 * Get a list of paths where to find a particular executable program.
	 * Returns an empty list if no valid path exists
	 * or if the resource type does not identify a program.
	 */
	QStringList getProgramLocations(MapperResource::RESOURCE_TYPE resource_type);
	
	/**
	 * Add a path to a string list only if that path exists.
	 */
	inline void addIfExists(QStringList& list, QString path)
	{
		if (QFile::exists(path))
			list << path;
	}
}


QStringList MapperResource::getLocations(MapperResource::RESOURCE_TYPE resource_type)
{
	QStringList locations;
	QString resource_path;
	QDir app_dir(QCoreApplication::applicationDirPath());
	
	switch (resource_type)
	{
		case ASSISTANT:
			return MapperResource::getProgramLocations(resource_type);
			
		case EXAMPLE:
			resource_path = "/examples";
			break;
			
		case MANUAL:
			// TODO: Support localized manual
			resource_path = "/doc/manual";
			break;
			
		case PROJ_DATA:
#if defined(Mapper_BUILD_PROJ)
			resource_path = "/proj";
			break;
#else
			// Don't fiddle with proj resource path.
			return locations;
#endif
			
		case SYMBOLSET:
			// TODO: Translate directory name "my symbol sets"?
			//       Possible Windows solution: desktop.ini
			addIfExists(locations, QDir::homePath() + "/my symbol sets");
			resource_path = "/symbol sets";
			break;
			
		case TEST_DATA:
			addIfExists(locations, app_dir.absoluteFilePath("data"));
			resource_path = "/test/data";
			break;
	
		case TRANSLATION:
#if defined(Mapper_TRANSLATIONS_EMBEDDED)
			// Always load embedded translations first if enabled
			addIfExists(locations, ":/translations");
#endif
			resource_path = "/translations";
			break;
			
		default:
			return locations;
	}
	
#if defined(MAPPER_DEVELOPMENT_BUILD) && defined(MAPPER_DEVELOPMENT_RES_DIR)
	// Use the directory where Mapper is built during development, 
	// even for the unit tests located in other directories.
	QString build_dir(MAPPER_DEVELOPMENT_RES_DIR + resource_path);
	addIfExists(locations, build_dir);
#endif
	
#if defined(MAPPER_PACKAGE_NAME)
	// Linux: program in xxx/bin, resources in xxx/bin/../share/PACKAGE_NAME
	QString linux_dir(app_dir.absoluteFilePath(QString("../share/") + MAPPER_PACKAGE_NAME + resource_path));
	addIfExists(locations, linux_dir);
#elif defined(Q_OS_MAC)
	// Mac OS X: load resources from the Resources directory of the bundle
	QString osx_dir(app_dir.absoluteFilePath("../Resources" + resource_path));
	addIfExists(locations, osx_dir);
#elif defined(Q_OS_WIN)
	// Windows: load resources from the application directory
	QString win_dir(app_dir.absolutePath() + resource_path);
	addIfExists(locations, win_dir);
#elif defined(Q_OS_ANDROID)
	// Android: load resources from the application directory
	QString assets_dir(QStringLiteral("assets:") + resource_path);
	addIfExists(locations, assets_dir);
#endif
	
	// General default path: Qt resource system
	addIfExists(locations, QString(":") + resource_path);
	
	return locations;
}


QStringList MapperResource::getProgramLocations(MapperResource::RESOURCE_TYPE resource_type)
{
	QStringList locations;
	QString program_name;
	
	switch (resource_type)
	{
		case ASSISTANT:
#if defined(Q_OS_WIN)
			program_name = "assistant.exe";
#elif defined(Q_OS_MAC)
			program_name = "Assistant";
#else
			program_name = "assistant";
#endif
			break;
		default:
			return locations;
	}
	
#if defined(MAPPER_DEVELOPMENT_BUILD) and defined(QT_QTASSISTANT_EXECUTABLE)
	addIfExists(locations, QString(QT_QTASSISTANT_EXECUTABLE));
#endif
	
	QDir app_dir(QCoreApplication::applicationDirPath());
	
#if defined(Mapper_PACKAGE_ASSISTANT) and defined(MAPPER_PACKAGE_NAME)
	// Linux: extra binaries in xxx/bin/../share/PACKAGE_NAME/bin
	addIfExists(locations, app_dir.absoluteFilePath(QString("../lib/") + MAPPER_PACKAGE_NAME + "/bin/" + program_name));
#endif
	
	// Find the program which is in the same directory as Mapper
	addIfExists(locations, app_dir.absoluteFilePath(program_name));
	
	// General: let system use its search path to find the program
	locations << program_name;
	
	return locations;
}


QString MapperResource::locate(MapperResource::RESOURCE_TYPE resource_type, const QString name)
{
	QStringList locations = getLocations(resource_type);
	if (locations.isEmpty())
		return QString();
	
	if (name.isEmpty())
		return locations.first();
	
	if (!QDir(locations.first()).exists(name))
		return QString();
	
	return QDir(locations.first()).absoluteFilePath(name);
}
