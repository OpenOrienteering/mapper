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

#ifndef OPENORIENTEERING_OCD_TYPES_V12_H
#define OPENORIENTEERING_OCD_TYPES_V12_H

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
	
	using PointSymbolElementV12 = PointSymbolElementV11;
	
	using PointSymbolV12 = PointSymbolV11;
	
	using LineSymbolV12 = LineSymbolV11;
	
	struct AreaSymbolV12
	{
		typedef BaseSymbolV12 BaseSymbol;
		typedef PointSymbolElementV12 Element;
		
		BaseSymbol base;
		
		quint32 border_symbol;
		AreaSymbolCommonV8 common;
		quint8  structure_variation_x;
		quint8  structure_variation_y;
		quint16 structure_minimum_dist;
		quint16 RESERVED_MEMBER;
		quint16 data_size;
		
		Element begin_of_elements[1];
	};
	
	using TextSymbolV12 = TextSymbolV11;
	
	using LineTextSymbolV12 = LineTextSymbolV11;
	
	using RectangleSymbolV12 = RectangleSymbolV11;
	
	using ObjectIndexEntryV12 = ObjectIndexEntryV11;
	
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
	};
}

#endif // OPENORIENTEERING_OCD_TYPES_V12_H
