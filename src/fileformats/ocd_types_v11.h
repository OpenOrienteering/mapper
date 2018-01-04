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

#ifndef OPENORIENTEERING_OCD_TYPES_V11_H
#define OPENORIENTEERING_OCD_TYPES_V11_H

#include "ocd_types.h"
#include "ocd_types_v10.h"

namespace Ocd
{
	
#pragma pack(push, 1)
	
	struct BaseSymbolV11
	{
		using IndexEntryType = SymbolIndexEntry;
		static const int symbol_number_factor = 1000;
		
		quint32 size;
		quint32 number;
		quint8  type;
		quint8  flags;
		quint8  selected;
		quint8  status;
		quint8  tool;
		quint8  cs_mode;
		quint8  cs_type;
		quint8  cd_flags;
		qint32  extent;
		quint32 file_pos;
		quint8  RESERVED_MEMBER[2];
		quint16 num_colors;
		quint16 colors[14];
		Utf16PascalString<64> description;
		quint8  icon_bits[484];
		quint16 group[64];
	};
	
	struct PointSymbolV11
	{
		using BaseSymbol = BaseSymbolV11;
		using Element    = FormatV10::PointSymbol::Element;
		
		BaseSymbol base;
		
		quint16 data_size;
		quint16 RESERVED_MEMBER;
		Element begin_of_elements[1];
	};
	
	struct LineSymbolV11
	{
		using BaseSymbol = BaseSymbolV11;
		using Element    = FormatV10::LineSymbol::Element;
		
		BaseSymbol base;
		
		LineSymbolCommonV8 common;
		
		Element begin_of_elements[1];
	};
	
	struct AreaSymbolV11
	{
		using BaseSymbol = BaseSymbolV11;
		using Element    = FormatV10::AreaSymbol::Element;
		
		BaseSymbol base;
		
		quint32 border_symbol;
		AreaSymbolCommonV8 common;
		quint16 RESERVED_MEMBER;
		quint16 data_size;
		
		Element begin_of_elements[1];
	};
	
	struct TextSymbolV11
	{
		using BaseSymbol = BaseSymbolV11;
		
		BaseSymbol base;
		
		Utf8PascalString<31>    font_name;
		BasicTextAttributesV8   basic;
		SpecialTextAttributesV8 special;
		quint16                 RESERVED_MEMBER;
		FramingAttributesV8     framing;
	};
	
	struct LineTextSymbolV11
	{
		using BaseSymbol = BaseSymbolV11;
		
		BaseSymbol base;
		
		Utf8PascalString<31>  font_name;
		BasicTextAttributesV8 basic;
		FramingAttributesV8   framing;
	};
	
	struct RectangleSymbolV11
	{
		using BaseSymbol = BaseSymbolV11;
		
		BaseSymbol base;
		
		quint16 line_color;
		quint16 line_width;
		quint16 corner_radius;
		quint16 grid_flags;
		quint16 cell_width;
		quint16 cell_height;
		quint16 RESERVED_MEMBER[2];
		quint16 unnumbered_cells;
		Utf8PascalString<3> unnumbered_text;
		quint16 line_style;
		Utf8PascalString<31> RESERVED_MEMBER;
		quint16 RESERVED_MEMBER;
		quint16 font_size_V10;      /// \since V10
		quint16 RESERVED_MEMBER[4];
	};
	
#pragma pack(pop)
	
	/** OCD file format version 11 trait. */
	struct FormatV11
	{
		using FileHeader      = FormatV10::FileHeader;
		using BaseSymbol      = BaseSymbolV11;
		using PointSymbol     = PointSymbolV11;
		using LineSymbol      = LineSymbolV11;
		using AreaSymbol      = AreaSymbolV11;
		using TextSymbol      = TextSymbolV11;
		using LineTextSymbol  = LineTextSymbolV11;
		using RectangleSymbol = RectangleSymbolV11;
		using Object          = FormatV10::Object;
		using Encoding        = Utf8Encoding;
	};
}

#endif // OPENORIENTEERING_OCD_TYPES_V11_H
