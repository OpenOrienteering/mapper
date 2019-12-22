/*
 * Copyright 2019 Kai Pastor
 *
 * This file is part of CoVe. 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COVE_POTRACE_H
#define COVE_POTRACE_H

/**
 * \file
 * A sane C++ header for potrace.
 * 
 * Potrace's (internal) headers used by CoVe export some macros which interfere
 * with C++ function names. This C++ header is to include all potrace C headers
 * needed for Cove, and undefine these macros.
 */

extern "C" {

// IWYU pragma: begin_exports
#include "potrace/auxiliary.h"
#include "potrace/curve.h"
#include "potrace/lists.h"
#include "potrace/trace.h"
// IWYU pragma: end_exports

}


// Get rid of the the macros from potrace/auxiliary.h.

#ifdef abs
#  undef abs
#endif

#ifdef max
#  undef max
#endif

#ifdef min
#  undef min
#endif


#endif  // COVE_POTRACE_H
