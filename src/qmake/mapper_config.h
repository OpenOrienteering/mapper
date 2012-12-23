/*
 *    Copyright 2012 Thomas Schöps, Kai Pastor
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

/** This file is used by qmake-driven builds only. */

#ifndef _OPENORIENTEERING_MAPPER_CONFIG_H_
#define _OPENORIENTEERING_MAPPER_CONFIG_H_

#if !defined(APP_NAME)
#define APP_NAME qApp->translate("Global", QT_TRANSLATE_NOOP("Global", "OpenOrienteering Mapper"))
#endif

#if !defined(APP_VERSION)
#define APP_VERSION "qmake 0.x.99"
#endif

#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
#define CPACK_DEBIAN_PACKAGE_NAME "openorienteering-mapper"
#define MAPPER_DEBIAN_PACKAGE_NAME CPACK_DEBIAN_PACKAGE_NAME
#endif

#endif
