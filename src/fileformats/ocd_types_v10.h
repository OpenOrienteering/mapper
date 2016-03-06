/*
 *    Copyright 2013, 2015, 2016 Kai Pastor
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

#ifndef OPENORIENTEERING_OCD_TYPES_V10
#define OPENORIENTEERING_OCD_TYPES_V10

#include "ocd_types.h"
#include "ocd_types_v9.h"

namespace Ocd
{
	
#pragma pack(push, 1)
	
	using FileHeaderV10 = FileHeaderV9;
	
	using BaseSymbolV10 = BaseSymbolV9;
	
	using PointSymbolElementV10 = PointSymbolElementV9;
	
	using PointSymbolV10 = PointSymbolV9;
	
	using LineSymbolV10 = LineSymbolV9;
	
	using AreaSymbolV10 = AreaSymbolV9;
	
	using TextSymbolV10 = TextSymbolV9;
	
	using LineTextSymbolV10 = LineTextSymbolV9;
	
	using RectangleSymbolV10 = RectangleSymbolV9;
	
	using ObjectIndexEntryV10 = ObjectIndexEntryV9;
	
	using ObjectV10 = ObjectV9;
	
#pragma pack(pop)
	
	/** OCD file format version 10 trait. */
	struct FormatV10
	{
		static constexpr int version() { return 10; }
		
		typedef FileHeaderV10 FileHeader;
		
		typedef BaseSymbolV10 BaseSymbol;
		typedef PointSymbolV10 PointSymbol;
		typedef LineSymbolV10 LineSymbol;
		typedef AreaSymbolV10 AreaSymbol;
		typedef TextSymbolV10 TextSymbol;
		typedef LineTextSymbolV10 LineTextSymbol;
		typedef RectangleSymbolV10 RectangleSymbol;
		
		typedef ObjectV10 Object;
		
		typedef Custom8BitEncoding Encoding;
		
		enum SymbolType
		{
			TypePoint     = 1,
			TypeLine      = 2,
			TypeArea      = 3,
			TypeText      = 4,
			TypeLineText  = 6,
			TypeRectangle = 7
		};
	};
}

#endif // OPENORIENTEERING_OCD_TYPES_V10_H
