/*
 *    Copyright 2013, 2015 Kai Pastor
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
	
	struct FileHeaderV10
	{
		quint16 vendor_mark;
		quint8  file_type;
		quint8  file_status;
		quint16 version;
		quint8  subversion;
		quint8  subsubversion;
		quint32 first_symbol_block;
		quint32 first_object_block;
		quint32 RESERVED_MEMBER[4];
		quint32 first_string_block;
		quint32 file_name_pos;
		quint32 file_name_size;
		quint32 RESERVED_MEMBER;
	};
	
	typedef BaseSymbolV9 BaseSymbolV10;
	
	typedef PointSymbolElementV9 PointSymbolElementV10;
	
	typedef PointSymbolV9 PointSymbolV10;
	
	typedef LineSymbolV9 LineSymbolV10;
	
	typedef AreaSymbolV9 AreaSymbolV10;
	
	struct TextSymbolV10
	{
		typedef BaseSymbolV10 BaseSymbol;
		
		BaseSymbol base;
		
		PascalString<31> font_name;
		quint16 font_color;
		quint16 font_size;
		quint16 font_weight;
		quint8  font_italic;
		quint8  RESERVED_MEMBER;
		quint16 char_spacing;
		quint16 word_spacing;
		quint16 alignment;           // TODO: changed from OCD9
		quint16 line_spacing;
		qint16  para_spacing;
		quint16 indent_first_line;
		quint16 indent_other_lines;
		quint16 num_tabs;
		quint32 tab_pos[32];
		quint16 line_below_on;
		quint16 line_below_color;
		quint16 line_below_width;
		quint16 line_below_offset;
		quint16 RESERVED_MEMBER;
		quint8  framing_mode;
		quint8  framing_line_style;
		quint8  point_symbol_on;
		quint32 point_symbol_number;
		QChar   RESERVED_MEMBER[18];
		quint16 framing_border_left;
		quint16 framing_border_bottom;
		quint16 framing_border_right;
		quint16 framing_border_top;
		quint16 framing_color;
		quint16 framing_line_width;
		quint16 RESERVED_MEMBER[2];
		quint16 framing_offset_x;
		quint16 framing_offset_y;
		
		enum TextAlignment
		{
			HAlignMask      = 0x03,
			HAlignLeft      = 0x00,
			HAlignCenter    = 0x01,
			HAlignRight     = 0x02,
			HAlignJustified = 0x03,
			VAlignMask      = 0x0c,
			VAlignBottom    = 0x00,
			VAlignMiddle    = 0x04,
			VAlignTop       = 0x08
		};
	};
	
	struct LineTextSymbolV10 // TODO: use and test...
	{
		typedef BaseSymbolV10 BaseSymbol;
		
		BaseSymbol base;
		
		PascalString<31> font_name;
		quint16 font_color;
		quint16 font_size;
		quint16 font_weight;
		quint8  font_italic;
		quint8  RESERVED_MEMBER;
		quint16 char_spacing;
		quint16 word_spacing;
		quint16 alignment;
		quint8  framing_mode;
		quint8  framing_line_style;
		char    RESERVED_MEMBER[32];
		quint16 framing_color;
		quint16 framing_line_width;
		quint16 RESERVED_MEMBER[2];
		quint16 framing_offset_x;
		quint16 framing_offset_y;
	};
	
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
