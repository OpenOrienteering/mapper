/*
 *    Copyright 2012-2014 Kai Pastor
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


#ifndef _OPENORIENTEERING_MAPPER_RESOURCE_H_
#define _OPENORIENTEERING_MAPPER_RESOURCE_H_

#include <QString>

class QStringList;

/**
 * Utility for locating and loading resources.
 * 
 * Depending on operating system and build configuration,
 * resources may be located in a number of different locations.
 * This interface hides all the complex dependencies.
 */
namespace MapperResource
{
	/**
	 * Various types of resources which are used by Mapper.
	 */
	enum RESOURCE_TYPE {
		ASSISTANT,
		EXAMPLE,
		MANUAL,
		PROJ_DATA,
		SYMBOLSET,
		TEST_DATA,
		TRANSLATION
	};
	
	/** 
	 * Get a list of paths where to find particular resources.
	 * Returns an empty list if no valid path exists.
	 */
	QStringList getLocations(MapperResource::RESOURCE_TYPE resource_type);
	
	/** 
	 * Get the first instance of a resource. 
	 * Returns an empty string if the resource is not found.
	 * 
	 * If the name parameter is given, it is understood as a filename in the 
	 * resource directory identified by resource_type.
	 */
	QString locate(MapperResource::RESOURCE_TYPE resource_type, const QString name = QString());
}

#endif
