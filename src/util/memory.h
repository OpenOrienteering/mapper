/*
 *    Copyright 2016 Kai Pastor
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


#ifndef OPENORIENTEERING_UTIL_MEMORY_H
#define OPENORIENTEERING_UTIL_MEMORY_H

#include <memory>


#if defined(__cpp_lib_make_unique) && __cpp_lib_make_unique >= 201304 // C++14, gcc

using std::make_unique;

#elif defined(_LIBCPP_STD_VER) &&  _LIBCPP_STD_VER > 11 // C++14, llvm

using std::make_unique;

#else

/**
 * Returns a new object for the given constructor arguments, wrapped in a std::unique_ptr.
 * 
 * \see std::make_unique
 */
template <class T, class... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#endif


#endif
