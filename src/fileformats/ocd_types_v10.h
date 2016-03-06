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

#ifndef _OPENORIENTEERING_OCD_TYPES_V10_
#define _OPENORIENTEERING_OCD_TYPES_V10_

#include "ocd_types.h"
#include "ocd_types_v9.h"

namespace Ocd
{
	
#pragma pack(push, 1)
	
	using FileHeaderV10 = FileHeaderV9;
	
	typedef BaseSymbolV9 BaseSymbolV10;
	
	typedef PointSymbolElementV9 PointSymbolElementV10;
	
	typedef PointSymbolV9 PointSymbolV10;
	
	typedef LineSymbolV9 LineSymbolV10;
	
	typedef AreaSymbolV9 AreaSymbolV10;
	
	using TextSymbolV10 = TextSymbolV9;
	
	using LineTextSymbolV10 = LineTextSymbolV9;
	
	struct RectangleSymbolV10
	{
		typedef BaseSymbolV10 BaseSymbol;
		
		BaseSymbol base;
		
		quint16 line_color;
		quint16 line_width;
		quint16 corner_radius;
		quint16 grid_flags;
		quint16 cell_width;
		quint16 cell_height;
		quint16 RESERVED_MEMBER[2];
		quint16 unnumbered_cells;
		PascalString<3> unnumbered_text;
		quint16 RESERVED_MEMBER;
		char    RESERVED_MEMBER[32];
		quint16 RESERVED_MEMBER;
		quint16 font_size;
		quint16 RESERVED_MEMBER[4];
	};
	
	typedef ObjectIndexEntryV9 ObjectIndexEntryV10;
	
	struct ObjectV10
	{
		typedef ObjectIndexEntryV10 IndexEntryType;
		
		qint32  symbol;
		quint8  type;
		quint8  RESERVED_MEMBER;
		qint16  angle;
		quint32 num_items;
		quint16 num_text;
		quint16 RESERVED_MEMBER;
		quint32 color;
		quint16 line_width;
		quint16 diam_flags;
		quint32 RESERVED_MEMBER[3];
		quint32 height_mm;
		
		OcdPoint32 coords[1];
	};
	
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

#endif // _OPENORIENTEERING_OCD_TYPES_V10_
