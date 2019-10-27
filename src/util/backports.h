/*
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


#ifndef OPENORIENTEERING_UTIL_BACKPORTS_H
#define OPENORIENTEERING_UTIL_BACKPORTS_H

#include <QtGlobal>

#if QT_VERSION < 0x050700
#  include "qasconst.h"  // IWYU pragma: export
#  include "qoverload.h" // IWYU pragma: export
#endif

#if defined(Q_OS_ANDROID) && defined(_GLIBCXX_CMATH)
namespace std
{

template <class T>
T hypot(T t1, T t2)
{
	return ::hypot(t1, t2);
}

}
#endif

#endif
