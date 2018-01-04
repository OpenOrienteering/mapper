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

#ifndef OPENORIENTEERING_OCD_TYPES_V9_H
#define OPENORIENTEERING_OCD_TYPES_V9_H

#include "ocd_types.h"
#include "ocd_types_v8.h"

namespace Ocd
{
	
#pragma pack(push, 1)
	
	struct FileHeaderV9 : public FileHeaderGeneric
	{
		quint32 first_symbol_block;
		quint32 first_object_block;
		quint32 offline_sync_serial_V11;  /// \since V11
		quint32 current_file_version_V12; /// \since V12
		quint32 RESERVED_MEMBER[2];
		quint32 first_string_block;
		quint32 file_name_pos;
		quint32 file_name_size;
		quint32 RESERVED_MEMBER;
	};
	
	struct BaseSymbolV9
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
		qint32  file_pos;
		quint16 group;
		quint16 num_colors;
		quint16 colors[14];
		PascalString<31> description;
		quint8  icon_bits[484];
	};
	
	struct PointSymbolV9
	{
		using BaseSymbol = BaseSymbolV9;
		using Element = FormatV8::PointSymbol::Element;
		
		BaseSymbol base;
		
		quint16 data_size;
		quint16 RESERVED_MEMBER;
		Element begin_of_elements[1];
	};
	
	struct LineSymbolV9
	{
		using BaseSymbol = BaseSymbolV9;
		using Element    = FormatV8::LineSymbol::Element;
		
		BaseSymbol base;
		
		LineSymbolCommonV8 common;
		
		Element begin_of_elements[1];
	};
	
	struct AreaSymbolV9
	{
		using BaseSymbol = BaseSymbolV9;
		using Element    = FormatV8::AreaSymbol::Element;
		
		BaseSymbol base;
		
		quint32 border_symbol;
		AreaSymbolCommonV8 common;
		quint16 RESERVED_MEMBER;
		quint16 data_size;
		
		Element begin_of_elements[1];
	};
	
	struct TextSymbolV9
	{
		using BaseSymbol = BaseSymbolV9;
		
		BaseSymbol base;
		
		PascalString<31>        font_name;
		BasicTextAttributesV8   basic;
		SpecialTextAttributesV8 special;
		quint16                 RESERVED_MEMBER;
		FramingAttributesV8     framing;
	};
	
	struct LineTextSymbolV9
	{
		using BaseSymbol = BaseSymbolV9;
		
		BaseSymbol base;
		
		PascalString<31>      font_name;
		BasicTextAttributesV8 basic;
		FramingAttributesV8   framing;
	};
	
	struct RectangleSymbolV9
	{
		using BaseSymbol = BaseSymbolV9;
		
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
		PascalString<31> RESERVED_MEMBER;
		quint16 RESERVED_MEMBER;
		quint16 font_size_V10;      /// \since V10
		quint16 RESERVED_MEMBER[4];
	};
	
	struct ObjectIndexEntryV9
	{
		OcdPoint32 bottom_left_bound;
		OcdPoint32 top_right_bound;
		quint32 pos;
		quint32 size;
		qint32  symbol;
		quint8  type;
		quint8  encryption_mode_V11;  /// \since V11
		quint8  status;
		quint8  view_type;
		quint16 color;
		quint16 group_V11;            /// \since V11
		quint16 layer;
		quint8  layout_font_V11_ONLY; /// only in V11
		quint8  RESERVED_MEMBER;
	};
	
	struct ObjectV9
	{
		using IndexEntryType = ObjectIndexEntryV9;
		
		quint32 symbol;
		quint8  type;
		quint8  customer_V11;      /// \since V11
		qint16  angle;
		quint32 num_items;
		quint16 num_text;
		quint8  mark_V11;          /// \since V11
		quint8  snapping_mark_V11; /// \since V11
		quint32 color;
		quint16 line_width;
		quint16 diam_flags;
		// The usage of the following 16 bytes has changed significantly in the
		// versions 9 to 12. This is an abstraction, capturing what seems most
		// relevant.
		quint32 RESERVED_MEMBER; /// V11: Server object ID
		quint32 height_V11;      /// \since V11; unit: 1/256 mm
		quint32 RESERVED_MEMBER;
		quint32 height_V10_ONLY; /// V10 only; unit: mm
		
		OcdPoint32 coords[1];
	};
	
#pragma pack(pop)
	
	/** OCD file format version 9 trait. */
	struct FormatV9
	{
		using FileHeader      = FileHeaderV9;
		using BaseSymbol      = BaseSymbolV9;
		using PointSymbol     = PointSymbolV9;
		using LineSymbol      = LineSymbolV9;
		using AreaSymbol      = AreaSymbolV9;
		using TextSymbol      = TextSymbolV9;
		using LineTextSymbol  = LineTextSymbolV9;
		using RectangleSymbol = RectangleSymbolV9;
		using Object          = ObjectV9;
		using Encoding        = Custom8BitEncoding;
	};
}

#endif // OPENORIENTEERING_OCD_TYPES_V9_H
