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
		typedef quint32 IndexEntryType;
		
		typedef BaseSymbolV9 BaseSymbol;
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
	
	using PointSymbolElementV9 = PointSymbolElementV8;
	
	struct PointSymbolV9
	{
		typedef BaseSymbolV9 BaseSymbol;
		typedef PointSymbolElementV9 Element;
		
		BaseSymbol base;
		
		quint16 data_size;
		quint16 RESERVED_MEMBER;
		Element begin_of_elements[1];
	};
	
	struct LineSymbolV9
	{
		typedef BaseSymbolV9 BaseSymbol;
		typedef PointSymbolElementV9 Element;
		
		BaseSymbol base;
		
		quint16 line_color;
		quint16 line_width;
		quint16 line_style;
		qint16  dist_from_start;
		qint16  dist_from_end;
		qint16  main_length;
		qint16  end_length;
		qint16  main_gap;
		qint16  sec_gap;
		qint16  end_gap;
		qint16  min_sym;
		qint16  num_prim_sym;
		qint16  prim_sym_dist;
		quint16 double_mode;
		quint16 double_flags;
		quint16 double_color;
		quint16 double_left_color;
		quint16 double_right_color;
		qint16  double_width;
		qint16  double_left_width;
		qint16  double_right_width;
		qint16  double_length;
		qint16  double_gap;
		quint16 RESERVED_MEMBER[3];
		quint16 dec_mode;
		quint16 dec_last;
		quint16 RESERVED_MEMBER;
		quint16 framing_color;
		qint16  framing_width;
		quint16 framing_style;
		quint16 primary_data_size;
		quint16 secondary_data_size;
		quint16 corner_data_size;
		quint16 start_data_size;
		quint16 end_data_size;
		quint16 RESERVED_MEMBER;
		
		Element begin_of_elements[1];
		
		enum LineStyleFlag
		{
			BevelJoin_FlatCap    = 0,
			RoundJoin_RoundCap   = 1,
			BevelJoin_PointedCap = 2,
			RoundJoin_PointedCap = 3,
			MiterJoin_FlatCap    = 4,
			MiterJoin_PointedCap = 6
		};
		
		enum DoubleLineFlag
		{
			DoubleFillColorOn       = 1,
			DoubleBackgroundColorOn = 2
		};
	};
	
	struct AreaSymbolV9
	{
		typedef BaseSymbolV9 BaseSymbol;
		typedef PointSymbolElementV9 Element;
		
		BaseSymbol base;
		
		quint32 border_symbol;
		quint16 fill_color;
		quint16 hatch_mode;
		quint16 hatch_color;
		quint16 hatch_line_width;
		quint16 hatch_dist;
		qint16  hatch_angle_1;
		qint16  hatch_angle_2;
		quint8  fill_on;
		quint8  border_on;
		quint16 structure_mode;
		quint16 structure_width;
		quint16 structure_height;
		qint16  structure_angle;
		quint16 RESERVED_MEMBER;
		quint16 data_size;
		
		Element begin_of_elements[1];
		
		enum HatchMode
		{
			HatchNone   = 0,
			HatchSingle = 1,
			HatchCross  = 2
		};
		
		enum StructureMode
		{
			StructureNone = 0,
			StructureAlignedRows = 1,
			StructureShiftedRows = 2
		};
	};
	
	struct TextSymbolV9
	{
		typedef BaseSymbolV9 BaseSymbol;
		
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
		quint8  point_symbol_on_V10;     /// \since V10
		quint32 point_symbol_number_V10; /// \since V10
		PascalString<18> RESERVED_MEMBER;
		quint16 framing_border_left;
		quint16 framing_border_bottom;
		quint16 framing_border_right;
		quint16 framing_border_top;
		quint16 framing_color;
		quint16 framing_line_width;
		quint16 RESERVED_MEMBER[2];
		quint16 framing_offset_x;
		quint16 framing_offset_y;
	};
	
	struct LineTextSymbolV9
	{
		typedef BaseSymbolV9 BaseSymbol;
		
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
		quint8  framing_line_style_V10; /// \since V10
		PascalString<31> RESERVED_MEMBER;
		quint16 framing_color;
		quint16 framing_line_width;
		quint16 RESERVED_MEMBER[2];
		quint16 framing_offset_x;
		quint16 framing_offset_y;
	};
	
	struct RectangleSymbolV9
	{
		typedef BaseSymbolV9 BaseSymbol;
		
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
		typedef ObjectIndexEntryV9 IndexEntryType;
		
		qint32  symbol;
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
		quint32 height_V11_ONLY; /// V11 only; unit: 1/256 mm
		quint32 RESERVED_MEMBER;
		quint32 height_V10_ONLY; /// V10 only; unit: mm
		
		OcdPoint32 coords[1];
	};
	
#pragma pack(pop)
	
	/** OCD file format version 9 trait. */
	struct FormatV9
	{
		static constexpr int version() { return 9; }
		
		typedef FileHeaderV9 FileHeader;
		
		typedef BaseSymbolV9 BaseSymbol;
		typedef PointSymbolV9 PointSymbol;
		typedef LineSymbolV9 LineSymbol;
		typedef AreaSymbolV9 AreaSymbol;
		typedef TextSymbolV9 TextSymbol;
		typedef LineTextSymbolV9 LineTextSymbol;
		typedef RectangleSymbolV9 RectangleSymbol;
		
		typedef ObjectV9 Object;
		
		typedef Custom8BitEncoding Encoding;
	};
}

#endif // OPENORIENTEERING_OCD_TYPES_V9_H
