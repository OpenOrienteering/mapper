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

#ifndef OPENORIENTEERING_OCD_TYPES_V12
#define OPENORIENTEERING_OCD_TYPES_V12

#include "ocd_types.h"
#include "ocd_types_v11.h"

namespace Ocd
{
	
#pragma pack(push, 1)
	
	struct FileHeaderV12 : public FileHeaderV11
	{
		quint32 RESERVED_MEMBER[2];
		quint32 first_multi_rep_block;
	};
	
	using BaseSymbolV12 = BaseSymbolV11;
	
	typedef PointSymbolElementV11 PointSymbolElementV12;
	
	using PointSymbolV12 = PointSymbolV11;
	
	using LineSymbolV12 = LineSymbolV11;
	
	struct AreaSymbolV12
	{
		typedef BaseSymbolV12 BaseSymbol;
		typedef PointSymbolElementV12 Element;
		
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
		quint8  structure_mode;
		quint8  structure_draw;
		quint16 structure_width;
		quint16 structure_height;
		qint16  structure_angle;
		quint8  structure_variation_x;
		quint8  structure_variation_y;
		quint16 structure_minimum_dist;
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
	
	struct TextSymbolV12
	{
		typedef BaseSymbolV12 BaseSymbol;
		
		BaseSymbol base;
		
		Utf8PascalString<31> font_name;
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
	
	struct LineTextSymbolV12 // TODO: use and test...
	{
		typedef BaseSymbolV12 BaseSymbol;
		
		BaseSymbol base;
		
		Utf8PascalString<31> font_name;
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
	
	struct RectangleSymbolV12
	{
		typedef BaseSymbolV12 BaseSymbol;
		
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
		quint16 font_size;
		quint16 RESERVED_MEMBER[4];
	};
	
	struct ObjectIndexEntryV12
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
		quint8  RESERVED_MEMBER[2];
		
		enum ObjectStatus
		{
			StatusDeleted = 0,
			StatusNormal  = 1,
			StatusHidden  = 2,
			StatusDeletedForUndo = 3
		};
	};
	
	struct ObjectV12
	{
		typedef ObjectIndexEntryV12 IndexEntryType;
		
		qint32  symbol;
		quint8  type;
		quint8  customer;
		qint16  angle;
		qint32  color;
		quint16 line_width;
		quint16 diam_flags;
		quint32 server_object_id;
		quint32 height;
		quint64 creation_date;
		quint32 multi_rep_id;
		quint64 modification_date;
		quint32 num_items;
		quint16 num_text;
		quint16 object_string_length;
		quint16 db_link_length;
		quint8  object_string_type;
		quint8  RESERVED_MEMBER;
		
		OcdPoint32 coords[1];
	};
	
#pragma pack(pop)
	
	/** OCD file format version 11 trait. */
	struct FormatV12
	{
		static constexpr int version() { return 12; }
		
		typedef FileHeaderV12 FileHeader;
		
		typedef BaseSymbolV12 BaseSymbol;
		typedef PointSymbolV12 PointSymbol;
		typedef LineSymbolV12 LineSymbol;
		typedef AreaSymbolV12 AreaSymbol;
		typedef TextSymbolV12 TextSymbol;
		typedef LineTextSymbolV12 LineTextSymbol;
		typedef RectangleSymbolV12 RectangleSymbol;
		
		typedef ObjectV12 Object;
		
		typedef Utf8Encoding Encoding;
		
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

#endif // OPENORIENTEERING_OCD_TYPES_V12
