/*
 *    Copyright 2013 Kai Pastor
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

#ifndef _OPENORIENTEERING_OCD_TYPES_V11_
#define _OPENORIENTEERING_OCD_TYPES_V11_

#include "ocd_types.h"

namespace Ocd
{
#pragma pack(push, 1)
	
	struct FileHeaderV11
	{
		quint16 vendor_mark;
		quint8  file_type;
		quint8  file_status;
		quint16 version;
		quint8  subversion;
		quint8  subsubversion;
		quint32 first_symbol_block;
		quint32 first_object_block;
		quint32 offline_sync_serial;
		quint32 RESERVED_MEMBER[3];
		quint32 first_string_block;
		quint32 file_name_pos;
		quint32 file_name_size;
		quint32 RESERVED_MEMBER;
	};
	
	struct BaseSymbolV11
	{
		typedef quint32 IndexEntryType;
		
		typedef BaseSymbolV11 BaseSymbol;
		static const int symbol_number_factor = 1000;
		static const int description_length   = 64;
		
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
		quint8  RESERVED_MEMBER[2];
		quint16 num_colors;
		quint16 colors[14];
		QChar   description[description_length];
		quint8  icon_bits[484];
		quint16 group[64];
		
		enum StatusFlag
		{
			StatusProtected = 1,
			StatusHidden    = 2
		};
	};
	
	struct PointSymbolElementV11
	{
		typedef OcdPoint32 PointCoord;
		
		quint16 type;
		quint16 flags;
		quint16 color;
		quint16 line_width;
		quint16 diameter;
		quint16 num_coords;
		quint32 RESERVED_MEMBER;
		
		enum PointSymbolElementTypes
		{
			TypeLine   = 1,
			TypeArea   = 2,
			TypeCircle = 3,
			TypeDot    = 4
		};
	};
	
	struct PointSymbolV11
	{
		typedef BaseSymbolV11 BaseSymbol;
		typedef PointSymbolElementV11 Element;
		
		BaseSymbol base;
		
		quint16 data_size;
		quint16 RESERVED_MEMBER;
		Element begin_of_elements[1];
	};
	
	struct LineSymbolV11
	{
		typedef BaseSymbolV11 BaseSymbol;
		typedef PointSymbolElementV11 Element;
		
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
		quint16 double_background_color;
		quint16 RESERVED_MEMBER[2];
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
		quint8  active_symbols;
		quint8  RESERVED_MEMBER;
		
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
	
	struct AreaSymbolV11
	{
		typedef BaseSymbolV11 BaseSymbol;
		typedef PointSymbolElementV11 Element;
		
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
	
	struct TextSymbolV11
	{
		typedef BaseSymbolV11 BaseSymbol;
		
		BaseSymbol base;
		
		char    font_name[32];
		quint16 font_color;
		quint16 font_size;
		quint16 font_weight;
		quint8  font_italic;
		quint8  RESERVED_MEMBER;
		quint16 char_spacing;
		quint16 word_spacing;
		quint16 alignment;
		quint16 line_spacing;
		quint16 para_spacing;
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
	};
	
	struct LineTextSymbolV11 // TODO: use and test...
	{
		typedef BaseSymbolV11 BaseSymbol;
		
		BaseSymbol base;
		
		char    font_name[32];
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
		QChar   RESERVED_MEMBER[32];
		quint16 framing_color;
		quint16 framing_line_width;
		quint16 RESERVED_MEMBER[2];
		quint16 framing_offset_x;
		quint16 framing_offset_y;
	};
	
	struct RectangleSymbolV11
	{
		typedef BaseSymbolV11 BaseSymbol;
		
		BaseSymbol base;
		
		quint16 line_color;
		quint16 line_width;
		quint16 corner_radius;
		quint16 grid_flags;
		quint16 cell_width;
		quint16 cell_height;
		quint16 RESERVED_MEMBER[2];
		quint16 unnumbered_cells;
		QChar   unnumbered_text[4];
		quint16 line_style;
		QChar   RESERVED_MEMBER[32];
		quint16 RESERVED_MEMBER;
		quint16 font_color;
		quint16 RESERVED_MEMBER[4];
	};
	
	struct ObjectIndexEntryV11
	{
		OcdPoint32 bottom_left_bound;
		OcdPoint32 top_right_bound;
		quint32 pos;
		quint32 size;
		qint32  symbol;
		quint8  type;
		quint8  encryption_mode;
		quint8  status;
		quint8  view_type;
		quint16 color;
		quint16 group;
		quint16 layer;
		quint8  layout_font;
		quint8  RESERVED_MEMBER;
	};
	
	struct ObjectV11
	{
		typedef ObjectIndexEntryV11 IndexEntryType;
		
		qint32  symbol;
		quint8  type;
		quint8  customer;
		qint16  angle;
		quint32 num_items;
		quint16 num_text;
		quint8  mark;
		quint8  snapping_mark;
		qint32  color;
		quint16 line_width;
		quint16 diam_flags;
		quint32 server_object_id;
		quint32 height;
		quint64 date;
		
		OcdPoint32 coords[1];
	};
	
#pragma pack(pop)
	
	/** OCD file format version 11 trait. */
	struct FormatV11
	{
		static inline int version() { return 11; }
		
		typedef FileHeaderV11 FileHeader;
		
		typedef BaseSymbolV11 BaseSymbol;
		typedef PointSymbolV11 PointSymbol;
		typedef LineSymbolV11 LineSymbol;
		typedef AreaSymbolV11 AreaSymbol;
		typedef TextSymbolV11 TextSymbol;
		typedef LineSymbolV11 LineTextSymbol;
		typedef RectangleSymbolV11 RectangleSymbol;
		
		typedef ObjectV11 Object;
		
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

#endif // _OPENORIENTEERING_OCD_TYPES_V11_