/*
 *    Copyright 2013, 2016 Kai Pastor
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

#include "ocd_types.h"
#include "ocd_types_v8.h"
#include "ocd_types_v9.h"
#include "ocd_types_v10.h"
#include "ocd_types_v11.h"
#include "ocd_types_v12.h"

namespace Ocd
{
	// Verify at compile time that a double is 8 bytes big.
	
	Q_STATIC_ASSERT(sizeof(double) == 8);
	
	// Verify at compile time that data structures are packed, not aligned.
	
	Q_STATIC_ASSERT(sizeof(FileHeaderGeneric) == 8);
	
	Q_STATIC_ASSERT(sizeof(FormatV8::FileHeader) - sizeof(SymbolHeaderV8) == 48);
	
	Q_STATIC_ASSERT(sizeof(FormatV9::FileHeader) == 48);
	
	Q_STATIC_ASSERT(sizeof(FormatV10::FileHeader) == 48);
	
	Q_STATIC_ASSERT(sizeof(FormatV11::FileHeader) == 48);
	
	Q_STATIC_ASSERT(sizeof(FormatV12::FileHeader) == 60);
	
} // namespace Ocd
