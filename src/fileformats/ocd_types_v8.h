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

#ifndef _OPENORIENTEERING_OCD_TYPES_V8_
#define _OPENORIENTEERING_OCD_TYPES_V8_

#include "ocd_types.h"

namespace Ocd
{
	
#pragma pack(push, 1)
	
	struct CmykV8
	{
		quint8 cyan;
		quint8 magenta;
		quint8 yellow;
		quint8 black;
	};
	
	struct ColorInfoV8
	{
		quint16 number;
		quint16 RESERVED_MEMBER;
		CmykV8  cmyk;
		PascalString<31> name;
		quint8  separations[32];
	};
	
	struct SeparationInfoV8
	{
		PascalString<15> name;
		CmykV8  cmyk;
		quint16 raster_freq;
		quint16 raster_angle;
	};
	
	struct SymbolHeaderV8
	{
		quint16 num_colors;
		quint16 num_separations;
		quint16 cyan_freq;
		quint16 cyan_angle;
		quint16 magenta_freq;
		quint16 magenta_angle;
		quint16 yellow_freq;
		quint16 yellow_angle;
		quint16 black_freq;
		quint16 black_angle;
		quint16 RESERVED_MEMBER[2];
		ColorInfoV8 color_info[256];
		SeparationInfoV8 separation_info[32];
	};
	
	struct FileHeaderV8
	{
		quint16 vendor_mark;
		quint16 section_mark;
		quint16 version;
		quint16 subversion;
		quint32 first_symbol_block;
		quint32 first_object_block;
		quint32 setup_pos;
		quint32 setup_size;
		quint32 info_pos;
		quint32 info_size;
		quint32 first_string_block;
		quint32 RESERVED_MEMBER[3];
		SymbolHeaderV8 symbol_header;
	};
	
	struct BaseSymbolV8
	{
		typedef quint32 IndexEntryType;
		
		typedef BaseSymbolV8 BaseSymbol;
		static const int symbol_number_factor = 10;
		
		quint16 size;
		quint16 number;
		quint16 type;
		quint8  type2;
		quint8  flags;
		quint16 extent;
		quint8  selected;
		quint8  status;
		quint16 RESERVED_MEMBER[2];
		qint32  file_pos;
		quint8  colors[32];
		PascalString<31> description;
		quint8  icon_bits[264];
		
		enum StatusFlag
		{
			StatusProtected = 1,
			StatusHidden    = 2
		};
	};
	
	struct PointSymbolElementV8
	{
		quint16 type;
		quint16 flags;
		quint16 color;
		qint16  line_width;
		qint16  diameter;
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
	
	struct PointSymbolV8
	{
		typedef BaseSymbolV8 BaseSymbol;
		typedef PointSymbolElementV8 Element;
		
		BaseSymbol base;
		
		quint16 data_size;
		quint16 RESERVED_MEMBER;
		Element begin_of_elements[1];
	};
	
	struct LineSymbolV8
	{
		typedef BaseSymbolV8 BaseSymbol;
		typedef PointSymbolElementV8 Element;
		
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
	
	struct AreaSymbolV8
	{
		typedef BaseSymbolV8 BaseSymbol;
		typedef PointSymbolElementV8 Element;
		
		BaseSymbol base;
		
		quint16 area_flags;
		quint16 fill_on;
		quint16 fill_color;
		quint16 hatch_mode;
		quint16 hatch_color;
		quint16 hatch_line_width;
		quint16 hatch_dist;
		qint16  hatch_angle_1;
		qint16  hatch_angle_2;
		quint16 RESERVED_MEMBER;
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
	
	struct TextSymbolV8
	{
		typedef BaseSymbolV8 BaseSymbol;
		
		BaseSymbol base;
		
		PascalString<31> font_name;
		quint16 font_color;
		quint16 font_size;
		quint16 font_weight;
		quint8  font_italic;
		quint8  charset;
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
		quint16 framing_mode;
		PascalString<31> framing_font;
		quint16 framing_color;
		quint16 framing_line_width;
		quint16 framing_font_weight;
		quint16 framing_italic;
		quint16 framing_offset_x;
		quint16 framing_offset_y;
		
		enum TextAlignment
		{
			HAlignMask      = 0x03,
			HAlignLeft      = 0x00,
			HAlignCenter    = 0x01,
			HAlignRight     = 0x02,
			HAlignJustified = 0x03,
			VAlignMask      = 0x00, /// \since V10
			VAlignBottom    = 0xfd, /// \since V10
			VAlignMiddle    = 0xfe, /// \since V10
			VAlignTop       = 0xff  /// \since V10
		};
	};
	
	struct LineTextSymbolV8 // TODO: use and test...
	{
		typedef BaseSymbolV8 BaseSymbol;
		
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
		quint8  RESERVED_MEMBER;
		PascalString<31> RESERVED_MEMBER;
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
			HAlignAllLine   = 0x03,
			VAlignMask      = 0x0c, /// \since V10
			VAlignBottom    = 0x00, /// \since V10
			VAlignMiddle    = 0x04, /// \since V10
			VAlignTop       = 0x08  /// \since V10
		};
	};
	
	struct RectangleSymbolV8
	{
		typedef BaseSymbolV8 BaseSymbol;
		
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
		quint16 RESERVED_MEMBER[6];
	};
	
	struct ObjectIndexEntryV8
	{
		OcdPoint32 bottom_left_bound;
		OcdPoint32 top_right_bound;
		quint32 pos;
		quint16 size_MISC; /// Different interpretation for version < 8
		qint16  symbol;
		
		enum ObjectStatus
		{
			StatusDeleted = 0,
			StatusNormal  = 1,
			StatusHidden  = 2,
			StatusDeletedForUndo = 3
		};
	};
	
	struct ObjectV8
	{
		typedef ObjectIndexEntryV8 IndexEntryType;
		
		qint16  symbol;
		quint8  type;
		quint8  unicode;
		quint16 num_items;
		quint16 num_text;
		qint16  angle;
		quint16 RESERVED_MEMBER;
		quint32 RESERVED_MEMBER;
		PascalString<15> reserved;
		
		OcdPoint32 coords[1];
	};
	
	typedef double PascalDouble;
	
	struct GpsPointV8
	{
		OcdPoint32 map_point;
		PascalDouble lat;
		PascalDouble lon;
		PascalString<15> name;
	};
	
	struct PrintSetupV8
	{
		OcdPoint32 bottom_left;
		OcdPoint32 top_right;
		quint16 grid_enabled;
		qint16  grid_color;
		qint16  overlap_x;
		qint16  overlap_y;
		PascalDouble scale;
		quint16 intensity;
		quint16 line_width;
		quint16 RESERVED_MEMBER[4];
	};
	
	struct ExportSetupV8
	{
		OcdPoint32 bottom_left;
		OcdPoint32 top_right;
	};
	
	struct ZoomRectV8
	{
		PascalDouble zoom;
		OcdPoint32 center;
	};
	
	struct SetupV8
	{
		OcdPoint32 center;
		PascalDouble grid_dist;
		quint16 work_mode;
		quint16 line_mode;
		quint16 edit_mode;
		qint16  selected_symbol;
		PascalDouble map_scale;
		PascalDouble real_offset_x;
		PascalDouble real_offset_y;
		PascalDouble real_angle;
		PascalDouble real_grid;
		PascalDouble gps_angle;
		GpsPointV8 gps_adjustment[12];
		quint32 num_gps_adjustments;
		PascalDouble RESERVED_MEMBER[2];
		OcdPoint32 RESERVED_MEMBER;
		char    RESERVED_MEMBER[256];
		quint16 RESERVED_MEMBER[2];
		PascalDouble RESERVED_MEMBER;
		OcdPoint32 RESERVED_MEMBER;
		PascalDouble RESERVED_MEMBER;
		PrintSetupV8 print_setup;
		ExportSetupV8 export_setup;
		PascalDouble zoom;
		ZoomRectV8 zoom_history[8];
		quint32 zoom_history_size;
		// V6 header ends here, but Mapper doesn't use the following fields.
		quint16 real_coords_IGNORED;
		char    filename_IGNORED[256];
		quint16 hatch_areas_IGNORED;
		quint16 dim_templates_IGNORED;
		quint16 hide_templates_IGNORED;
		quint16 template_mode_IGNORED;
		qint16  template_color_IGNORED;
	};
	
#pragma pack(pop)
	
	/** OCD file format version 8 trait. */
	struct FormatV8
	{
		static constexpr int version() { return 8; }
		
		typedef FileHeaderV8 FileHeader;
		
		typedef SymbolHeaderV8 SymbolHeader;
		
		typedef BaseSymbolV8 BaseSymbol;
		typedef PointSymbolV8 PointSymbol;
		typedef LineSymbolV8 LineSymbol;
		typedef AreaSymbolV8 AreaSymbol;
		typedef TextSymbolV8 TextSymbol;
		typedef LineTextSymbolV8 LineTextSymbol;
		typedef RectangleSymbolV8 RectangleSymbol;
		
		typedef ObjectV8 Object;
		
		typedef Custom8BitEncoding Encoding;
		
		enum SymbolType
		{
			TypePoint     = 1,
			TypeLine      = 2,
			TypeLineText  = 2, // same as TypeLine!
			TypeArea      = 3,
			TypeText      = 4,
			TypeRectangle = 5  // or formatted text...
		};
	};
}

#endif // _OPENORIENTEERING_OCD_TYPES_V8_
