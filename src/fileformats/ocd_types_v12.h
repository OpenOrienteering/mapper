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
	
	using TextSymbolV12 = TextSymbolV11;
	
	using LineTextSymbolV12 = LineTextSymbolV11;
	
	using RectangleSymbolV12 = RectangleSymbolV11;
	
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
